#include "latency_trace_gpio.h"

#include <string.h>

namespace {

#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
LatencyPhaseTraceSample phaseTraceRing[LATENCY_PHASE_TRACE_RING_SIZE];
uint16_t phaseTraceHead = 0;
uint16_t phaseTraceCount = 0;
uint32_t phaseTraceSeq = 0;
#endif

}  // namespace

bool latencyPhaseTraceEnabled = false;

bool latencyTracePhaseShouldRecord(uint8_t phase) {
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
#if defined(ENABLE_N64_LATENCY_FOCUSED_TRACE)
  switch ((LatencyTracePhase)phase) {
    case LATENCY_TRACE_PHASE_POLL:
    case LATENCY_TRACE_PHASE_PROCESS:
    case LATENCY_TRACE_PHASE_PREPARE:
    case LATENCY_TRACE_PHASE_SEND:
    case LATENCY_TRACE_PHASE_USB_READY:
    case LATENCY_TRACE_PHASE_USB_BUILD:
    case LATENCY_TRACE_PHASE_USB_SUBMIT:
    case LATENCY_TRACE_PHASE_USB_NOT_READY:
    case LATENCY_TRACE_PHASE_GC64_UPDATE:
    case LATENCY_TRACE_PHASE_JOYBUS_PIO_TXRX:
    case LATENCY_TRACE_PHASE_JOYBUS_INFO:
    case LATENCY_TRACE_PHASE_N64_READ:
    case LATENCY_TRACE_PHASE_N64_ACCESSORY:
    case LATENCY_TRACE_PHASE_N64_RUMBLE:
    case LATENCY_TRACE_PHASE_N64_MAP:
    case LATENCY_TRACE_PHASE_DREAMCAST_UPDATE:
    case LATENCY_TRACE_PHASE_DREAMCAST_LABEL:
    case LATENCY_TRACE_PHASE_DREAMCAST_MAP:
    case LATENCY_TRACE_PHASE_DREAMCAST_WEBHID:
    case LATENCY_TRACE_PHASE_MAPLE_TX:
    case LATENCY_TRACE_PHASE_MAPLE_POSTTX_GPIO:
    case LATENCY_TRACE_PHASE_MAPLE_READ:
    case LATENCY_TRACE_PHASE_PSX_READ:
    case LATENCY_TRACE_PHASE_PSX_MAP:
      return true;
    case LATENCY_TRACE_PHASE_NONE:
    default:
      return false;
  }
#else
  return phase != LATENCY_TRACE_PHASE_NONE;
#endif
#else
  (void)phase;
  return false;
#endif
}

const char* latencyTracePhaseName(uint8_t phase) {
  switch ((LatencyTracePhase)phase) {
    case LATENCY_TRACE_PHASE_POLL:
      return "POLL";
    case LATENCY_TRACE_PHASE_PROCESS:
      return "PROCESS";
    case LATENCY_TRACE_PHASE_PREPARE:
      return "PREPARE";
    case LATENCY_TRACE_PHASE_SEND:
      return "SEND";
    case LATENCY_TRACE_PHASE_UI:
      return "UI";
    case LATENCY_TRACE_PHASE_SNES_UPDATE:
      return "SNES_UPDATE";
    case LATENCY_TRACE_PHASE_SNES_MAP:
      return "SNES_MAP";
    case LATENCY_TRACE_PHASE_NEOGEO_READ:
      return "NEOGEO_READ";
    case LATENCY_TRACE_PHASE_NEOGEO_MAP:
      return "NEOGEO_MAP";
    case LATENCY_TRACE_PHASE_OUTPUT_BG:
      return "OUTPUT_BG";
    case LATENCY_TRACE_PHASE_PLATFORM_BG:
      return "PLATFORM_BG";
    case LATENCY_TRACE_PHASE_TURBO:
      return "TURBO";
    case LATENCY_TRACE_PHASE_PREPOLL_UI:
      return "PREPOLL_UI";
    case LATENCY_TRACE_PHASE_PENDING_OUTPUT:
      return "PENDING_OUTPUT";
    case LATENCY_TRACE_PHASE_MODE_BUTTON:
      return "MODE_BUTTON";
    case LATENCY_TRACE_PHASE_RESET_BUTTON:
      return "RESET_BUTTON";
    case LATENCY_TRACE_PHASE_MENU_HANDLE:
      return "MENU_HANDLE";
    case LATENCY_TRACE_PHASE_USB_FEEDBACK:
      return "USB_FEEDBACK";
    case LATENCY_TRACE_PHASE_USB_READY:
      return "USB_READY";
    case LATENCY_TRACE_PHASE_USB_BUILD:
      return "USB_BUILD";
    case LATENCY_TRACE_PHASE_USB_SUBMIT:
      return "USB_SUBMIT";
    case LATENCY_TRACE_PHASE_USB_NOT_READY:
      return "USB_NOT_READY";
    case LATENCY_TRACE_PHASE_GC64_UPDATE:
      return "GC64_UPDATE";
    case LATENCY_TRACE_PHASE_JOYBUS_PIO_TXRX:
      return "JOYBUS_PIO_TXRX";
    case LATENCY_TRACE_PHASE_JOYBUS_INFO:
      return "JOYBUS_INFO";
    case LATENCY_TRACE_PHASE_N64_READ:
      return "N64_READ";
    case LATENCY_TRACE_PHASE_N64_ACCESSORY:
      return "N64_ACCESSORY";
    case LATENCY_TRACE_PHASE_N64_RUMBLE:
      return "N64_RUMBLE";
    case LATENCY_TRACE_PHASE_N64_MAP:
      return "N64_MAP";
    case LATENCY_TRACE_PHASE_DREAMCAST_UPDATE:
      return "DC_UPDATE";
    case LATENCY_TRACE_PHASE_DREAMCAST_LABEL:
      return "DC_LABEL";
    case LATENCY_TRACE_PHASE_DREAMCAST_MAP:
      return "DC_MAP";
    case LATENCY_TRACE_PHASE_DREAMCAST_WEBHID:
      return "DC_WEBHID";
    case LATENCY_TRACE_PHASE_MAPLE_TX:
      return "MAPLE_TX";
    case LATENCY_TRACE_PHASE_MAPLE_POSTTX_GPIO:
      return "MAPLE_POSTTX_GPIO";
    case LATENCY_TRACE_PHASE_MAPLE_READ:
      return "MAPLE_READ";
    case LATENCY_TRACE_PHASE_PSX_READ:
      return "PSX_READ";
    case LATENCY_TRACE_PHASE_PSX_MAP:
      return "PSX_MAP";
    case LATENCY_TRACE_PHASE_NONE:
    default:
      return "NONE";
  }
}

void latencyPhaseTraceClear() {
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
  phaseTraceHead = 0;
  phaseTraceCount = 0;
  phaseTraceSeq = 0;
  memset(phaseTraceRing, 0, sizeof(phaseTraceRing));
#endif
}

void latencyPhaseTraceSetEnabled(bool enabled) {
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
  if (enabled) {
    latencyPhaseTraceClear();
  }
  latencyPhaseTraceEnabled = enabled;
#else
  (void)enabled;
  latencyPhaseTraceEnabled = false;
#endif
}

void latencyPhaseTraceRecord(uint8_t phase, uint32_t startUs, uint32_t endUs) {
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
  if (!latencyPhaseTraceEnabled || !latencyTracePhaseShouldRecord(phase)) {
    return;
  }

  LatencyPhaseTraceSample& sample = phaseTraceRing[phaseTraceHead];
  sample.seq = ++phaseTraceSeq;
  sample.phase = phase;
  sample.start_us = startUs;
  sample.duration_us = endUs - startUs;

  phaseTraceHead = (uint16_t)((phaseTraceHead + 1u) % LATENCY_PHASE_TRACE_RING_SIZE);
  if (phaseTraceCount < LATENCY_PHASE_TRACE_RING_SIZE) {
    ++phaseTraceCount;
  }
#else
  (void)phase;
  (void)startUs;
  (void)endUs;
#endif
}

void latencyPhaseTraceWriteStatus(Print& out) {
  out.print(F("LATTRACE EN="));
  out.print(latencyPhaseTraceEnabled ? 1 : 0);
  out.print(F(" COUNT="));
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
  out.print(phaseTraceCount);
  out.print(F(" CAP="));
  out.print((uint16_t)LATENCY_PHASE_TRACE_RING_SIZE);
  out.print(F(" SEQ="));
  out.println(phaseTraceSeq);
#else
  out.print(0);
  out.print(F(" CAP="));
  out.print(0);
  out.print(F(" SEQ="));
  out.println(0);
#endif
}

void latencyPhaseTraceDump(Print& out) {
  latencyPhaseTraceWriteStatus(out);
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
  const uint16_t start = (uint16_t)((phaseTraceHead + LATENCY_PHASE_TRACE_RING_SIZE - phaseTraceCount) % LATENCY_PHASE_TRACE_RING_SIZE);
  for (uint16_t i = 0; i < phaseTraceCount; ++i) {
    const uint16_t index = (uint16_t)((start + i) % LATENCY_PHASE_TRACE_RING_SIZE);
    const LatencyPhaseTraceSample& sample = phaseTraceRing[index];
    out.print(F("LATTRACE SEQ="));
    out.print(sample.seq);
    out.print(F(" PHASE="));
    out.print(latencyTracePhaseName(sample.phase));
    out.print(F(" START="));
    out.print(sample.start_us);
    out.print(F(" DUR="));
    out.println(sample.duration_us);
  }
#else
  (void)out;
#endif
}

#if defined(ENABLE_LATENCY_TRACE_GPIO)

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/gpio.h>
#endif

namespace {

constexpr uint8_t kMaxRp2040Gpio = 29;

bool latencyTracePinValid(uint8_t pin) {
  return pin != LATENCY_TRACE_PIN_UNUSED && pin <= kMaxRp2040Gpio;
}

void latencyTraceGpioConfigure(uint8_t pin) {
  if (!latencyTracePinValid(pin)) {
    return;
  }

#if defined(ARDUINO_ARCH_RP2040)
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, 0);
#else
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
#endif
}

}  // namespace

void latencyTraceGpioBegin() {
  latencyTraceGpioConfigure(PIN_LATENCY_TRACE_POLL);
  latencyTraceGpioConfigure(PIN_LATENCY_TRACE_PROCESS);
  latencyTraceGpioConfigure(PIN_LATENCY_TRACE_PREPARE);
  latencyTraceGpioConfigure(PIN_LATENCY_TRACE_SEND);
}

void latencyTraceGpioWrite(uint8_t pin, bool high) {
  if (!latencyTracePinValid(pin)) {
    return;
  }

#if defined(ARDUINO_ARCH_RP2040)
  gpio_put(pin, high ? 1 : 0);
#else
  digitalWrite(pin, high ? HIGH : LOW);
#endif
}

#endif
