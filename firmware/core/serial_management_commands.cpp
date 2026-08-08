#include "../product_config.h"

#include "serial_management_commands.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "button_remap.h"
#include "controller_frame_state.h"
#include "device_runtime_state.h"
#include "firmware_support.h"
#include "serial_command_parser.h"
#include "serial_core_commands.h"
#include "settings_store.h"
#include "turbo.h"
#include "../firmware_platform_config.h"
#include "../menu/menu_input_mode.h"
#include "../menu/menu_mode_labels.h"
#include "../menu/menu_mode_state.h"
#include "../output/auth/auth_storage.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE
#include "../output/auth/webhid_auth_runtime.h"
#endif
#include "../output/output_runtime_state.h"
#include "../platform/runtime/platform_menu_runtime.h"
#include "../platform/webhid_runtime.h"

namespace {

char upperAscii(char value) {
  return value >= 'a' && value <= 'z'
      ? static_cast<char>(value - ('a' - 'A'))
      : value;
}

bool ignoredNamePunctuation(char value) {
  return value == ' ' || value == '\t' || value == '_' || value == '-' ||
         value == '+' || value == '/' || value == '.';
}

bool normalizedNameEquals(const char* left, const char* right) {
  if (left == nullptr || right == nullptr) return false;
  while (true) {
    while (ignoredNamePunctuation(*left)) ++left;
    while (ignoredNamePunctuation(*right)) ++right;
    if (*left == '\0' || *right == '\0') {
      while (ignoredNamePunctuation(*left)) ++left;
      while (ignoredNamePunctuation(*right)) ++right;
      return *left == '\0' && *right == '\0';
    }
    if (upperAscii(*left) != upperAscii(*right)) return false;
    ++left;
    ++right;
  }
}

bool readToken(char*& text, char* token, size_t tokenSize) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || token == nullptr || tokenSize == 0) return false;
  size_t length = 0;
  while (*text != '\0' && *text != ' ' && *text != '\t') {
    if (length + 1 < tokenSize) token[length++] = *text;
    ++text;
  }
  token[length] = '\0';
  text = serialSkipSpaces(text);
  return length != 0;
}

bool parseSingleLong(char* text, long* value) {
  return serialParseLongToken(text, value) &&
         *serialSkipSpaces(text) == '\0';
}

bool verifyGlobalSettingWrite(SettingId id, int32_t expected, Print& out) {
  const int32_t stored = loadSettingValue(id, RZORD_NONE);
  if (stored == expected) return true;
  out.print(F("ERR:SETTING_WRITE_FAILED ID="));
  out.print((int)id);
  out.print(F(" EXPECT="));
  out.print(expected);
  out.print(F(" VAL="));
  out.println(stored);
  return false;
}

bool parseInputMode(const char* text, DeviceEnum* mode) {
  if (text == nullptr || mode == nullptr) return false;
  char* mutableText = const_cast<char*>(text);
  long numeric = -1;
  if (parseSingleLong(mutableText, &numeric)) {
    if (numeric > RZORD_NONE && numeric < RZORD_LAST &&
        !should_hide_input_mode(static_cast<DeviceEnum>(numeric))) {
      *mode = static_cast<DeviceEnum>(numeric);
      return true;
    }
    return false;
  }

  for (uint8_t raw = 1; raw < static_cast<uint8_t>(RZORD_LAST); ++raw) {
    const DeviceEnum candidate = static_cast<DeviceEnum>(raw);
    if (!should_hide_input_mode(candidate) &&
        normalizedNameEquals(text, getInputModeName(candidate))) {
      *mode = candidate;
      return true;
    }
  }
  return false;
}

bool parseOutputMode(const char* text, outputMode_t* mode) {
  if (text == nullptr || mode == nullptr) return false;
  char* mutableText = const_cast<char*>(text);
  long numeric = -1;
  if (parseSingleLong(mutableText, &numeric)) {
    if (numeric >= 0 && numeric < OUTPUT_LAST &&
        !should_hide_output_mode(static_cast<outputMode_t>(numeric))) {
      *mode = canonicalizeOutputMode(static_cast<outputMode_t>(numeric));
      return true;
    }
    return false;
  }

  for (uint8_t raw = 0; raw < static_cast<uint8_t>(OUTPUT_LAST); ++raw) {
    const outputMode_t candidate = static_cast<outputMode_t>(raw);
    if (!should_hide_output_mode(candidate) &&
        normalizedNameEquals(text, getOutputShortName(candidate))) {
      *mode = canonicalizeOutputMode(candidate);
      return true;
    }
  }

  struct Alias {
    const char* name;
    outputMode_t mode;
  };
  static const Alias aliases[] = {
      {"HID", OUTPUT_HID}, {"DINPUT", OUTPUT_MISTER},
      {"MISTER", OUTPUT_MISTER}, {"LINUX", OUTPUT_MISTER},
      {"XINPUT", OUTPUT_XINPUT2P}, {"XINPUTPC", OUTPUT_XINPUT2P},
      {"XBOX360", OUTPUT_XINPUT}, {"X360", OUTPUT_XINPUT},
      {"XID", OUTPUT_XID}, {"XBOXOG", OUTPUT_XID},
      {"XBOXONE", OUTPUT_XBOXONE}, {"XBONE", OUTPUT_XBOXONE},
      {"SWITCH", OUTPUT_SWITCHPRO}, {"SWITCHPRO", OUTPUT_SWITCHPRO},
      {"GC", OUTPUT_GCWIIU}, {"GCWIIU", OUTPUT_GCWIIU},
      {"KBD", OUTPUT_KEYBOARD},
  };
  for (const Alias& alias : aliases) {
    if (normalizedNameEquals(text, alias.name) &&
        alias.mode < OUTPUT_LAST && !should_hide_output_mode(alias.mode)) {
      *mode = canonicalizeOutputMode(alias.mode);
      return true;
    }
  }
  return false;
}

void printInputStatus(Print& out) {
  out.print(F("INPUT CURRENT="));
  out.print((int)deviceMode);
  out.print(F(" NAME=\""));
  out.print(getInputModeName(deviceMode));
  out.print(F("\" SAVED="));
  out.print((int)savedDeviceMode);
  out.print(F(" SAVED_NAME=\""));
  out.print(getInputModeName(savedDeviceMode));
  out.println('"');
}

void printInputList(Print& out) {
  for (uint8_t raw = 1; raw < static_cast<uint8_t>(RZORD_LAST); ++raw) {
    const DeviceEnum mode = static_cast<DeviceEnum>(raw);
    if (should_hide_input_mode(mode)) continue;
    out.print(F("INPUTMODE ID="));
    out.print((int)raw);
    out.print(F(" NAME=\""));
    out.print(getInputModeName(mode));
    out.println('"');
  }
  out.println(F("OK:INPUT_LIST"));
}

bool handleInputCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || serialTextEqualsExact(text, "STATUS") ||
      serialTextEqualsExact(text, "GET")) {
    printInputStatus(out);
    return true;
  }
  if (serialTextEqualsExact(text, "LIST")) {
    printInputList(out);
    return true;
  }
  char* modeText = text;
  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "SET", &remainder)) modeText = remainder;

  DeviceEnum mode = RZORD_NONE;
  if (!parseInputMode(modeText, &mode)) {
    out.println(F("ERR:BAD_INPUT_MODE"));
    return true;
  }
  persistConfiguredInputMode(mode);
  if (!verifyGlobalSettingWrite(SettingId::PersistedInputMode, mode, out)) {
    return true;
  }
  out.print(F("OK:INPUT="));
  out.print((int)mode);
  out.print(F(" NAME=\""));
  out.print(getInputModeName(mode));
  out.println(F("\" REBOOT=1"));
  delay(100);
  reboot();
  return true;
}

void printOutputStatus(Print& out) {
  out.print(F("OUTPUT CONFIGURED="));
  out.print((int)configuredOutputMode);
  out.print(F(" NAME=\""));
  out.print(getOutputShortName(configuredOutputMode));
  out.print(F("\" RUNTIME="));
  out.print((int)outputMode);
  out.print(F(" RUNTIME_NAME=\""));
  out.print(getOutputShortName(outputMode));
  out.println('"');
}

void printOutputList(Print& out) {
  for (uint8_t raw = 0; raw < static_cast<uint8_t>(OUTPUT_LAST); ++raw) {
    const outputMode_t mode = static_cast<outputMode_t>(raw);
    if (should_hide_output_mode(mode) ||
        canonicalizeOutputMode(mode) != mode) {
      continue;
    }
    out.print(F("OUTPUTMODE ID="));
    out.print((int)raw);
    out.print(F(" NAME=\""));
    out.print(getOutputShortName(mode));
    out.println('"');
  }
  out.println(F("OK:OUTPUT_LIST"));
}

bool handleOutputCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || serialTextEqualsExact(text, "STATUS") ||
      serialTextEqualsExact(text, "GET")) {
    printOutputStatus(out);
    return true;
  }
  if (serialTextEqualsExact(text, "LIST")) {
    printOutputList(out);
    return true;
  }
  char* modeText = text;
  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "SET", &remainder)) modeText = remainder;

  outputMode_t mode = OUTPUT_MISTER;
  if (!parseOutputMode(modeText, &mode)) {
    out.println(F("ERR:BAD_OUTPUT_MODE"));
    return true;
  }
  saveSettingValue(SettingId::ConfiguredOutputMode, mode, RZORD_NONE);
  if (!verifyGlobalSettingWrite(SettingId::ConfiguredOutputMode, mode, out)) {
    return true;
  }
  if (mode == OUTPUT_AUTO) {
    autoDetectState = AUTO_STATE_IDLE;
    auto_detect_clear_scratch_state();
  }
  out.print(F("OK:OUTPUT="));
  out.print((int)mode);
  out.print(F(" NAME=\""));
  out.print(getOutputShortName(mode));
  out.println(F("\" REBOOT=1"));
  delay(100);
  reboot();
  return true;
}

bool handlePlayerCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text != '\0' && !serialTextEqualsExact(text, "STATUS") &&
      !serialTextEqualsExact(text, "LIST")) {
    out.println(F("ERR:PLAYER_LIST_ONLY"));
    return true;
  }
  for (uint8_t player = 0; player < MAX_USB_OUT; ++player) {
    const controller_state_t& frame = controllerFrameConst(player);
    out.print(F("PLAYER N="));
    out.print((int)player + 1);
    out.print(F(" CONNECTED="));
    out.print(frame.connected ? 1 : 0);
    out.print(F(" TYPE=\""));
    out.print(frame.controller_type_name);
    out.println('"');
  }
  out.println(F("OK:PLAYER_LIST"));
  return true;
}

bool parseTurboRate(const char* text, TurboRate* rate) {
  if (text == nullptr || rate == nullptr) return false;
  char* mutableText = const_cast<char*>(text);
  long numeric = -1;
  if (parseSingleLong(mutableText, &numeric)) {
    if (numeric >= 0 && numeric < TURBO_RATE_LAST) {
      *rate = static_cast<TurboRate>(numeric);
      return true;
    }
    return false;
  }
  static const char* const names[TURBO_RATE_LAST] = {
      "OFF", "SLOW", "MEDIUM", "FAST", "MAX", "ULTRA",
      "SLOW_LATCHED", "MEDIUM_LATCHED", "FAST_LATCHED",
      "MAX_LATCHED", "ULTRA_LATCHED"};
  for (uint8_t raw = 0; raw < TURBO_RATE_LAST; ++raw) {
    if (normalizedNameEquals(text, names[raw])) {
      *rate = static_cast<TurboRate>(raw);
      return true;
    }
  }
  return false;
}

bool parseTurboButton(DeviceEnum mode, const char* text, uint8_t* button) {
  if (text == nullptr || button == nullptr) return false;
  char* mutableText = const_cast<char*>(text);
  long numeric = -1;
  if (parseSingleLong(mutableText, &numeric)) {
    if (numeric >= 0 && numeric < TURBO_BTN_COUNT) {
      *button = static_cast<uint8_t>(numeric);
      return true;
    }
    return false;
  }
  const TurboButtonConfig& config =
      getTurboButtonConfig(getTurboInputModeForDeviceMode(mode));
  for (uint8_t index = 0; index < config.count; ++index) {
    if (normalizedNameEquals(text, config.names[index])) {
      *button = config.indices[index];
      return true;
    }
  }
  return false;
}

void printTurboList(Print& out) {
  PerModeSettingsRecord settings{};
  readPerModeSettings(deviceMode, settings);
  const TurboButtonConfig& config =
      getTurboButtonConfig(getTurboInputModeForDeviceMode(deviceMode));
  for (uint8_t index = 0; index < config.count; ++index) {
    const uint8_t button = config.indices[index];
    out.print(F("TURBO BUTTON="));
    out.print((int)button);
    out.print(F(" NAME="));
    out.print(config.names[index]);
    out.print(F(" RATE="));
    out.print((int)settings.turbo_rates[button]);
    out.print(F(" RATE_NAME=\""));
    out.print(getTurboRateFullName(
        static_cast<TurboRate>(settings.turbo_rates[button])));
    out.println('"');
  }
  out.println(F("OK:TURBO_LIST"));
}

bool handleTurboCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || serialTextEqualsExact(text, "STATUS") ||
      serialTextEqualsExact(text, "LIST")) {
    printTurboList(out);
    return true;
  }
  char* clearRemainder = nullptr;
  if (serialCommandStartsWith(text, "CLEAR", &clearRemainder) ||
      serialCommandStartsWith(text, "DEFAULT", &clearRemainder)) {
    if (!serialTextEqualsExact(clearRemainder, "CONFIRM")) {
      out.println(F("ERR:CONFIRM_REQUIRED"));
      return true;
    }
    PerModeSettingsRecord settings{};
    readPerModeSettings(deviceMode, settings);
    memset(settings.turbo_rates, TURBO_OFF, sizeof(settings.turbo_rates));
    writePerModeSettings(deviceMode, settings);
    turbo.setAllRates(settings.turbo_rates);
    out.println(F("OK:TURBO_CLEAR"));
    return true;
  }
  char* remainder = nullptr;
  if (!serialCommandStartsWith(text, "SET", &remainder)) {
    out.println(F("ERR:BAD_TURBO_CMD"));
    return true;
  }
  char buttonToken[24] = {};
  char rateToken[24] = {};
  if (!readToken(remainder, buttonToken, sizeof(buttonToken)) ||
      !readToken(remainder, rateToken, sizeof(rateToken)) ||
      *remainder != '\0') {
    out.println(F("ERR:BAD_TURBO_ARGS"));
    return true;
  }
  uint8_t button = 0;
  TurboRate rate = TURBO_OFF;
  if (!parseTurboButton(deviceMode, buttonToken, &button) ||
      !parseTurboRate(rateToken, &rate)) {
    out.println(F("ERR:BAD_TURBO_ARGS"));
    return true;
  }
  PerModeSettingsRecord settings{};
  readPerModeSettings(deviceMode, settings);
  settings.turbo_rates[button] = static_cast<uint8_t>(rate);
  writePerModeSettings(deviceMode, settings);
  turbo.setButtonRate(static_cast<TurboButton>(button), rate);
  out.print(F("OK:TURBO_SET BUTTON="));
  out.print((int)button);
  out.print(F(" RATE="));
  out.println((int)rate);
  return true;
}

bool parseRemapButton(DeviceEnum mode, const char* text, uint8_t* slot) {
  if (text == nullptr || slot == nullptr) return false;
  char* mutableText = const_cast<char*>(text);
  long numeric = -1;
  if (parseSingleLong(mutableText, &numeric)) {
    if (numeric >= 0 && numeric < REMAP_MAX_BUTTONS) {
      *slot = static_cast<uint8_t>(numeric);
      return true;
    }
    return false;
  }
  const uint8_t count = getRemapButtonCount(mode);
  for (uint8_t display = 0; display < count; ++display) {
    if (normalizedNameEquals(text, getRemapButtonName(mode, display))) {
      *slot = getRemapButtonSlot(mode, display);
      return *slot < REMAP_MAX_BUTTONS;
    }
  }
  return false;
}

void printRemapList(Print& out) {
  PerModeSettingsRecord settings{};
  readPerModeSettings(deviceMode, settings);
  const uint8_t count = getRemapButtonCount(deviceMode);
  for (uint8_t display = 0; display < count; ++display) {
    const uint8_t source = getRemapButtonSlot(deviceMode, display);
    const uint8_t destination = settings.remaps[source];
    out.print(F("REMAP SRC="));
    out.print((int)source);
    out.print(F(" SRC_NAME="));
    out.print(getRemapButtonNameForSlot(deviceMode, source));
    out.print(F(" DST="));
    out.print((int)destination);
    out.print(F(" DST_NAME="));
    out.println(getRemapButtonNameForSlot(deviceMode, destination));
  }
  out.println(F("OK:REMAP_LIST"));
}

bool handleRemapCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || serialTextEqualsExact(text, "STATUS") ||
      serialTextEqualsExact(text, "LIST")) {
    printRemapList(out);
    return true;
  }
  char* clearRemainder = nullptr;
  if (serialCommandStartsWith(text, "CLEAR", &clearRemainder) ||
      serialCommandStartsWith(text, "DEFAULT", &clearRemainder)) {
    if (!serialTextEqualsExact(clearRemainder, "CONFIRM")) {
      out.println(F("ERR:CONFIRM_REQUIRED"));
      return true;
    }
    PerModeSettingsRecord settings{};
    readPerModeSettings(deviceMode, settings);
    for (uint8_t index = 0; index < REMAP_MAX_BUTTONS; ++index) {
      settings.remaps[index] = index;
      active_remaps[index] = index;
    }
    writePerModeSettings(deviceMode, settings);
    out.println(F("OK:REMAP_CLEAR"));
    return true;
  }
  char* remainder = nullptr;
  if (!serialCommandStartsWith(text, "SET", &remainder)) {
    out.println(F("ERR:BAD_REMAP_CMD"));
    return true;
  }
  char sourceToken[24] = {};
  char destinationToken[24] = {};
  if (!readToken(remainder, sourceToken, sizeof(sourceToken)) ||
      !readToken(remainder, destinationToken, sizeof(destinationToken)) ||
      *remainder != '\0') {
    out.println(F("ERR:BAD_REMAP_ARGS"));
    return true;
  }
  uint8_t source = 0;
  uint8_t destination = 0;
  if (!parseRemapButton(deviceMode, sourceToken, &source) ||
      !parseRemapButton(deviceMode, destinationToken, &destination)) {
    out.println(F("ERR:BAD_REMAP_ARGS"));
    return true;
  }
  PerModeSettingsRecord settings{};
  readPerModeSettings(deviceMode, settings);
  settings.remaps[source] = destination;
  writePerModeSettings(deviceMode, settings);
  active_remaps[source] = destination;
  out.print(F("OK:REMAP_SET SRC="));
  out.print((int)source);
  out.print(F(" DST="));
  out.println((int)destination);
  return true;
}

bool handleAuthCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || serialTextEqualsExact(text, "STATUS") ||
      serialTextEqualsExact(text, "KEY STATUS")) {
    const AuthStorageDiagnostics diagnostics = authStorageLastDiagnostics();
    out.print(F("AUTHKEY PS4="));
    out.print(authStorageHasValidKey(AUTH_KEY_TYPE_PS4) ? 1 : 0);
    out.print(F(" UPLOAD="));
    out.print((int)authStorageLastUploadStatus());
    out.print(F(" CRC=0x"));
    out.print(authStorageLastUploadCrc32(), HEX);
    out.print(F(" REGION="));
    out.print((int)diagnostics.serial_region_state);
    out.print(F(" CHUNKS="));
    out.print((int)diagnostics.received_chunk_count);
    out.print('/');
    out.print((int)diagnostics.total_chunk_count);
    out.print(F(" MISSING="));
    out.println(diagnostics.first_missing_offset);
#ifdef ADAPT_OUTPUT_USB_DEVICE
    ps4Auth.writeDiagnostics(out);
#endif
    return true;
  }
  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "CLEAR", &remainder) ||
      serialCommandStartsWith(text, "KEY CLEAR", &remainder)) {
    char* confirm = remainder;
    if (serialCommandStartsWith(remainder, "PS4", &confirm)) {
      remainder = confirm;
    }
    if (!serialTextEqualsExact(remainder, "CONFIRM")) {
      out.println(F("ERR:CONFIRM_REQUIRED"));
      return true;
    }
    clearAuthBlob(AUTH_KEY_TYPE_PS4);
    updateAuthKeyStatus();
    out.println(F("OK:AUTH_PS4_CLEARED"));
    return true;
  }
  out.println(F("ERR:BAD_AUTH_CMD"));
  return true;
}

bool handleStatsCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "CLEAR", &remainder)) {
    if (!serialTextEqualsExact(remainder, "CONFIRM")) {
      out.println(F("ERR:CONFIRM_REQUIRED"));
      return true;
    }
    webhid_clear_stats();
    out.println(F("OK:STATS_CLEAR"));
    return true;
  }
  if (*text != '\0' && !serialTextEqualsExact(text, "STATUS") &&
      !serialTextEqualsExact(text, "LIST")) {
    out.println(F("ERR:BAD_STATS_CMD"));
    return true;
  }
  out.print(F("STATS POLL_HZ="));
  out.print((int)poll_rate_hz);
  out.print(F(" TOTAL="));
  out.print(total_polls);
  out.print(F(" MODE="));
  out.println((int)deviceMode);
  for (uint8_t index = 0; index < 32; ++index) {
    if (button_press_count[index] == 0) continue;
    out.print(F("STAT BUTTON="));
    out.print((int)index);
    out.print(F(" COUNT="));
    out.println(button_press_count[index]);
  }
  out.println(F("OK:STATS"));
  return true;
}

bool handleHistoryCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "CLEAR", &remainder)) {
    if (!serialTextEqualsExact(remainder, "CONFIRM")) {
      out.println(F("ERR:CONFIRM_REQUIRED"));
      return true;
    }
    input_history_index = 0;
    input_history_count = 0;
    out.println(F("OK:HISTORY_CLEAR"));
    return true;
  }
  if (*text != '\0' && !serialTextEqualsExact(text, "STATUS") &&
      !serialTextEqualsExact(text, "LIST")) {
    out.println(F("ERR:BAD_HISTORY_CMD"));
    return true;
  }
  const uint8_t count = input_history_count < INPUT_HISTORY_SIZE
      ? static_cast<uint8_t>(input_history_count)
      : INPUT_HISTORY_SIZE;
  out.print(F("HISTORY COUNT="));
  out.println((int)count);
  for (uint8_t age = 0; age < count; ++age) {
    const uint8_t index = static_cast<uint8_t>(
        (input_history_index - 1 - age + INPUT_HISTORY_SIZE) %
        INPUT_HISTORY_SIZE);
    const InputHistoryEntry& entry = input_history[index];
    out.print(F("HIST AGE="));
    out.print((int)age);
    out.print(F(" MS="));
    out.print(entry.timestamp);
    out.print(F(" BTN=0x"));
    out.print(entry.buttons, HEX);
    out.print(F(" LX="));
    out.print(entry.lx);
    out.print(F(" LY="));
    out.print(entry.ly);
    out.print(F(" RX="));
    out.print(entry.rx);
    out.print(F(" RY="));
    out.println(entry.ry);
  }
  out.println(F("OK:HISTORY"));
  return true;
}

bool handleMenuCommand(char* text, Print& out) {
  text = serialSkipSpaces(text);
  if (*text == '\0' || serialTextEqualsExact(text, "CONTROLLER") ||
      serialTextEqualsExact(text, "QUICK") ||
      serialTextEqualsExact(text, "OPEN")) {
    openControllerMenuFromSerial();
    out.println(F("OK:MENU_CONTROLLER"));
    return true;
  }
  if (serialTextEqualsExact(text, "SYSTEM")) {
    openSystemMenuFromSerial();
    out.println(F("OK:MENU_SYSTEM"));
    return true;
  }
  if (serialTextEqualsExact(text, "CLOSE") ||
      serialTextEqualsExact(text, "BACK")) {
    closeMenusFromSerial();
    out.println(F("OK:MENU_CLOSED"));
    return true;
  }
  out.println(F("ERR:BAD_MENU_CMD"));
  return true;
}

void printManagementHelp(Print& out) {
  out.println(F("MGMT CORE:INPUT [STATUS|LIST|SET <MODE>],OUTPUT [STATUS|LIST|SET <MODE>],PLAYER LIST,MENU <CONTROLLER|SYSTEM|CLOSE>,CONFIG <SET command>,TURBO,REMAP,AUTH STATUS|CLEAR CONFIRM,FACTORY RESET CONFIRM,RESET,BOOT"));
  out.println(F("MGMT TELEMETRY:STATS [LIST|CLEAR CONFIRM],HISTORY [LIST|CLEAR CONFIRM]"));
  out.println(F("MGMT CONFIG:TURBO LIST|SET <BUTTON> <RATE>|CLEAR CONFIRM; REMAP LIST|SET <SRC> <DST>|CLEAR CONFIRM; SET LIST exposes every named setting"));
}

}  // namespace

bool handleSerialManagementCommand(const char* command, Print& out) {
  char* remainder = nullptr;
  if (serialTextEqualsExact(command, "CAPS") ||
      serialTextEqualsExact(command, "COMMANDS") ||
      serialTextEqualsExact(command, "MGMT") ||
      serialTextEqualsExact(command, "MGMT HELP")) {
    printManagementHelp(out);
    out.println(F("OK:MGMT_HELP"));
    return true;
  }
  if (serialTextEqualsExact(command, "ABOUT")) {
    out.println(F("OK:ABOUT USE_INFO_COMMAND"));
    return true;
  }
  if (serialTextEqualsExact(command, "SAVE")) {
    out.println(F("OK:SAVE AUTO=1"));
    return true;
  }
  if (serialCommandStartsWith(command, "INPUT", &remainder) ||
      serialCommandStartsWith(command, "IN", &remainder)) {
    return handleInputCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "OUTPUT", &remainder)) {
    return handleOutputCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "PLAYER", &remainder)) {
    return handlePlayerCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "MENU", &remainder)) {
    return handleMenuCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "CONFIG", &remainder) ||
      serialCommandStartsWith(command, "SETTING", &remainder) ||
      serialCommandStartsWith(command, "SETTINGS", &remainder)) {
    return handleSerialSetCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "TURBO", &remainder)) {
    return handleTurboCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "REMAP", &remainder)) {
    return handleRemapCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "AUTH", &remainder) ||
      serialCommandStartsWith(command, "PSAUTH", &remainder) ||
      serialCommandStartsWith(command, "AUTHKEY", &remainder)) {
    return handleAuthCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "STATS", &remainder)) {
    return handleStatsCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "HISTORY", &remainder)) {
    return handleHistoryCommand(remainder, out);
  }
  if (serialCommandStartsWith(command, "FACTORY RESET", &remainder) ||
      serialCommandStartsWith(command, "FACTORY_RESET", &remainder)) {
    if (!serialTextEqualsExact(remainder, "CONFIRM")) {
      out.println(F("ERR:CONFIRM_REQUIRED"));
      return true;
    }
    factoryResetSettings();
    out.println(F("OK:FACTORY_RESET REBOOT=1"));
    delay(100);
    reboot();
    return true;
  }
  return false;
}

void appendSerialManagementHelp(Print& out) {
  out.print(F(",CAPS,INPUT,OUTPUT,PLAYER,MENU,CONFIG,TURBO,REMAP,AUTH,STATS,HISTORY,FACTORY RESET CONFIRM"));
}
