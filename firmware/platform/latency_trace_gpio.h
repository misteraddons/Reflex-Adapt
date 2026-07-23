#pragma once

#include <Arduino.h>

#include "latency_test.h"

#ifndef LATENCY_TRACE_PIN_UNUSED
#define LATENCY_TRACE_PIN_UNUSED LATENCY_PIN_UNUSED
#endif

// Trace GPIOs are intentionally unassigned by default. Classic2USB GPIOs are
// mostly live controller, OLED, button, or LED pins, so trace builds must opt in
// with known-safe test pads or temporary wiring.
#ifndef PIN_LATENCY_TRACE_POLL
#define PIN_LATENCY_TRACE_POLL LATENCY_TRACE_PIN_UNUSED
#endif

#ifndef PIN_LATENCY_TRACE_PROCESS
#define PIN_LATENCY_TRACE_PROCESS LATENCY_TRACE_PIN_UNUSED
#endif

#ifndef PIN_LATENCY_TRACE_PREPARE
#define PIN_LATENCY_TRACE_PREPARE LATENCY_TRACE_PIN_UNUSED
#endif

#ifndef PIN_LATENCY_TRACE_SEND
#define PIN_LATENCY_TRACE_SEND LATENCY_TRACE_PIN_UNUSED
#endif

#ifndef LATENCY_PHASE_TRACE_RING_SIZE
#define LATENCY_PHASE_TRACE_RING_SIZE 256
#endif

enum LatencyTracePhase : uint8_t {
  LATENCY_TRACE_PHASE_NONE = 0,
  LATENCY_TRACE_PHASE_POLL,
  LATENCY_TRACE_PHASE_PROCESS,
  LATENCY_TRACE_PHASE_PREPARE,
  LATENCY_TRACE_PHASE_SEND,
  LATENCY_TRACE_PHASE_UI,
  LATENCY_TRACE_PHASE_SNES_UPDATE,
  LATENCY_TRACE_PHASE_SNES_MAP,
  LATENCY_TRACE_PHASE_NEOGEO_READ,
  LATENCY_TRACE_PHASE_NEOGEO_MAP,
  LATENCY_TRACE_PHASE_OUTPUT_BG,
  LATENCY_TRACE_PHASE_PLATFORM_BG,
  LATENCY_TRACE_PHASE_TURBO,
  LATENCY_TRACE_PHASE_PREPOLL_UI,
  LATENCY_TRACE_PHASE_PENDING_OUTPUT,
  LATENCY_TRACE_PHASE_MODE_BUTTON,
  LATENCY_TRACE_PHASE_RESET_BUTTON,
  LATENCY_TRACE_PHASE_MENU_HANDLE,
  LATENCY_TRACE_PHASE_USB_FEEDBACK,
  LATENCY_TRACE_PHASE_USB_READY,
  LATENCY_TRACE_PHASE_USB_BUILD,
  LATENCY_TRACE_PHASE_USB_SUBMIT,
  LATENCY_TRACE_PHASE_USB_NOT_READY,
  LATENCY_TRACE_PHASE_GC64_UPDATE,
  LATENCY_TRACE_PHASE_JOYBUS_PIO_TXRX,
  LATENCY_TRACE_PHASE_JOYBUS_INFO,
  LATENCY_TRACE_PHASE_N64_READ,
  LATENCY_TRACE_PHASE_N64_ACCESSORY,
  LATENCY_TRACE_PHASE_N64_RUMBLE,
  LATENCY_TRACE_PHASE_N64_MAP,
  LATENCY_TRACE_PHASE_DREAMCAST_UPDATE,
  LATENCY_TRACE_PHASE_DREAMCAST_LABEL,
  LATENCY_TRACE_PHASE_DREAMCAST_MAP,
  LATENCY_TRACE_PHASE_DREAMCAST_WEBHID,
  LATENCY_TRACE_PHASE_MAPLE_TX,
  LATENCY_TRACE_PHASE_MAPLE_POSTTX_GPIO,
  LATENCY_TRACE_PHASE_MAPLE_READ,
  LATENCY_TRACE_PHASE_PSX_READ,
  LATENCY_TRACE_PHASE_PSX_MAP,
};

struct LatencyPhaseTraceSample {
  uint32_t seq;
  uint32_t start_us;
  uint32_t duration_us;
  uint8_t phase;
};

extern bool latencyPhaseTraceEnabled;

const char* latencyTracePhaseName(uint8_t phase);
void latencyPhaseTraceSetEnabled(bool enabled);
void latencyPhaseTraceClear();
void latencyPhaseTraceRecord(uint8_t phase, uint32_t startUs, uint32_t endUs);
void latencyPhaseTraceWriteStatus(Print& out);
void latencyPhaseTraceDump(Print& out);

#if defined(ENABLE_LATENCY_TRACE_GPIO)
void latencyTraceGpioBegin();
void latencyTraceGpioWrite(uint8_t pin, bool high);
#else
inline void latencyTraceGpioBegin() {}
inline void latencyTraceGpioWrite(uint8_t, bool) {}
#endif

class LatencyTraceGpioScope {
public:
  explicit LatencyTraceGpioScope(uint8_t tracePin) : pin(tracePin) {
    latencyTraceGpioWrite(pin, true);
  }

  ~LatencyTraceGpioScope() {
    latencyTraceGpioWrite(pin, false);
  }

private:
  uint8_t pin;
};

#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
class LatencyPhaseTraceScope {
public:
  explicit LatencyPhaseTraceScope(uint8_t tracePhase)
    : phase(tracePhase),
      startUs(0),
      active(latencyPhaseTraceEnabled && tracePhase != LATENCY_TRACE_PHASE_NONE) {
    if (active) {
      startUs = micros();
    }
  }

  ~LatencyPhaseTraceScope() {
    if (active) {
      latencyPhaseTraceRecord(phase, startUs, micros());
    }
  }

  void cancel() {
    active = false;
  }

private:
  uint8_t phase;
  uint32_t startUs;
  bool active;
};
#else
class LatencyPhaseTraceScope {
public:
  explicit LatencyPhaseTraceScope(uint8_t) {}
  void cancel() {}
};
#endif
