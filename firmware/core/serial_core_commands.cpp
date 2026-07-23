#include "../product_config.h"

#include "serial_core_commands.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <hardware/gpio.h>
#include <string.h>

#include "button_chord_remap.h"
#include "controller_frame_state.h"
#include "controller_output_cache_state.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "firmware_support.h"
#include "hotkey_combo.h"
#include "serial_command_parser.h"
#include "settings_registry.h"
#include "settings_store.h"
#include "../features/feature_module.h"
#include "../input/mixed/input_mixed_runtime_state.h"
#include "../input/runtime/input_frame_runtime.h"
#include "../input/runtime/input_module_runtime.h"
#include "../menu/quick_config.h"
#include "../menu/menu_idle_runtime.h"
#include "../menu/menu_runtime_state.h"
#include "../output/auth/auth_storage.h"
#include "../output/output_runtime_state.h"
#include "../output/runtime/output_loop_runtime.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE
#include "../output/xinput/output_xinput_auth_runtime_state.h"
#endif
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

#ifdef ENABLE_INPUT_N64
#include "../input/gc64/Input_GC64.h"
#endif

#ifdef ENABLE_USB_AUTH_SIDECAR
#include "../input/usb_host/input_usb_host_service.h"
#include "../output/auth/ps_auth_dongle_runtime.h"
#endif

namespace {

const char* settingScopeName(SettingScope scope) {
  return scope == SettingScope::Global ? "G" : "P";
}

const char* settingTypeName(SettingValueType type) {
  switch (type) {
    case SettingValueType::UInt8: return "U8";
    case SettingValueType::Int8: return "I8";
    case SettingValueType::UInt16: return "U16";
  }
  return "?";
}

void printHex32(Print& out, uint32_t value) {
  out.print(F("0x"));
  for (int shift = 28; shift >= 0; shift -= 4) {
    out.print((value >> shift) & 0x0F, HEX);
  }
}

bool parsedAllKeyword(char* text) {
  text = serialSkipSpaces(text);
  return serialTokenEquals(text, "ALL");
}

DeviceEnum parseSettingModeOrCurrent(char*& text) {
  long rawMode = (long)deviceMode;
  if (*serialSkipSpaces(text) != '\0') {
    serialParseLongToken(text, &rawMode);
  }
  if (rawMode < 0 || rawMode >= RZORD_LAST) {
    return deviceMode;
  }
  return (DeviceEnum)rawMode;
}

bool sanitizedValueInSpecRange(SettingId id, int32_t value) {
  const SettingSpec& spec = settingSpec(id);
  return value >= spec.min_value && value <= spec.max_value;
}

void printSettingMatrixFailure(Print& out, SettingId id, DeviceEnum mode, const char* what,
                               int32_t value, int32_t sanitized) {
  out.print(F("SETMFAIL ID="));
  out.print((int)id);
  out.print(F(" MODE="));
  out.print((int)mode);
  out.print(F(" WHAT="));
  out.print(what);
  out.print(F(" VAL="));
  out.print(value);
  out.print(F(" SAN="));
  out.println(sanitized);
}

bool checkSettingCandidate(Print& out, SettingId id, DeviceEnum mode, const char* what,
                           int32_t value, uint16_t* passCount, uint16_t* failCount) {
  const int32_t sanitized = sanitizeSettingValue(id, mode, value);
  const bool ok = sanitizedValueInSpecRange(id, sanitized) &&
                  sanitizeSettingValue(id, mode, sanitized) == sanitized;
  if (ok) {
    ++(*passCount);
  } else {
    ++(*failCount);
    printSettingMatrixFailure(out, id, mode, what, value, sanitized);
  }
  return ok;
}

void runSettingMatrixForMode(Print& out, DeviceEnum mode, uint16_t* passCount, uint16_t* failCount) {
  for (uint8_t raw = 0; raw < (uint8_t)SettingId::Count; ++raw) {
    const SettingId id = (SettingId)raw;
    if (settingIsPerMode(id) && !isConcreteModeForPerSettings(mode)) {
      continue;
    }
    if (settingIsGlobal(id) && mode != RZORD_NONE) {
      continue;
    }

    const SettingSpec& spec = settingSpec(id);
    const int32_t def = defaultSettingValue(id, mode);
    const int32_t sanDef = sanitizeSettingValue(id, mode, def);
    if (sanDef == def && sanitizedValueInSpecRange(id, sanDef)) {
      ++(*passCount);
    } else {
      ++(*failCount);
      printSettingMatrixFailure(out, id, mode, "DEFAULT", def, sanDef);
    }

    checkSettingCandidate(out, id, mode, "MIN", spec.min_value, passCount, failCount);
    checkSettingCandidate(out, id, mode, "MAX", spec.max_value, passCount, failCount);
    if (spec.min_value > INT32_MIN) {
      checkSettingCandidate(out, id, mode, "BELOW", spec.min_value - 1, passCount, failCount);
    }
    if (spec.max_value < INT32_MAX) {
      checkSettingCandidate(out, id, mode, "ABOVE", spec.max_value + 1, passCount, failCount);
    }

    const int32_t loaded = loadSettingValue(id, mode);
    const int32_t sanLoaded = sanitizeSettingValue(id, mode, loaded);
    if (loaded == sanLoaded && sanitizedValueInSpecRange(id, loaded)) {
      ++(*passCount);
    } else {
      ++(*failCount);
      printSettingMatrixFailure(out, id, mode, "LOADED", loaded, sanLoaded);
    }
  }
}

bool handleSettingMatrixCommand(char* text, Print& out) {
  const bool allModes = parsedAllKeyword(text);
  uint16_t passCount = 0;
  uint16_t failCount = 0;
  uint8_t modeCount = 0;

  runSettingMatrixForMode(out, RZORD_NONE, &passCount, &failCount);
  ++modeCount;

  if (allModes) {
    for (uint8_t rawMode = 0; rawMode < (uint8_t)RZORD_LAST; ++rawMode) {
      const DeviceEnum mode = (DeviceEnum)rawMode;
      if (!isConcreteModeForPerSettings(mode)) {
        continue;
      }
      runSettingMatrixForMode(out, mode, &passCount, &failCount);
      ++modeCount;
    }
  } else {
    DeviceEnum mode = parseSettingModeOrCurrent(text);
    if (isConcreteModeForPerSettings(mode)) {
      runSettingMatrixForMode(out, mode, &passCount, &failCount);
      ++modeCount;
    }
  }

  out.print(F("OK:SET_MATRIX PASS="));
  out.print(passCount);
  out.print(F(" FAIL="));
  out.print(failCount);
  out.print(F(" MODES="));
  out.println((int)modeCount);
  return true;
}

bool handleMixedMatrixCommand(Print& out) {
  DeviceEnum supportedModes[(uint8_t)RZORD_LAST] = {};
  uint8_t modeCount = 0;
  for (uint8_t rawMode = 0; rawMode < (uint8_t)RZORD_LAST; ++rawMode) {
    const DeviceEnum mode = (DeviceEnum)rawMode;
    if (inputMixedPortModeSupported(0, mode) &&
        inputMixedPortModeSupported(1, mode)) {
      supportedModes[modeCount++] = mode;
    }
  }

  uint16_t passCount = 0;
  uint16_t failCount = 0;
  uint16_t pairCount = 0;
  uint16_t supervisorCount = 0;
  for (uint8_t i = 0; i < modeCount; ++i) {
    for (uint8_t j = 0; j < modeCount; ++j) {
      const DeviceEnum port0Mode = supportedModes[i];
      const DeviceEnum port1Mode = supportedModes[j];
      const bool need01 = inputMixedModesNeedSupervisor(port0Mode, port1Mode);
      const bool need10 = inputMixedModesNeedSupervisor(port1Mode, port0Mode);
      const bool self0 = inputMixedModesNeedSupervisor(port0Mode, port0Mode);
      const bool self1 = inputMixedModesNeedSupervisor(port1Mode, port1Mode);
      const bool ok = need01 == need10 && !self0 && !self1;

      ++pairCount;
      if (need01) {
        ++supervisorCount;
      }
      if (ok) {
        ++passCount;
      } else {
        ++failCount;
        out.print(F("MIXED FAIL P0="));
        out.print((int)port0Mode);
        out.print(F(" P1="));
        out.print((int)port1Mode);
        out.print(F(" NEED01="));
        out.print(need01 ? 1 : 0);
        out.print(F(" NEED10="));
        out.print(need10 ? 1 : 0);
        out.print(F(" SELF0="));
        out.print(self0 ? 1 : 0);
        out.print(F(" SELF1="));
        out.println(self1 ? 1 : 0);
      }
    }
  }

  out.print(F("OK:MIXED_MATRIX PASS="));
  out.print(passCount);
  out.print(F(" FAIL="));
  out.print(failCount);
  out.print(F(" MODES="));
  out.print((int)modeCount);
  out.print(F(" PAIRS="));
  out.print(pairCount);
  out.print(F(" SUP="));
  out.println(supervisorCount);
  return true;
}

bool parseOutputModeOrConfigured(char*& text, outputMode_t* mode) {
  long rawMode = (long)configuredOutputMode;
  if (*serialSkipSpaces(text) != '\0') {
    serialParseLongToken(text, &rawMode);
  }
  if (rawMode < 0 || rawMode >= OUTPUT_LAST) {
    return false;
  }
  *mode = canonicalizeOutputMode((outputMode_t)rawMode);
  return true;
}

bool handleSettingVisibleCommand(char* text, Print& out) {
  DeviceEnum inputMode = parseSettingModeOrCurrent(text);
  outputMode_t output = configuredOutputMode;
  if (!parseOutputModeOrConfigured(text, &output)) {
    out.println(F("ERR:BAD_OUTPUT_MODE"));
    return true;
  }

  QuickConfigMenu probe;
  QuickConfigItem items[QCI_COUNT * 2];
  const uint8_t itemCount = probe.debugBuildVisibleItems(
    inputMode,
    output,
    menu_win_output,
    items,
    (uint8_t)(sizeof(items) / sizeof(items[0])));

  bool seen[(uint8_t)SettingId::Count] = {};
  uint8_t visibleSettingCount = 0;
  for (uint8_t i = 0; i < itemCount; ++i) {
    SettingId ids[10];
    const uint8_t idCount = quickConfigItemSettingIds(
      items[i],
      inputMode,
      ids,
      (uint8_t)(sizeof(ids) / sizeof(ids[0])));

    if (idCount == 0) {
      out.print(F("VIS ITEM="));
      out.print((int)items[i]);
      out.println(F(" SET=-"));
      continue;
    }

    for (uint8_t idIndex = 0; idIndex < idCount; ++idIndex) {
      const uint8_t rawId = (uint8_t)ids[idIndex];
      if (rawId >= (uint8_t)SettingId::Count) {
        continue;
      }
      if (!seen[rawId]) {
        seen[rawId] = true;
        ++visibleSettingCount;
      }
      out.print(F("VIS SET="));
      out.print((int)rawId);
      out.print(F(" ITEM="));
      out.print((int)items[i]);
      out.print(F(" MODE="));
      out.print((int)inputMode);
      out.print(F(" OUT="));
      out.println((int)output);
    }
  }

  out.print(F("OK:SET_VISIBLE MODE="));
  out.print((int)inputMode);
  out.print(F(" OUT="));
  out.print((int)output);
  out.print(F(" ITEMS="));
  out.print((int)itemCount);
  out.print(F(" SETTINGS="));
  out.println((int)visibleSettingCount);
  return true;
}

void printSettingLine(Print& out, SettingId id, DeviceEnum mode) {
  const SettingSpec& spec = settingSpec(id);
  out.print(F("SET ID="));
  out.print((int)id);
  out.print(F(" S="));
  out.print(settingScopeName(spec.scope));
  out.print(F(" T="));
  out.print(settingTypeName(spec.value_type));
  out.print(F(" MIN="));
  out.print(spec.min_value);
  out.print(F(" MAX="));
  out.print(spec.max_value);
  out.print(F(" DEF="));
  out.print(defaultSettingValue(id, mode));
  out.print(F(" VAL="));
  out.print(loadSettingValue(id, mode));
  if (settingIsPerMode(id)) {
    out.print(F(" MODE="));
    out.print((int)mode);
  }
  out.println();
}

const char* hotkeyBindingName(HotkeyBindingId id) {
  switch (id) {
    case HotkeyBindingId::Menu: return "MENU";
    case HotkeyBindingId::SystemMenu: return "SYSTEM";
    case HotkeyBindingId::Home: return "HOME";
    case HotkeyBindingId::Capture: return "CAPTURE";
    case HotkeyBindingId::Count: break;
  }
  return "?";
}

bool parseHotkeyBindingName(char*& text, HotkeyBindingId* id) {
  text = serialSkipSpaces(text);
  if (serialTokenEquals(text, "MENU") || serialTokenEquals(text, "OSD")) {
    *id = HotkeyBindingId::Menu;
  } else if (serialTokenEquals(text, "SYSTEM") || serialTokenEquals(text, "SYSMENU") || serialTokenEquals(text, "CONFIG")) {
    *id = HotkeyBindingId::SystemMenu;
  } else if (serialTokenEquals(text, "HOME") || serialTokenEquals(text, "GUIDE")) {
    *id = HotkeyBindingId::Home;
  } else if (serialTokenEquals(text, "CAPTURE") || serialTokenEquals(text, "CAP")) {
    *id = HotkeyBindingId::Capture;
  } else {
    return false;
  }

  while (*text != '\0' && *text != ' ' && *text != '\t') {
    ++text;
  }
  text = serialSkipSpaces(text);
  return true;
}

void applyHotkeysToRuntime(const HotkeyBindingSet& bindings) {
  HotkeyBindingSet sanitized = bindings;
  sanitizeHotkeyBindings(sanitized);
  menu_hotkey_combo = sanitized.menu;
  menu_system_hotkey_combo = sanitized.system_menu;
  menu_home_hotkey_combo = sanitized.home;
  menu_capture_hotkey_combo = sanitized.capture;
  menu_menu_hotkey = menu_hotkey_combo != 0 ? 1 : 0;
  menu_system_menu_hotkey = menu_system_hotkey_combo != 0 ? 1 : 0;
  menu_home_hotkey = menu_home_hotkey_combo != 0 ? 1 : 0;
  menu_capture_hotkey = menu_capture_hotkey_combo != 0 ? 1 : 0;
}

void printHotkeyLine(Print& out, const HotkeyBindingSet& bindings, HotkeyBindingId id) {
  char formatted[32];
  const uint32_t combo = hotkeyBindingValue(bindings, id);
  formatHotkeyCombo(formatted, sizeof(formatted), combo, deviceMode);
  out.print(F("HOTKEY ID="));
  out.print(hotkeyBindingName(id));
  out.print(F(" MASK="));
  printHex32(out, combo);
  out.print(F(" NAME="));
  out.println(formatted);
}

bool handleSerialHotkeyCommandImpl(char* text, Print& out) {
  if (*text == '\0' || serialTokenEquals(text, "HELP")) {
    out.println(F("HOTKEY CMDS:HOTKEY LIST,HOTKEY GET <ID>,HOTKEY SET <ID> <MASK>,HOTKEY HOLD <0-6>,HOTKEY DEFAULTS"));
    return true;
  }

  HotkeyBindingSet bindings{};
  readHotkeyBindings(bindings);
  if (serialTokenEquals(text, "LIST")) {
    for (uint8_t raw = 0; raw < (uint8_t)HotkeyBindingId::Count; ++raw) {
      printHotkeyLine(out, bindings, (HotkeyBindingId)raw);
    }
    out.print(F("HOTKEY HOLD="));
    out.print((int)menu_hotkey_hold_time);
    out.print(F(" MS="));
    out.println(hotkeyHoldMsForSetting(menu_hotkey_hold_time));
    out.println(F("OK:HOTKEY_LIST"));
    return true;
  }

  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "GET", &remainder)) {
    HotkeyBindingId id = HotkeyBindingId::Count;
    if (!parseHotkeyBindingName(remainder, &id)) {
      out.println(F("ERR:BAD_HOTKEY_ID"));
      return true;
    }
    printHotkeyLine(out, bindings, id);
    return true;
  }

  if (serialCommandStartsWith(text, "SET", &remainder)) {
    HotkeyBindingId id = HotkeyBindingId::Count;
    uint32_t combo = 0;
    if (!parseHotkeyBindingName(remainder, &id) ||
        !serialParseUint32Token(remainder, &combo)) {
      out.println(F("ERR:BAD_HOTKEY_ARGS"));
      return true;
    }
    if (combo != 0 && !isValidHotkeyCombo(combo)) {
      out.println(F("ERR:BAD_HOTKEY_COMBO"));
      return true;
    }
    if (combo != 0 && hotkeyComboConflicts(bindings, id, combo)) {
      out.println(F("ERR:HOTKEY_CONFLICT"));
      return true;
    }
    uint32_t* slot = hotkeyBindingSlot(bindings, id);
    if (slot == nullptr) {
      out.println(F("ERR:BAD_HOTKEY_ID"));
      return true;
    }
    *slot = combo;
    sanitizeHotkeyBindings(bindings);
    writeHotkeyBindings(bindings);
    applyHotkeysToRuntime(bindings);
    printHotkeyLine(out, bindings, id);
    out.println(F("OK:HOTKEY_SET"));
    return true;
  }

  if (serialCommandStartsWith(text, "HOLD", &remainder)) {
    long rawValue = 0;
    if (!serialParseLongToken(remainder, &rawValue) || rawValue < 0 || rawValue > 6) {
      out.println(F("ERR:BAD_HOLD_VALUE"));
      return true;
    }
    const int32_t sanitized = sanitizeSettingValue(SettingId::HotkeyHoldTime, RZORD_NONE, rawValue);
    saveSettingValue(SettingId::HotkeyHoldTime, sanitized, RZORD_NONE);
    menu_hotkey_hold_time = (uint8_t)sanitized;
    out.print(F("OK:HOTKEY_HOLD VAL="));
    out.print((int)menu_hotkey_hold_time);
    out.print(F(" MS="));
    out.println(hotkeyHoldMsForSetting(menu_hotkey_hold_time));
    return true;
  }

  if (serialTokenEquals(text, "DEFAULTS") || serialTokenEquals(text, "RESET")) {
    setDefaultHotkeyBindings(bindings);
    writeHotkeyBindings(bindings);
    applyHotkeysToRuntime(bindings);
    out.println(F("OK:HOTKEY_DEFAULTS"));
    return true;
  }

  out.println(F("ERR:BAD_HOTKEY_CMD"));
  return true;
}

void printChordLine(Print& out, uint8_t slot) {
  out.print(F("CHORD SLOT="));
  out.print((int)slot);
  out.print(F(" SRC="));
  printHex32(out, active_chord_remaps.slots[slot].source_combo);
  out.print(F(" OUT="));
  printHex32(out, active_chord_remaps.slots[slot].output_combo);
  out.println();
}

bool handleSerialChordCommandImpl(char* text, Print& out) {
  if (*text == '\0' || serialTokenEquals(text, "HELP")) {
    out.println(F("CHORD CMDS:CHORD LIST,CHORD SET <SLOT> <SRC_MASK> <OUT_MASK>,CHORD CLR <SLOT|ALL>"));
    return true;
  }

  if (serialTokenEquals(text, "LIST")) {
    for (uint8_t slot = 0; slot < BUTTON_CHORD_REMAP_SLOT_COUNT; ++slot) {
      printChordLine(out, slot);
    }
    out.println(F("OK:CHORD_LIST"));
    return true;
  }

  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "SET", &remainder)) {
    long rawSlot = -1;
    uint32_t source = 0;
    uint32_t output = 0;
    if (!serialParseLongToken(remainder, &rawSlot) ||
        rawSlot < 0 ||
        rawSlot >= BUTTON_CHORD_REMAP_SLOT_COUNT ||
        !serialParseUint32Token(remainder, &source) ||
        !serialParseUint32Token(remainder, &output)) {
      out.println(F("ERR:BAD_CHORD_ARGS"));
      return true;
    }
    if (!setButtonChordRemap((uint8_t)rawSlot, source, output)) {
      out.println(F("ERR:BAD_CHORD_MASK"));
      return true;
    }
    printChordLine(out, (uint8_t)rawSlot);
    out.println(F("OK:CHORD_SET"));
    return true;
  }

  if (serialCommandStartsWith(text, "CLR", &remainder) ||
      serialCommandStartsWith(text, "CLEAR", &remainder)) {
    if (serialTokenEquals(remainder, "ALL") || *remainder == '\0') {
      clearButtonChordRemaps();
      out.println(F("OK:CHORD_CLEAR_ALL"));
      return true;
    }
    long rawSlot = -1;
    if (!serialParseLongToken(remainder, &rawSlot) ||
        rawSlot < 0 ||
        rawSlot >= BUTTON_CHORD_REMAP_SLOT_COUNT) {
      out.println(F("ERR:BAD_CHORD_SLOT"));
      return true;
    }
    ButtonChordRemapSet remaps = active_chord_remaps;
    remaps.slots[rawSlot].source_combo = 0;
    remaps.slots[rawSlot].output_combo = 0;
    writeButtonChordRemaps(remaps);
    out.println(F("OK:CHORD_CLEAR"));
    return true;
  }

  out.println(F("ERR:BAD_CHORD_CMD"));
  return true;
}

void printFrameLine(Print& out, uint8_t index) {
  const controller_state_t& frame = controllerFrameConst(index);
  out.print(F("FRAME I="));
  out.print((int)index);
  out.print(F(" CONN="));
  out.print(frame.connected ? 1 : 0);
  out.print(F(" CFG="));
  out.print((int)frame.config);
  out.print(F(" BTN="));
  printHex32(out, frame.digital_buttons);
  out.print(F(" LX="));
  out.print(frame.LX);
  out.print(F(" LY="));
  out.print(frame.LY);
  out.print(F(" RX="));
  out.print(frame.RX);
  out.print(F(" RY="));
  out.print(frame.RY);
  out.print(F(" L2="));
  out.print((int)frame.ANALOG_L2);
  out.print(F(" R2="));
  out.print((int)frame.ANALOG_R2);
  out.print(F(" POX="));
  out.print(post_output_lx[index]);
  out.print(F(" POY="));
  out.print(post_output_ly[index]);
  out.print(F(" PORX="));
  out.print(post_output_rx[index]);
  out.print(F(" PORY="));
  out.print(post_output_ry[index]);
  out.print(F(" HIDX="));
  out.print((int)debug_hid_x[index]);
  out.print(F(" HIDY="));
  out.print((int)debug_hid_y[index]);
  out.print(F(" NAME="));
  out.println(frame.controller_type_name);
}

void printClassicInputDebug(Print& out) {
#ifdef ADAPT_INPUT_RETRO
  out.print(F("CLASSIC MODE="));
  out.print((int)deviceMode);
  out.print(F(" SAVED="));
  out.print((int)savedDeviceMode);
  out.print(F(" PORTS="));
  out.print((int)inputPortCount());
  out.print(F(" POLL_US="));
  out.print(currentInputModulePollIntervalUs());
  out.print(F(" DESC="));
  out.println(currentInputModuleDescriptionOr("-"));

  #ifdef ENABLE_INPUT_PCE
  if (deviceMode == RZORD_PCE || savedDeviceMode == RZORD_PCE) {
    const PceInputDebugSnapshot pce = input_pce_debug_snapshot();
    out.print(F("PCECFG UP="));
    out.print(PCE_TYPE_UPGRADE_CONFIRM_POLLS);
    out.print(F(" CH="));
    out.print(PCE_TYPE_CHANGE_CONFIRM_POLLS);
    out.print(F(" DOWN="));
    out.println(PCE_TYPE_DOWNGRADE_CONFIRM_POLLS);
    for (uint8_t i = 0; i < pce.slot_count && i < MAX_USB_OUT; ++i) {
      const PceInputDebugSlot& slot = pce.slots[i];
      out.print(F("PCE I="));
      out.print((int)i);
      out.print(F(" ST="));
      out.print((int)slot.stable_type);
      out.print(F(" OBS="));
      out.print((int)slot.observed_type);
      out.print(F(" PEND="));
      out.print((int)slot.pending_type);
      out.print(F(" CNT="));
      out.print(slot.pending_count);
      out.print(F(" PORT="));
      out.print((int)slot.source_port);
      out.print(F(" IDX="));
      out.println((int)slot.source_index);
    }
  }
  #endif

  #ifdef ENABLE_INPUT_SATURN
  if (deviceMode == RZORD_SATURN || savedDeviceMode == RZORD_SATURN ||
      deviceMode == RZORD_MEGADRIVE || savedDeviceMode == RZORD_MEGADRIVE) {
    const SaturnInputDebugSnapshot saturn = input_saturn_debug_snapshot();
    for (uint8_t i = 0; i < saturn.port_count && i < MAX_USB_OUT; ++i) {
      const SaturnInputDebugPort& port = saturn.ports[i];
      out.print(F("SATP I="));
      out.print((int)i);
      out.print(F(" TAP="));
      out.print((int)port.tap_ports);
      out.print(F(" LAST="));
      out.print((int)port.last_tap_ports);
      out.print(F(" CNT="));
      out.print((int)port.controller_count);
      out.print(F(" RSV="));
      out.println((int)port.reserved_slots);
    }
    for (uint8_t i = 0; i < saturn.slot_count && i < MAX_USB_OUT; ++i) {
      const SaturnInputDebugSlot& slot = saturn.slots[i];
      out.print(F("SAT I="));
      out.print((int)i);
      out.print(F(" ST="));
      out.print((int)slot.stable_type);
      out.print(F(" OBS="));
      out.print((int)slot.observed_type);
      out.print(F(" MST="));
      out.print((int)slot.megadrive_stable_type);
      out.print(F(" MDOWN="));
      out.print(slot.megadrive_downgrade_ms);
      out.print(F(" TAP="));
      out.print((int)slot.tap_ports);
      out.print(F(" PORT="));
      out.print((int)slot.source_port);
      out.print(F(" IDX="));
      out.println((int)slot.source_index);
    }
    out.print(F("SATM SLOT="));
    out.print((int)saturn_debug_mouse_slot);
    out.print(F(" FLAGS=0x"));
    out.print((int)saturn_debug_mouse_flags, HEX);
    out.print(F(" MX="));
    out.print((int)saturn_debug_mouse_x);
    out.print(F(" MY="));
    out.print((int)saturn_debug_mouse_y);
    out.print(F(" MRX="));
    out.print((int)saturn_debug_mouse_x_min);
    out.print(F("/"));
    out.print((int)saturn_debug_mouse_x_max);
    out.print(F(" MRY="));
    out.print((int)saturn_debug_mouse_y_min);
    out.print(F("/"));
    out.print((int)saturn_debug_mouse_y_max);
    out.print(F(" RAWX="));
    out.print((int)saturn_debug_mouse_raw_x);
    out.print(F(" RAWY="));
    out.print((int)saturn_debug_mouse_raw_y);
    out.print(F(" RRX="));
    out.print((int)saturn_debug_mouse_raw_x_min);
    out.print(F("/"));
    out.print((int)saturn_debug_mouse_raw_x_max);
    out.print(F(" RRY="));
    out.print((int)saturn_debug_mouse_raw_y_min);
    out.print(F("/"));
    out.print((int)saturn_debug_mouse_raw_y_max);
    out.print(F(" MBTN="));
    out.print((int)saturn_debug_mouse_buttons);
    out.print(F(" MOVF="));
    out.println(saturn_debug_mouse_overflow ? 1 : 0);

    out.print(F("SATRAW SLOT="));
    out.print((int)saturn_debug_raw_slot);
    out.print(F(" CT="));
    out.print((int)saturn_debug_raw_type);
    out.print(F(" DS="));
    out.print((int)saturn_debug_raw_size);
    out.print(F(" N="));
    out.print((int)saturn_debug_raw_count);
    out.print(F(" X="));
    out.print((int)saturn_debug_raw_x);
    out.print(F(" XR="));
    out.print((int)saturn_debug_raw_x_min);
    out.print(F("/"));
    out.print((int)saturn_debug_raw_x_max);
    out.print(F(" Y="));
    out.print((int)saturn_debug_raw_y);
    out.print(F(" L="));
    out.print((int)saturn_debug_raw_l);
    out.print(F(" R="));
    out.print((int)saturn_debug_raw_r);
    out.print(F(" NB="));
    for (uint8_t i = 0; i < saturn_debug_raw_count; ++i) {
      out.print((int)(saturn_debug_raw_nibbles[i] & 0x0F), HEX);
    }
    out.println();

    out.print(F("SATMAP SLOT="));
    out.print((int)saturn_debug_map_slot);
    out.print(F(" LX="));
    out.print(saturn_debug_map_lx);
    out.print(F(" LXR="));
    out.print(saturn_debug_map_lx_min);
    out.print(F("/"));
    out.print(saturn_debug_map_lx_max);
    out.print(F(" PAD="));
    out.print((int)saturn_debug_map_paddle);
    out.print(F(" FL="));
    out.println((int)saturn_debug_map_flags, HEX);
  }
  #endif

  #ifdef ENABLE_INPUT_SNES
  if (deviceMode == RZORD_NES || savedDeviceMode == RZORD_NES ||
      deviceMode == RZORD_SNES || savedDeviceMode == RZORD_SNES ||
      deviceMode == RZORD_VBOY || savedDeviceMode == RZORD_VBOY) {
    out.print(F("SNES P0 ID="));
    out.print((int)snes_debug_id0);
    out.print(F(" OBS="));
    out.print((int)snes_debug_dtype0);
    out.print(F(" ST="));
    out.print((int)snes_debug_stable_dtype0);
    out.print(F(" CAND="));
    out.print((int)snes_debug_candidate_dtype0);
    out.print(F("/"));
    out.print((int)snes_debug_candidate_count0);
    out.print(F(" GLITCH="));
    out.print(snes_debug_glitch_frames0);
    out.print(F(" RAW=0x"));
    out.print(snes_debug_raw_digital0, HEX);
    out.print(F(" FILT=0x"));
    out.print(snes_debug_filtered_digital0, HEX);
    out.print(F(" EXT=0x"));
    out.print(snes_debug_extended0, HEX);
    out.print(F(" FDROP="));
    out.print(snes_debug_filter_drop0);
    out.print(F(" BAD="));
    out.print(snes_debug_invalid_frame0);
    out.print(F("/"));
    out.print(snes_debug_invalid_id0);
    out.print(F("/"));
    out.print(snes_debug_all_pressed0);
    out.print(F(" RTECH="));
    out.print(snes_rumbletech_detected0 ? 1 : 0);
    out.print(F(" SAFE="));
    out.print(snes_debug_safe_poll0 ? 1 : 0);
    out.print(F(" RDATA=0x"));
    out.print((int)snes_rumble_debug_data0, HEX);
    out.print(F(" RTX="));
    out.print(snes_rumble_debug_tx0);
    out.print(F(" TAP="));
    out.print((int)snes_debug_tap0);
    out.print(F(" MTAP="));
    out.print(snes_debug_mtap_enabled0 ? 1 : 0);
    out.print(F(" FS="));
    out.print(snes_debug_fourscore0 ? 1 : 0);
    out.print(F(" MEXT=0x"));
    out.print((int)snes_debug_mouse_ext, HEX);
    out.print(F(" MX="));
    out.print((int)snes_debug_mouse_x);
    out.print(F(" MY="));
    out.print((int)snes_debug_mouse_y);
    out.print(F(" MRX="));
    out.print((int)snes_debug_mouse_x_min);
    out.print(F("/"));
    out.print((int)snes_debug_mouse_x_max);
    out.print(F(" MRXE=0x"));
    out.print((int)snes_debug_mouse_x_min_ext, HEX);
    out.print(F("/0x"));
    out.print((int)snes_debug_mouse_x_max_ext, HEX);
    out.print(F(" MRY="));
    out.print((int)snes_debug_mouse_y_min);
    out.print(F("/"));
    out.print((int)snes_debug_mouse_y_max);
    out.print(F(" MRYE=0x"));
    out.print((int)snes_debug_mouse_y_min_ext, HEX);
    out.print(F("/0x"));
    out.print((int)snes_debug_mouse_y_max_ext, HEX);
    out.print(F(" MBTN="));
    out.println((int)snes_debug_mouse_buttons);

    out.print(F("SNES P1 ID="));
    out.print((int)snes_debug_id1);
    out.print(F(" OBS="));
    out.print((int)snes_debug_dtype1);
    out.print(F(" ST="));
    out.print((int)snes_debug_stable_dtype1);
    out.print(F(" CAND="));
    out.print((int)snes_debug_candidate_dtype1);
    out.print(F("/"));
    out.print((int)snes_debug_candidate_count1);
    out.print(F(" GLITCH="));
    out.print(snes_debug_glitch_frames1);
    out.print(F(" RAW=0x"));
    out.print(snes_debug_raw_digital1, HEX);
    out.print(F(" FILT=0x"));
    out.print(snes_debug_filtered_digital1, HEX);
    out.print(F(" EXT=0x"));
    out.print(snes_debug_extended1, HEX);
    out.print(F(" FDROP="));
    out.print(snes_debug_filter_drop1);
    out.print(F(" BAD="));
    out.print(snes_debug_invalid_frame1);
    out.print(F("/"));
    out.print(snes_debug_invalid_id1);
    out.print(F("/"));
    out.print(snes_debug_all_pressed1);
    out.print(F(" RTECH="));
    out.print(snes_rumbletech_detected1 ? 1 : 0);
    out.print(F(" SAFE="));
    out.print(snes_debug_safe_poll1 ? 1 : 0);
    out.print(F(" RDATA=0x"));
    out.print((int)snes_rumble_debug_data1, HEX);
    out.print(F(" RTX="));
    out.print(snes_rumble_debug_tx1);
    out.print(F(" TAP="));
    out.print((int)snes_debug_tap1);
    out.print(F(" MTAP="));
    out.print(snes_debug_mtap_enabled1 ? 1 : 0);
    out.print(F(" FS="));
    out.println(snes_debug_fourscore1 ? 1 : 0);
  }
  #endif

  #ifdef ENABLE_INPUT_N64
  if (deviceMode == RZORD_N64 || savedDeviceMode == RZORD_N64) {
    for (uint8_t i = 0; i < GC64_DEBUG_PORTS; ++i) {
      out.print(F("N64 P"));
      out.print((int)i);
      out.print(F(" TYPE="));
      out.print((int)gc64_debug_device_type[i]);
      out.print(F(" AUX=0x"));
      out.print((int)gc64_n64_accessory_aux[i], HEX);
      out.print(F(" RPAK="));
      out.print(gc64_n64_rumble_pak_detected[i] ? 1 : 0);
      out.print(F(" RPEND="));
      out.print(gc64_n64_rumble_command_pending[i] ? 1 : 0);
      out.print(F(" RTRY="));
      out.print((int)gc64_n64_rumble_probe_attempts[i]);
      out.print(F(" RRES="));
      out.print(gc64_n64_rumble_probe_result[i]);
      out.print(F(" RBYTE=0x"));
      out.print((int)gc64_n64_rumble_probe_byte[i], HEX);
      out.print(F(" WM="));
      out.print(gc64_n64_rumble_motor_result[i]);
      out.print(F(" WX="));
      out.print(gc64_n64_rumble_motor_transport[i]);
      out.print(F(" WEXP=0x"));
      out.print((int)gc64_n64_rumble_motor_expected[i], HEX);
      out.print(F(" WACK=0x"));
      out.println((int)gc64_n64_rumble_motor_response[i], HEX);
    }
  }
  #endif

  #ifdef ENABLE_INPUT_PSX
  if (deviceMode == RZORD_PSX || savedDeviceMode == RZORD_PSX ||
      deviceMode == RZORD_PSX_JOG || savedDeviceMode == RZORD_PSX_JOG) {
    const PsxInputDebugSnapshot psx = input_psx_debug_snapshot();
    for (uint8_t i = 0; i < psx.slot_count && i < MAX_USB_OUT; ++i) {
      const PsxInputDebugSlot& slot = psx.slots[i];
      out.print(F("PSX I="));
      out.print((int)i);
      out.print(F(" CONN="));
      out.print((int)slot.connected);
      out.print(F(" PROTO="));
      out.print((int)slot.protocol);
      out.print(F(" MASK="));
      out.print((int)slot.special_mask);
      out.print(F(" FLAGS="));
      out.print((int)slot.flags);
      out.print(F(" BTN=0x"));
      out.println(slot.buttons, HEX);
    }
  }
  #endif
#endif
}

void printState(Print& out) {
  out.print(F("STATE INPUT="));
  out.print((int)deviceMode);
  out.print(F(" SAVED_INPUT="));
  out.print((int)savedDeviceMode);
  out.print(F(" CONFIG_OUT="));
  out.print((int)configuredOutputMode);
  out.print(F(" RUNTIME_OUT="));
  out.print((int)outputMode);
  out.print(F(" MAX="));
  out.print((int)max_devices);
  out.print(F(" QUIET_PS5="));
  out.println(is_ps5_timing_quiet_mode_active() ? 1 : 0);

  for (uint8_t index = 0; index < max_devices && index < MAX_USB_OUT; ++index) {
    printFrameLine(out, index);
  }

  printClassicInputDebug(out);

  featureModulesAppendSerialState(out);

#ifdef ADAPT_OUTPUT_USB_DEVICE
  const AuthStorageDiagnostics authDiag = authStorageLastDiagnostics();
  out.print(F("AUTH PS4="));
  out.print(authStorageKeyStatus() ? 1 : 0);
  out.print(F(" UP="));
  out.print((int)authStorageLastUploadStatus());
  out.print(F(" CRC="));
  printHex32(out, authStorageLastUploadCrc32());
  out.print(F(" SER="));
  out.print((int)authDiag.serial_region_state);
  out.print(F(" CH="));
  out.print((int)authDiag.received_chunk_count);
  out.print(F("/"));
  out.print((int)authDiag.total_chunk_count);
  out.print(F(" MISS="));
  out.print(authDiag.first_missing_offset);
  out.print(F(" ADDR="));
  out.println(authDiag.target_payload_address);
  // Single-XInput rumble is host/tool dependent; expose the last OUT frame.
  if (get_effective_output_mode() == OUTPUT_XINPUT) {
    const XInputAuthRuntimeState& xin = xinputAuthRuntimeState();
    out.print(F("XIN OUT="));
    out.print(xin.debug_xfer_out_count);
    out.print(F(" SZ="));
    out.print((int)xin.debug_last_out_size);
    out.print(F(" RUM="));
    out.print((int)xin.debug_last_rumble_left);
    out.print(F("/"));
    out.print((int)xin.debug_last_rumble_right);
    out.print(F(" PKT="));
    for (uint8_t b = 0; b < 8; ++b) {
      if (xin.debug_last_out_packet[b] < 16) {
        out.print('0');
      }
      out.print(xin.debug_last_out_packet[b], HEX);
    }
    out.println();
  }
  #if defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  if (get_effective_output_mode() == OUTPUT_XINPUT2P) {
    XInputMultiDiagInfo x2p{};
    xinput_multi_get_diag_info(&x2p);
    out.print(F("X2P CT="));
    out.print((int)x2p.controller_count);
    out.print(F(" AUTH="));
    out.print(x2p.console_auth_observed ? 1 : 0);
    out.print(F(" SER="));
    out.print(x2p.serial_count);
    out.print(F(" CTRL="));
    out.print(x2p.control_setup_count);
    out.print(F(" LC="));
    out.print((int)x2p.last_control_request, HEX);
    out.print('/');
    out.print((int)x2p.last_control_recipient);
    out.print('/');
    out.print((int)x2p.last_control_wIndex);
    out.print('/');
    out.println((int)x2p.last_control_wValue, HEX);
    for (uint8_t index = 0; index < x2p.controller_count && index < XINPUT_MULTI_CONTROLLERS; ++index) {
      out.print(F("X2P I="));
      out.print((int)index);
      out.print(F(" IB="));
      out.print((int)x2p.interface_base[index]);
      out.print(F(" OP="));
      out.print((int)x2p.opened_part_mask[index], HEX);
      out.print(F(" CAP="));
      out.print(x2p.capability_count[index]);
      out.print(F(" D21="));
      out.print(x2p.desc_21_count[index]);
      out.print(F(" IN="));
      out.print(x2p.in_xfer_count[index]);
      out.print(F(" OUT="));
      out.print(x2p.out_xfer_count[index]);
      out.print(F(" SZ="));
      out.print((int)x2p.last_out_size[index]);
      out.print(F(" RUM="));
      out.print((int)x2p.last_rumble_left[index]);
      out.print(F("/"));
      out.print((int)x2p.last_rumble_right[index]);
      out.print(F(" PKT="));
      for (uint8_t b = 0; b < 8; ++b) {
        if (x2p.last_out_packet[index][b] < 16) {
          out.print('0');
        }
        out.print(x2p.last_out_packet[index][b], HEX);
      }
      out.println();
    }
  }
  #endif
#endif

#ifdef ENABLE_USB_AUTH_SIDECAR
  const UsbHostServiceStatus host = usb_host_service_status();
  const PsAuthDongleStatus auth = ps_auth_dongle_status();
  out.print(F("USBHOST AVAIL="));
  out.print(host.available ? 1 : 0);
  out.print(F(" STARTED="));
  out.print(host.started ? 1 : 0);
  out.print(F(" TASK="));
  out.print(host.task_count);
  out.print(F(" VID="));
  out.print(auth.vid, HEX);
  out.print(F(" PID="));
  out.print(auth.pid, HEX);
  out.print(F(" READY="));
  out.print(auth.signature_ready ? 1 : 0);
  out.print(F(" ERR="));
  out.println((int)auth.last_error);
#endif
}

void printGpioSnapshot(Print& out) {
  out.println(F("GPIO COUNT=30 FMT=P,V,DIR,FUNC,PU,PD"));
  for (uint8_t pin = 0; pin < 30; ++pin) {
    out.print(F("GPIO P="));
    out.print((int)pin);
    out.print(F(" V="));
    out.print(gpio_get(pin) ? 1 : 0);
    out.print(F(" DIR="));
    out.print(gpio_get_dir(pin) ? 'O' : 'I');
    out.print(F(" FUNC="));
    out.print((int)gpio_get_function(pin));
    out.print(F(" PU="));
    out.print(gpio_is_pulled_up(pin) ? 1 : 0);
    out.print(F(" PD="));
    out.println(gpio_is_pulled_down(pin) ? 1 : 0);
  }
  out.println(F("OK:GPIO"));
}

bool handleSerialSetCommandImpl(char* text, Print& out) {
  if (*text == '\0' || serialTokenEquals(text, "HELP")) {
    out.println(F("SET CMDS:SET COUNT,SET LIST [MODE],SET VISIBLE [MODE] [OUT],SET MATRIX [MODE|ALL],SET MIXED_MATRIX,SET FACTORY_RESET CONFIRM,SET GET <ID> [MODE],SET SET <ID> <VALUE> [MODE],SET DEFAULT <ID> [MODE]"));
    return true;
  }

  if (serialTokenEquals(text, "COUNT")) {
    out.print(F("SET COUNT="));
    out.println((int)SettingId::Count);
    return true;
  }

  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "LIST", &remainder)) {
    DeviceEnum mode = parseSettingModeOrCurrent(remainder);
    for (uint8_t raw = 0; raw < (uint8_t)SettingId::Count; ++raw) {
      printSettingLine(out, (SettingId)raw, mode);
    }
    out.println(F("OK:SET_LIST"));
    return true;
  }

  if (serialCommandStartsWith(text, "MATRIX", &remainder) ||
      serialCommandStartsWith(text, "TEST", &remainder)) {
    return handleSettingMatrixCommand(remainder, out);
  }

  if (serialCommandStartsWith(text, "MIXED_MATRIX", &remainder) ||
      serialCommandStartsWith(text, "MIXED", &remainder)) {
    return handleMixedMatrixCommand(out);
  }

  if (serialCommandStartsWith(text, "VISIBLE", &remainder) ||
      serialCommandStartsWith(text, "VIS", &remainder)) {
    return handleSettingVisibleCommand(remainder, out);
  }

  if (serialCommandStartsWith(text, "FACTORY_RESET", &remainder)) {
    if (!serialTokenEquals(serialSkipSpaces(remainder), "CONFIRM")) {
      out.println(F("ERR:CONFIRM_REQUIRED"));
      return true;
    }
    factoryResetSettings();
    out.println(F("OK:SET_FACTORY_RESET"));
    return true;
  }

  if (serialCommandStartsWith(text, "GET", &remainder)) {
    long rawId = -1;
    if (!serialParseLongToken(remainder, &rawId) ||
        rawId < 0 ||
        rawId >= (long)SettingId::Count) {
      out.println(F("ERR:BAD_SETTING_ID"));
      return true;
    }
    DeviceEnum mode = parseSettingModeOrCurrent(remainder);
    printSettingLine(out, (SettingId)rawId, mode);
    return true;
  }

  if (serialCommandStartsWith(text, "SET", &remainder)) {
    long rawId = -1;
    long rawValue = 0;
    if (!serialParseLongToken(remainder, &rawId) ||
        rawId < 0 ||
        rawId >= (long)SettingId::Count ||
        !serialParseLongToken(remainder, &rawValue)) {
      out.println(F("ERR:BAD_SET_ARGS"));
      return true;
    }
    const SettingId id = (SettingId)rawId;
    const DeviceEnum mode = parseSettingModeOrCurrent(remainder);
    const int32_t sanitized = sanitizeSettingValue(id, mode, (int32_t)rawValue);
    const bool liveClassicDualMergeSetting =
      (id == SettingId::ClassicDualMerge) && (mode == deviceMode);
    const bool descriptorRebootRequired =
      liveClassicDualMergeSetting && ((uint8_t)sanitized != menu_classic_dual_merge);
    saveSettingValue(id, sanitized, mode);
    if (id == SettingId::ConfiguredOutputMode) {
      configuredOutputMode = canonicalizeOutputMode((outputMode_t)sanitized);
      outputMode = sanitizeRuntimeOutputMode(configuredOutputMode);
    }
    if (id == SettingId::WinOutput) {
      menu_win_output = (uint8_t)sanitized;
    }
    if (id == SettingId::HotkeyHoldTime) {
      menu_hotkey_hold_time = (uint8_t)sanitized;
    }
    if (id == SettingId::MenuHotkey) {
      menu_menu_hotkey = (uint8_t)sanitized;
    }
    if (id == SettingId::SystemMenuHotkey) {
      menu_system_menu_hotkey = (uint8_t)sanitized;
    }
    if (id == SettingId::HomeHotkey) {
      menu_home_hotkey = (uint8_t)sanitized;
    }
    if (id == SettingId::CaptureHotkey) {
      menu_capture_hotkey = (uint8_t)sanitized;
    }
    if (id == SettingId::KioskMode) {
      menu_kiosk_mode = (uint8_t)sanitized;
    }
    if (liveClassicDualMergeSetting) {
      menu_classic_dual_merge = (uint8_t)sanitized;
      if (descriptorRebootRequired) {
        menu_usb_descriptor_reboot_required = 1;
      } else {
        classic_dual_merge_enabled = menu_classic_dual_merge;
      }
    }
    out.print(F("OK:SET ID="));
    out.print(rawId);
    out.print(F(" VAL="));
    out.print(sanitized);
    if (descriptorRebootRequired) {
      out.print(F(" REBOOT=1"));
    }
    out.println();
    return true;
  }

  if (serialCommandStartsWith(text, "DEFAULT", &remainder) ||
      serialCommandStartsWith(text, "RESET", &remainder)) {
    long rawId = -1;
    if (!serialParseLongToken(remainder, &rawId) ||
        rawId < 0 ||
        rawId >= (long)SettingId::Count) {
      out.println(F("ERR:BAD_SETTING_ID"));
      return true;
    }
    const SettingId id = (SettingId)rawId;
    const DeviceEnum mode = parseSettingModeOrCurrent(remainder);
    const int32_t value = defaultSettingValue(id, mode);
    const bool liveClassicDualMergeSetting =
      (id == SettingId::ClassicDualMerge) && (mode == deviceMode);
    const bool descriptorRebootRequired =
      liveClassicDualMergeSetting && ((uint8_t)value != menu_classic_dual_merge);
    saveSettingValue(id, value, mode);
    if (id == SettingId::HotkeyHoldTime) {
      menu_hotkey_hold_time = (uint8_t)value;
    }
    if (id == SettingId::MenuHotkey) {
      menu_menu_hotkey = (uint8_t)value;
    }
    if (id == SettingId::SystemMenuHotkey) {
      menu_system_menu_hotkey = (uint8_t)value;
    }
    if (id == SettingId::HomeHotkey) {
      menu_home_hotkey = (uint8_t)value;
    }
    if (id == SettingId::CaptureHotkey) {
      menu_capture_hotkey = (uint8_t)value;
    }
    if (id == SettingId::KioskMode) {
      menu_kiosk_mode = (uint8_t)value;
    }
    if (liveClassicDualMergeSetting) {
      menu_classic_dual_merge = (uint8_t)value;
      if (descriptorRebootRequired) {
        menu_usb_descriptor_reboot_required = 1;
      } else {
        classic_dual_merge_enabled = menu_classic_dual_merge;
      }
    }
    out.print(F("OK:SET_DEFAULT ID="));
    out.print(rawId);
    out.print(F(" VAL="));
    out.print(value);
    if (descriptorRebootRequired) {
      out.print(F(" REBOOT=1"));
    }
    out.println();
    return true;
  }

  out.println(F("ERR:BAD_SET_CMD"));
  return true;
}


}  // namespace

bool handleSerialSetCommand(char* text, Print& out) {
  return handleSerialSetCommandImpl(text, out);
}

bool handleSerialHotkeyCommand(char* text, Print& out) {
  return handleSerialHotkeyCommandImpl(text, out);
}

bool handleSerialChordCommand(char* text, Print& out) {
  return handleSerialChordCommandImpl(text, out);
}

bool handleSerialUiCommand(char* text, Print& out) {
  if (serialTokenEquals(text, "RESET") || serialTokenEquals(text, "REBOOT")) {
    out.println(F("OK:UI RESET"));
    delay(100);
    reboot();
    return true;
  }
  if (queuePlatformMenuControlAction(text)) {
    out.print(F("OK:UI "));
    out.println(text);
  } else {
    out.println(F("ERR:BAD_UI_CMD"));
  }
  return true;
}

bool handleSerialBootCommand(const char* command, Print& out) {
  if (serialTokenEquals(command, "RESET") || serialTokenEquals(command, "REBOOT")) {
    out.println(F("OK:REBOOT"));
    delay(100);
    reboot();
    return true;
  }
  if (serialTokenEquals(command, "B") ||
      serialTokenEquals(command, "BOOT") ||
      serialTokenEquals(command, "BOOTLOADER")) {
    out.println(F("OK:BOOTLOADER"));
    EEPROM.commit();
    delay(100);
    rp2040.rebootToBootloader();
    return true;
  }
  return false;
}

bool handleSerialStateCommand(const char* command, Print& out) {
  if (serialTokenEquals(command, "STATE") ||
      serialTokenEquals(command, "FRAME") ||
      serialTokenEquals(command, "FRAMES")) {
    printState(out);
    return true;
  }
#ifdef ENABLE_INPUT_SNES
  if (serialTokenEquals(command, "SNESDBG") ||
      serialTokenEquals(command, "SNESDEBUG")) {
    resetSnesPadDebugCounters();
    out.println(F("OK:SNESDEBUG_RESET"));
    return true;
  }
  if (serialTokenEquals(command, "SNESMOUSE")) {
    resetSnesMouseDebugRange();
    out.println(F("OK:SNESMOUSE_RESET"));
    return true;
  }
#endif
#ifdef ENABLE_INPUT_SATURN
  if (serialTokenEquals(command, "SATMOUSE")) {
    resetSaturnMouseDebugRange();
    out.println(F("OK:SATMOUSE_RESET"));
    return true;
  }
#endif
  return false;
}

bool handleSerialGpioCommand(const char* command, Print& out) {
  if (serialTokenEquals(command, "GPIO") || serialTokenEquals(command, "PINS")) {
    printGpioSnapshot(out);
    return true;
  }
  return false;
}

bool handleSerialDreamcastCommand(const char* command, Print& out) {
#ifdef ENABLE_INPUT_DREAMCAST
  if (serialTokenEquals(command, "DCSTAT") ||
      serialTokenEquals(command, "DREAMCAST") ||
      serialTokenEquals(command, "DREAMCAST STATUS")) {
    RZInputDreamcast* dreamcast = currentDreamcastInputModule();
    if (dreamcast == nullptr) {
      out.println(F("ERR:DCSTAT_NOT_DREAMCAST"));
    } else {
      dreamcast->printDiagnostics(out);
    }
    return true;
  }
#endif
  return false;
}
