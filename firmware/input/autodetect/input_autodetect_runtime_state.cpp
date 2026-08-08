#include "../../product_config.h"

#include "input_autodetect_runtime_state.h"

#include <Arduino.h>

bool isAutoDetectMode = false;
#ifdef ENABLE_INPUT_AUTODETECT
bool pendingAutoDetectReboot = false;
#endif
uint16_t input_autodetect_last_ms = 0;
uint8_t input_autodetect_last_port = 0xFF;
uint8_t input_autodetect_last_flags = 0;
uint8_t input_auto_resolve_mode = 0;
uint8_t input_auto_resolve_source = 0;
uint8_t input_auto_resolve_port = 0xFF;
uint8_t input_autodetect_saturn_tap_port = 0xFF;
uint8_t input_autodetect_saturn_tap_ports = 0;
uint32_t input_autodetect_saturn_tap_ms = 0;
uint32_t autoInputResolvedGraceUntil = 0;
uint32_t input_hotswap_next_detect_allowed_at = 0;
uint32_t input_hotswap_disconnected_since = 0;
bool input_hotswap_was_connected = false;
bool input_hotswap_initialized = false;
bool input_hotswap_reverted_to_auto_while_disconnected = false;
bool input_boot_autodetect_pending = true;
uint8_t input_reenum_reserved_port_count = 0;

void setInputAutoDetectModeActive(bool active) {
  isAutoDetectMode = active;
}

bool inputAutoDetectModeActive() {
  return isAutoDetectMode;
}

void clearInputAutoDetectBootPending() {
  input_boot_autodetect_pending = false;
}

bool inputAutoDetectBootPending() {
  return input_boot_autodetect_pending;
}

void setInputReenumReservedPortCount(uint8_t port_count) {
  input_reenum_reserved_port_count = port_count;
}

uint8_t inputReenumReservedPortCount() {
  return input_reenum_reserved_port_count;
}

void clearInputReenumReservedPortCount() {
  setInputReenumReservedPortCount(0);
}

#ifdef ENABLE_INPUT_AUTODETECT
void setPendingAutoDetectReboot(bool pending) {
  pendingAutoDetectReboot = pending;
}

bool inputAutoDetectRebootPending() {
  return pendingAutoDetectReboot;
}
#endif

void recordInputAutodetectTiming(uint32_t elapsed_ms, uint8_t port, uint8_t flags) {
  input_autodetect_last_ms = (elapsed_ms > 65535u) ? 65535u : (uint16_t)elapsed_ms;
  input_autodetect_last_port = port;
  input_autodetect_last_flags = flags;
}

void recordInputAutoResolve(uint8_t mode, uint8_t source, uint8_t port) {
  input_auto_resolve_mode = mode;
  input_auto_resolve_source = source;
  input_auto_resolve_port = port;
}

void recordInputAutoDetectSaturnTap(uint8_t port, uint8_t tap_ports) {
  input_autodetect_saturn_tap_port = port;
  input_autodetect_saturn_tap_ports = tap_ports;
  input_autodetect_saturn_tap_ms = millis();
}

bool inputAutoDetectRecentSaturnTap(uint8_t port, uint32_t now, uint32_t window_ms) {
  return input_autodetect_saturn_tap_ports != 0 &&
         input_autodetect_saturn_tap_port == port &&
         (uint32_t)(now - input_autodetect_saturn_tap_ms) <= window_ms;
}

void setAutoInputResolvedGraceUntil(uint32_t expires_at_ms) {
  autoInputResolvedGraceUntil = expires_at_ms;
}

void clearAutoInputResolvedGrace() {
  setAutoInputResolvedGraceUntil(0);
}

bool inputAutoResolvedGraceActive(uint32_t now) {
  return autoInputResolvedGraceUntil != 0 &&
         (int32_t)(now - autoInputResolvedGraceUntil) < 0;
}

bool inputHotSwapWasConnected() {
  return input_hotswap_was_connected;
}

bool inputHotSwapInitialized() {
  return input_hotswap_initialized;
}

bool inputHotSwapRevertedToAutoWhileDisconnected() {
  return input_hotswap_reverted_to_auto_while_disconnected;
}

void clearInputHotSwapNextDetectAllowedAt() {
  input_hotswap_next_detect_allowed_at = 0;
}

bool inputHotSwapDetectDue(uint32_t now) {
  return input_hotswap_next_detect_allowed_at == 0 ||
         (int32_t)(now - input_hotswap_next_detect_allowed_at) >= 0;
}

void scheduleInputHotSwapDetectAt(uint32_t next_detect_ms) {
  input_hotswap_next_detect_allowed_at = next_detect_ms;
}

void scheduleInputHotSwapDetectAfter(uint32_t now, uint32_t interval_ms) {
  scheduleInputHotSwapDetectAt(now + interval_ms);
}

void markInputHotSwapConnected() {
  input_hotswap_was_connected = true;
  input_hotswap_disconnected_since = 0;
  input_hotswap_initialized = true;
  input_hotswap_reverted_to_auto_while_disconnected = false;
}

void initializeInputHotSwapDisconnected(uint32_t now) {
  input_hotswap_initialized = true;
  input_hotswap_disconnected_since = now;
  clearInputHotSwapNextDetectAllowedAt();
}

void markInputHotSwapDisconnected(uint32_t now) {
  input_hotswap_was_connected = false;
  input_hotswap_disconnected_since = now;
  clearInputHotSwapNextDetectAllowedAt();
  input_hotswap_reverted_to_auto_while_disconnected = false;
}

bool inputHotSwapDisconnectDelayElapsed(uint32_t now, uint32_t delay_ms) {
  return input_hotswap_disconnected_since != 0 &&
         (now - input_hotswap_disconnected_since) >= delay_ms;
}

void markInputHotSwapRevertedToAutoWhileDisconnected(bool reverted) {
  input_hotswap_reverted_to_auto_while_disconnected = reverted;
}
