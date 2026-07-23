#include "../product_config.h"

#include "../core/device_runtime_state.h"
#include "../input/autodetect/input_autodetect_runtime_state.h"
#include "../output/output_runtime_state.h"
#include "menu_capabilities.h"
#include "menu_working_state.h"
#include "menu_mode_state.h"

DeviceEnum menu_input = (DeviceEnum)1;
outputMode_t menu_output = OUTPUT_AUTO;

namespace {

const outputMode_t kOutputModeMenuOrder[] = {
  OUTPUT_AUTO,
  OUTPUT_XINPUT2P,
  OUTPUT_HID,
  OUTPUT_MISTER,
  OUTPUT_KEYBOARD,
  OUTPUT_MISTER_JOGCON,
  OUTPUT_MISTER_NEGCON,
  OUTPUT_MISTER_GUNCON,
  OUTPUT_XID,
  OUTPUT_XINPUT,
  OUTPUT_XBOXONE,
  OUTPUT_PS3,
  OUTPUT_PS4,
  OUTPUT_PS5,
  OUTPUT_SWITCH,
  OUTPUT_SWITCHPRO,
  OUTPUT_PANTHERLORD,
  OUTPUT_GCWIIU,
  OUTPUT_MDMINI,
#ifdef ENABLE_OUTPUT_JVS
  OUTPUT_JVS,
#endif
#ifdef ENABLE_OUTPUT_ESP32_BT
  OUTPUT_ESP32_BT,
#endif
#ifdef ENABLE_OUTPUT_CONSOLE
  OUTPUT_CONSOLE_NES,
  OUTPUT_CONSOLE_SNES,
  OUTPUT_CONSOLE_N64,
  OUTPUT_CONSOLE_GC,
  OUTPUT_CONSOLE_SATURN,
  OUTPUT_CONSOLE_GENESIS,
  OUTPUT_CONSOLE_WII,
  OUTPUT_CONSOLE_AUTO,
#endif
#ifdef ENABLE_OUTPUT_DB15
  OUTPUT_DB15_SUPERGUN,
#endif
};

constexpr uint8_t kOutputModeMenuOrderCount =
    sizeof(kOutputModeMenuOrder) / sizeof(kOutputModeMenuOrder[0]);

int8_t findOutputMenuOrderIndex(outputMode_t mode) {
  const outputMode_t canonicalMode = canonicalizeOutputMode(mode);
  for (uint8_t i = 0; i < kOutputModeMenuOrderCount; ++i) {
    if (kOutputModeMenuOrder[i] == canonicalMode) {
      return (int8_t)i;
    }
  }
  return -1;
}

}  // namespace

bool input_has_analog() {
  return menuModeCurrentControllerHasAnalogStick(menu_input);
}

bool input_has_right_stick() {
  return menuModeCurrentControllerHasRightStick(menu_input);
}

bool input_has_rumble() {
  return menuModeHasRumble(menu_input);
}

bool is_passive_controller_mode(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
    #endif
      return true;
    default:
      return false;
  }
}

bool input_has_analog_triggers() {
  return menuModeCurrentControllerHasAnalogTriggers(menu_input);
}

bool output_has_rumble() {
  switch (menu_output) {
    case OUTPUT_MISTER:
    case OUTPUT_MISTER_JOGCON:
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XBOXONE:
    case OUTPUT_XID:
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
    case OUTPUT_GCWIIU:
      return true;
    default:
      return false;
  }
}

bool should_hide_output_mode(outputMode_t mode) {
  return shouldHideMenuOutputMode(mode);
}

outputMode_t cycle_visible_output_mode(outputMode_t current, bool forward) {
  int8_t index = findOutputMenuOrderIndex(current);
  if (index < 0) {
    index = forward ? -1 : 0;
  }

  for (uint8_t attempts = 0; attempts < kOutputModeMenuOrderCount; ++attempts) {
    if (forward) {
      index = (int8_t)((index + 1) % kOutputModeMenuOrderCount);
    } else if (index <= 0) {
      index = (int8_t)(kOutputModeMenuOrderCount - 1);
    } else {
      --index;
    }

    const outputMode_t candidate = kOutputModeMenuOrder[index];
    if (!should_hide_output_mode(candidate)) {
      return candidate;
    }
  }

  return canonicalizeOutputMode(current);
}

DeviceEnum getMenuSavedInputMode(DeviceEnum selectedMode) {
  if (isAutoDetectMode &&
      selectedMode == deviceMode &&
      savedDeviceMode > RZORD_NONE &&
      savedDeviceMode < RZORD_LAST) {
    return savedDeviceMode;
  }
  return selectedMode;
}
