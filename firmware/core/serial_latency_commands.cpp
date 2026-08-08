#include "../product_config.h"

#include "serial_latency_commands.h"
#include "serial_command_parser.h"

#include <Arduino.h>

#include "../platform/latency_trace_gpio.h"
#include "../platform/latency_test.h"
#include "../platform/latency_service_mask.h"

namespace {

bool readSerialToken(char*& text, char* token, size_t tokenSize) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || tokenSize == 0) {
    return false;
  }

  size_t index = 0;
  while (*text != '\0' && *text != ' ' && *text != '\t') {
    if (index + 1 < tokenSize) {
      token[index++] = *text;
    }
    ++text;
  }
  token[index] = '\0';
  text = serialSkipSpaces(text);
  return index != 0;
}

bool parseLatencyBooleanToken(char* text, bool* value) {
  text = serialSkipSpaces(text);
  if (serialTokenEquals(text, "1") ||
      serialTokenEquals(text, "ON") ||
      serialTokenEquals(text, "TRUE") ||
      serialTokenEquals(text, "YES") ||
      serialTokenEquals(text, "CONTROLLER") ||
      serialTokenEquals(text, "CTRL")) {
    *value = true;
    return true;
  }
  if (serialTokenEquals(text, "0") ||
      serialTokenEquals(text, "OFF") ||
      serialTokenEquals(text, "FALSE") ||
      serialTokenEquals(text, "NO") ||
      serialTokenEquals(text, "INTERNAL") ||
      serialTokenEquals(text, "NOCTRL")) {
    *value = false;
    return true;
  }
  return false;
}

bool parseLatencyHostToken(char* text, uint8_t* hostType) {
  text = serialSkipSpaces(text);
  if (serialTokenEquals(text, "0") ||
      serialTokenEquals(text, "INTERNAL") ||
      serialTokenEquals(text, "LOOP")) {
    *hostType = LATENCY_HOST_INTERNAL;
    return true;
  }
  if (serialTokenEquals(text, "1") ||
      serialTokenEquals(text, "PC")) {
    *hostType = LATENCY_HOST_PC;
    return true;
  }
  if (serialTokenEquals(text, "2") ||
      serialTokenEquals(text, "MISTER") ||
      serialTokenEquals(text, "FPGA")) {
    *hostType = LATENCY_HOST_MISTER;
    return true;
  }
  return false;
}

bool parseLatencyModeToken(char* text, uint8_t* mode) {
  text = serialSkipSpaces(text);
  if (serialTokenEquals(text, "OFF")) {
    *mode = LATENCY_MODE_OFF;
    return true;
  }
  if (serialTokenEquals(text, "ON") ||
      serialTokenEquals(text, "PULSE") ||
      serialTokenEquals(text, "SYNTH") ||
      serialTokenEquals(text, "SYNTH_INTERNAL") ||
      serialTokenEquals(text, "NOCTRL")) {
    *mode = LATENCY_MODE_SYNTH_INTERNAL;
    return true;
  }
  if (serialTokenEquals(text, "SYNTHPC") ||
      serialTokenEquals(text, "SYNTH_PC") ||
      serialTokenEquals(text, "NOCTRLPC")) {
    *mode = LATENCY_MODE_SYNTH_PC;
    return true;
  }
  if (serialTokenEquals(text, "SYNTHM") ||
      serialTokenEquals(text, "SYNTH_MISTER") ||
      serialTokenEquals(text, "NOCTRLMISTER")) {
    *mode = LATENCY_MODE_SYNTH_MISTER;
    return true;
  }
  if (serialTokenEquals(text, "TRIG") ||
      serialTokenEquals(text, "TRIG_INTERNAL") ||
      serialTokenEquals(text, "CTRL")) {
    *mode = LATENCY_MODE_TRIGGER_INTERNAL;
    return true;
  }
  if (serialTokenEquals(text, "TRIGPC") ||
      serialTokenEquals(text, "TRIG_PC") ||
      serialTokenEquals(text, "CTRLPC")) {
    *mode = LATENCY_MODE_TRIGGER_PC;
    return true;
  }
  if (serialTokenEquals(text, "TRIGM") ||
      serialTokenEquals(text, "TRIG_MISTER") ||
      serialTokenEquals(text, "CTRLMISTER")) {
    *mode = LATENCY_MODE_TRIGGER_MISTER;
    return true;
  }
  return false;
}

bool handleLatencyServiceCommand(char* text, Print& out) {
  if (*text == '\0' || serialTokenEquals(text, "STATUS")) {
    latencyServicePrintStatus(out);
    return true;
  }
  if (serialTokenEquals(text, "CLEAR") || serialTokenEquals(text, "RESET")) {
    latencyServiceClearDisabledMask();
    latencyServicePrintStatus(out);
    return true;
  }

  char token[24] = {};
  if (!readSerialToken(text, token, sizeof(token))) {
    out.println(F("ERR:BAD_LATENCY_SERVICE"));
    return true;
  }

  uint32_t service = 0;
  if (!latencyServiceNameToMask(token, &service)) {
    out.println(F("ERR:BAD_LATENCY_SERVICE_NAME"));
    return true;
  }

  if (serialTokenEquals(text, "OFF") || serialTokenEquals(text, "0") ||
      serialTokenEquals(text, "DISABLE")) {
    latencyServiceSetDisabled(service, true);
  } else if (serialTokenEquals(text, "ON") || serialTokenEquals(text, "1") ||
             serialTokenEquals(text, "ENABLE")) {
    latencyServiceSetDisabled(service, false);
  } else {
    out.println(F("ERR:BAD_LATENCY_SERVICE_STATE"));
    return true;
  }

  latencyServicePrintStatus(out);
  return true;
}

}

bool handleSerialLatencyCommand(char* text, Print& out) {
  char* remainder = nullptr;
  if (*text == '\0' || serialTokenEquals(text, "STATUS")) {
    latencyTest.writeStatus(out);
    return true;
  }
  if (serialTokenEquals(text, "HELP")) {
    out.print(F("LATENCY CMDS:LATENCY STATUS,LATENCY START [SAMPLES],LATENCY STOP,LATENCY CLEAR,LATENCY DUMP,LATENCY ENABLE <0|1>,LATENCY LOG <0|1>,LATENCY CTRL <0|1|INTERNAL|CONTROLLER>,LATENCY HOST <0|1|2|INTERNAL|PC|MISTER>,LATENCY MODE <SYNTH|TRIG|TRIGM>,LATENCY TRIGGER <LOW|HIGH>,LATENCY ACK [SEQ],LATENCY SVC <NAME> <ON|OFF>"));
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
    out.print(F(",LATENCY TRACE <ON|OFF|DUMP|CLEAR>"));
#endif
    out.println();
    return true;
  }

  if (serialCommandStartsWith(text, "SVC", &remainder) ||
      serialCommandStartsWith(text, "SERVICE", &remainder)) {
    return handleLatencyServiceCommand(remainder, out);
  }

#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
  if (serialCommandStartsWith(text, "TRACE", &remainder) ||
      serialCommandStartsWith(text, "PHASE", &remainder)) {
    if (*remainder == '\0' || serialTokenEquals(remainder, "STATUS")) {
      latencyPhaseTraceWriteStatus(out);
      return true;
    }
    if (serialTokenEquals(remainder, "ON") ||
        serialTokenEquals(remainder, "START")) {
      latencyPhaseTraceSetEnabled(true);
      latencyPhaseTraceWriteStatus(out);
      return true;
    }
    if (serialTokenEquals(remainder, "OFF") ||
        serialTokenEquals(remainder, "STOP")) {
      latencyPhaseTraceSetEnabled(false);
      latencyPhaseTraceWriteStatus(out);
      return true;
    }
    if (serialTokenEquals(remainder, "DUMP")) {
      latencyPhaseTraceDump(out);
      return true;
    }
    if (serialTokenEquals(remainder, "CLEAR") ||
        serialTokenEquals(remainder, "CLR")) {
      latencyPhaseTraceClear();
      latencyPhaseTraceWriteStatus(out);
      return true;
    }
    out.println(F("ERR:BAD_LATENCY_TRACE_CMD"));
    return true;
  }
#endif

  if (serialCommandStartsWith(text, "START", &remainder) ||
      serialCommandStartsWith(text, "RUN", &remainder)) {
    long rawSamples = 0;
    uint32_t sampleCount = LATENCY_DEFAULT_SAMPLE_COUNT;
    if (*remainder != '\0') {
      if (!serialParseLongToken(remainder, &rawSamples) || rawSamples < 1) {
        out.println(F("ERR:BAD_LATENCY_SAMPLES"));
        return true;
      }
      sampleCount = (uint32_t)rawSamples;
    }
    latencyTest.startBench(sampleCount);
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialTokenEquals(text, "STOP")) {
    latencyTest.stopBench();
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialTokenEquals(text, "CLEAR")) {
    latencyTest.clearSamples();
    out.println(F("OK:LATENCY_CLEAR"));
    return true;
  }

  if (serialTokenEquals(text, "DUMP")) {
    latencyTest.dumpSamples(out);
    return true;
  }

  if (serialCommandStartsWith(text, "ENABLE", &remainder) ||
      serialCommandStartsWith(text, "ON", &remainder) ||
      serialCommandStartsWith(text, "OFF", &remainder)) {
    bool enable = true;
    if (serialTokenEquals(text, "OFF")) {
      enable = false;
    } else if (*remainder != '\0') {
      long rawEnable = 0;
      if (!serialParseLongToken(remainder, &rawEnable)) {
        out.println(F("ERR:BAD_LATENCY_ENABLE"));
        return true;
      }
      enable = rawEnable != 0;
    }
    latencyTest.setEnabled(enable);
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialCommandStartsWith(text, "LOG", &remainder)) {
    long rawLog = 0;
    if (!serialParseLongToken(remainder, &rawLog)) {
      out.println(F("ERR:BAD_LATENCY_LOG"));
      return true;
    }
    latencyTest.setSerialLogEnabled(rawLog != 0);
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialCommandStartsWith(text, "MODE", &remainder)) {
    uint8_t mode = LATENCY_MODE_OFF;
    if (!parseLatencyModeToken(remainder, &mode)) {
      out.println(F("ERR:BAD_LATENCY_MODE"));
      return true;
    }
    latencyTest.setMode(mode);
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialCommandStartsWith(text, "CTRL", &remainder) ||
      serialCommandStartsWith(text, "CONTROLLER", &remainder)) {
    bool controllerInLoop = false;
    if (!parseLatencyBooleanToken(remainder, &controllerInLoop)) {
      out.println(F("ERR:BAD_LATENCY_CTRL"));
      return true;
    }
    latencyTest.setControllerInLoop(controllerInLoop);
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialCommandStartsWith(text, "HOST", &remainder)) {
    uint8_t hostType = LATENCY_HOST_INTERNAL;
    if (!parseLatencyHostToken(remainder, &hostType)) {
      out.println(F("ERR:BAD_LATENCY_HOST"));
      return true;
    }
    latencyTest.setHostType(hostType);
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialCommandStartsWith(text, "TRIGGER", &remainder) ||
      serialCommandStartsWith(text, "TRIG", &remainder) ||
      serialCommandStartsWith(text, "POLARITY", &remainder)) {
    if (serialTokenEquals(remainder, "HIGH") ||
        serialTokenEquals(remainder, "ACTIVE_HIGH") ||
        serialTokenEquals(remainder, "ACTIVEHIGH") ||
        serialTokenEquals(remainder, "1")) {
      latencyTest.setTriggerActiveHigh(true);
    } else if (serialTokenEquals(remainder, "LOW") ||
               serialTokenEquals(remainder, "ACTIVE_LOW") ||
               serialTokenEquals(remainder, "ACTIVELOW") ||
               serialTokenEquals(remainder, "0")) {
      latencyTest.setTriggerActiveHigh(false);
    } else {
      out.println(F("ERR:BAD_LATENCY_TRIGGER"));
      return true;
    }
    latencyTest.writeStatus(out);
    return true;
  }

  if (serialCommandStartsWith(text, "ACK", &remainder)) {
    long rawSeq = 0;
    uint32_t seq = 0;
    if (*remainder != '\0') {
      if (!serialParseLongToken(remainder, &rawSeq) || rawSeq < 0) {
        out.println(F("ERR:BAD_LATENCY_ACK"));
        return true;
      }
      seq = (uint32_t)rawSeq;
    }
    latencyTest.acknowledgeHost(seq);
    out.println(F("OK:LATENCY_ACK"));
    return true;
  }

  out.println(F("ERR:BAD_LATENCY_CMD"));
  return true;
}
