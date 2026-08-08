#pragma once

#include <stdint.h>

extern bool isAutoDetectMode;
#ifdef ENABLE_INPUT_AUTODETECT
extern bool pendingAutoDetectReboot;
#endif
extern uint16_t input_autodetect_last_ms;
extern uint8_t input_autodetect_last_port;
extern uint8_t input_autodetect_last_flags;
extern uint8_t input_auto_resolve_mode;
extern uint8_t input_auto_resolve_source;
extern uint8_t input_auto_resolve_port;
extern uint8_t input_autodetect_saturn_tap_port;
extern uint8_t input_autodetect_saturn_tap_ports;
extern uint32_t input_autodetect_saturn_tap_ms;
extern uint32_t autoInputResolvedGraceUntil;
extern uint32_t input_hotswap_next_detect_allowed_at;
extern uint32_t input_hotswap_disconnected_since;
extern bool input_hotswap_was_connected;
extern bool input_hotswap_initialized;
extern bool input_hotswap_reverted_to_auto_while_disconnected;
extern bool input_boot_autodetect_pending;
extern uint8_t input_reenum_reserved_port_count;

void setInputAutoDetectModeActive(bool active);

bool inputAutoDetectModeActive();

void clearInputAutoDetectBootPending();

bool inputAutoDetectBootPending();

void setInputReenumReservedPortCount(uint8_t port_count);

uint8_t inputReenumReservedPortCount();

void clearInputReenumReservedPortCount();

#ifdef ENABLE_INPUT_AUTODETECT
void setPendingAutoDetectReboot(bool pending);

bool inputAutoDetectRebootPending();
#endif

void recordInputAutodetectTiming(uint32_t elapsed_ms, uint8_t port, uint8_t flags);

void recordInputAutoResolve(uint8_t mode, uint8_t source, uint8_t port = 0xFF);

void recordInputAutoDetectSaturnTap(uint8_t port, uint8_t tap_ports);

bool inputAutoDetectRecentSaturnTap(uint8_t port, uint32_t now, uint32_t window_ms);

void setAutoInputResolvedGraceUntil(uint32_t expires_at_ms);

void clearAutoInputResolvedGrace();

bool inputAutoResolvedGraceActive(uint32_t now);

bool inputHotSwapWasConnected();

bool inputHotSwapInitialized();

bool inputHotSwapRevertedToAutoWhileDisconnected();

void clearInputHotSwapNextDetectAllowedAt();

bool inputHotSwapDetectDue(uint32_t now);

void scheduleInputHotSwapDetectAt(uint32_t next_detect_ms);

void scheduleInputHotSwapDetectAfter(uint32_t now, uint32_t interval_ms);

void markInputHotSwapConnected();

void initializeInputHotSwapDisconnected(uint32_t now);

void markInputHotSwapDisconnected(uint32_t now);

bool inputHotSwapDisconnectDelayElapsed(uint32_t now, uint32_t delay_ms);

void markInputHotSwapRevertedToAutoWhileDisconnected(bool reverted = true);
