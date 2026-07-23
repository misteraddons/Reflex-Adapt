#include "input_wii_trace.h"

#include "../../core/serial_command_parser.h"

#include <string.h>

#ifndef WII_TRACE_RING_SIZE
#define WII_TRACE_RING_SIZE 256
#endif

namespace {

struct WiiTraceSample {
  uint32_t seq;
  uint32_t time_ms;
  uint32_t duration_us;
  uint8_t event;
  uint8_t port;
  uint8_t ok;
  uint8_t fail_count;
  uint8_t controller_type;
};

WiiTraceSample wiiTraceRing[WII_TRACE_RING_SIZE];
uint16_t wiiTraceHead = 0;
uint16_t wiiTraceCount = 0;
uint32_t wiiTraceSeq = 0;
uint32_t wiiTraceThresholdUs = 3000;

const char* wiiTraceEventName(uint8_t event) {
  switch ((WiiTraceEvent)event) {
    case WII_TRACE_EVENT_CONNECT:
      return "CONNECT";
    case WII_TRACE_EVENT_UPDATE:
      return "UPDATE";
    case WII_TRACE_EVENT_DISCONNECT:
      return "DISCONNECT";
    case WII_TRACE_EVENT_PORT:
      return "PORT";
    case WII_TRACE_EVENT_POLL:
      return "POLL";
    case WII_TRACE_EVENT_STAGE_RAW:
      return "STAGE_RAW";
    case WII_TRACE_EVENT_FLUSH_RAW:
      return "FLUSH_RAW";
    default:
      return "UNKNOWN";
  }
}

bool wiiTraceShouldRecord(uint8_t event, uint32_t durationUs, uint8_t ok) {
  if (!g_wiiTraceEnabled) {
    return false;
  }
  if (event == WII_TRACE_EVENT_DISCONNECT) {
    return true;
  }
  if (event == WII_TRACE_EVENT_CONNECT && ok) {
    return true;
  }
  return durationUs >= wiiTraceThresholdUs;
}

void wiiTraceWriteStatus(Print& out) {
  out.print(F("WIITRACE EN="));
  out.print(g_wiiTraceEnabled ? 1 : 0);
  out.print(F(" COUNT="));
  out.print(wiiTraceCount);
  out.print(F(" CAP="));
  out.print((uint16_t)WII_TRACE_RING_SIZE);
  out.print(F(" SEQ="));
  out.print(wiiTraceSeq);
  out.print(F(" THRESH_US="));
  out.println(wiiTraceThresholdUs);
}

void wiiTraceDump(Print& out) {
  wiiTraceWriteStatus(out);
  const uint16_t start =
      (uint16_t)((wiiTraceHead + WII_TRACE_RING_SIZE - wiiTraceCount) %
                 WII_TRACE_RING_SIZE);
  for (uint16_t i = 0; i < wiiTraceCount; ++i) {
    const uint16_t index = (uint16_t)((start + i) % WII_TRACE_RING_SIZE);
    const WiiTraceSample& sample = wiiTraceRing[index];
    out.print(F("WIITRACE SEQ="));
    out.print(sample.seq);
    out.print(F(" T_MS="));
    out.print(sample.time_ms);
    out.print(F(" EVENT="));
    out.print(wiiTraceEventName(sample.event));
    out.print(F(" PORT="));
    out.print(sample.port);
    out.print(F(" DUR_US="));
    out.print(sample.duration_us);
    out.print(F(" OK="));
    out.print(sample.ok);
    out.print(F(" FAIL="));
    out.print(sample.fail_count);
    out.print(F(" TYPE="));
    out.println(sample.controller_type);
  }
}

}  // namespace

bool g_wiiTraceEnabled = false;

void wiiTraceClear() {
  wiiTraceHead = 0;
  wiiTraceCount = 0;
  wiiTraceSeq = 0;
  memset(wiiTraceRing, 0, sizeof(wiiTraceRing));
}

void wiiTraceRecord(uint8_t event, uint8_t port, uint32_t startUs, uint32_t endUs,
                    uint8_t ok, uint8_t failCount, uint8_t controllerType) {
  const uint32_t durationUs = endUs - startUs;
  if (!wiiTraceShouldRecord(event, durationUs, ok)) {
    return;
  }

  WiiTraceSample& sample = wiiTraceRing[wiiTraceHead];
  sample.seq = ++wiiTraceSeq;
  sample.time_ms = millis();
  sample.duration_us = durationUs;
  sample.event = event;
  sample.port = port;
  sample.ok = ok;
  sample.fail_count = failCount;
  sample.controller_type = controllerType;

  wiiTraceHead = (uint16_t)((wiiTraceHead + 1u) % WII_TRACE_RING_SIZE);
  if (wiiTraceCount < WII_TRACE_RING_SIZE) {
    ++wiiTraceCount;
  }
}

void wiiTraceRecordInstant(uint8_t event, uint8_t port, uint8_t ok,
                           uint8_t failCount, uint8_t controllerType) {
  const uint32_t now = micros();
  wiiTraceRecord(event, port, now, now, ok, failCount, controllerType);
}

bool handleWiiTraceCommand(const char* command, Print& out) {
  char* remainder = nullptr;
  if (!serialCommandStartsWith(command, "WII", &remainder)) {
    return false;
  }

  char* traceCommand = nullptr;
  if (!serialCommandStartsWith(remainder, "TRACE", &traceCommand)) {
    return false;
  }

  if (*traceCommand == '\0' ||
      serialTokenEquals(traceCommand, "STATUS") ||
      serialTokenEquals(traceCommand, "HELP") ||
      serialTokenEquals(traceCommand, "?")) {
    out.println(F("WIITRACE CMDS:WII TRACE ON [THRESH_US],OFF,CLEAR,DUMP,THRESH <US>"));
    wiiTraceWriteStatus(out);
    return true;
  }

  if (serialTokenEquals(traceCommand, "ON")) {
    char* value = traceCommand + 2;
    uint32_t threshold = 0;
    if (serialParseUint32Token(value, &threshold)) {
      wiiTraceThresholdUs = threshold;
    }
    wiiTraceClear();
    g_wiiTraceEnabled = true;
    out.println(F("OK:WIITRACE_ON"));
    wiiTraceWriteStatus(out);
    return true;
  }

  if (serialTokenEquals(traceCommand, "OFF")) {
    g_wiiTraceEnabled = false;
    out.println(F("OK:WIITRACE_OFF"));
    wiiTraceWriteStatus(out);
    return true;
  }

  if (serialTokenEquals(traceCommand, "CLEAR") ||
      serialTokenEquals(traceCommand, "CLR")) {
    wiiTraceClear();
    out.println(F("OK:WIITRACE_CLEAR"));
    return true;
  }

  if (serialTokenEquals(traceCommand, "DUMP")) {
    wiiTraceDump(out);
    out.println(F("OK:WIITRACE_DUMP"));
    return true;
  }

  char* thresholdText = nullptr;
  if (serialCommandStartsWith(traceCommand, "THRESH", &thresholdText) ||
      serialCommandStartsWith(traceCommand, "THRESHOLD", &thresholdText)) {
    uint32_t threshold = 0;
    if (!serialParseUint32Token(thresholdText, &threshold)) {
      out.println(F("ERR:BAD_WIITRACE_THRESHOLD"));
      return true;
    }
    wiiTraceThresholdUs = threshold;
    out.println(F("OK:WIITRACE_THRESHOLD"));
    wiiTraceWriteStatus(out);
    return true;
  }

  out.println(F("ERR:BAD_WIITRACE_CMD"));
  return true;
}
