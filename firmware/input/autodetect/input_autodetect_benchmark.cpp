#include "../../product_config.h"

#include "input_autodetect_benchmark.h"

#include "../../core/controller_frame_state.h"
#include "../../core/device_runtime_state.h"
#include "../../core/serial_command_parser.h"
#include "../../output/output_runtime_state.h"
#include "input_autodetect_runtime.h"
#include "input_autodetect_runtime_state.h"
#include "../runtime/input_frame_runtime.h"

namespace {

constexpr uint8_t kAutoDetectBenchmarkCapacity = 96;

struct AutoDetectBenchmarkRecord {
  uint32_t ms;
  uint8_t event;
  uint8_t input_mode;
  uint8_t saved_input_mode;
  uint8_t output_mode;
  uint8_t configured_output_mode;
  uint8_t max_usb;
  uint8_t input_ports;
  uint8_t aux;
  uint16_t value;
};

AutoDetectBenchmarkRecord records[kAutoDetectBenchmarkCapacity];
uint16_t next_sequence = 0;
uint8_t record_count = 0;
uint8_t write_index = 0;

const char* benchmarkEventName(uint8_t event) {
  switch ((AutoDetectBenchmarkEvent)event) {
    case ADBENCH_BOOT_SETUP_START: return "BOOT_SETUP_START";
    case ADBENCH_BOOT_SPLASH: return "BOOT_SPLASH";
    case ADBENCH_BOOT_INPUT_START: return "BOOT_INPUT_START";
    case ADBENCH_BOOT_INPUT_END: return "BOOT_INPUT_END";
    case ADBENCH_BOOT_OUTPUT_START: return "BOOT_OUTPUT_START";
    case ADBENCH_BOOT_OUTPUT_END: return "BOOT_OUTPUT_END";
    case ADBENCH_USB_CONFIG_START: return "USB_CONFIG_START";
    case ADBENCH_USB_CONFIG_END: return "USB_CONFIG_END";
    case ADBENCH_USB_CONNECT: return "USB_CONNECT";
    case ADBENCH_BOOT_SETUP_DONE: return "BOOT_SETUP_DONE";
    case ADBENCH_DETECT_START: return "DETECT_START";
    case ADBENCH_DETECT_END: return "DETECT_END";
    case ADBENCH_HOTSWAP_CONNECTED_EDGE: return "HOTSWAP_CONNECTED_EDGE";
    case ADBENCH_HOTSWAP_DISCONNECTED_EDGE: return "HOTSWAP_DISCONNECTED_EDGE";
    case ADBENCH_MODE_APPLY_START: return "MODE_APPLY_START";
    case ADBENCH_MODE_APPLY_END: return "MODE_APPLY_END";
    case ADBENCH_AUTO_HOME_RESTORE: return "AUTO_HOME_RESTORE";
    case ADBENCH_DESCRIPTOR_REENUM_REQUEST: return "DESCRIPTOR_REENUM_REQUEST";
    case ADBENCH_BOOT_AUTO_HOME_RESTORE: return "BOOT_AUTO_HOME_RESTORE";
    case ADBENCH_BOOT_INPUT_RESTORE: return "BOOT_INPUT_RESTORE";
    case ADBENCH_BOOT_USB_IDENTITY_RESTORE: return "BOOT_USB_IDENTITY_RESTORE";
    case ADBENCH_MAIN_REFRESH: return "MAIN_REFRESH";
    case ADBENCH_MODE_REJECT: return "MODE_REJECT";
    case ADBENCH_USER_MARK: return "USER_MARK";
    case ADBENCH_HOTSWAP_WAIT_FIRST_FRAME: return "HOTSWAP_WAIT_FIRST_FRAME";
    case ADBENCH_HOTSWAP_WAIT_DISCONNECT_DELAY: return "HOTSWAP_WAIT_DISCONNECT_DELAY";
    case ADBENCH_HOTSWAP_SKIP_IDLE_UI: return "HOTSWAP_SKIP_IDLE_UI";
    case ADBENCH_HOTSWAP_WAIT_DUE: return "HOTSWAP_WAIT_DUE";
    case ADBENCH_HOTSWAP_QUICK_SCAN_START: return "HOTSWAP_QUICK_SCAN_START";
    case ADBENCH_HOTSWAP_QUICK_SCAN_END: return "HOTSWAP_QUICK_SCAN_END";
    case ADBENCH_HOTSWAP_FULL_SCAN_START: return "HOTSWAP_FULL_SCAN_START";
    case ADBENCH_HOTSWAP_FULL_SCAN_END: return "HOTSWAP_FULL_SCAN_END";
    case ADBENCH_HOTSWAP_CONNECTED_SCAN_START: return "HOTSWAP_CONNECTED_SCAN_START";
    case ADBENCH_HOTSWAP_CONNECTED_SCAN_END: return "HOTSWAP_CONNECTED_SCAN_END";
    case ADBENCH_HOTSWAP_CHECK_ENTER: return "HOTSWAP_CHECK_ENTER";
    case ADBENCH_HOTSWAP_WAIT_AUTO_REVERTED: return "HOTSWAP_WAIT_AUTO_REVERTED";
    case ADBENCH_HOTSWAP_CLEAR_AUTO_REVERTED: return "HOTSWAP_CLEAR_AUTO_REVERTED";
    case ADBENCH_MODE_INIT_DONE: return "MODE_INIT_DONE";
    case ADBENCH_MODE_BEEP_QUEUED: return "MODE_BEEP_QUEUED";
    case ADBENCH_SERIAL_AUTO_REQUEST: return "SERIAL_AUTO_REQUEST";
  }
  return "UNKNOWN";
}

void queueAutoDetectRuntimeBenchmark() {
  deviceMode = RZORD_AUTODETECT;
  savedDeviceMode = RZORD_AUTODETECT;
  setInputAutoDetectModeActive(true);
  clearAutoDetectResult();
  clearAutoInputResolvedGrace();
  markInputHotSwapRevertedToAutoWhileDisconnected(true);
  scheduleInputHotSwapDetectAt(millis());
  autoDetectBenchmarkMark(ADBENCH_SERIAL_AUTO_REQUEST);
}

uint8_t currentEffectiveOutputMode() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  return (uint8_t)get_effective_output_mode();
  #else
  return (uint8_t)outputMode;
  #endif
}

uint8_t currentInputPortCount() {
  #ifdef ADAPT_INPUT_RETRO
  return inputPortCount();
  #else
  return 0;
  #endif
}

void printRecord(Print& out, uint16_t logical_index, const AutoDetectBenchmarkRecord& record) {
  out.print(F("ADBENCH IDX="));
  out.print(logical_index);
  out.print(F(" MS="));
  out.print(record.ms);
  out.print(F(" EVT="));
  out.print(benchmarkEventName(record.event));
  out.print(F(" EVN="));
  out.print((int)record.event);
  out.print(F(" IN="));
  out.print((int)record.input_mode);
  out.print(F(" SAVED="));
  out.print((int)record.saved_input_mode);
  out.print(F(" OUT="));
  out.print((int)record.output_mode);
  out.print(F(" CFGOUT="));
  out.print((int)record.configured_output_mode);
  out.print(F(" MAX="));
  out.print((int)record.max_usb);
  out.print(F(" PORTS="));
  out.print((int)record.input_ports);
  out.print(F(" AUX="));
  out.print((int)record.aux);
  out.print(F(" VAL="));
  out.println(record.value);
}

}  // namespace

void autoDetectBenchmarkReset() {
  record_count = 0;
  write_index = 0;
  next_sequence = 0;
}

void autoDetectBenchmarkMark(AutoDetectBenchmarkEvent event, uint8_t aux, uint16_t value) {
  AutoDetectBenchmarkRecord& record = records[write_index];
  record.ms = millis();
  record.event = (uint8_t)event;
  record.input_mode = (uint8_t)deviceMode;
  record.saved_input_mode = (uint8_t)savedDeviceMode;
  record.output_mode = currentEffectiveOutputMode();
  record.configured_output_mode = (uint8_t)configuredOutputMode;
  record.max_usb = max_devices;
  record.input_ports = currentInputPortCount();
  record.aux = aux;
  record.value = value;

  write_index = (uint8_t)((write_index + 1) % kAutoDetectBenchmarkCapacity);
  if (record_count < kAutoDetectBenchmarkCapacity) {
    ++record_count;
  }
  ++next_sequence;
}

void autoDetectBenchmarkDump(Print& out) {
  const uint16_t first_sequence = next_sequence - record_count;
  out.print(F("ADBENCH COUNT="));
  out.print((int)record_count);
  out.print(F(" CAP="));
  out.print((int)kAutoDetectBenchmarkCapacity);
  out.print(F(" SEQ="));
  out.println(next_sequence);
  for (uint8_t offset = 0; offset < record_count; ++offset) {
    const uint8_t index =
        (uint8_t)((write_index + kAutoDetectBenchmarkCapacity - record_count + offset) %
                  kAutoDetectBenchmarkCapacity);
    printRecord(out, first_sequence + offset, records[index]);
  }
  out.println(F("OK:ADBENCH_DUMP"));
}

bool handleAutoDetectBenchmarkCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || serialTokenEquals(text, "HELP")) {
    out.println(F("ADBENCH CMDS:ADBENCH DUMP,ADBENCH CLEAR,ADBENCH MARK [VALUE],ADBENCH AUTO"));
    return true;
  }
  if (serialTokenEquals(text, "DUMP")) {
    autoDetectBenchmarkDump(out);
    return true;
  }
  if (serialTokenEquals(text, "CLEAR") || serialTokenEquals(text, "RESET")) {
    autoDetectBenchmarkReset();
    out.println(F("OK:ADBENCH_CLEAR"));
    return true;
  }
  if (serialTokenEquals(text, "AUTO") || serialTokenEquals(text, "REDETECT")) {
    queueAutoDetectRuntimeBenchmark();
    out.println(F("OK:ADBENCH_AUTO"));
    return true;
  }
  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "MARK", &remainder) || serialCommandStartsWith(text, "M", &remainder)) {
    uint32_t value = 0;
    serialParseUint32Token(remainder, &value);
    autoDetectBenchmarkMark(ADBENCH_USER_MARK, 0, value > 65535u ? 65535u : (uint16_t)value);
    out.println(F("OK:ADBENCH_MARK"));
    return true;
  }
  out.println(F("ERR:BAD_ADBENCH_CMD"));
  return true;
}
