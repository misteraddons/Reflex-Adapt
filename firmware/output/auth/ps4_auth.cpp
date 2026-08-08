#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include "ps4_auth.h"

#include "auth_storage.h"

bool PS4Auth::begin(uint16_t eeprom_base) {
  if (eeprom_base == 0xFFFFu) {
    _key_loaded = false;
    _state = PS4_AUTH_IDLE;
    return false;
  }

  _eeprom_base = eeprom_base;

  AuthBlobRecord blob{};
  if (!readAuthBlob(AUTH_KEY_TYPE_PS4, blob)) {
    _key_loaded = false;
    _state = PS4_AUTH_IDLE;
    return false;
  }

  memcpy(_serial,  blob.data + PS4_KEY_SERIAL_OFF, PS4_KEY_SERIAL_SZ);
  memcpy(_dev_sig, blob.data + PS4_KEY_SIG_OFF,    PS4_KEY_SIG_SZ);
  memcpy(_rsa_n,   blob.data + PS4_KEY_N_OFF,      PS4_KEY_N_SZ);
  memcpy(_rsa_e,   blob.data + PS4_KEY_E_OFF,      PS4_KEY_E_SZ);
  memcpy(_rsa_p,   blob.data + PS4_KEY_P_OFF,      PS4_KEY_P_SZ);
  memcpy(_rsa_q,   blob.data + PS4_KEY_Q_OFF,      PS4_KEY_Q_SZ);
  memcpy(_rsa_dp,  blob.data + PS4_KEY_DP_OFF,     PS4_KEY_DP_SZ);
  memcpy(_rsa_dq,  blob.data + PS4_KEY_DQ_OFF,     PS4_KEY_DQ_SZ);
  memcpy(_rsa_qp,  blob.data + PS4_KEY_QP_OFF,     PS4_KEY_QP_SZ);

  _sk.n_bitlen = 2048;
  _sk.p    = _rsa_p;   _sk.plen  = PS4_KEY_P_SZ;
  _sk.q    = _rsa_q;   _sk.qlen  = PS4_KEY_Q_SZ;
  _sk.dp   = _rsa_dp;  _sk.dplen = PS4_KEY_DP_SZ;
  _sk.dq   = _rsa_dq;  _sk.dqlen = PS4_KEY_DQ_SZ;
  _sk.iq   = _rsa_qp;  _sk.iqlen = PS4_KEY_QP_SZ;

  br_hmac_drbg_init(&_rng, &br_sha256_vtable, "PS4Auth", 7);
  uint32_t entropy[2] = { micros(), (uint32_t)(_serial[0] | (_serial[1] << 8) | (_serial[2] << 16) | (_serial[3] << 24)) };
  br_hmac_drbg_update(&_rng, entropy, sizeof(entropy));

  _key_loaded = true;
  _state = PS4_AUTH_IDLE;
  return true;
}

bool PS4Auth::isReady() const {
  return _key_loaded;
}

void PS4Auth::receiveNoncePage(const uint8_t* data, uint16_t len) {
  if (!_key_loaded || data == nullptr || len < 63) return;

  uint8_t nonce_id = data[0];
  uint8_t page = data[1];
  if (page >= PS4_NONCE_PAGES) return;

  const uint32_t expected_crc =
    (uint32_t)data[59] |
    ((uint32_t)data[60] << 8) |
    ((uint32_t)data[61] << 16) |
    ((uint32_t)data[62] << 24);
  if (crc32_calc_with_report_id(0xF0, data, 59) != expected_crc) {
    return;
  }

  if (page == 0) {
    _nonce_id = nonce_id;
    _state = PS4_AUTH_RECEIVING_NONCE;
    memset(_nonce, 0, PS4_NONCE_SZ);
  }

  if (nonce_id != _nonce_id) return;

  uint16_t offset = page * PS4_PAGE_SZ;
  uint16_t copy_len = (page < 4) ? PS4_PAGE_SZ : 32;
  if (offset + copy_len > PS4_NONCE_SZ) copy_len = PS4_NONCE_SZ - offset;

  memcpy(_nonce + offset, data + 3, copy_len);

  if (page == PS4_NONCE_PAGES - 1) {
    _state = PS4_AUTH_SIGNING;
    _sign_pending = true;
  }
}

void PS4Auth::process() {
  if (!_sign_pending) return;
  _sign_pending = false;

  uint32_t entropy = micros();
  br_hmac_drbg_update(&_rng, &entropy, sizeof(entropy));

  uint8_t hashed_nonce[32];
  br_sha256_context sha_ctx;
  br_sha256_init(&sha_ctx);
  br_sha256_update(&sha_ctx, _nonce, PS4_NONCE_SZ);
  br_sha256_out(&sha_ctx, hashed_nonce);

  uint32_t ret = br_rsa_i15_pss_sign(
    &_rng.vtable,
    &br_sha256_vtable,
    &br_sha256_vtable,
    hashed_nonce,
    32,
    &_sk,
    _auth_buffer
  );

  if (ret == 1) {
    memcpy(_auth_buffer + 256, _serial, PS4_KEY_SERIAL_SZ);
    memcpy(_auth_buffer + 272, _rsa_n, PS4_KEY_N_SZ);
    memcpy(_auth_buffer + 528, _rsa_e, PS4_KEY_E_SZ);
    memcpy(_auth_buffer + 784, _dev_sig, PS4_KEY_SIG_SZ);
    memset(_auth_buffer + 1040, 0, 24);

    _cur_auth_page = 0;
    _state = PS4_AUTH_READY;
  } else {
    _state = PS4_AUTH_IDLE;
  }
}

uint16_t PS4Auth::getAuthPage(uint8_t* buffer, uint16_t reqlen) {
  if (reqlen < 63) {
    return 0;
  }

  if (_state != PS4_AUTH_READY) {
    memset(buffer, 0, reqlen);
    return reqlen;
  }

  memset(buffer, 0, reqlen);
  buffer[0] = _nonce_id;
  buffer[1] = _cur_auth_page;
  buffer[2] = 0;

  uint16_t offset = _cur_auth_page * PS4_PAGE_SZ;
  uint16_t copy_len = PS4_PAGE_SZ;
  if (offset + copy_len > PS4_AUTH_BUFFER_SZ) {
    copy_len = PS4_AUTH_BUFFER_SZ - offset;
  }

  memcpy(buffer + 3, _auth_buffer + offset, copy_len);

  if (copy_len < PS4_PAGE_SZ) {
    memset(buffer + 3 + copy_len, 0, PS4_PAGE_SZ - copy_len);
  }

  uint32_t crc = crc32_calc_with_report_id(0xF1, buffer, 59);
  buffer[59] = (crc >> 0)  & 0xFF;
  buffer[60] = (crc >> 8)  & 0xFF;
  buffer[61] = (crc >> 16) & 0xFF;
  buffer[62] = (crc >> 24) & 0xFF;

  _cur_auth_page++;
  if (_cur_auth_page >= PS4_AUTH_PAGES) {
    _state = PS4_AUTH_IDLE;
  }

  return reqlen;
}

uint16_t PS4Auth::getSigningState(uint8_t* buffer, uint16_t reqlen) {
  if (reqlen < 15) {
    return 0;
  }

  memset(buffer, 0, reqlen);
  buffer[0] = _nonce_id;
  buffer[1] = (_state == PS4_AUTH_READY) ? 0x00 : 0x10;

  uint32_t crc = crc32_calc_with_report_id(0xF2, buffer, 11);
  buffer[11] = (crc >> 0)  & 0xFF;
  buffer[12] = (crc >> 8)  & 0xFF;
  buffer[13] = (crc >> 16) & 0xFF;
  buffer[14] = (crc >> 24) & 0xFF;
  return reqlen;
}

uint16_t PS4Auth::getResetConfig(uint8_t* buffer, uint16_t reqlen) {
  _state = PS4_AUTH_IDLE;
  _cur_auth_page = 0;
  static const uint8_t config[] = { 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00 };
  uint16_t copy = (reqlen < sizeof(config)) ? reqlen : sizeof(config);
  memcpy(buffer, config, copy);
  if (reqlen > copy) memset(buffer + copy, 0, reqlen - copy);
  return reqlen;
}

PS4AuthState PS4Auth::getState() const {
  return _state;
}

uint32_t PS4Auth::crc32_calc(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }
  return ~crc;
}

uint32_t PS4Auth::crc32_calc_with_report_id(uint8_t report_id, const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  crc ^= report_id;
  for (int j = 0; j < 8; j++) {
    crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }
  return ~crc;
}

void PS4Auth::loadEEPROM(uint16_t offset, uint8_t* dest, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    dest[i] = EEPROM.read(_eeprom_base + offset + i);
  }
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
