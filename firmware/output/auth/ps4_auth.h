#pragma once

// PS4 Authentication using BearSSL RSA-PSS-SHA256
//
// Handles the PS4 challenge-response authentication protocol:
// - PS4 sends 256-byte nonce via SET_REPORT(0xF0) in 5 pages of 56 bytes
// - Controller signs nonce with RSA-2048-PSS-SHA256 using stored private key
// - Controller responds via GET_REPORT(0xF1) with signed response in 19 pages
// - PS4 also queries GET_REPORT(0xF2) for signing state and GET_REPORT(0xF3) for reset/config
//
// Uses BearSSL i15 engine (optimized for Cortex-M0+, no 64-bit multiply needed)
// Pre-compiled libbearssl.a is linked by default in Arduino-Pico

#include <Arduino.h>
#include <EEPROM.h>

extern "C" {
#include <bearssl/bearssl_rsa.h>
#include <bearssl/bearssl_hash.h>
#include <bearssl/bearssl_rand.h>
}

#include "ps4_key_layout.h"

#define PS4_AUTH_BUFFER_SZ    1064   // Signed response to PS4
#define PS4_NONCE_SZ          256    // Challenge nonce from PS4
#define PS4_NONCE_PAGES       5      // ceil(256/56)
#define PS4_AUTH_PAGES        19     // ceil(1064/56)
#define PS4_PAGE_SZ           56     // Data bytes per page

enum PS4AuthState {
  PS4_AUTH_IDLE,
  PS4_AUTH_RECEIVING_NONCE,
  PS4_AUTH_SIGNING,
  PS4_AUTH_READY,
};

class PS4Auth {
public:
  bool begin(uint16_t eeprom_base);
  bool isReady() const;

  // SET_REPORT(0xF0): receive nonce page from PS4.
  // data points to the 63-byte report payload after the report ID:
  // nonce_id, page, reserved, 56-byte page data, CRC32.
  void receiveNoncePage(const uint8_t* data, uint16_t len);

  // Call from main loop. Performs RSA-PSS signing when nonce is complete.
  // This takes ~100-200ms on RP2040 — it's a heavy crypto operation.
  void process();

  // GET_REPORT(0xF1): return one page of signed auth response
  uint16_t getAuthPage(uint8_t* buffer, uint16_t reqlen);

  // GET_REPORT(0xF2): return signing state
  uint16_t getSigningState(uint8_t* buffer, uint16_t reqlen);

  // GET_REPORT(0xF3): return page config and reset auth state
  uint16_t getResetConfig(uint8_t* buffer, uint16_t reqlen);

  PS4AuthState getState() const;

private:
  static uint32_t crc32_calc(const uint8_t* data, size_t len);
  static uint32_t crc32_calc_with_report_id(uint8_t report_id, const uint8_t* data, size_t len);
  void loadEEPROM(uint16_t offset, uint8_t* dest, uint16_t len);

  uint16_t _eeprom_base = 0;
  bool _key_loaded = false;
  bool _sign_pending = false;
  PS4AuthState _state = PS4_AUTH_IDLE;
  uint8_t _nonce_id = 0;
  uint8_t _cur_auth_page = 0;

  // Key components loaded from EEPROM
  uint8_t _serial[PS4_KEY_SERIAL_SZ];
  uint8_t _dev_sig[PS4_KEY_SIG_SZ];
  uint8_t _rsa_n[PS4_KEY_N_SZ];
  uint8_t _rsa_e[PS4_KEY_E_SZ];
  uint8_t _rsa_p[PS4_KEY_P_SZ];
  uint8_t _rsa_q[PS4_KEY_Q_SZ];
  uint8_t _rsa_dp[PS4_KEY_DP_SZ];
  uint8_t _rsa_dq[PS4_KEY_DQ_SZ];
  uint8_t _rsa_qp[PS4_KEY_QP_SZ];

  // BearSSL RSA private key (CRT form, points to buffers above)
  br_rsa_private_key _sk;
  br_hmac_drbg_context _rng;

  // Runtime buffers
  uint8_t _nonce[PS4_NONCE_SZ];
  uint8_t _auth_buffer[PS4_AUTH_BUFFER_SZ];
};
