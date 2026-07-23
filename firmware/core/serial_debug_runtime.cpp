#include "../product_config.h"

#include "serial_debug_runtime.h"
#include "serial_command_parser.h"
#include "serial_core_commands.h"
#include "serial_latency_commands.h"
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
    out.print(F("DEBUG CMDS:SET,HOTKEY,CHORD,LATENCY,RUMBLE,STATE,GPIO,RESET,BOOT"));
#ifdef ENABLE_INPUT_DREAMCAST
    out.print(F(",DCSTAT"));
#endif
#ifdef ENABLE_INPUT_WII
    out.print(F(",WII TRACE"));
#endif
#ifdef ADAPT_FEATURE_SERIAL_MEMCARD_API
    out.print(F(",CARD"));
#endif
    out.print(F(",AUTODETECT,ADSCAN,UI <MENU|UP|DOWN|LEFT|RIGHT|OK|BACK|RESET>,OLED,OLED ON,OLED OFF,OLED RATE <HZ>,OLED FRAME"));
    featureModulesAppendSerialHelp(out);
    out.println();
    return true;
  }
  return false;
}
