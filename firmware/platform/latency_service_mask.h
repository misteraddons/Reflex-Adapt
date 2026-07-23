#pragma once

#include <Arduino.h>

enum LatencyServiceMask : uint32_t {
  LATENCY_SERVICE_SERIAL_LOG = 1u << 0,
  LATENCY_SERVICE_ADAPT_STATE = 1u << 1,
  LATENCY_SERVICE_INPUT_OVERLAY = 1u << 2,
  LATENCY_SERVICE_OLED_SERIAL = 1u << 3,
  LATENCY_SERVICE_OUTPUT_BG = 1u << 4,
  LATENCY_SERVICE_PLATFORM_BG = 1u << 5,
  LATENCY_SERVICE_TURBO_TASK = 1u << 6,
  LATENCY_SERVICE_PREPOLL_UI = 1u << 7,
  LATENCY_SERVICE_POSTPOLL_UI = 1u << 8,
  LATENCY_SERVICE_PENDING_OUTPUT = 1u << 9,
  LATENCY_SERVICE_MENU_HOTKEYS = 1u << 10,
  LATENCY_SERVICE_BUTTON_REMAP = 1u << 11,
  LATENCY_SERVICE_CHORD_REMAP = 1u << 12,
  LATENCY_SERVICE_TURBO_TRANSFORM = 1u << 13,
  LATENCY_SERVICE_VIRTUAL_HOTKEYS = 1u << 14,
  LATENCY_SERVICE_SOCD = 1u << 15,
  LATENCY_SERVICE_CONTROLLER_CACHE = 1u << 16,
  LATENCY_SERVICE_IDLE_WAKE = 1u << 17,
};

#if defined(ADAPT_ENABLE_LATENCY_TEST)
bool latencyServiceDisabled(uint32_t service);
uint32_t latencyServiceDisabledMask();
#else
__attribute__((always_inline)) inline bool latencyServiceDisabled(uint32_t service) {
  (void)service;
  return false;
}

__attribute__((always_inline)) inline uint32_t latencyServiceDisabledMask() {
  return 0;
}
#endif
void latencyServiceSetDisabled(uint32_t service, bool disabled);
void latencyServiceSetDisabledMask(uint32_t mask);
void latencyServiceClearDisabledMask();
bool latencyServiceNameToMask(const char* name, uint32_t* service);
void latencyServicePrintStatus(Print& out);
