#pragma once

// PS4 auth key blob layout (1712 bytes at AUTH_KEY_EEPROM_BASE + AUTH_KEY_PS4_OFFSET)
// Format matches the fixed-width GP2040-CE raw key binary:
//   serial(16) + signature(256) + N(256) + E(256) + D(256) +
//   P(128) + Q(128) + DP(128) + DQ(128) + QP(128) + reserved(32)

#define PS4_KEY_SERIAL_OFF    0
#define PS4_KEY_SERIAL_SZ     16
#define PS4_KEY_SIG_OFF       16
#define PS4_KEY_SIG_SZ        256
#define PS4_KEY_N_OFF         272
#define PS4_KEY_N_SZ          256
#define PS4_KEY_E_OFF         528
#define PS4_KEY_E_SZ          256
#define PS4_KEY_D_OFF         784
#define PS4_KEY_D_SZ          256
#define PS4_KEY_P_OFF         1040
#define PS4_KEY_P_SZ          128
#define PS4_KEY_Q_OFF         1168
#define PS4_KEY_Q_SZ          128
#define PS4_KEY_DP_OFF        1296
#define PS4_KEY_DP_SZ         128
#define PS4_KEY_DQ_OFF        1424
#define PS4_KEY_DQ_SZ         128
#define PS4_KEY_QP_OFF        1552
#define PS4_KEY_QP_SZ         128
