#include "latency_service_mask.h"

#include <string.h>

namespace {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
uint32_t g_latency_service_disabled_mask = 0;

struct ServiceName {
  const char* name;
  uint32_t mask;
};

const ServiceName kServiceNames[] = {
  { "serial", LATENCY_SERVICE_SERIAL_LOG },
  { "log", LATENCY_SERVICE_SERIAL_LOG },
  { "adaptstate", LATENCY_SERVICE_ADAPT_STATE },
  { "state", LATENCY_SERVICE_ADAPT_STATE },
  { "overlay", LATENCY_SERVICE_INPUT_OVERLAY },
  { "oledserial", LATENCY_SERVICE_OLED_SERIAL },
  { "tty2oled", LATENCY_SERVICE_OLED_SERIAL },
  { "outputbg", LATENCY_SERVICE_OUTPUT_BG },
  { "platformbg", LATENCY_SERVICE_PLATFORM_BG },
  { "turbo", LATENCY_SERVICE_TURBO_TASK },
  { "prepollui", LATENCY_SERVICE_PREPOLL_UI },
  { "postpollui", LATENCY_SERVICE_POSTPOLL_UI },
  { "pendingoutput", LATENCY_SERVICE_PENDING_OUTPUT },
  { "menuhotkeys", LATENCY_SERVICE_MENU_HOTKEYS },
  { "remap", LATENCY_SERVICE_BUTTON_REMAP },
  { "chord", LATENCY_SERVICE_CHORD_REMAP },
  { "turboxform", LATENCY_SERVICE_TURBO_TRANSFORM },
  { "virtualhotkeys", LATENCY_SERVICE_VIRTUAL_HOTKEYS },
  { "socd", LATENCY_SERVICE_SOCD },
  { "cache", LATENCY_SERVICE_CONTROLLER_CACHE },
  { "controllercache", LATENCY_SERVICE_CONTROLLER_CACHE },
  { "idlewake", LATENCY_SERVICE_IDLE_WAKE },
};
#endif
}

#if defined(ADAPT_ENABLE_LATENCY_TEST)
bool latencyServiceDisabled(uint32_t service) {
  return (g_latency_service_disabled_mask & service) != 0;
}

uint32_t latencyServiceDisabledMask() {
  return g_latency_service_disabled_mask;
}
#endif

void latencyServiceSetDisabled(uint32_t service, bool disabled) {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  if (disabled) {
    g_latency_service_disabled_mask |= service;
  } else {
    g_latency_service_disabled_mask &= ~service;
  }
#else
  (void)service;
  (void)disabled;
#endif
}

void latencyServiceSetDisabledMask(uint32_t mask) {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  g_latency_service_disabled_mask = mask;
#else
  (void)mask;
#endif
}

void latencyServiceClearDisabledMask() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  g_latency_service_disabled_mask = 0;
#endif
}

bool latencyServiceNameToMask(const char* name, uint32_t* service) {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  if (name == nullptr || service == nullptr) {
    return false;
  }
  for (const ServiceName& entry : kServiceNames) {
    if (strcasecmp(name, entry.name) == 0) {
      *service = entry.mask;
      return true;
    }
  }
  return false;
#else
  (void)name;
  (void)service;
  return false;
#endif
}

void latencyServicePrintStatus(Print& out) {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  out.print(F("LATSVC MASK=0x"));
  out.println(g_latency_service_disabled_mask, HEX);
  for (const ServiceName& entry : kServiceNames) {
    out.print(F("LATSVC "));
    out.print(entry.name);
    out.print(F("="));
    out.println(latencyServiceDisabled(entry.mask) ? F("OFF") : F("ON"));
  }
#else
  out.println(F("LATSVC UNAVAILABLE"));
#endif
}
