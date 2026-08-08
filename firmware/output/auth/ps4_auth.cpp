#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include "ps4_auth.h"

#include "auth_storage.h"
#include "ps4_auth_crc.h"

bool PS4Auth::begin(uint16_t eeprom_base) {
  _f0_report_count = 0;
  _f0_crc_error_count = 0;
  _f0_reject_count = 0;
  _sign_attempt_count = 0;
  _sign_success_count = 0;
  _f1_request_count = 0;
  _f2_request_count = 0;
  _f3_request_count = 0;
  _last_sign_us = 0;
  _sign_pending = false;
  _nonce_pages_received = 0;

  if (eeprom_base == 0xFFFFu) {
    _key_loaded = false;
    _state = PS4_AUTH_IDLE;
    return false;
  }

  _eeprom_base = eeprom_base;

  AuthBlobRecord blob{};
  if (!readAuthBlob(AUTH_KEY_TYPE_PS4, blob)) {
    _key_loaded = false;
    _sign_pending = false;
    _nonce_pages_received = 0;
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
  _sign_pending = false;
  _nonce_pages_received = 0;
  _state = PS4_AUTH_IDLE;
  return true;
}

bool PS4Auth::isReady() const {
  return _key_loaded;
}

void PS4Auth::receiveNoncePage(const uint8_t* data, uint16_t len) {
  ++_f0_report_count;
  if (!_key_loaded || data == nullptr || len < 63) {
    ++_f0_reject_count;
    return;
  }

  const uint8_t nonce_id = data[0];
  const uint8_t page = data[1];
  if (page >= PS4_NONCE_PAGES) {
    ++_f0_reject_count;
    return;
  }

  const uint32_t expected_crc =
    (uint32_t)data[59] |
    ((uint32_t)data[60] << 8) |
    ((uint32_t)data[61] << 16) |
    ((uint32_t)data[62] << 24);
  if (ps4AuthReportCrc32(0xF0, data, 59) != expected_crc) {
    ++_f0_crc_error_count;
    ++_f0_reject_count;
    return;
  }

  if (page == 0) {
    _nonce_id = nonce_id;
    _state = PS4_AUTH_RECEIVING_NONCE;
    _sign_pending = false;
    _nonce_pages_received = 0;
    memset(_nonce, 0, PS4_NONCE_SZ);
  }

  if (_state != PS4_AUTH_RECEIVING_NONCE || nonce_id != _nonce_id) {
    ++_f0_reject_count;
    return;
  }

  const uint16_t offset = page * PS4_PAGE_SZ;
  uint16_t copy_len = (page < 4) ? PS4_PAGE_SZ : 32;
  if (offset + copy_len > PS4_NONCE_SZ) copy_len = PS4_NONCE_SZ - offset;

  memcpy(_nonce + offset, data + 3, copy_len);
  _nonce_pages_received |= static_cast<uint8_t>(1u << page);

  if (_nonce_pages_received ==
      static_cast<uint8_t>((1u << PS4_NONCE_PAGES) - 1u)) {
    _state = PS4_AUTH_SIGNING;
    _sign_pending = true;
  }
}

bool PS4Auth::signLocally() {
  const uint32_t signStartedUs = micros();
  uint32_t entropy = micros();
  br_hmac_drbg_update(&_rng, &entropy, sizeof(entropy));

  uint8_t hashedNonce[32];
  br_sha256_context shaContext;
  br_sha256_init(&shaContext);
  br_sha256_update(&shaContext, _nonce, PS4_NONCE_SZ);
  br_sha256_out(&shaContext, hashedNonce);

  const uint32_t result = br_rsa_i15_pss_sign(
      &_rng.vtable, &br_sha256_vtable, &br_sha256_vtable,
      hashedNonce, sizeof(hashedNonce), &_sk, _auth_buffer);
  _last_sign_us = micros() - signStartedUs;
  return result == 1;
}

void PS4Auth::completeSignature() {
  memcpy(_auth_buffer + 256, _serial, PS4_KEY_SERIAL_SZ);
  memcpy(_auth_buffer + 272, _rsa_n, PS4_KEY_N_SZ);
  memcpy(_auth_buffer + 528, _rsa_e, PS4_KEY_E_SZ);
  memcpy(_auth_buffer + 784, _dev_sig, PS4_KEY_SIG_SZ);
  memset(_auth_buffer + 1040, 0, 24);
  _cur_auth_page = 0;
  _state = PS4_AUTH_READY;
}

void PS4Auth::process() {
  if (!_sign_pending) return;
  _sign_pending = false;
  ++_sign_attempt_count;

  if (signLocally()) {
    ++_sign_success_count;
    completeSignature();
  } else {
    _state = PS4_AUTH_IDLE;
  }
}

uint16_t PS4Auth::getAuthPage(uint8_t* buffer, uint16_t reqlen) {
  ++_f1_request_count;
  if (reqlen < 63) {
    return 0;
  }

  if (_state != PS4_AUTH_READY) {
    memset(buffer, 0, reqlen);
    buffer[0] = _nonce_id;
    writePs4AuthReportCrc(0xF1, buffer, 59);
    return reqlen;
  }

  memset(buffer, 0, reqlen);
  buffer[0] = _nonce_id;
  buffer[1] = _cur_auth_page;
  buffer[2] = 0;

  const uint16_t offset = _cur_auth_page * PS4_PAGE_SZ;
  uint16_t copy_len = PS4_PAGE_SZ;
  if (offset + copy_len > PS4_AUTH_BUFFER_SZ) {
    copy_len = PS4_AUTH_BUFFER_SZ - offset;
  }

  memcpy(buffer + 3, _auth_buffer + offset, copy_len);
  if (copy_len < PS4_PAGE_SZ) {
    memset(buffer + 3 + copy_len, 0, PS4_PAGE_SZ - copy_len);
  }

  writePs4AuthReportCrc(0xF1, buffer, 59);

  ++_cur_auth_page;
  if (_cur_auth_page >= PS4_AUTH_PAGES) {
    _state = PS4_AUTH_IDLE;
  }
  return reqlen;
}

uint16_t PS4Auth::getSigningState(uint8_t* buffer, uint16_t reqlen) {
  ++_f2_request_count;
  if (reqlen < 15) {
    return 0;
  }

  memset(buffer, 0, reqlen);
  buffer[0] = _nonce_id;
  buffer[1] = (_state == PS4_AUTH_READY) ? 0x00 : 0x10;
  writePs4AuthReportCrc(0xF2, buffer, 11);
  return reqlen;
}

uint16_t PS4Auth::getResetConfig(uint8_t* buffer, uint16_t reqlen) {
  ++_f3_request_count;
  _sign_pending = false;
  _nonce_pages_received = 0;
  _state = PS4_AUTH_IDLE;
  _cur_auth_page = 0;
  static const uint8_t config[] = { 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00 };
  const uint16_t copy = (reqlen < sizeof(config)) ? reqlen : sizeof(config);
  memcpy(buffer, config, copy);
  if (reqlen > copy) memset(buffer + copy, 0, reqlen - copy);
  return reqlen;
}

PS4AuthState PS4Auth::getState() const {
  return _state;
}

void PS4Auth::writeDiagnostics(Print& out) const {
  out.print(F("PS4AUTH KEY="));
  out.print(_key_loaded ? 1 : 0);
  out.print(F(" STATE="));
  out.print((int)_state);
  out.print(F(" NONCE="));
  out.print((int)_nonce_id);
  out.print(F(" MASK=0x"));
  out.print((int)_nonce_pages_received, HEX);
  out.print(F(" PAGE="));
  out.print((int)_cur_auth_page);
  out.print(F(" F0="));
  out.print(_f0_report_count);
  out.print(F(" CRCERR="));
  out.print(_f0_crc_error_count);
  out.print(F(" REJECT="));
  out.print(_f0_reject_count);
  out.print(F(" SIGN="));
  out.print(_sign_success_count);
  out.print('/');
  out.print(_sign_attempt_count);
  out.print(F(" SIGN_US="));
  out.print(_last_sign_us);
  out.print(F(" F1="));
  out.print(_f1_request_count);
  out.print(F(" F2="));
  out.print(_f2_request_count);
  out.print(F(" F3="));
  out.println(_f3_request_count);
}

void PS4Auth::loadEEPROM(uint16_t offset, uint8_t* dest, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    dest[i] = EEPROM.read(_eeprom_base + offset + i);
  }
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
