#include "../product_config.h"

#include "serial_debug_runtime.h"
#include "serial_command_parser.h"
#include "serial_core_commands.h"
#include "serial_latency_commands.h"
#include "serial_management_commands.h"
#include "serial_memcard_commands.h"
#include "serial_oled_commands.h"
#include "serial_rumble_commands.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <Arduino.h>
#include <EEPROM.h>
#include <hardware/gpio.h>

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include <Adafruit_TinyUSB.h>
#endif

#include "button_chord_remap.h"
#include "controller_frame_state.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "firmware_support.h"
#include "hotkey_combo.h"
#include "oled_serial_runtime.h"
#include "settings_registry.h"
#include "settings_store.h"
#include "../features/feature_module.h"
#ifdef ENABLE_TTY2OLED_SERIAL
#include "../features/tty2oled/tty2oled_feature.h"
#endif
#include "../input/runtime/input_frame_runtime.h"
#include "../input/runtime/input_module_runtime.h"
#ifdef ENABLE_INPUT_AUTODETECT
#include "../input/autodetect/Input_AutoDetect.h"
#include "../input/autodetect/input_autodetect_benchmark.h"
#endif
#include "../menu/quick_config.h"
#include "../menu/menu_idle_runtime.h"
#include "../menu/menu_runtime_state.h"
#include "../output/auth/auth_storage.h"
#include "../output/output_runtime_state.h"
#include "../output/runtime/output_loop_runtime.h"
#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
#include "../output/xinput/out_xinput_multi.h"
#endif
#include "../platform/runtime/platform_menu_runtime.h"
#ifdef USE_WS2812
#include "../platform/rgb_led.h"
#endif

#ifdef ENABLE_INPUT_PCE
#include "../input/pce/Input_Pce.h"
#include "../input/pce/input_pce_runtime_state.h"
#endif

#ifdef ENABLE_INPUT_SATURN
#include "../input/saturn/input_saturn_runtime_state.h"
#endif

#ifdef ENABLE_INPUT_SNES
#include "../input/snes/input_snes_runtime_state.h"
#endif

#ifdef ENABLE_INPUT_PSX
#include "../input/psx/Input_Psx.h"
#include "../input/psx/input_psx_runtime_state.h"
#endif

#ifdef ENABLE_INPUT_DREAMCAST
#include "../input/dreamcast/Input_Dreamcast.h"
#endif

#ifdef ENABLE_INPUT_WII
#include "../input/wii/input_wii_trace.h"
#endif

#ifdef ENABLE_INPUT_N64
#include "../input/gc64/Input_GC64.h"
#endif

#ifdef ENABLE_USB_AUTH_SIDECAR
#include "../input/usb_host/input_usb_host_service.h"
#include "../output/auth/ps_auth_dongle_runtime.h"
#endif

namespace {

constexpr size_t kDebugCommandBufferSize = 256;
char debugUartCommandBuffer[kDebugCommandBufferSize];
size_t debugUartCommandLength = 0;
bool debugUartOledStreamActive = false;

#ifndef DEBUG_UART_BAUD
constexpr uint32_t kDebugUartBaud = 115200;
#else
constexpr uint32_t kDebugUartBaud = DEBUG_UART_BAUD;
#endif

void printAutodetectDebugLine(Print& out, uint8_t port) {
#ifdef ENABLE_INPUT_AUTODETECT
  char dbgLine[448];
  getAutoDetectSerialDebugLine(port, dbgLine, sizeof(dbgLine));
  out.print(F("AUTODETECT "));
  out.println(dbgLine);
  if (getAutoDetectJaguarSerialDebugLine(port, dbgLine, sizeof(dbgLine))) {
    out.print(F("AUTODETECT "));
    out.println(dbgLine);
  }
#else
  (void)out;
  (void)port;
#endif
}

bool handleAutodetectSerialCommand(const char* command, Print& out) {
#ifdef ENABLE_INPUT_AUTODETECT
  if (!serialTokenEquals(command, "AUTODETECT") &&
      !serialTokenEquals(command, "ADSCAN")) {
    return false;
  }

  const DeviceEnum detectedMode = runAutoDetection(true);
  out.print(F("AUTODETECT RESULT="));
  out.print(autoDetectResultName(autoDetectResult));
  out.print(F(" MODE="));
  out.print((int)detectedMode);
  out.print(F(" RAW="));
  out.println((int)autoDetectResult);
  printAutodetectDebugLine(out, 0);
  printAutodetectDebugLine(out, 1);
  out.println(F("OK:AUTODETECT"));
  return true;
#else
  (void)command;
  (void)out;
  return false;
#endif
}

bool handleThreeDoProbeSerialCommand(const char* command, Print& out) {
#if defined(ENABLE_INPUT_AUTODETECT) && defined(ENABLE_INPUT_3DO)
  if (!serialTokenEquals(command, "3DOPROBE")) {
    return false;
  }

  constexpr uint8_t kFrameCount = 4;
  for (uint8_t port = 0; port < kAutoDetectPortCount; ++port) {
    uint16_t frames[kFrameCount] = {};
    uint8_t idleLevel = 0;
    const uint8_t captured = AutoDetector::capture3doDiagnosticFrames(
        port, frames, kFrameCount, &idleLevel);
    uint8_t directValid = 0;
    uint8_t invertedValid = 0;

    out.print(F("3DOPROBE P="));
    out.print((int)(port + 1));
    out.print(F(" IDLE="));
    out.print((int)idleLevel);
    out.print(F(" FRAMES="));
    for (uint8_t i = 0; i < captured; ++i) {
      if (i != 0) out.print(',');
      if (frames[i] < 0x1000u) out.print('0');
      if (frames[i] < 0x0100u) out.print('0');
      if (frames[i] < 0x0010u) out.print('0');
      out.print(frames[i], HEX);
      if (frames[i] != 0xFFFFu && frames[i] != 0x0000u &&
          (frames[i] & 0xC007u) == 0x0001u) {
        ++directValid;
      }
      const uint16_t inverted = (uint16_t)~frames[i];
      if (inverted != 0xFFFFu && inverted != 0x0000u &&
          (inverted & 0xC007u) == 0x0001u) {
        ++invertedValid;
      }
    }
    out.print(F(" VALID="));
    out.print((int)directValid);
    out.print(F(" INV_VALID="));
    out.println((int)invertedValid);
  }
  out.println(F("OK:3DOPROBE"));
  return true;
#else
  (void)command;
  (void)out;
  return false;
#endif
}

bool handleLedSerialCommand(char* text, Print& out) {
#ifdef USE_WS2812
  if (*text == '\0' || serialTextEqualsExact(text, "STATUS")) {
    out.print(F("LED MODE="));
    out.print((int)rgbLed.getMode());
    out.print(F(" MODE_NAME=\""));
    out.print(getLedModeName(rgbLed.getMode()));
    out.print(F("\" BRIGHT="));
    out.println((int)rgbLed.getBrightness());
    return true;
  }

  char* valueText = nullptr;
  if (serialCommandStartsWith(text, "MODE", &valueText)) {
    long value = -1;
    char* numeric = valueText;
    const bool numericMode =
        serialParseLongToken(numeric, &value) &&
        *serialSkipSpaces(numeric) == '\0';
    if (!numericMode && serialTextEqualsExact(valueText, "OFF")) {
      value = LED_MODE_OFF;
    } else if (!numericMode && serialTextEqualsExact(valueText, "STATIC")) {
      value = LED_MODE_STATIC;
    } else if (!numericMode &&
               (serialTextEqualsExact(valueText, "BREATHING") ||
                serialTextEqualsExact(valueText, "BREATHE"))) {
      value = LED_MODE_BREATHING;
    } else if (!numericMode && serialTextEqualsExact(valueText, "RAINBOW")) {
      value = LED_MODE_RAINBOW;
    } else if (!numericMode && serialTextEqualsExact(valueText, "REACTIVE")) {
      value = LED_MODE_REACTIVE;
    } else if (!numericMode && serialTextEqualsExact(valueText, "RUMBLE")) {
      value = LED_MODE_RUMBLE;
    } else if (!numericMode && serialTextEqualsExact(valueText, "ANALOG")) {
      value = LED_MODE_ANALOG;
    }
    if (value < LED_MODE_OFF || value >= LED_MODE_LAST) {
      out.println(F("ERR:BAD_LED_MODE"));
      return true;
    }
    led_mode = static_cast<uint8_t>(value);
    saveSettingValue(SettingId::LedMode, led_mode, RZORD_NONE);
    rgbLed.setMode(static_cast<led_mode_t>(led_mode));
    rgbLed.setEnabled(led_mode != LED_MODE_OFF);
    out.print(F("OK:LED_MODE="));
    out.print(value);
    out.print(F(" NAME=\""));
    out.print(getLedModeName(static_cast<led_mode_t>(value)));
    out.println('"');
    return true;
  }

  if (serialCommandStartsWith(text, "BRIGHTNESS", &valueText) ||
      serialCommandStartsWith(text, "BRIGHT", &valueText)) {
    long value = -1;
    if (!serialParseLongToken(valueText, &value) ||
        *serialSkipSpaces(valueText) != '\0' || value < 0 || value > 254) {
      out.println(F("ERR:BAD_LED_BRIGHTNESS"));
      return true;
    }
    led_brightness = static_cast<uint8_t>(value);
    saveSettingValue(
        SettingId::LedBrightness, led_brightness, RZORD_NONE);
    rgbLed.setBrightness(led_brightness);
    out.print(F("OK:LED_BRIGHTNESS="));
    out.println(value);
    return true;
  }

  out.println(F("LED CMDS:LED STATUS,LED MODE <OFF|STATIC|BREATHING|RAINBOW|REACTIVE|RUMBLE|ANALOG>,LED BRIGHTNESS <0-254>"));
  return true;
#else
  (void)text;
  out.println(F("ERR:LED_UNAVAILABLE"));
  return true;
#endif
}

}  // namespace

void serialDebugRuntimeSetup() {
#if defined(ENABLE_DEBUG_UART) && defined(PIN_DEBUG_UART_TX) && defined(PIN_DEBUG_UART_RX)
  Serial1.setTX(PIN_DEBUG_UART_TX);
  Serial1.setRX(PIN_DEBUG_UART_RX);
  Serial1.begin(kDebugUartBaud);
#endif
}

void serialDebugRuntimeTask() {
#if defined(ENABLE_DEBUG_UART) && defined(PIN_DEBUG_UART_TX) && defined(PIN_DEBUG_UART_RX)
  bool sawSerialActivity = false;
  while (Serial1.available() > 0) {
    const int raw = Serial1.read();
    if (raw < 0) {
      break;
    }
    sawSerialActivity = true;

#ifdef ENABLE_TTY2OLED_SERIAL
    if (tty2oledSerialDrainByte((uint8_t)raw, Serial1)) {
      continue;
    }
#endif

    const char ch = (char)raw;
    if (ch == '\r' || ch == '\n') {
      if (debugUartCommandLength != 0) {
        debugUartCommandBuffer[debugUartCommandLength] = '\0';
        const bool oledCommand = serialIsOledCommand(debugUartCommandBuffer);
        if (!handleSerialDebugCommand(debugUartCommandBuffer, Serial1)) {
          Serial1.println(F("ERR:UNKNOWN_CMD"));
        }
        if (oledCommand) {
          debugUartOledStreamActive = oledSerialIsEnabled();
        }
        debugUartCommandLength = 0;
      }
      continue;
    }

    if (debugUartCommandLength >= kDebugCommandBufferSize - 1) {
      debugUartCommandLength = 0;
      Serial1.println(F("ERR:CMD_TOO_LONG"));
      continue;
    }
    debugUartCommandBuffer[debugUartCommandLength++] = ch;
  }
  if (sawSerialActivity) {
    resetIdleTimer();
  }
  if (debugUartOledStreamActive) {
    oledSerialTask(Serial1);
  }
#endif
}

bool handleSerialDebugCommand(const char* command, Print& out) {
  char* remainder = nullptr;
  if (handleSerialManagementCommand(command, out)) {
    return true;
  }
  if (serialCommandStartsWith(command, "SET", &remainder)) {
    return handleSerialSetCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "HOTKEY", &remainder)) {
    return handleSerialHotkeyCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "CHORD", &remainder)) {
    return handleSerialChordCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "LATENCY", &remainder) ||
      serialCommandStartsWith(command, "LAT", &remainder)) {
    return handleSerialLatencyCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "RUMBLE", &remainder)) {
    return handleSerialRumbleCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "LED", &remainder)) {
    return handleLedSerialCommand(remainder, out);
  }
  if (handleSerialDreamcastCommand(command, out)) {
    return true;
  }
#ifdef ENABLE_INPUT_WII
  if (handleWiiTraceCommand(command, out)) {
    return true;
  }
#endif
#ifdef ADAPT_FEATURE_SERIAL_MEMCARD_API
  if (serialCommandStartsWith(command, "CARD", &remainder)) {
    return handleSerialMemcardCommand(remainder, out);
  }
#endif
  if (featureModulesHandleSerialCommand(command, out)) {
    return true;
  }
#ifdef ENABLE_INPUT_AUTODETECT
  if (serialCommandStartsWith(command, "ADBENCH", &remainder)) {
    return handleAutoDetectBenchmarkCommand(remainder, out);
  }
#endif
  if (handleAutodetectSerialCommand(command, out)) {
    return true;
  }
  if (handleThreeDoProbeSerialCommand(command, out)) {
    return true;
  }
#ifdef ENABLE_INPUT_PSX
  if (serialTokenEquals(command, "PSXSTAT") ||
      serialTokenEquals(command, "PSX STATUS")) {
    RZInputPSX* psx = currentPsxInputModule();
    if (psx == nullptr) {
      out.println(F("STATUS PSX NONE"));
    } else {
      psx->printDebugStatus(out);
    }
    return true;
  }
  if (serialTokenEquals(command, "PSXPROBE")) {
    RZInputPSX* psx = currentPsxInputModule();
    if (psx == nullptr) {
      out.println(F("PSXPROBE NONE"));
    } else {
      psx->printDebugProbe(out);
    }
    return true;
  }
#endif
  if (serialCommandStartsWith(command, "UI", &remainder)) {
    return handleSerialUiCommand(remainder, out);
  }
  if (handleSerialOledCommand(command, out)) {
    return true;
  }
  if (handleSerialBootCommand(command, out) ||
      handleSerialStateCommand(command, out) ||
      handleSerialGpioCommand(command, out)) {
    return true;
  }
  if (serialTokenEquals(command, "DHELP") ||
      serialTokenEquals(command, "DEBUG HELP")) {
    out.print(F("DEBUG CMDS:SET,HOTKEY,CHORD,LATENCY,RUMBLE,LED,STATE,GPIO,RESET,BOOT"));
    appendSerialManagementHelp(out);
#ifdef ENABLE_INPUT_DREAMCAST
    out.print(F(",DCSTAT"));
#endif
#ifdef ENABLE_INPUT_WII
    out.print(F(",WII TRACE"));
#endif
#ifdef ADAPT_FEATURE_SERIAL_MEMCARD_API
    out.print(F(",CARD"));
#endif
    out.print(F(",AUTODETECT,ADSCAN,3DOPROBE,UI <MENU|UP|DOWN|LEFT|RIGHT|OK|BACK|RESET>,OLED,OLED ON,OLED OFF,OLED RATE <HZ>,OLED FRAME"));
    featureModulesAppendSerialHelp(out);
    out.println();
    return true;
  }
  return false;
}
