#include "../product_config.h"

#include "quick_config.h"

#include "menu_capabilities.h"

namespace {

bool isPsxMode(DeviceEnum mode) {
#ifdef ENABLE_INPUT_PSX
  return mode == RZORD_PSX;
#else
  (void)mode;
  return false;
#endif
}

outputMode_t resolvedCapabilityOutputMode(outputMode_t mode) {
  return (mode == OUTPUT_AUTO) ? get_effective_output_mode() : mode;
}

}  // namespace

bool QuickConfigMenu::hasAnalogSticks(DeviceEnum mode) {
  return menuModeCurrentControllerHasAnalogStick(mode);
}

bool QuickConfigMenu::hasAnalogTriggers(DeviceEnum mode) {
  return menuModeCurrentControllerHasAnalogTriggers(mode);
}

bool QuickConfigMenu::isSnesMode(DeviceEnum mode) {
#ifdef ENABLE_INPUT_SNES
  return mode == RZORD_SNES;
#else
  (void)mode;
  return false;
#endif
}

bool QuickConfigMenu::hasRumble(DeviceEnum mode) {
  if (!menuModeHasRumble(mode)) {
    return false;
  }
  if (isSnesMode(mode)) {
    return true;
  }
  return !isLiveInputSelection() || rumbleMotorCount() > 0;
}

uint8_t QuickConfigMenu::rumbleMotorCount() {
  switch (temp_input_mode) {
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      return 2;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (isLiveInputSelection()) {
        const char* typeName = controllerFrameConst(0).controller_type_name;
        if (strncmp(typeName, "JogCon", 6) == 0) {
          return 1;
        }
        if (strcmp(typeName, "DualShock") == 0 ||
            strcmp(typeName, "DualShock2") == 0) {
          return 2;
        }
        if (typeName[0] != '\0') {
          return 0;
        }
      }
      return 2;
    case RZORD_PSX_JOG:
      return 1;
    #ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE:
      return 0;
    #endif
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return 1;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return 1;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return 1;
    #endif
    #ifdef ENABLE_INPUT_USB
    case RZORD_USB:
      return 2;
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
    case RZORD_ESP32_SPI:
      return 2;
    #endif
    default:
      return 0;
  }
}

bool QuickConfigMenu::isRumbleMenuItemVisible(RumbleMenuItem item) {
  const uint8_t motors = rumbleMotorCount();
  const bool enabled = temp_rumble != 0;
  switch (item) {
    case QCI_RUMBLE_LEVEL:
    case QCI_RUMBLE_BACK:
      return true;
    case QCI_RUMBLE_TEST_HEAVY:
    case QCI_RUMBLE_TEST_LIGHT:
      return enabled && motors >= 2;
    case QCI_RUMBLE_TEST_BOTH:
      return enabled && motors >= 1;
    case QCI_RUMBLE_HEAVY:
    case QCI_RUMBLE_LIGHT:
    default:
      return false;
  }
}

void QuickConfigMenu::advanceRumbleIndex(bool forward) {
  for (uint8_t attempts = 0; attempts < QCI_RUMBLE_COUNT; ++attempts) {
    if (forward) {
      rumble_index = (uint8_t)((rumble_index + 1) % QCI_RUMBLE_COUNT);
    } else {
      rumble_index = (rumble_index == 0) ? (QCI_RUMBLE_COUNT - 1) : (uint8_t)(rumble_index - 1);
    }
    if (isRumbleMenuItemVisible((RumbleMenuItem)rumble_index)) {
      return;
    }
  }
  rumble_index = QCI_RUMBLE_LEVEL;
}

bool QuickConfigMenu::isGuncon(DeviceEnum mode) {
  return isPsxMode(mode);
}

bool QuickConfigMenu::isJogcon(DeviceEnum mode) {
  return isPsxMode(mode);
}

bool QuickConfigMenu::isDrivingFallbackActive(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_DRIVING
  if (mode == RZORD_SMS) {
    for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
      if (strncmp(controllerFrameConst(i).controller_type_name, "Driving", 7) == 0) {
        return true;
      }
    }
  }
  #endif
  return false;
}

bool QuickConfigMenu::hasSpinner(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
    #endif
      #ifdef ENABLE_INPUT_DRIVING
      if (mode == RZORD_SMS) {
        return isDrivingFallbackActive(mode);
      }
      #endif
      return true;
    default:
      return false;
  }
}

bool QuickConfigMenu::isNintendoController(DeviceEnum mode) {
  return menuModeIsNintendoController(mode);
}

bool QuickConfigMenu::isN64(DeviceEnum mode) {
  return menuModeIsN64Controller(mode);
}

bool QuickConfigMenu::isGameCube(DeviceEnum mode) {
#ifdef ENABLE_INPUT_GAMECUBE
  return mode == RZORD_GAMECUBE;
#else
  (void)mode;
  return false;
#endif
}

bool QuickConfigMenu::hasClassicAnalogRange(DeviceEnum mode) {
  return classicModeHasRangeSetting(mode);
}

uint8_t QuickConfigMenu::getTempClassicAnalogRange() {
  return (temp_input_mode == RZORD_N64) ? temp_n64_range : temp_wii_range;
}

void QuickConfigMenu::cycleTempClassicAnalogRange(bool forward) {
  uint8_t& value = (temp_input_mode == RZORD_N64) ? temp_n64_range : temp_wii_range;
  value = sanitizeClassicAnalogRange(value);
  if (forward) {
    value = (value + 1) % CLASSIC_ANALOG_RANGE_COUNT;
  } else {
    value = (value == 0) ? (CLASSIC_ANALOG_RANGE_COUNT - 1) : value - 1;
  }
}

bool QuickConfigMenu::hasRightStick(DeviceEnum mode) {
  return menuModeCurrentControllerHasRightStick(mode);
}

uint8_t QuickConfigMenu::getDpadModeMax(outputMode_t mode) {
  return output_supports_dpad_buttons(mode) ? 3 : 2;
}

bool QuickConfigMenu::isTempDpadModeAllowed(uint8_t mode) {
  switch (mode) {
    case DPAD_MODE_DPAD:
      return true;
    case DPAD_MODE_LEFT_STICK:
    case DPAD_MODE_RIGHT_STICK:
      return true;
    case DPAD_MODE_BUTTONS:
      return output_supports_dpad_buttons(temp_output_mode);
    default:
      return false;
  }
}

void QuickConfigMenu::cycleTempDpadMode(bool forward) {
  uint8_t value = temp_dpad_mode;
  for (uint8_t attempts = 0; attempts < 4; ++attempts) {
    if (forward) {
      value = (uint8_t)((value + 1) % 4);
    } else {
      value = (value == 0) ? 3 : (uint8_t)(value - 1);
    }
    if (isTempDpadModeAllowed(value)) {
      temp_dpad_mode = value;
      return;
    }
  }
  temp_dpad_mode = DPAD_MODE_DPAD;
}

uint8_t QuickConfigMenu::getStickInvertMax(DeviceEnum mode) {
  return hasRightStick(mode) ? 3 : 1;
}

uint8_t QuickConfigMenu::getTriggerModeMax(outputMode_t mode) {
  if (!hasAnalogTriggers(temp_input_mode)) {
    return TRIGGER_MODE_ANALOG;
  }
  if (outputModeSupportsTriggerBothMode(resolvedCapabilityOutputMode(mode)) &&
      menuModeSupportsTriggerBothMode(temp_input_mode)) {
    return TRIGGER_MODE_BOTH;
  }
  return TRIGGER_MODE_DIGITAL;
}

bool QuickConfigMenu::isTempTriggerModeAllowed(uint8_t mode) {
  switch (mode) {
    case TRIGGER_MODE_ANALOG:
    case TRIGGER_MODE_DIGITAL:
      return hasAnalogTriggers(temp_input_mode);
    case TRIGGER_MODE_RSTICK:
      return supportsTempTriggerRightStick();
    case TRIGGER_MODE_BOTH:
      return hasAnalogTriggers(temp_input_mode) &&
             outputModeSupportsTriggerBothMode(resolvedCapabilityOutputMode(temp_output_mode)) &&
             menuModeSupportsTriggerBothMode(temp_input_mode);
    default:
      return false;
  }
}

bool QuickConfigMenu::supportsTempTriggerRightStick() {
  return temp_output_mode == OUTPUT_SWITCHPRO &&
         hasAnalogTriggers(temp_input_mode) &&
         menuModeSupportsTriggerAxisMode(temp_input_mode);
}

void QuickConfigMenu::cycleTempTriggerMode(bool forward) {
  uint8_t value = temp_triggers;
  for (uint8_t attempts = 0; attempts < 4; ++attempts) {
    if (forward) {
      value = (uint8_t)((value + 1) % 4);
    } else {
      value = (value == 0) ? 3 : (uint8_t)(value - 1);
    }
    if (value == TRIGGER_MODE_RSTICK) {
      continue;
    }
    if (isTempTriggerModeAllowed(value)) {
      temp_triggers = value;
      return;
    }
  }
  temp_triggers = TRIGGER_MODE_ANALOG;
}

bool QuickConfigMenu::isTurboConfigIndexVisible(uint8_t configIndex) {
  const TurboButtonConfig& cfg = getSelectedTurboConfig();
  if (configIndex >= cfg.count) {
    return false;
  }

  const uint8_t btn = cfg.indices[configIndex];
  const bool analogTriggerSlot = btn == TURBO_BTN_6 || btn == TURBO_BTN_7;
  if (analogTriggerSlot &&
      temp_triggers != TRIGGER_MODE_DIGITAL &&
      hasAnalogTriggers(temp_input_mode) &&
      menuModeSupportsTriggerBothMode(temp_input_mode) &&
      !menuModeHasIndependentDigitalTriggerButtons(temp_input_mode)) {
    return false;
  }
  return true;
}

uint8_t QuickConfigMenu::getVisibleTurboCount() {
  const TurboButtonConfig& cfg = getSelectedTurboConfig();
  uint8_t count = 0;
  for (uint8_t i = 0; i < cfg.count; ++i) {
    if (isTurboConfigIndexVisible(i)) {
      ++count;
    }
  }
  return count;
}

uint8_t QuickConfigMenu::getTurboConfigIndexForVisibleIndex(uint8_t visibleIndex) {
  const TurboButtonConfig& cfg = getSelectedTurboConfig();
  uint8_t visible = 0;
  for (uint8_t i = 0; i < cfg.count; ++i) {
    if (!isTurboConfigIndexVisible(i)) {
      continue;
    }
    if (visible == visibleIndex) {
      return i;
    }
    ++visible;
  }
  return 0xFF;
}

uint8_t QuickConfigMenu::getTurboButtonIndexForVisibleIndex(uint8_t visibleIndex) {
  const uint8_t configIndex = getTurboConfigIndexForVisibleIndex(visibleIndex);
  if (configIndex == 0xFF) {
    return 0xFF;
  }
  return getSelectedTurboConfig().indices[configIndex];
}

const char* QuickConfigMenu::getTurboButtonNameForVisibleIndex(uint8_t visibleIndex) {
  const uint8_t configIndex = getTurboConfigIndexForVisibleIndex(visibleIndex);
  if (configIndex == 0xFF) {
    return "?";
  }
  return getSelectedTurboConfig().names[configIndex];
}

uint8_t QuickConfigMenu::getStickCount(DeviceEnum mode) {
  if (mode == deviceMode) {
    if (menuModeCurrentControllerHasRightStick(mode)) {
      return 2;
    }
    if (menuModeCurrentControllerHasAnalogStick(mode)) {
      return 1;
    }
    return 0;
  }

  switch (mode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return 1;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return 1;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
    #endif
    #ifdef ENABLE_INPUT_USB
    case RZORD_USB:
    #endif
      return 2;
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      return 1;
    #endif
    default:
      return 0;
  }
}
