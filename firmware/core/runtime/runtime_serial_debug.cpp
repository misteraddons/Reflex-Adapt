#include "../../product_config.h"
#include "../../firmware_build_info.h"

#include "runtime_serial_debug.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <stdlib.h>

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/adc.h>
#include <hardware/clocks.h>
#include <hardware/watchdog.h>
#endif

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include <Adafruit_TinyUSB.h>
#endif

#ifdef ENABLE_INPUT_JVS
#include "../../input/jvs/jvs_host_runtime.h"
#endif
#ifdef ENABLE_INPUT_DREAMCAST
#include "../../input/dreamcast/Input_Dreamcast.h"
#endif
#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB)
#include "../../input/usb_host/input_usb_host_service.h"
#endif
#ifdef ENABLE_USB_BT_HCI
#include "../../input/usb_bt/usb_bt_hci_host.h"
#endif
#ifdef ENABLE_USB_XINPUT_AUTH_HOST
#include "../../input/usb_host/usb_xinput_host.h"
#endif
#ifdef ENABLE_INPUT_USB
#include <input_usb.h>
#endif
#ifdef ENABLE_INPUT_AUTODETECT
#include "../../input/autodetect/input_autodetect_runtime_state.h"
#endif
#ifdef ENABLE_USB_AUTH_SIDECAR
#include "../../output/auth/ps_auth_dongle_runtime.h"
#endif
#ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
#include "../../output/auth/xbone_auth_passthrough.h"
#include "../../output/xboxone/out_xboxone.h"
#endif
#include "../../features/feature_module.h"
#ifdef ENABLE_TTY2OLED_SERIAL
#include "../../features/tty2oled/tty2oled_feature.h"
#endif
#include "../../menu/menu_idle_runtime.h"
#include "../../menu/menu_mode_state.h"
#include "../../menu/menu_runtime_state.h"
#include "../../menu/menu_working_state.h"
#include "../../output/output_runtime_state.h"
#include "../../output/runtime/output_loop_runtime.h"
#include "../../platform/latency_service_mask.h"
#include "../../platform/latency_trace_gpio.h"
#include "../../platform/latency_test.h"
#include "../adapt_state_stream_runtime.h"
#include "../device_runtime_state.h"
#include "../firmware_support.h"
#include "../input_overlay_runtime.h"
#include "../oled_serial_runtime.h"
#include "../serial_debug_runtime.h"
#include "../settings_store.h"
#include "runtime_debug_cdc_state.h"

namespace {

#ifdef ENABLE_RUNTIME_SERIAL_DEBUG_CDC
constexpr size_t kSerialCommandBufferSize = 160;
char serialCommandBuffer[kSerialCommandBufferSize];
size_t serialCommandLength = 0;

char uppercaseAscii(char value) {
  return (value >= 'a' && value <= 'z') ? (char)(value - ('a' - 'A')) : value;
}

bool isSerialCommandWhitespace(char value) {
  return value == ' ' || value == '\t';
}

bool isPrintableSerialCommandByte(char value) {
  const uint8_t byte = (uint8_t)value;
  return byte >= 0x20u && byte < 0x7Fu;
}

void normalizeSerialCommandBuffer() {
  size_t start = 0;
  while (start < serialCommandLength &&
         isSerialCommandWhitespace(serialCommandBuffer[start])) {
    ++start;
  }

  size_t end = serialCommandLength;
  while (end > start && isSerialCommandWhitespace(serialCommandBuffer[end - 1])) {
    --end;
  }

  if (start > 0 && end > start) {
    memmove(serialCommandBuffer, serialCommandBuffer + start, end - start);
  }
  serialCommandLength = end - start;
}

bool serialCommandEquals(const char* expected) {
  size_t index = 0;
  while (expected[index] != '\0') {
    if (index >= serialCommandLength ||
        uppercaseAscii(serialCommandBuffer[index]) != expected[index]) {
      return false;
    }
    ++index;
  }

  return index == serialCommandLength;
}

char* skipSerialSpaces(char* text) {
  while (*text == ' ' || *text == '\t') {
    ++text;
  }
  return text;
}

bool serialCommandStartsWith(const char* expected, char** remainder) {
  size_t index = 0;
  while (expected[index] != '\0') {
    if (uppercaseAscii(serialCommandBuffer[index]) != expected[index]) {
      return false;
    }
    ++index;
  }

  char* tail = &serialCommandBuffer[index];
  if (*tail == '\0') {
    *remainder = tail;
    return true;
  }
  if (*tail != ' ' && *tail != '\t') {
    return false;
  }

  *remainder = skipSerialSpaces(tail);
  return true;
}

bool serialTextStartsWith(char* text, const char* expected, char** remainder) {
  text = skipSerialSpaces(text);
  size_t index = 0;
  while (expected[index] != '\0') {
    if (text[index] == '\0' ||
        uppercaseAscii(text[index]) != expected[index]) {
      return false;
    }
    ++index;
  }

  char* tail = &text[index];
  if (*tail == '\0') {
    *remainder = tail;
    return true;
  }
  if (*tail != ' ' && *tail != '\t') {
    return false;
  }

  *remainder = skipSerialSpaces(tail);
  return true;
}

bool serialTokenEquals(const char* token, const char* expected) {
  size_t index = 0;
  while (expected[index] != '\0') {
    if (token[index] == '\0' ||
        token[index] == ' ' ||
        token[index] == '\t' ||
        uppercaseAscii(token[index]) != expected[index]) {
      return false;
    }
    ++index;
  }

  return token[index] == '\0' || token[index] == ' ' || token[index] == '\t';
}

#if defined(ARDUINO_ARCH_RP2040)
void printRp2040TemperatureStatus() {
  static bool adcInitialized = false;
  if (!adcInitialized) {
    adc_init();
    adcInitialized = true;
  }

  const uint previousInput = adc_get_selected_input();
  const bool tempWasEnabled = (adc_hw->cs & ADC_CS_TS_EN_BITS) != 0;

  adc_set_temp_sensor_enabled(true);
  adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM);

  constexpr uint8_t kSamples = 16;
  uint32_t rawSum = 0;
  for (uint8_t sample = 0; sample < kSamples; ++sample) {
    rawSum += adc_read();
  }

  adc_select_input(previousInput);
  if (!tempWasEnabled) {
    adc_set_temp_sensor_enabled(false);
  }

  const float rawAverage = (float)rawSum / (float)kSamples;
  const float voltage = rawAverage * 3.3f / 4096.0f;
  const float temperatureC = 27.0f - ((voltage - 0.706f) / 0.001721f);

  runtimeDebugCdc.print(F("TEMP CLK_HZ="));
  runtimeDebugCdc.print((uint32_t)clock_get_hz(clk_sys));
  runtimeDebugCdc.print(F(" RAW="));
  runtimeDebugCdc.print(rawAverage, 1);
  runtimeDebugCdc.print(F(" MV="));
  runtimeDebugCdc.print(voltage * 1000.0f, 1);
  runtimeDebugCdc.print(F(" C="));
  runtimeDebugCdc.println(temperatureC, 1);
}
#endif

bool readSerialToken(char*& text, char* token, size_t tokenSize) {
  text = skipSerialSpaces(text);
  if (*text == '\0' || tokenSize == 0) {
    return false;
  }

  size_t index = 0;
  while (*text != '\0' && !isSerialCommandWhitespace(*text)) {
    if (index + 1 < tokenSize) {
      token[index++] = *text;
    }
    ++text;
  }
  token[index] = '\0';
  text = skipSerialSpaces(text);
  return index != 0;
}

void handleLatencyServiceSerialCommand(char* text) {
  if (*text == '\0' || serialTokenEquals(text, "STATUS")) {
    latencyServicePrintStatus(runtimeDebugCdc);
    return;
  }
  if (serialTokenEquals(text, "CLEAR") || serialTokenEquals(text, "RESET")) {
    latencyServiceClearDisabledMask();
    latencyServicePrintStatus(runtimeDebugCdc);
    return;
  }

  char token[24] = {};
  if (!readSerialToken(text, token, sizeof(token))) {
    runtimeDebugCdc.println(F("ERR:BAD_LATENCY_SERVICE"));
    return;
  }

  uint32_t service = 0;
  if (!latencyServiceNameToMask(token, &service)) {
    runtimeDebugCdc.println(F("ERR:BAD_LATENCY_SERVICE_NAME"));
    return;
  }

  if (serialTokenEquals(text, "OFF") || serialTokenEquals(text, "0") ||
      serialTokenEquals(text, "DISABLE")) {
    latencyServiceSetDisabled(service, true);
  } else if (serialTokenEquals(text, "ON") || serialTokenEquals(text, "1") ||
             serialTokenEquals(text, "ENABLE")) {
    latencyServiceSetDisabled(service, false);
  } else {
    runtimeDebugCdc.println(F("ERR:BAD_LATENCY_SERVICE_STATE"));
    return;
  }

  latencyServicePrintStatus(runtimeDebugCdc);
}

void printHexByte(uint8_t value) {
  if (value < 0x10) {
    runtimeDebugCdc.print('0');
  }
  runtimeDebugCdc.print(value, HEX);
}

void printHex32(uint32_t value) {
  runtimeDebugCdc.print(F("0x"));
  for (int shift = 28; shift >= 0; shift -= 4) {
    runtimeDebugCdc.print((value >> shift) & 0x0F, HEX);
  }
}

void printHexByteArray(const uint8_t* values, size_t count) {
  for (size_t index = 0; index < count; ++index) {
    if (index != 0) {
      runtimeDebugCdc.print(' ');
    }
    printHexByte(values[index]);
  }
}

#ifdef ENABLE_USB_BT_HCI
void printBtAddress(Print& out, const uint8_t addr[6]) {
  for (int i = 5; i >= 0; --i) {
    if (addr[i] < 16) {
      out.print('0');
    }
    out.print(addr[i], HEX);
    if (i > 0) {
      out.print(':');
    }
  }
}
#endif

bool parseRuntimeOutputModeToken(const char* token, outputMode_t* mode) {
  if (serialTokenEquals(token, "MISTER") ||
      serialTokenEquals(token, "LINUX") ||
      serialTokenEquals(token, "MISTERDINPUT")) {
    *mode = OUTPUT_MISTER;
    return true;
  }
  if (serialTokenEquals(token, "DINPUT") ||
      serialTokenEquals(token, "HID") ||
      serialTokenEquals(token, "WINDINPUT") ||
      serialTokenEquals(token, "WINDOWSDINPUT")) {
    *mode = OUTPUT_HID;
    return true;
  }
  if (serialTokenEquals(token, "AUTO")) {
    *mode = OUTPUT_AUTO;
    return true;
  }
  if (serialTokenEquals(token, "WINDOWS") ||
      serialTokenEquals(token, "WINXINPUT") ||
      serialTokenEquals(token, "XINPUT") ||
      serialTokenEquals(token, "XINPUT2") ||
      serialTokenEquals(token, "XINPUT2P") ||
      serialTokenEquals(token, "XINPUTW") ||
      serialTokenEquals(token, "XINPUT4") ||
      serialTokenEquals(token, "XINPUT4P") ||
      serialTokenEquals(token, "ANALOGUE") ||
      serialTokenEquals(token, "POCKET") ||
      serialTokenEquals(token, "POCKETDOCK")) {
    *mode = OUTPUT_XINPUT2P;
    return true;
  }
  if (serialTokenEquals(token, "XBOX360") ||
      serialTokenEquals(token, "X360")) {
    *mode = OUTPUT_XINPUT;
    return true;
  }
  if (serialTokenEquals(token, "XID") ||
      serialTokenEquals(token, "XBOXOG")) {
    *mode = OUTPUT_XID;
    return true;
  }
  if (serialTokenEquals(token, "XBOXONE") ||
      serialTokenEquals(token, "XBONE") ||
      serialTokenEquals(token, "XB1")) {
    *mode = OUTPUT_XBOXONE;
    return true;
  }
  if (serialTokenEquals(token, "PS3")) {
    *mode = OUTPUT_PS3;
    return true;
  }
  if (serialTokenEquals(token, "PS4")) {
    *mode = OUTPUT_PS4;
    return true;
  }
  if (serialTokenEquals(token, "PS5")) {
    *mode = OUTPUT_PS5;
    return true;
  }
  if (serialTokenEquals(token, "SWITCH")) {
    *mode = OUTPUT_SWITCHPRO;
    return true;
  }
  if (serialTokenEquals(token, "SWITCHPRO")) {
    *mode = OUTPUT_SWITCHPRO;
    return true;
  }
  if (serialTokenEquals(token, "GC") ||
      serialTokenEquals(token, "GCWIIU") ||
      serialTokenEquals(token, "GCADAPTER")) {
    *mode = OUTPUT_GCWIIU;
    return true;
  }
  if (serialTokenEquals(token, "KEYBOARD") ||
      serialTokenEquals(token, "KBD") ||
      serialTokenEquals(token, "MAME")) {
    *mode = OUTPUT_KEYBOARD;
    return true;
  }
  return false;
}

bool parseWindowsOutputToken(const char* token, uint8_t* winOutput) {
  if (serialTokenEquals(token, "XINPUT") ||
      serialTokenEquals(token, "WINDOWS") ||
      serialTokenEquals(token, "XINPUT2") ||
      serialTokenEquals(token, "XINPUT2P") ||
      serialTokenEquals(token, "XINPUTW") ||
      serialTokenEquals(token, "XINPUT4") ||
      serialTokenEquals(token, "XINPUT4P") ||
      serialTokenEquals(token, "ANALOGUE") ||
      serialTokenEquals(token, "POCKET") ||
      serialTokenEquals(token, "POCKETDOCK")) {
    *winOutput = 1;
    return true;
  }
  if (serialTokenEquals(token, "MISTER") ||
      serialTokenEquals(token, "DINPUT") ||
      serialTokenEquals(token, "HID") ||
      serialTokenEquals(token, "WINDINPUT") ||
      serialTokenEquals(token, "WINDOWSDINPUT")) {
    *winOutput = 0;
    return true;
  }
  if (serialTokenEquals(token, "KEYBOARD") ||
      serialTokenEquals(token, "KBD") ||
      serialTokenEquals(token, "MAME") ||
      serialTokenEquals(token, "WINKEYBOARD")) {
    *winOutput = 2;
    return true;
  }
  return false;
}

bool parseLatencyModeToken(const char* token, uint8_t* mode) {
  if (serialTokenEquals(token, "OFF")) {
    *mode = LATENCY_MODE_OFF;
    return true;
  }
  if (serialTokenEquals(token, "ON") ||
      serialTokenEquals(token, "PULSE") ||
      serialTokenEquals(token, "SYNTH") ||
      serialTokenEquals(token, "SYNTH_INTERNAL") ||
      serialTokenEquals(token, "NOCTRL")) {
    *mode = LATENCY_MODE_SYNTH_INTERNAL;
    return true;
  }
  if (serialTokenEquals(token, "SYNTHPC") ||
      serialTokenEquals(token, "SYNTH_PC") ||
      serialTokenEquals(token, "NOCTRLPC")) {
    *mode = LATENCY_MODE_SYNTH_PC;
    return true;
  }
  if (serialTokenEquals(token, "SYNTHM") ||
      serialTokenEquals(token, "SYNTH_MISTER") ||
      serialTokenEquals(token, "NOCTRLMISTER")) {
    *mode = LATENCY_MODE_SYNTH_MISTER;
    return true;
  }
  if (serialTokenEquals(token, "TRIGPC") ||
      serialTokenEquals(token, "TRIG_PC") ||
      serialTokenEquals(token, "CTRLPC")) {
    *mode = LATENCY_MODE_TRIGGER_PC;
    return true;
  }
  if (serialTokenEquals(token, "TRIG") ||
      serialTokenEquals(token, "TRIG_INTERNAL") ||
      serialTokenEquals(token, "CTRL")) {
    *mode = LATENCY_MODE_TRIGGER_INTERNAL;
    return true;
  }
  if (serialTokenEquals(token, "TRIGM") ||
      serialTokenEquals(token, "TRIG_MISTER") ||
      serialTokenEquals(token, "CTRLMISTER")) {
    *mode = LATENCY_MODE_TRIGGER_MISTER;
    return true;
  }
  return false;
}

bool parseLatencyHostTypeToken(const char* token, uint8_t* hostType) {
  if (serialTokenEquals(token, "INTERNAL") ||
      serialTokenEquals(token, "LOOP")) {
    *hostType = LATENCY_HOST_INTERNAL;
    return true;
  }
  if (serialTokenEquals(token, "PC")) {
    *hostType = LATENCY_HOST_PC;
    return true;
  }
  if (serialTokenEquals(token, "MISTER") ||
      serialTokenEquals(token, "FPGA")) {
    *hostType = LATENCY_HOST_MISTER;
    return true;
  }
  return false;
}

bool parseUnsignedLongToken(const char* token, uint32_t* value) {
  if (token == nullptr) {
    return false;
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(token, &end, 10);
  if (end == token) {
    return false;
  }
  while (*end == ' ' || *end == '\t') {
    ++end;
  }
  if (*end != '\0') {
    return false;
  }

  *value = (uint32_t)parsed;
  return true;
}

void saveRuntimeOutputModeFromSerial(outputMode_t newMode) {
  GlobalSettingsRecord globalSettings{};
  readGlobalSettings(globalSettings);
  globalSettings.configured_output_mode = (uint8_t)canonicalizeOutputMode(newMode);
  writeGlobalSettings(globalSettings);

  configuredOutputMode = canonicalizeOutputMode(newMode);
  outputMode = sanitizeRuntimeOutputMode(configuredOutputMode);
  autoDetectState = AUTO_STATE_IDLE;
  auto_detect_clear_scratch_state();

  runtimeDebugCdc.print(F("OK:OUT="));
  runtimeDebugCdc.println(get_mode_name(configuredOutputMode));
  runtimeDebugCdc.flush();
  delay(50);
  reboot();
}

void saveWindowsOutputPreferenceFromSerial(uint8_t newWinOutput) {
  GlobalSettingsRecord globalSettings{};
  readGlobalSettings(globalSettings);
  globalSettings.win_output = newWinOutput;
  writeGlobalSettings(globalSettings);
  const bool windowsOutputDescriptorChanged =
    newWinOutput != menu_win_output &&
    configuredOutputMode == OUTPUT_AUTO &&
    autoDetectState == AUTO_STATE_WINDOWS;
  if (newWinOutput != menu_win_output) {
    auto_detect_clear_scratch_state();
  }
  menu_win_output = newWinOutput;

  runtimeDebugCdc.print(F("OK:WIN_OUT="));
  runtimeDebugCdc.println((int)menu_win_output);
  if (windowsOutputDescriptorChanged) {
    runtimeDebugCdc.println(F("OK:REBOOT=WINDOWS_OUTPUT"));
    runtimeDebugCdc.flush();
    delay(50);
    reboot();
  }
}

void saveLatencyConfigurationFromSerial(bool enabled, bool controllerInLoop, uint8_t hostType) {
  GlobalSettingsRecord globalSettings{};
  readGlobalSettings(globalSettings);
  globalSettings.latency_test = enabled ? 1 : 0;
  globalSettings.latency_controller_in_loop = controllerInLoop ? 1 : 0;
  globalSettings.latency_host_type = hostType;
  writeGlobalSettings(globalSettings);
  menu_latency_test = globalSettings.latency_test;
  menu_latency_controller_in_loop = globalSettings.latency_controller_in_loop;
  menu_latency_host_type = globalSettings.latency_host_type;
  latencyTest.setEnabled(menu_latency_test != 0);
  latencyTest.setControllerInLoop(menu_latency_controller_in_loop != 0);
  latencyTest.setHostType(menu_latency_host_type);
}

void saveLatencyModeFromSerial(uint8_t newMode) {
  latencyTest.setMode(newMode);
  saveLatencyConfigurationFromSerial(latencyTest.isEnabled(),
                                     latencyTest.isControllerInLoop(),
                                     latencyTest.getHostType());

  runtimeDebugCdc.print(F("OK:LAT_MODE="));
  runtimeDebugCdc.println(latencyTestModeName(latencyTest.getMode()));
}

void saveLatencyEnabledFromSerial(bool enabled) {
  saveLatencyConfigurationFromSerial(enabled,
                                     latencyTest.isControllerInLoop(),
                                     latencyTest.getHostType());
  runtimeDebugCdc.print(F("OK:LAT_ENABLED="));
  runtimeDebugCdc.println(enabled ? 1 : 0);
}

void saveLatencyControllerFromSerial(bool controllerInLoop) {
  saveLatencyConfigurationFromSerial(latencyTest.isEnabled(),
                                     controllerInLoop,
                                     latencyTest.getHostType());
  runtimeDebugCdc.print(F("OK:LAT_CTRL="));
  runtimeDebugCdc.println(controllerInLoop ? F("CONTROLLER") : F("INTERNAL"));
}

void saveLatencyHostFromSerial(uint8_t hostType) {
  saveLatencyConfigurationFromSerial(latencyTest.isEnabled(),
                                     latencyTest.isControllerInLoop(),
                                     hostType);
  runtimeDebugCdc.print(F("OK:LAT_HOST="));
  runtimeDebugCdc.println(latencyHostTypeName(hostType));
}

#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB)
void printUsbHostSerialStatus();
#endif
#ifdef ENABLE_USB_AUTH_SIDECAR
void printAuthSidecarSerialStatus();
#endif

void printRuntimeSerialStatus() {
  runtimeDebugCdc.print(F("INFO PRODUCT=Classic2USB VERSION="));
  runtimeDebugCdc.print(F(FIRMWARE_VERSION_STRING));
  runtimeDebugCdc.print(F(" TAG="));
  runtimeDebugCdc.print(F(FIRMWARE_VERSION_TAG));
  runtimeDebugCdc.print(F(" HARDWARE=\""));
  runtimeDebugCdc.print(F(FIRMWARE_HARDWARE_STRING));
  runtimeDebugCdc.println(F("\""));

  runtimeDebugCdc.print(F("STATUS INPUT="));
  runtimeDebugCdc.print((int)deviceMode);
  runtimeDebugCdc.print(F(" SAVED_INPUT="));
  runtimeDebugCdc.print((int)savedDeviceMode);
  runtimeDebugCdc.print(F(" CONFIG_OUT="));
  runtimeDebugCdc.print((int)configuredOutputMode);
  runtimeDebugCdc.print(F(" RUNTIME_OUT="));
  runtimeDebugCdc.print((int)outputMode);
  runtimeDebugCdc.print(F(" WIN_OUT="));
  runtimeDebugCdc.print((int)menu_win_output);
  runtimeDebugCdc.print(F(" AUTO_STATE="));
  runtimeDebugCdc.print((int)autoDetectState);
  runtimeDebugCdc.print(F(" PLAYERS="));
  runtimeDebugCdc.print((int)max_devices);
  runtimeDebugCdc.println();

  runtimeDebugCdc.print(F("STATUS CONFIG_NAME="));
  runtimeDebugCdc.print(get_mode_name(configuredOutputMode));
  runtimeDebugCdc.print(F(" RUNTIME_NAME="));
  runtimeDebugCdc.println(get_mode_name(outputMode));

#ifdef ENABLE_INPUT_AUTODETECT
  runtimeDebugCdc.print(F("AUTORESOLVE IN="));
  runtimeDebugCdc.print((int)input_auto_resolve_mode);
  runtimeDebugCdc.print(F(" INSRC="));
  runtimeDebugCdc.print((int)input_auto_resolve_source);
  runtimeDebugCdc.print(F(" OUT="));
  runtimeDebugCdc.print((int)outputMode);
  runtimeDebugCdc.print(F(" OUTSTATE="));
  runtimeDebugCdc.print((int)autoDetectState);
  runtimeDebugCdc.print(F(" PROBE="));
  runtimeDebugCdc.print((int)autoDetectProbeStage);
  runtimeDebugCdc.print(F(" TRANS="));
  runtimeDebugCdc.print((int)output_autodetect_transition_reason);
  runtimeDebugCdc.print(F(" MS="));
  runtimeDebugCdc.print((int)output_autodetect_last_ms);
  runtimeDebugCdc.print(F(" FLAGS=0x"));
  runtimeDebugCdc.print((int)output_autodetect_last_flags, HEX);
  runtimeDebugCdc.print(F(" AUX=0x"));
  runtimeDebugCdc.println((int)output_autodetect_aux_flags, HEX);
#endif

  const UsbDeviceRuntimeDiagnostics& usbDiag = usbDeviceRuntimeDiagnostics();
  runtimeDebugCdc.print(F("STATUS UPTIME_MS="));
  runtimeDebugCdc.print(millis());
#if defined(ARDUINO_ARCH_RP2040)
  runtimeDebugCdc.print(F(" WATCHDOG_REBOOT="));
  runtimeDebugCdc.print(watchdog_caused_reboot() ? 1 : 0);
#endif
  runtimeDebugCdc.print(F(" USB_TASKS="));
  runtimeDebugCdc.print(usbDiag.task_count);
  runtimeDebugCdc.print(F(" USB_GAP_US="));
  runtimeDebugCdc.print(usbDiag.last_task_gap_us);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(usbDiag.max_task_gap_us);
  runtimeDebugCdc.print(F(" USB_MOUNT="));
  runtimeDebugCdc.print(usbDiag.mount_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(usbDiag.umount_count);
  runtimeDebugCdc.print(F(" USB_SUSP="));
  runtimeDebugCdc.print(usbDiag.suspend_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.println(usbDiag.resume_count);

#ifdef ENABLE_INPUT_DREAMCAST
  const DreamcastVmuSerialStats& vmuStats = dreamcastVmuSerialStats();
  runtimeDebugCdc.print(F("STATUS VMU_READS="));
  runtimeDebugCdc.print(vmuStats.read_count);
  runtimeDebugCdc.print(F(" OK="));
  runtimeDebugCdc.print(vmuStats.success_count);
  runtimeDebugCdc.print(F(" TO="));
  runtimeDebugCdc.print(vmuStats.timeout_count);
  runtimeDebugCdc.print(F(" TO_ATT="));
  runtimeDebugCdc.print(vmuStats.timeout_attempt_count);
  runtimeDebugCdc.print(F(" RETRY="));
  runtimeDebugCdc.print(vmuStats.retry_count);
  runtimeDebugCdc.print(F(" LAST="));
  runtimeDebugCdc.print((int)vmuStats.last_port);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)vmuStats.last_slot);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)vmuStats.last_block);
  runtimeDebugCdc.print(F(" RES="));
  runtimeDebugCdc.print((int)vmuStats.last_result);
  runtimeDebugCdc.print(F(" US="));
  runtimeDebugCdc.print(vmuStats.last_read_us);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.println(vmuStats.max_read_us);
#endif

  latencyTest.writeStatus(runtimeDebugCdc);
  adaptStateWriteStatus(runtimeDebugCdc);
  inputOverlayWriteStatus(runtimeDebugCdc);
  featureModulesAppendSerialState(runtimeDebugCdc);
#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB)
  printUsbHostSerialStatus();
#endif
#ifdef ENABLE_USB_AUTH_SIDECAR
  printAuthSidecarSerialStatus();
#endif
}

#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB)
void printUsbHostSerialStatus() {
  const UsbHostServiceStatus host = usb_host_service_status();

  runtimeDebugCdc.print(F("USBHOST AVAIL="));
  runtimeDebugCdc.print(host.available ? 1 : 0);
  runtimeDebugCdc.print(F(" STARTED="));
  runtimeDebugCdc.print(host.started ? 1 : 0);
  runtimeDebugCdc.print(F(" REQ="));
  runtimeDebugCdc.print(host.start_requested ? 1 : 0);
  runtimeDebugCdc.print(F(" TUH="));
  runtimeDebugCdc.print(host.tuh_inited ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.tuh_mounted_addr1 ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.tuh_ready_addr1 ? 1 : 0);
  runtimeDebugCdc.print(F(" A1="));
  runtimeDebugCdc.print(host.addr1_vid_pid_valid ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.addr1_desc_valid ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.addr1_vid, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(host.addr1_pid, HEX);
  runtimeDebugCdc.print(F(" CLS="));
  runtimeDebugCdc.print((int)host.addr1_class);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)host.addr1_subclass);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)host.addr1_protocol);
  runtimeDebugCdc.print(F(" EP0="));
  runtimeDebugCdc.print((int)host.addr1_ep0_size);
  runtimeDebugCdc.print(F(" CFG="));
  runtimeDebugCdc.print((int)host.addr1_config_count);
  runtimeDebugCdc.print(F(" STR="));
  runtimeDebugCdc.print((int)host.addr1_manufacturer_index);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)host.addr1_product_index);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)host.addr1_serial_index);
  runtimeDebugCdc.print(F(" BCD="));
  runtimeDebugCdc.print(host.addr1_bcd_usb, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.addr1_bcd_device, HEX);
  runtimeDebugCdc.print(F(" HID="));
  runtimeDebugCdc.print((int)host.hid_total_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)host.hid_addr1_count);
  runtimeDebugCdc.print(F(" RHP="));
  runtimeDebugCdc.print((int)host.rhport);
  runtimeDebugCdc.print(F(" DP="));
  runtimeDebugCdc.print((int)host.dp_pin);
  runtimeDebugCdc.print(F(" DM="));
  runtimeDebugCdc.print((int)host.dm_pin);
  runtimeDebugCdc.print(F(" PINOUT="));
  runtimeDebugCdc.print(host.pinout == 1 ? F("DMDP") : F("DPDM"));
  runtimeDebugCdc.print(F(" RAW="));
  runtimeDebugCdc.print((int)host.raw_dp_level);
  runtimeDebugCdc.print((int)host.raw_dm_level);
  runtimeDebugCdc.print(F(" PHY="));
  runtimeDebugCdc.print((int)host.physical_dp_level);
  runtimeDebugCdc.print((int)host.physical_dm_level);
  runtimeDebugCdc.print(F(" LINE="));
  if (host.pio_line_state == 1) {
    runtimeDebugCdc.print(F("FS"));
  } else if (host.pio_line_state == 2) {
    runtimeDebugCdc.print(F("LS"));
  } else if (host.pio_line_state == 0) {
    runtimeDebugCdc.print(F("SE0"));
  } else {
    runtimeDebugCdc.print(F("SE1"));
  }
  runtimeDebugCdc.print(F(" ROOT="));
  runtimeDebugCdc.print(host.root_initialized ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_connected ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_suspended ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_fullspeed ? 1 : 0);
  runtimeDebugCdc.print(F(" REVT="));
  runtimeDebugCdc.print((int)host.root_event);
  runtimeDebugCdc.print(F(" RINT="));
  runtimeDebugCdc.print(host.root_ints, HEX);
  runtimeDebugCdc.print(F(" EP="));
  runtimeDebugCdc.print(host.root_ep_complete, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_ep_error, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_ep_stalled, HEX);
  runtimeDebugCdc.print(F(" RF="));
  runtimeDebugCdc.print((int)host.root_failed_count);
  runtimeDebugCdc.print(F(" RDEV="));
  runtimeDebugCdc.print(host.root_device_present ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_device_connected ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_device_enumerated ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_device_fullspeed ? 1 : 0);
  runtimeDebugCdc.print(F(" DEV="));
  runtimeDebugCdc.print((int)host.root_device_address);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.root_device_vid, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(host.root_device_pid, HEX);
  runtimeDebugCdc.print(F(" DEVT="));
  runtimeDebugCdc.print((int)host.root_device_event);
  runtimeDebugCdc.print(F(" KICK="));
  runtimeDebugCdc.print((int)host.attach_kick_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(host.attach_kick_age_ms);
  runtimeDebugCdc.print(F(" TXPIO="));
  runtimeDebugCdc.print((int)host.tx_pio);
  runtimeDebugCdc.print(F(" TXSM="));
  runtimeDebugCdc.print((int)host.tx_sm);
  runtimeDebugCdc.print(F(" TXDMA="));
  runtimeDebugCdc.print((int)host.tx_dma);
  runtimeDebugCdc.print(F(" RXPIO="));
  runtimeDebugCdc.print((int)host.rx_pio);
  runtimeDebugCdc.print(F(" RXSM="));
  runtimeDebugCdc.print((int)host.rx_sm);
  runtimeDebugCdc.print(F(" EOPSM="));
  runtimeDebugCdc.print((int)host.eop_sm);
  runtimeDebugCdc.print(F(" TASKS="));
  runtimeDebugCdc.println(host.task_count);

  const UsbHostCallbackStatus hostCb = usb_host_callback_status();
  runtimeDebugCdc.print(F("USBHCB MOUNT="));
  runtimeDebugCdc.print(hostCb.device_mount_count);
  runtimeDebugCdc.print(F(" UMOUNT="));
  runtimeDebugCdc.print(hostCb.device_umount_count);
  runtimeDebugCdc.print(F(" CFGD="));
  runtimeDebugCdc.print(hostCb.config_desc_count);
  runtimeDebugCdc.print(F(" HIDM="));
  runtimeDebugCdc.print(hostCb.hid_mount_count);
  runtimeDebugCdc.print(F(" HIDU="));
  runtimeDebugCdc.print(hostCb.hid_umount_count);
  runtimeDebugCdc.print(F(" HIDR="));
  runtimeDebugCdc.print(hostCb.hid_report_count);
  runtimeDebugCdc.print(F(" XIM="));
  runtimeDebugCdc.print(hostCb.xinput_mount_count);
  runtimeDebugCdc.print(F(" XIR="));
  runtimeDebugCdc.print(hostCb.xinput_report_count);
  runtimeDebugCdc.print(F(" LAST="));
  runtimeDebugCdc.print(hostCb.last_vid, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(hostCb.last_pid, HEX);
  runtimeDebugCdc.print(F(" A="));
  runtimeDebugCdc.print((int)hostCb.last_dev_addr);
  runtimeDebugCdc.print(F(" I="));
  runtimeDebugCdc.print((int)hostCb.last_instance);
  runtimeDebugCdc.print(F(" DESC="));
  runtimeDebugCdc.print(hostCb.last_hid_desc_len);
  runtimeDebugCdc.print(F(" LEN="));
  runtimeDebugCdc.print((int)hostCb.last_report_len);
  runtimeDebugCdc.print(F(" CFG="));
  runtimeDebugCdc.print((int)hostCb.last_config_index);
  runtimeDebugCdc.print(F(" ITF="));
  runtimeDebugCdc.print((int)hostCb.last_itf_class);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)hostCb.last_itf_subclass);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)hostCb.last_itf_protocol);
  runtimeDebugCdc.print('#');
  runtimeDebugCdc.print((int)hostCb.last_itf_number);
  runtimeDebugCdc.print(F(" BTHCI="));
  runtimeDebugCdc.print(hostCb.bluetooth_hci_detect_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(hostCb.bluetooth_hci_vid, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(hostCb.bluetooth_hci_pid, HEX);
  runtimeDebugCdc.print(F(" A="));
  runtimeDebugCdc.println((int)hostCb.bluetooth_hci_addr);

  runtimeDebugCdc.print(F("USBHITF COUNT="));
  runtimeDebugCdc.print((int)hostCb.interface_snapshot_count);
  for (uint8_t index = 0;
       index < hostCb.interface_snapshot_count &&
       index < USB_HOST_INTERFACE_SNAPSHOT_COUNT;
       ++index) {
    const UsbHostInterfaceSnapshot& itf = hostCb.interfaces[index];
    runtimeDebugCdc.print(F(" ["));
    runtimeDebugCdc.print((int)itf.number);
    runtimeDebugCdc.print(':');
    runtimeDebugCdc.print((int)itf.cls);
    runtimeDebugCdc.print('/');
    runtimeDebugCdc.print((int)itf.subclass);
    runtimeDebugCdc.print('/');
    runtimeDebugCdc.print((int)itf.protocol);
    runtimeDebugCdc.print(F(" EP"));
    runtimeDebugCdc.print((int)itf.endpoint_count);
    runtimeDebugCdc.print(']');
  }
  runtimeDebugCdc.println();

#ifdef ENABLE_USB_XINPUT_AUTH_HOST
  const UsbXInputHostStatus xinputHost = usb_xinput_host_status();
  runtimeDebugCdc.print(F("XGIPHOST SUP="));
  runtimeDebugCdc.print(xinputHost.supported ? 1 : 0);
  runtimeDebugCdc.print(F(" MNT="));
  runtimeDebugCdc.print(xinputHost.mounted ? 1 : 0);
  runtimeDebugCdc.print(F(" RDY="));
  runtimeDebugCdc.print(xinputHost.ready ? 1 : 0);
  runtimeDebugCdc.print(F(" DEV="));
  runtimeDebugCdc.print((int)xinputHost.dev_addr);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)xinputHost.instance);
  runtimeDebugCdc.print(F(" TYPE="));
  runtimeDebugCdc.print((int)xinputHost.type);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)xinputHost.subtype);
  runtimeDebugCdc.print(F(" ITF="));
  runtimeDebugCdc.print((int)xinputHost.itf_num);
  runtimeDebugCdc.print(F(" EP="));
  runtimeDebugCdc.print(xinputHost.ep_in, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xinputHost.ep_out, HEX);
  runtimeDebugCdc.print(F(" SZ="));
  runtimeDebugCdc.print(xinputHost.ep_in_size);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xinputHost.ep_out_size);
  runtimeDebugCdc.print(F(" VID=0x"));
  runtimeDebugCdc.print(xinputHost.vid, HEX);
  runtimeDebugCdc.print(F(" PID=0x"));
  runtimeDebugCdc.print(xinputHost.pid, HEX);
  runtimeDebugCdc.print(F(" CNT="));
  runtimeDebugCdc.print(xinputHost.open_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xinputHost.mount_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xinputHost.close_count);
  runtimeDebugCdc.print(F(" IO="));
  runtimeDebugCdc.print(xinputHost.input_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xinputHost.output_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xinputHost.output_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xinputHost.receive_requeue_fail_count);
  runtimeDebugCdc.print(F(" LAST="));
  runtimeDebugCdc.print((int)xinputHost.last_report_len);
  runtimeDebugCdc.print('/');
  for (uint8_t i = 0; i < sizeof(xinputHost.last_report_head); ++i) {
    if (xinputHost.last_report_head[i] < 0x10) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(xinputHost.last_report_head[i], HEX);
  }
  runtimeDebugCdc.println();
#endif

#ifdef ENABLE_USB_BT_HCI
  const UsbBtHciStatus btHci = usb_bt_hci_status();
  runtimeDebugCdc.print(F("BTHCI SUP="));
  runtimeDebugCdc.print(btHci.supported ? 1 : 0);
  runtimeDebugCdc.print(F(" OPEN="));
  runtimeDebugCdc.print(btHci.opened ? 1 : 0);
  runtimeDebugCdc.print(F(" MNT="));
  runtimeDebugCdc.print(btHci.mounted ? 1 : 0);
  runtimeDebugCdc.print(F(" RDY="));
  runtimeDebugCdc.print(btHci.ready ? 1 : 0);
  runtimeDebugCdc.print(F(" PEND="));
  runtimeDebugCdc.print(btHci.command_pending ? 1 : 0);
  runtimeDebugCdc.print(F(" PH="));
  runtimeDebugCdc.print((int)btHci.phase);
  runtimeDebugCdc.print(F(" DEV="));
  runtimeDebugCdc.print((int)btHci.dev_addr);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.vid, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(btHci.pid, HEX);
  runtimeDebugCdc.print(F(" ITF="));
  runtimeDebugCdc.print((int)btHci.interface_number);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.interface_alt);
  runtimeDebugCdc.print(F(" EP="));
  runtimeDebugCdc.print(btHci.event_ep, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.event_size);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.event_interval);
  runtimeDebugCdc.print(' ');
  runtimeDebugCdc.print(btHci.acl_in_ep, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_in_size);
  runtimeDebugCdc.print(' ');
  runtimeDebugCdc.print(btHci.acl_out_ep, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_out_size);
  runtimeDebugCdc.print(F(" CNT="));
  runtimeDebugCdc.print(btHci.open_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.mount_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.umount_count);
  runtimeDebugCdc.print(F(" EVT="));
  runtimeDebugCdc.print(btHci.event_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.event_requeue_fail_count);
  runtimeDebugCdc.print(F(" ACL="));
  runtimeDebugCdc.print(btHci.acl_in_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_out_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_in_parse_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_in_requeue_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_out_fail_count);
  runtimeDebugCdc.print(F(" CMD="));
  runtimeDebugCdc.print(btHci.cmd_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.cmd_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.cmd_fail_count);
  runtimeDebugCdc.print(F(" PAIR="));
  runtimeDebugCdc.print(btHci.pairing_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.pairing_not_ready_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.pairing_not_implemented_count);
  runtimeDebugCdc.print(F(" INQ="));
  runtimeDebugCdc.print(btHci.inquiry_active ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.inquiry_start_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.inquiry_result_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.inquiry_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.inquiry_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_inquiry_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_inquiry_rssi);
  runtimeDebugCdc.print(F(" CON="));
  runtimeDebugCdc.print(btHci.connection_pending ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connected ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connect_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connect_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connect_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connect_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connect_timeout_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.recovery_reset_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connection_pending_ms);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.disconnect_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_connect_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.connection_handle, HEX);
  runtimeDebugCdc.print(F("/D"));
  runtimeDebugCdc.print(btHci.last_disconnect_handle, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print((int)btHci.last_disconnect_status, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print((int)btHci.last_disconnect_reason, HEX);
  runtimeDebugCdc.print(F(" BOND="));
  runtimeDebugCdc.print(btHci.bond_valid ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.bond_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.active_bond_slot);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.reconnect_attempt_slot);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.bond_store_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.bond_unchanged_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.bond_persist_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.bond_persist_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.bond_persist_pending ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.auto_connect_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.auto_connect_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.auto_connect_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.auto_connect_success_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.bond_key_type);
  runtimeDebugCdc.print(F(" CT="));
  runtimeDebugCdc.print((int)btHci.bond_controller_type);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.active_controller_type);
  runtimeDebugCdc.print(F(" PAGE="));
  runtimeDebugCdc.print(btHci.page_scan_enabled ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.inquiry_scan_enabled ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.scan_enable);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.page_scan_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.page_scan_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.page_scan_fail_count);
  runtimeDebugCdc.print(F(" INCON="));
  runtimeDebugCdc.print(btHci.incoming_connection_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.incoming_connection_accept_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.incoming_connection_reject_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.incoming_connection_busy_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_incoming_reject_reason);
  runtimeDebugCdc.print(F(" BADDR="));
  printBtAddress(runtimeDebugCdc, btHci.bond_addr);
  runtimeDebugCdc.print(F(" AUTHBT="));
  runtimeDebugCdc.print(btHci.link_key_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.link_key_reply_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.link_key_negative_reply_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.link_key_notification_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.pin_code_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.pin_code_reply_count);
  runtimeDebugCdc.print(F(" SSP="));
  runtimeDebugCdc.print(btHci.simple_pairing_enabled ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.simple_pairing_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.simple_pairing_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.simple_pairing_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.io_capability_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.io_capability_reply_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.user_confirmation_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.user_confirmation_reply_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.user_passkey_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.user_passkey_negative_reply_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.simple_pairing_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_simple_pairing_status);
  runtimeDebugCdc.print(F(" RN="));
  runtimeDebugCdc.print(btHci.remote_name_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.remote_name_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.remote_name_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.remote_name_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_remote_name_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.remote_name);
  runtimeDebugCdc.print(F(" DID="));
  runtimeDebugCdc.print(btHci.sdp_device_id_valid ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_vendor_id, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(btHci.sdp_device_id_product_id, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_version, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_vendor_source, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_query_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_connect_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_request_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_response_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.sdp_device_id_fail_count);
  runtimeDebugCdc.print(F(" AUTH="));
  runtimeDebugCdc.print(btHci.auth_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.auth_request_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.auth_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_auth_status);
  runtimeDebugCdc.print(F(" ENC="));
  runtimeDebugCdc.print(btHci.encrypt_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.encrypt_request_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.encrypt_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_encrypt_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.encryption_enabled);
  runtimeDebugCdc.print(F(" L2="));
  runtimeDebugCdc.print(btHci.l2cap_control_pending ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_open ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_response_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_local_cid, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_remote_cid, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_result);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_control_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.l2cap_last_code, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.l2cap_last_identifier);
  runtimeDebugCdc.print(F(" L2I="));
  runtimeDebugCdc.print(btHci.l2cap_interrupt_pending ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_open ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_response_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_local_cid, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_remote_cid, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_result);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_interrupt_status);
  runtimeDebugCdc.print(F(" CFG="));
  runtimeDebugCdc.print(btHci.l2cap_config_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_config_response_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_config_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_last_psm, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_last_scid, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_last_dcid, HEX);
  runtimeDebugCdc.print(F(" L2D="));
  runtimeDebugCdc.print(btHci.l2cap_data_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_unknown_data_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_in_short_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_last_channel_id, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.l2cap_last_len);
  runtimeDebugCdc.print('/');
  for (uint8_t i = 0; i < sizeof(btHci.l2cap_last_data); ++i) {
    if (btHci.l2cap_last_data[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.l2cap_last_data[i], HEX);
  }
  runtimeDebugCdc.print(F(" ACLRAW="));
  runtimeDebugCdc.print(btHci.acl_in_last_len);
  runtimeDebugCdc.print('/');
  for (uint8_t i = 0; i < sizeof(btHci.acl_in_last_data); ++i) {
    if (btHci.acl_in_last_data[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.acl_in_last_data[i], HEX);
  }
  runtimeDebugCdc.print(F(" REASM="));
  runtimeDebugCdc.print(btHci.acl_reassembly_start_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_reassembly_continue_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_reassembly_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_reassembly_orphan_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.acl_reassembly_overflow_count);
  runtimeDebugCdc.print(F(" HIDBT="));
  runtimeDebugCdc.print(btHci.hid_control_data_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_interrupt_data_count);
  runtimeDebugCdc.print('(');
  runtimeDebugCdc.print(btHci.hid_link_interrupt_data_count);
  runtimeDebugCdc.print('@');
  runtimeDebugCdc.print(btHci.hid_link_start_interrupt_count);
  runtimeDebugCdc.print(')');
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_init_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_init_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_init_skip_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_output_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_output_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_last_cid, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_last_len);
  runtimeDebugCdc.print('/');
  for (uint8_t i = 0; i < sizeof(btHci.hid_last_report); ++i) {
    if (btHci.hid_last_report[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.hid_last_report[i], HEX);
  }
  runtimeDebugCdc.print(F(" HSTALL="));
  runtimeDebugCdc.print(btHci.hid_stream_stall_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_stream_recover_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hid_stream_disconnect_count);
  runtimeDebugCdc.print('/');
  if (btHci.hid_last_interrupt_ms == 0) {
    runtimeDebugCdc.print(0);
  } else {
    runtimeDebugCdc.print((unsigned long)(millis() - btHci.hid_last_interrupt_ms));
  }
  runtimeDebugCdc.print(F(" ED="));
  runtimeDebugCdc.print(btHci.endpoint_candidate_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.endpoint_open_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.missing_endpoint_fail_count);
  runtimeDebugCdc.print(F(" EF="));
  runtimeDebugCdc.print((int)btHci.last_open_fail_code);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_open_fail_interface);
  runtimeDebugCdc.print(F(" LEP="));
  runtimeDebugCdc.print(btHci.last_ep_addr, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_ep_attr);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_ep_dir);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_ep_xfer);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.last_ep_size);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_ep_interval);
  runtimeDebugCdc.print(F(" CAND="));
  runtimeDebugCdc.print(btHci.candidate_event_ep, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.candidate_acl_in_ep, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.candidate_acl_out_ep, HEX);
  runtimeDebugCdc.print(F(" LAST="));
  runtimeDebugCdc.print(btHci.last_opcode, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_event_code);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_event_len);
  runtimeDebugCdc.print(F(" VER="));
  runtimeDebugCdc.print((int)btHci.hci_version);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.hci_revision, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.lmp_version);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.manufacturer, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.lmp_subversion, HEX);
  runtimeDebugCdc.print(F(" BD="));
  for (int i = 5; i >= 0; --i) {
    if (btHci.bd_addr[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.bd_addr[i], HEX);
    if (i > 0) {
      runtimeDebugCdc.print(':');
    }
  }
  runtimeDebugCdc.print(F(" REM="));
  for (int i = 5; i >= 0; --i) {
    if (btHci.last_remote_addr[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.last_remote_addr[i], HEX);
    if (i > 0) {
      runtimeDebugCdc.print(':');
    }
  }
  runtimeDebugCdc.print(F(" COD="));
  runtimeDebugCdc.print(btHci.last_remote_class[2], HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(btHci.last_remote_class[1], HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(btHci.last_remote_class[0], HEX);
  runtimeDebugCdc.print(F(" TGT="));
  printBtAddress(runtimeDebugCdc, btHci.connect_addr);
  runtimeDebugCdc.print(F(" BLE="));
  runtimeDebugCdc.print(btHci.le_event_mask_configured ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_scan_params_configured ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_scan_enabled ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_event_mask_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_scan_param_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_scan_enable_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_adv_report_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_subevent, HEX);
  runtimeDebugCdc.print(F(" LEADDR="));
  for (int i = 5; i >= 0; --i) {
    if (btHci.last_le_addr[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.last_le_addr[i], HEX);
    if (i > 0) {
      runtimeDebugCdc.print(':');
    }
  }
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_addr_type);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_event_type);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_rssi);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_adv_len);
  runtimeDebugCdc.print('/');
  for (uint8_t i = 0; i < sizeof(btHci.last_le_adv_data); ++i) {
    if (btHci.last_le_adv_data[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.last_le_adv_data[i], HEX);
  }
  runtimeDebugCdc.print(F(" LENAME="));
  runtimeDebugCdc.print(btHci.last_le_name);
  runtimeDebugCdc.print(F(" LETYPE="));
  runtimeDebugCdc.print((int)btHci.last_le_controller_type);
  runtimeDebugCdc.print(F(" LECTRL="));
  runtimeDebugCdc.print(btHci.le_controller_seen_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_seen_controller_type);
  runtimeDebugCdc.print('/');
  for (int i = 5; i >= 0; --i) {
    if (btHci.last_le_controller_addr[i] < 16) {
      runtimeDebugCdc.print('0');
    }
    runtimeDebugCdc.print(btHci.last_le_controller_addr[i], HEX);
    if (i > 0) {
      runtimeDebugCdc.print(':');
    }
  }
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_controller_addr_type);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.last_le_controller_name);
  runtimeDebugCdc.print(F(" LECON="));
  runtimeDebugCdc.print(btHci.le_connection_pending ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_connected ? 1 : 0);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_connect_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_connect_submit_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_connect_complete_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_connect_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)btHci.last_le_connect_status);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(btHci.le_connection_handle, HEX);
  runtimeDebugCdc.println();

  if (btHci.inquiry_slot_count > 0) {
    runtimeDebugCdc.print(F("BTDEV COUNT="));
    runtimeDebugCdc.print((int)btHci.inquiry_slot_count);
    for (uint8_t slotIndex = 0;
         slotIndex < btHci.inquiry_slot_count &&
         slotIndex < USB_BT_HCI_INQUIRY_SLOT_COUNT;
         ++slotIndex) {
      const UsbBtInquiryDevice& device = btHci.inquiry_slots[slotIndex];
      runtimeDebugCdc.print(F(" ["));
      runtimeDebugCdc.print((int)slotIndex);
      runtimeDebugCdc.print(':');
      for (int i = 5; i >= 0; --i) {
        if (device.addr[i] < 16) {
          runtimeDebugCdc.print('0');
        }
        runtimeDebugCdc.print(device.addr[i], HEX);
        if (i > 0) {
          runtimeDebugCdc.print(':');
        }
      }
      runtimeDebugCdc.print(F(" COD="));
      runtimeDebugCdc.print(device.class_of_device[2], HEX);
      runtimeDebugCdc.print(':');
      runtimeDebugCdc.print(device.class_of_device[1], HEX);
      runtimeDebugCdc.print(':');
      runtimeDebugCdc.print(device.class_of_device[0], HEX);
      runtimeDebugCdc.print(F(" RSSI="));
      runtimeDebugCdc.print((int)device.rssi);
      runtimeDebugCdc.print(F(" PS="));
      runtimeDebugCdc.print((int)device.page_scan_repetition_mode);
      runtimeDebugCdc.print(F(" CLK="));
      runtimeDebugCdc.print(device.clock_offset, HEX);
      runtimeDebugCdc.print(F(" EV="));
      runtimeDebugCdc.print((int)device.event_code, HEX);
      runtimeDebugCdc.print(F(" TYPE="));
      runtimeDebugCdc.print((int)device.controller_type);
      runtimeDebugCdc.print(']');
    }
    runtimeDebugCdc.println();
  }
#endif

#ifdef ENABLE_INPUT_USB
  const usb_input_debug_status_t usbInput = inputusb_debug_status();
  runtimeDebugCdc.print(F("USBIN DEVCOUNT="));
  runtimeDebugCdc.print((int)get_usb_device_count());
  runtimeDebugCdc.print(F(" MOUNT="));
  runtimeDebugCdc.print(usbInput.device_mount_count);
  runtimeDebugCdc.print(F(" UMOUNT="));
  runtimeDebugCdc.print(usbInput.device_umount_count);
  runtimeDebugCdc.print(F(" HIDM="));
  runtimeDebugCdc.print(usbInput.hid_mount_count);
  runtimeDebugCdc.print(F(" HIDU="));
  runtimeDebugCdc.print(usbInput.hid_umount_count);
  runtimeDebugCdc.print(F(" RPT="));
  runtimeDebugCdc.print(usbInput.report_count);
  runtimeDebugCdc.print(F(" RQ="));
  runtimeDebugCdc.print(usbInput.report_request_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(usbInput.report_request_success_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(usbInput.report_request_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)usbInput.last_report_request_ok);
  runtimeDebugCdc.print(F(" TX="));
  runtimeDebugCdc.print(usbInput.report_send_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(usbInput.report_send_success_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(usbInput.report_send_fail_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)usbInput.last_report_send_ok);
  runtimeDebugCdc.print(F(" REC="));
  runtimeDebugCdc.print(usbInput.hid_recover_count);
  runtimeDebugCdc.print(F(" SCAN="));
  runtimeDebugCdc.print(usbInput.mounted_scan_count);
  runtimeDebugCdc.print(F(" HC="));
  runtimeDebugCdc.print((int)usbInput.last_hid_count);
  runtimeDebugCdc.print(F(" LAST="));
  runtimeDebugCdc.print(usbInput.last_vid, HEX);
  runtimeDebugCdc.print(':');
  runtimeDebugCdc.print(usbInput.last_pid, HEX);
  runtimeDebugCdc.print(F(" A="));
  runtimeDebugCdc.print((int)usbInput.last_dev_addr);
  runtimeDebugCdc.print(F(" I="));
  runtimeDebugCdc.print((int)usbInput.last_instance);
  runtimeDebugCdc.print(F(" LEN="));
  runtimeDebugCdc.print((int)usbInput.last_report_len);
  runtimeDebugCdc.print(F(" RAW="));
  for (uint8_t i = 0; i < sizeof(usbInput.last_report_head); ++i) {
    if (usbInput.last_report_head[i] < 0x10) runtimeDebugCdc.print('0');
    runtimeDebugCdc.print(usbInput.last_report_head[i], HEX);
  }
  runtimeDebugCdc.println();
#endif
}
#endif

#ifdef ENABLE_USB_AUTH_SIDECAR
void printAuthSidecarSerialStatus() {
  const PsAuthDongleStatus auth = ps_auth_dongle_status();

  runtimeDebugCdc.print(F("PSAUTH SUP="));
  runtimeDebugCdc.print(auth.supported ? 1 : 0);
  runtimeDebugCdc.print(F(" CONN="));
  runtimeDebugCdc.print(auth.connected ? 1 : 0);
  runtimeDebugCdc.print(F(" READY="));
  runtimeDebugCdc.print(auth.signature_ready ? 1 : 0);
  runtimeDebugCdc.print(F(" BUSY="));
  runtimeDebugCdc.print(auth.busy ? 1 : 0);
  runtimeDebugCdc.print(F(" DEV="));
  runtimeDebugCdc.print((int)auth.dev_addr);
  runtimeDebugCdc.print(F(" IF="));
  runtimeDebugCdc.print((int)auth.instance);
  runtimeDebugCdc.print(F(" VID=0x"));
  runtimeDebugCdc.print(auth.vid, HEX);
  runtimeDebugCdc.print(F(" PID=0x"));
  runtimeDebugCdc.print(auth.pid, HEX);
  runtimeDebugCdc.print(F(" LASTVID=0x"));
  runtimeDebugCdc.print(auth.last_vid, HEX);
  runtimeDebugCdc.print(F(" LASTPID=0x"));
  runtimeDebugCdc.print(auth.last_pid, HEX);
  runtimeDebugCdc.print(F(" DESC="));
  runtimeDebugCdc.print(auth.last_desc_len);
  runtimeDebugCdc.print(F(" RC="));
  runtimeDebugCdc.print((int)auth.last_report_count);
  runtimeDebugCdc.print(F(" CAND="));
  runtimeDebugCdc.print(auth.last_descriptor_candidate ? 1 : 0);
  runtimeDebugCdc.print(F(" PROTO="));
  runtimeDebugCdc.print((int)auth.protocol);
  runtimeDebugCdc.print(F(" ERR="));
  runtimeDebugCdc.print((int)auth.last_error);
  runtimeDebugCdc.print(F(" ST="));
  runtimeDebugCdc.print((int)auth.state);
  runtimeDebugCdc.print(F(" NP="));
  runtimeDebugCdc.print((int)auth.nonce_part);
  runtimeDebugCdc.print(F(" SP="));
  runtimeDebugCdc.print((int)auth.signature_part);
  runtimeDebugCdc.print(F(" GET="));
  runtimeDebugCdc.print(auth.get_report_count);
  runtimeDebugCdc.print(F(" SET="));
  runtimeDebugCdc.print(auth.set_report_count);
  runtimeDebugCdc.print(F(" SENT="));
  runtimeDebugCdc.print(auth.sent_report_count);
  runtimeDebugCdc.print(F(" IN="));
  runtimeDebugCdc.print(auth.input_report_count);
  runtimeDebugCdc.print(F(" P5RB="));
  runtimeDebugCdc.print((int)auth.ps5_raw_buttons0, HEX);
  runtimeDebugCdc.print(F("/"));
  runtimeDebugCdc.print((int)auth.ps5_raw_buttons1, HEX);
  runtimeDebugCdc.print(F("/"));
  runtimeDebugCdc.print((int)auth.ps5_raw_buttons2, HEX);
  runtimeDebugCdc.print(F(" P5SB="));
  runtimeDebugCdc.print((int)auth.ps5_signed_buttons0, HEX);
  runtimeDebugCdc.print(F("/"));
  runtimeDebugCdc.print((int)auth.ps5_signed_buttons1, HEX);
  runtimeDebugCdc.print(F("/"));
  runtimeDebugCdc.print((int)auth.ps5_signed_buttons2, HEX);
  runtimeDebugCdc.print(F(" P5Q=0x"));
  runtimeDebugCdc.print((int)auth.ps5_hash_flags, HEX);
  runtimeDebugCdc.print(F(" P5M="));
  runtimeDebugCdc.print(auth.ps5_hash_mismatch_count);
  runtimeDebugCdc.print(F(" P5PREP="));
  runtimeDebugCdc.print(auth.ps5_prepare_count);
  runtimeDebugCdc.print(F(" P5HOST="));
  runtimeDebugCdc.print(auth.ps5_host_submit_count);
  runtimeDebugCdc.print(F(" P5BUSY="));
  runtimeDebugCdc.print(auth.ps5_queue_busy_count);
  runtimeDebugCdc.print(F(" P5HDT="));
  runtimeDebugCdc.print(auth.ps5_last_host_delta_us);
  runtimeDebugCdc.print(F(" P5DDT="));
  runtimeDebugCdc.println(auth.ps5_last_dongle_delta_us);

#ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
  const XboxOneAuthSidecarStatus xbone = xbone_auth_passthrough_status();
  runtimeDebugCdc.print(F("XBAUTH SUP="));
  runtimeDebugCdc.print(xbone.supported ? 1 : 0);
  runtimeDebugCdc.print(F(" MNT="));
  runtimeDebugCdc.print(xbone.mounted ? 1 : 0);
  runtimeDebugCdc.print(F(" RDY="));
  runtimeDebugCdc.print(xbone.dongle_ready ? 1 : 0);
  runtimeDebugCdc.print(F(" AUTH="));
  runtimeDebugCdc.print(xbone.auth_completed ? 1 : 0);
  runtimeDebugCdc.print(F(" DEV="));
  runtimeDebugCdc.print((int)xbone.dev_addr);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print((int)xbone.instance);
  runtimeDebugCdc.print(F(" VID=0x"));
  runtimeDebugCdc.print(xbone.vid, HEX);
  runtimeDebugCdc.print(F(" PID=0x"));
  runtimeDebugCdc.print(xbone.pid, HEX);
  runtimeDebugCdc.print(F(" ST="));
  runtimeDebugCdc.print((int)xbone.state);
  runtimeDebugCdc.print(F(" CMD=0x"));
  runtimeDebugCdc.print((int)xbone.last_command, HEX);
  runtimeDebugCdc.print(F(" SEQ="));
  runtimeDebugCdc.print((int)xbone.last_sequence);
  runtimeDebugCdc.print(F(" LEN="));
  runtimeDebugCdc.print(xbone.last_length);
  runtimeDebugCdc.print(F(" CNT="));
  runtimeDebugCdc.print(xbone.mount_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xbone.input_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xbone.sent_count);
  runtimeDebugCdc.print(F(" Q="));
  runtimeDebugCdc.print(xbone.queue_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xbone.queue_drop_count);
  runtimeDebugCdc.print(F(" BAD="));
  runtimeDebugCdc.print(xbone.invalid_count);
  runtimeDebugCdc.print(F(" AUTHIO="));
  runtimeDebugCdc.print(xbone.console_to_dongle_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.println(xbone.dongle_to_console_count);

  const XboxOneOutputStatus xboneOut = xboxone_output_status();
  runtimeDebugCdc.print(F("XBOUT SUP="));
  runtimeDebugCdc.print(xboneOut.supported ? 1 : 0);
  runtimeDebugCdc.print(F(" MNT="));
  runtimeDebugCdc.print(xboneOut.mounted ? 1 : 0);
  runtimeDebugCdc.print(F(" RDY="));
  runtimeDebugCdc.print(xboneOut.ready ? 1 : 0);
  runtimeDebugCdc.print(F(" AUTH="));
  runtimeDebugCdc.print(xboneOut.auth_ready ? 1 : 0);
  runtimeDebugCdc.print(F(" ST="));
  runtimeDebugCdc.print((int)xboneOut.state);
  runtimeDebugCdc.print(F(" EP="));
  runtimeDebugCdc.print(xboneOut.ep_in, HEX);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xboneOut.ep_out, HEX);
  runtimeDebugCdc.print(F(" CMD=0x"));
  runtimeDebugCdc.print((int)xboneOut.last_command, HEX);
  runtimeDebugCdc.print(F(" SEQ="));
  runtimeDebugCdc.print((int)xboneOut.last_sequence);
  runtimeDebugCdc.print(F(" LEN="));
  runtimeDebugCdc.print(xboneOut.last_len);
  runtimeDebugCdc.print(F(" IO="));
  runtimeDebugCdc.print(xboneOut.out_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xboneOut.in_count);
  runtimeDebugCdc.print(F(" Q="));
  runtimeDebugCdc.print(xboneOut.queue_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.print(xboneOut.queue_drop_count);
  runtimeDebugCdc.print(F(" AUTHIO="));
  runtimeDebugCdc.print(xboneOut.auth_console_count);
  runtimeDebugCdc.print('/');
  runtimeDebugCdc.println(xboneOut.auth_dongle_count);
#endif
}
#endif

#ifdef ENABLE_INPUT_JVS
void printJvsPadStatus(uint8_t index, const JvsHostState& state) {
  const controller_state_t& frame = controllerFrameConst(index);
  runtimeDebugCdc.print(F("JVS P"));
  runtimeDebugCdc.print((int)(index + 1));
  runtimeDebugCdc.print(F(" SW0=0x"));
  printHexByte(state.sw_state0[index]);
  runtimeDebugCdc.print(F(" SW1=0x"));
  printHexByte(state.sw_state1[index]);
  runtimeDebugCdc.print(F(" PAD="));
  printHex32(frame.digital_buttons);
  runtimeDebugCdc.print(F(" CONN="));
  runtimeDebugCdc.print(frame.connected ? 1 : 0);
  runtimeDebugCdc.print(F(" TYPE="));
  runtimeDebugCdc.println(frame.controller_type_name[0] ? frame.controller_type_name : "-");
}

void printJvsSerialStatus() {
  constexpr uint32_t kJvsFreshSyncMs = 250;
  const JvsHostState& state = getJvsHostState();
  const JvsBoardInfo& board = getJvsBoardInfo();
  const JvsDebugSnapshot debug = getJvsDebugSnapshot();

  runtimeDebugCdc.print(F("JVS HOST="));
  runtimeDebugCdc.print((int)debug.machine_state);
  runtimeDebugCdc.print(F(" CONN="));
  runtimeDebugCdc.print(state.connected ? 1 : 0);
  runtimeDebugCdc.print(F(" READY="));
  runtimeDebugCdc.print(state.ready ? 1 : 0);
  runtimeDebugCdc.print(F(" FRESH="));
  runtimeDebugCdc.print(isJvsHostSyncedRecently(kJvsFreshSyncMs) ? 1 : 0);
  runtimeDebugCdc.print(F(" SYNCS="));
  runtimeDebugCdc.print(debug.synced_count);
  runtimeDebugCdc.print(F(" AGE_MS="));
  if (debug.last_sync_at_ms == 0) {
    runtimeDebugCdc.print(F("-1"));
  } else {
    runtimeDebugCdc.print((unsigned long)(millis() - debug.last_sync_at_ms));
  }
  runtimeDebugCdc.print(F(" SVC="));
  runtimeDebugCdc.print(debug.service_count);
  runtimeDebugCdc.print(F(" READY_RET="));
  runtimeDebugCdc.print(debug.host_ready_count);
  runtimeDebugCdc.print(F(" SYNC_REQ="));
  runtimeDebugCdc.println(debug.sync_request_count);

  runtimeDebugCdc.print(F("JVS BUS RXB="));
  runtimeDebugCdc.print(debug.uart_rx_byte_count);
  runtimeDebugCdc.print(F(" TXB="));
  runtimeDebugCdc.print(debug.uart_tx_byte_count);
  runtimeDebugCdc.print(F(" RX_ERR="));
  runtimeDebugCdc.print(debug.uart_rx_error_count);
  runtimeDebugCdc.print(F(" LAST_ERR=0x"));
  printHexByte(debug.last_uart_rx_error_flags);
  runtimeDebugCdc.print(F(" LAST_RX=0x"));
  printHexByte(debug.last_rx_byte);
  runtimeDebugCdc.print(F(" LAST_TX="));
  runtimeDebugCdc.print((int)debug.last_tx_size);
  runtimeDebugCdc.print(F(" TX_ACTIVE="));
  runtimeDebugCdc.print(debug.tx_active ? 1 : 0);
  runtimeDebugCdc.print(F(" RX_READY="));
  runtimeDebugCdc.println(debug.rx_available ? 1 : 0);

  runtimeDebugCdc.print(F("JVS BOARD P="));
  runtimeDebugCdc.print((int)board.players);
  runtimeDebugCdc.print(F(" B="));
  runtimeDebugCdc.print((int)board.buttons);
  runtimeDebugCdc.print(F(" C="));
  runtimeDebugCdc.print((int)board.coins);
  runtimeDebugCdc.print(F(" A="));
  runtimeDebugCdc.print((int)board.analog_channels);
  runtimeDebugCdc.print(F(" ID="));
  runtimeDebugCdc.print(board.id_received ? 1 : 0);
  runtimeDebugCdc.print(F(" FC="));
  runtimeDebugCdc.print(board.function_check_received ? 1 : 0);
  runtimeDebugCdc.print(F(" NAME="));
  runtimeDebugCdc.println((board.id_received && board.io_id[0]) ? board.io_id : "-");

  runtimeDebugCdc.print(F("JVS INPUT COIN=0x"));
  printHexByte(state.coin_state);
  runtimeDebugCdc.print(F(" TEST="));
  runtimeDebugCdc.println((state.coin_state & 0x80) ? 1 : 0);

  for (uint8_t index = 0; index < min((uint8_t)2, (uint8_t)MAX_USB_OUT); ++index) {
    printJvsPadStatus(index, state);
  }
}

void printJvsRawSerialStatus() {
  const JvsDebugSnapshot debug = getJvsDebugSnapshot();
  const JvsHostState& state = getJvsHostState();

  runtimeDebugCdc.print(F("JVSRAW SENSE_MV="));
  runtimeDebugCdc.print((int)debug.sense_mv);
  runtimeDebugCdc.print(F(" HOST="));
  runtimeDebugCdc.print((int)debug.machine_state);
  runtimeDebugCdc.print(F(" SYNCS="));
  runtimeDebugCdc.print(debug.synced_count);
  runtimeDebugCdc.print(F(" RXB="));
  runtimeDebugCdc.print(debug.uart_rx_byte_count);
  runtimeDebugCdc.print(F(" TXB="));
  runtimeDebugCdc.print(debug.uart_tx_byte_count);
  runtimeDebugCdc.print(F(" RX_ERRS="));
  runtimeDebugCdc.print(debug.uart_rx_error_count);
  runtimeDebugCdc.print(F(" LAST_ERR=0x"));
  printHexByte(debug.last_uart_rx_error_flags);
  runtimeDebugCdc.print(F(" LAST_RX=0x"));
  printHexByte(debug.last_rx_byte);
  runtimeDebugCdc.print(F(" LAST_TX="));
  runtimeDebugCdc.print((int)debug.last_tx_size);
  runtimeDebugCdc.print(F(" TX_ACTIVE="));
  runtimeDebugCdc.print(debug.tx_active ? 1 : 0);
  runtimeDebugCdc.print(F(" RX_READY="));
  runtimeDebugCdc.println(debug.rx_available ? 1 : 0);

  runtimeDebugCdc.print(F("JVSRAW DONOR SETIN="));
  runtimeDebugCdc.print(debug.donor_set_input_count);
  runtimeDebugCdc.print(F(" SETOUT="));
  runtimeDebugCdc.print(debug.donor_set_output_count);
  runtimeDebugCdc.print(F(" TXN="));
  runtimeDebugCdc.print(debug.donor_transaction_count);
  runtimeDebugCdc.print(F(" DRAIN="));
  runtimeDebugCdc.print(debug.donor_drain_count);
  runtimeDebugCdc.print(F(" SVC="));
  runtimeDebugCdc.print(debug.service_count);
  runtimeDebugCdc.print(F(" READY_RET="));
  runtimeDebugCdc.print(debug.host_ready_count);
  runtimeDebugCdc.print(F(" SYNC_REQ="));
  runtimeDebugCdc.println(debug.sync_request_count);

  runtimeDebugCdc.print(F("JVSRAW STATE COIN=0x"));
  printHexByte(state.coin_state);
  runtimeDebugCdc.print(F(" TEST="));
  runtimeDebugCdc.println((state.coin_state & 0x80) ? 1 : 0);

  runtimeDebugCdc.print(F("JVSRAW RXDBG="));
  printHexByteArray(debug.last_rx_debug, sizeof(debug.last_rx_debug));
  runtimeDebugCdc.println();

  runtimeDebugCdc.print(F("JVSRAW TXDBG="));
  printHexByteArray(debug.last_tx_debug, sizeof(debug.last_tx_debug));
  runtimeDebugCdc.println();
}
#endif

void handleRuntimeSerialCommand() {
  normalizeSerialCommandBuffer();
  if (serialCommandLength == 0) {
    return;
  }

  serialCommandBuffer[serialCommandLength] = '\0';
  char* remainder = nullptr;

  if (serialCommandEquals("PING")) {
    runtimeDebugCdc.println(F("PONG"));
  } else if (serialCommandEquals("STATUS") ||
             serialCommandEquals("INFO")) {
    printRuntimeSerialStatus();
#if defined(ARDUINO_ARCH_RP2040)
  } else if (serialCommandEquals("TEMP") ||
             serialCommandEquals("TEMPERATURE")) {
    printRp2040TemperatureStatus();
#endif
#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB)
  } else if (serialCommandEquals("USBHOST START")) {
    usb_host_service_request_start();
    runtimeDebugCdc.println(F("OK:USBHOST_START_REQUESTED"));
    printUsbHostSerialStatus();
  } else if (serialCommandEquals("USBHOST RESET")) {
    if (usb_host_service_force_root_reset()) {
      runtimeDebugCdc.println(F("OK:USBHOST_RESET"));
      delay(120);
    } else {
      runtimeDebugCdc.println(F("ERR:USBHOST_NOT_STARTED"));
    }
    printUsbHostSerialStatus();
  } else if (serialCommandEquals("USBHOST")) {
    printUsbHostSerialStatus();
#endif
#ifdef ENABLE_USB_AUTH_SIDECAR
  } else if (serialCommandEquals("AUTH START") ||
             serialCommandEquals("PSAUTH START")) {
    usb_host_service_request_start();
    runtimeDebugCdc.println(F("OK:USBHOST_START_REQUESTED"));
    printUsbHostSerialStatus();
    printAuthSidecarSerialStatus();
#ifdef ENABLE_USB_AUTH_SERIAL_TRACE
  } else if (serialCommandEquals("AUTH P5SIM") ||
             serialCommandEquals("PSAUTH P5SIM") ||
             serialCommandEquals("PS5SIM")) {
    ps_auth_dongle_debug_ps5_simulate(runtimeDebugCdc);
    printAuthSidecarSerialStatus();
#endif
  } else if (serialCommandEquals("AUTH") ||
             serialCommandEquals("PSAUTH")) {
    printAuthSidecarSerialStatus();
#endif
#ifdef ENABLE_INPUT_JVS
  } else if (serialCommandEquals("JVS") ||
             serialCommandEquals("JVSSTATUS")) {
    printJvsSerialStatus();
  } else if (serialCommandEquals("JVSRAW")) {
    printJvsRawSerialStatus();
  } else if (serialCommandEquals("JVSSYNC")) {
    requestJvsHostSync();
    runtimeDebugCdc.println(F("OK:JVSSYNC"));
  } else if (serialCommandEquals("JVSCLR")) {
    clearJvsDebugCounters();
    runtimeDebugCdc.println(F("OK:JVSCLR"));
#endif
  } else if (featureModulesHandleSerialCommand(serialCommandBuffer, runtimeDebugCdc)) {
    // Handled by a feature module such as Bluetooth.
  } else if (handleSerialDebugCommand(serialCommandBuffer, runtimeDebugCdc)) {
    // Handled by the shared machine-readable debug/settings command layer.
  } else if (serialCommandEquals("HELP") ||
             serialCommandEquals("?")) {
#ifdef ENABLE_USB_AUTH_SIDECAR
    runtimeDebugCdc.print(F("CMDS:PING,STATUS,TEMP,STATE,GPIO,DHELP,SET,HOTKEY,CHORD,USBHOST,USBHOST START,USBHOST RESET,AUTH,AUTH START,AUTH P5SIM,AUTH PS5"));
#elif defined(ENABLE_INPUT_USB)
    runtimeDebugCdc.print(F("CMDS:PING,STATUS,TEMP,STATE,GPIO,DHELP,SET,HOTKEY,CHORD,USBHOST,USBHOST START,USBHOST RESET"));
#else
    runtimeDebugCdc.print(F("CMDS:PING,STATUS,TEMP,STATE,GPIO,DHELP,SET,HOTKEY,CHORD"));
#endif
    featureModulesAppendSerialHelp(runtimeDebugCdc);
#ifdef ENABLE_INPUT_JVS
    runtimeDebugCdc.print(F(",JVS,JVSRAW,JVSSYNC,JVSCLR"));
#endif
    runtimeDebugCdc.print(F(",ADAPTSTATE,ADAPTSTATE ON,ADAPTSTATE OFF,ADAPTSTATE RATE <HZ>,OVERLAY,OVERLAY ON,OVERLAY OFF,OVERLAY RATE <HZ>,OLED,OLED ON,OLED OFF,OLED RATE <HZ>,OLED FRAME,OUT <MODE>,WIN <MODE>,LAT,LAT CONFIG,LAT ON,LAT OFF,LAT CTRL <INTERNAL|CONTROLLER>,LAT HOST <INTERNAL|PC|MISTER>,LAT MODE <MODE>,LAT START [COUNT],LAT STOP,LAT ACK [SEQ],LAT DUMP,LAT CLR"));
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
    runtimeDebugCdc.print(F(",LAT TRACE <ON|OFF|DUMP|CLR>"));
#endif
    runtimeDebugCdc.println(F(",RESET,BOOT"));
  } else {
    char* remainder = nullptr;
    if (serialCommandStartsWith("OUT", &remainder)) {
      outputMode_t newMode = OUTPUT_MISTER;
      if (!parseRuntimeOutputModeToken(remainder, &newMode)) {
        runtimeDebugCdc.println(F("ERR:BAD_OUT_MODE"));
      } else {
        saveRuntimeOutputModeFromSerial(newMode);
      }
    } else if (serialCommandStartsWith("WIN", &remainder)) {
      uint8_t newWinOutput = 0;
      if (!parseWindowsOutputToken(remainder, &newWinOutput)) {
        runtimeDebugCdc.println(F("ERR:BAD_WIN_MODE"));
      } else {
        saveWindowsOutputPreferenceFromSerial(newWinOutput);
      }
    } else if (serialCommandStartsWith("ADAPTSTATE", &remainder) ||
               serialCommandStartsWith("STATESTREAM", &remainder)) {
      if (*remainder == '\0' || serialTokenEquals(remainder, "STATUS")) {
        adaptStateWriteStatus(runtimeDebugCdc);
      } else if (serialTokenEquals(remainder, "ON") ||
                 serialTokenEquals(remainder, "START")) {
        inputOverlaySetEnabled(false);
        oledSerialSetEnabled(false);
        adaptStateSetEnabled(true);
        runtimeDebugCdc.println(F("OK:ADAPTSTATE=1"));
      } else if (serialTokenEquals(remainder, "OFF") ||
                 serialTokenEquals(remainder, "STOP")) {
        adaptStateSetEnabled(false);
        runtimeDebugCdc.println(F("OK:ADAPTSTATE=0"));
      } else if (serialCommandStartsWith("ADAPTSTATE RATE", &remainder) ||
                 serialCommandStartsWith("STATESTREAM RATE", &remainder)) {
        uint32_t rateHz = 0;
        if (!parseUnsignedLongToken(remainder, &rateHz) ||
            rateHz < ADAPT_STATE_MIN_RATE_HZ ||
            rateHz > ADAPT_STATE_MAX_RATE_HZ) {
          runtimeDebugCdc.println(F("ERR:BAD_ADAPTSTATE_RATE"));
        } else {
          adaptStateSetRateHz((uint16_t)rateHz);
          runtimeDebugCdc.print(F("OK:ADAPTSTATE_RATE="));
          runtimeDebugCdc.println((int)adaptStateRateHz());
        }
      } else {
        runtimeDebugCdc.println(F("ERR:BAD_ADAPTSTATE_CMD"));
      }
    } else if (serialCommandStartsWith("OVERLAY", &remainder) ||
               serialCommandStartsWith("INPUTOVERLAY", &remainder) ||
               serialCommandStartsWith("RETROSPY", &remainder) ||
               serialCommandStartsWith("RSPY", &remainder)) {
      if (*remainder == '\0' || serialTokenEquals(remainder, "STATUS")) {
        inputOverlayWriteStatus(runtimeDebugCdc);
      } else if (serialTokenEquals(remainder, "ON") ||
                 serialTokenEquals(remainder, "START")) {
        adaptStateSetEnabled(false);
        oledSerialSetEnabled(false);
        inputOverlaySetEnabled(true);
        runtimeDebugCdc.println(F("OK:OVERLAY=1"));
      } else if (serialTokenEquals(remainder, "OFF") ||
                 serialTokenEquals(remainder, "STOP")) {
        inputOverlaySetEnabled(false);
        runtimeDebugCdc.println(F("OK:OVERLAY=0"));
      } else if (serialCommandStartsWith("OVERLAY RATE", &remainder) ||
                 serialCommandStartsWith("INPUTOVERLAY RATE", &remainder) ||
                 serialCommandStartsWith("RETROSPY RATE", &remainder) ||
                 serialCommandStartsWith("RSPY RATE", &remainder)) {
        uint32_t rateHz = 0;
        if (!parseUnsignedLongToken(remainder, &rateHz) ||
            rateHz < INPUT_OVERLAY_MIN_RATE_HZ ||
            rateHz > INPUT_OVERLAY_MAX_RATE_HZ) {
          runtimeDebugCdc.println(F("ERR:BAD_OVERLAY_RATE"));
        } else {
          inputOverlaySetRateHz((uint16_t)rateHz);
          runtimeDebugCdc.print(F("OK:OVERLAY_RATE="));
          runtimeDebugCdc.println((int)inputOverlayRateHz());
        }
      } else {
        runtimeDebugCdc.println(F("ERR:BAD_OVERLAY_CMD"));
      }
    } else if (serialCommandStartsWith("OLED", &remainder) ||
               serialCommandStartsWith("OLEDMIRROR", &remainder) ||
               serialCommandStartsWith("OLEDSTREAM", &remainder)) {
      if (*remainder == '\0' || serialTokenEquals(remainder, "STATUS")) {
        oledSerialWriteStatus(runtimeDebugCdc);
      } else if (serialTokenEquals(remainder, "ON") ||
                 serialTokenEquals(remainder, "START")) {
        adaptStateSetEnabled(false);
        inputOverlaySetEnabled(false);
        oledSerialSetEnabled(true);
        runtimeDebugCdc.println(F("OK:OLED=1"));
      } else if (serialTokenEquals(remainder, "OFF") ||
                 serialTokenEquals(remainder, "STOP")) {
        oledSerialSetEnabled(false);
        runtimeDebugCdc.println(F("OK:OLED=0"));
      } else if (serialTokenEquals(remainder, "FRAME") ||
                 serialTokenEquals(remainder, "SNAP")) {
        oledSerialWriteFrame(runtimeDebugCdc);
      } else if (serialCommandStartsWith("OLED RATE", &remainder) ||
                 serialCommandStartsWith("OLEDMIRROR RATE", &remainder) ||
                 serialCommandStartsWith("OLEDSTREAM RATE", &remainder)) {
        uint32_t rateHz = 0;
        if (!parseUnsignedLongToken(remainder, &rateHz) ||
            rateHz < OLED_SERIAL_MIN_RATE_HZ ||
            rateHz > OLED_SERIAL_MAX_RATE_HZ) {
          runtimeDebugCdc.println(F("ERR:BAD_OLED_RATE"));
        } else {
          oledSerialSetRateHz((uint16_t)rateHz);
          runtimeDebugCdc.print(F("OK:OLED_RATE="));
          runtimeDebugCdc.println((int)oledSerialRateHz());
        }
      } else {
        runtimeDebugCdc.println(F("ERR:BAD_OLED_CMD"));
      }
    } else if (serialCommandStartsWith("LAT", &remainder) ||
               serialCommandStartsWith("LATENCY", &remainder)) {
      if (*remainder == '\0' || serialTokenEquals(remainder, "STATUS")) {
        latencyTest.writeStatus(runtimeDebugCdc);
      } else if (serialTokenEquals(remainder, "CONFIG")) {
        latencyTest.writeStatus(runtimeDebugCdc);
      } else if (serialTokenEquals(remainder, "ON")) {
        saveLatencyEnabledFromSerial(true);
      } else if (serialTokenEquals(remainder, "OFF")) {
        saveLatencyEnabledFromSerial(false);
      } else if (serialTextStartsWith(remainder, "CTRL", &remainder)) {
        bool controllerInLoop = false;
        if (serialTokenEquals(remainder, "CONTROLLER") ||
            serialTokenEquals(remainder, "CTRL")) {
          controllerInLoop = true;
        } else if (!(serialTokenEquals(remainder, "INTERNAL") ||
                     serialTokenEquals(remainder, "NOCTRL"))) {
          runtimeDebugCdc.println(F("ERR:BAD_LAT_CTRL"));
          serialCommandLength = 0;
          return;
        }
        saveLatencyControllerFromSerial(controllerInLoop);
      } else if (serialTextStartsWith(remainder, "HOST", &remainder)) {
        uint8_t hostType = LATENCY_HOST_INTERNAL;
        if (!parseLatencyHostTypeToken(remainder, &hostType)) {
          runtimeDebugCdc.println(F("ERR:BAD_LAT_HOST"));
        } else {
          saveLatencyHostFromSerial(hostType);
        }
      } else if (serialTextStartsWith(remainder, "MODE", &remainder)) {
        uint8_t newMode = LATENCY_MODE_OFF;
        if (!parseLatencyModeToken(remainder, &newMode)) {
          runtimeDebugCdc.println(F("ERR:BAD_LAT_MODE"));
        } else {
          saveLatencyModeFromSerial(newMode);
        }
      } else if (serialTextStartsWith(remainder, "TRIGGER", &remainder) ||
                 serialTextStartsWith(remainder, "TRIG", &remainder) ||
                 serialTextStartsWith(remainder, "POLARITY", &remainder)) {
        if (serialTokenEquals(remainder, "HIGH") ||
            serialTokenEquals(remainder, "ACTIVE_HIGH") ||
            serialTokenEquals(remainder, "ACTIVEHIGH") ||
            serialTokenEquals(remainder, "1")) {
          latencyTest.setTriggerActiveHigh(true);
          latencyTest.writeStatus(runtimeDebugCdc);
        } else if (serialTokenEquals(remainder, "LOW") ||
                   serialTokenEquals(remainder, "ACTIVE_LOW") ||
                   serialTokenEquals(remainder, "ACTIVELOW") ||
                   serialTokenEquals(remainder, "0")) {
          latencyTest.setTriggerActiveHigh(false);
          latencyTest.writeStatus(runtimeDebugCdc);
        } else {
          runtimeDebugCdc.println(F("ERR:BAD_LAT_TRIGGER"));
        }
      } else if (serialTextStartsWith(remainder, "SVC", &remainder) ||
                 serialTextStartsWith(remainder, "SERVICE", &remainder)) {
        handleLatencyServiceSerialCommand(remainder);
      } else if (serialTokenEquals(remainder, "STOP")) {
        latencyTest.stopBench();
        runtimeDebugCdc.println(F("OK:LAT_STOP"));
      } else if (serialTokenEquals(remainder, "CLR")) {
        latencyTest.clearSamples();
        runtimeDebugCdc.println(F("OK:LAT_CLR"));
      } else if (serialTokenEquals(remainder, "DUMP")) {
        latencyTest.dumpSamples(runtimeDebugCdc);
      } else if (serialTextStartsWith(remainder, "START", &remainder)) {
        uint32_t sampleCount = 0;
        if (*remainder != '\0' && !parseUnsignedLongToken(remainder, &sampleCount)) {
          runtimeDebugCdc.println(F("ERR:BAD_LAT_COUNT"));
        } else if (!latencyTest.isCurrentModeReady()) {
          runtimeDebugCdc.println(F("ERR:LAT_NOT_READY"));
        } else {
          latencyTest.startBench(sampleCount);
          runtimeDebugCdc.print(F("OK:LAT_START="));
          runtimeDebugCdc.println(latencyTest.getRunTargetSamples());
        }
      } else if (serialTextStartsWith(remainder, "ACK", &remainder)) {
        uint32_t seq = 0;
        if (*remainder != '\0' && !parseUnsignedLongToken(remainder, &seq)) {
          runtimeDebugCdc.println(F("ERR:BAD_LAT_ACK"));
        } else {
          latencyTest.acknowledgeHost(seq);
          runtimeDebugCdc.println(F("OK:LAT_ACK"));
        }
      } else {
#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)
        char* traceRemainder = nullptr;
        if (serialTextStartsWith(remainder, "TRACE", &traceRemainder) ||
            serialTextStartsWith(remainder, "PHASE", &traceRemainder)) {
          if (*traceRemainder == '\0' || serialTokenEquals(traceRemainder, "STATUS")) {
            latencyPhaseTraceWriteStatus(runtimeDebugCdc);
          } else if (serialTokenEquals(traceRemainder, "ON") ||
                     serialTokenEquals(traceRemainder, "START")) {
            latencyPhaseTraceSetEnabled(true);
            latencyPhaseTraceWriteStatus(runtimeDebugCdc);
          } else if (serialTokenEquals(traceRemainder, "OFF") ||
                     serialTokenEquals(traceRemainder, "STOP")) {
            latencyPhaseTraceSetEnabled(false);
            latencyPhaseTraceWriteStatus(runtimeDebugCdc);
          } else if (serialTokenEquals(traceRemainder, "CLR") ||
                     serialTokenEquals(traceRemainder, "CLEAR")) {
            latencyPhaseTraceClear();
            latencyPhaseTraceWriteStatus(runtimeDebugCdc);
          } else if (serialTokenEquals(traceRemainder, "DUMP")) {
            latencyPhaseTraceDump(runtimeDebugCdc);
          } else {
            runtimeDebugCdc.println(F("ERR:BAD_LAT_TRACE_CMD"));
          }
        } else {
          runtimeDebugCdc.println(F("ERR:BAD_LAT_CMD"));
        }
#else
        runtimeDebugCdc.println(F("ERR:BAD_LAT_CMD"));
#endif
      }
    } else if (serialCommandEquals("RESET") ||
               serialCommandEquals("REBOOT")) {
      runtimeDebugCdc.println(F("OK:REBOOT"));
      runtimeDebugCdc.flush();
      delay(20);
      reboot();
    } else if (serialCommandEquals("B") ||
               serialCommandEquals("BOOT") ||
               serialCommandEquals("BOOTLOADER")) {
      runtimeDebugCdc.println(F("OK:BOOTLOADER"));
      runtimeDebugCdc.flush();
      EEPROM.commit();
      delay(20);
      rp2040.rebootToBootloader();
    } else {
      runtimeDebugCdc.println(F("ERR:UNKNOWN_CMD"));
    }
  }

  serialCommandLength = 0;
}

void processRuntimeSerialCommandsImpl() {
  bool sawSerialActivity = false;
  while (runtimeDebugCdc.available() > 0) {
    const int raw = runtimeDebugCdc.read();
    if (raw < 0) {
      break;
    }
    sawSerialActivity = true;

#ifdef ENABLE_TTY2OLED_SERIAL
    if (tty2oledSerialDrainByte((uint8_t)raw, runtimeDebugCdc)) {
      continue;
    }
#endif

    const char ch = (char)raw;
    if (ch == '\r' || ch == '\n') {
      handleRuntimeSerialCommand();
      continue;
    }
    if (!isPrintableSerialCommandByte(ch)) {
      continue;
    }

    if (serialCommandLength >= kSerialCommandBufferSize - 1) {
      serialCommandLength = 0;
      runtimeDebugCdc.println(F("ERR:CMD_TOO_LONG"));
      continue;
    }

    serialCommandBuffer[serialCommandLength++] = ch;
  }

  if (sawSerialActivity) {
    resetIdleTimer();
  }
}
#endif


}  // namespace

void processRuntimeSerialCommands() {
#ifdef ENABLE_RUNTIME_SERIAL_DEBUG_CDC
  processRuntimeSerialCommandsImpl();
#endif
}

