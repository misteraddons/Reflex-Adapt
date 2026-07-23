#include "settings_store_per_mode_internal.h"

#include <string.h>

#include "classic_analog_range.h"
#include "button_map_mode.h"
#include "controller_settings_state.h"

uint8_t defaultButtonMapModeForInputMode(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_GAMECUBE
  if (mode == RZORD_GAMECUBE) {
    return 0;
  }
  #endif
  return buttonMapModeAppliesToInputMode(mode) ? 1 : 0;
}

uint8_t defaultZButtonModeForInputMode(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_GAMECUBE
  if (mode == RZORD_GAMECUBE) {
    return 0;  // GameCube Z defaults to RB/R1 on modern outputs.
  }
  #endif
  return 1;    // N64 keeps the existing default: Z as L2/separate.
}

uint8_t defaultSpinnerSpeedForInputMode(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_DRIVING
  if (mode == RZORD_DRIVING) {
    return 3;
  }
  #endif
  return 2;
}

uint8_t defaultRumbleLevelForInputMode(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_SNES
  if (mode == RZORD_SNES) {
    // SNES RumbleTech support is automatic. Keep the per-mode strength nonzero
    // so menu rumble tests and host rumble are usable without extra setup.
    return 3;
  }
  #endif
  return 3;
}

uint8_t defaultTriggerModeForInputMode(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_GAMECUBE
  if (mode == RZORD_GAMECUBE) {
    return TRIGGER_MODE_BOTH;
  }
  #endif
  #ifdef ENABLE_INPUT_WII
  if (mode == RZORD_WII) {
    return TRIGGER_MODE_BOTH;
  }
  #endif
  return TRIGGER_MODE_ANALOG;
}

bool isConcreteModeForPerSettings(DeviceEnum mode) {
  if (mode <= RZORD_NONE || mode >= RZORD_LAST || mode >= MAX_INPUT_MODES) {
    return false;
  }
  #ifdef ENABLE_INPUT_AUTODETECT
  if (mode == RZORD_AUTODETECT) {
    return false;
  }
  #endif
  return true;
}

void sanitizePerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings) {
  if (settings.deadzone_percent > 30) settings.deadzone_percent = 0;
  if (settings.socd >= SOCD_LAST_ENUM) settings.socd = SOCD_OFF;
#ifdef PRODUCT_CLASSIC2USB
  settings.socd = SOCD_OFF;
#endif
  if (settings.dpad_mode > 3) settings.dpad_mode = 0;
  if (settings.stick_invert > 3) settings.stick_invert = 0;
  if (settings.rumble_level > 3) settings.rumble_level = defaultRumbleLevelForInputMode(mode);
  if (settings.trigger_mode > TRIGGER_MODE_BOTH) settings.trigger_mode = defaultTriggerModeForInputMode(mode);
  if (settings.spinner_speed > 4) settings.spinner_speed = defaultSpinnerSpeedForInputMode(mode);
  if (settings.button_map_mode > 1) settings.button_map_mode = defaultButtonMapModeForInputMode(mode);
  if (settings.n64_z_mode > 1) settings.n64_z_mode = defaultZButtonModeForInputMode(mode);
  if (settings.n64_cstick_mode > 2) settings.n64_cstick_mode = 0;
  settings.wii_analog_range = sanitizeClassicAnalogRange(settings.wii_analog_range);
  settings.n64_analog_range = sanitizeClassicAnalogRange(settings.n64_analog_range);
  if (settings.nso_special > 1) settings.nso_special = 0;
  if (settings.powerpad_mode > 1) settings.powerpad_mode = 0;
  settings.guncon_offset_x = normalizeGunconAlignmentOffset(settings.guncon_offset_x);
  settings.guncon_offset_y = normalizeGunconAlignmentOffset(settings.guncon_offset_y);
  settings.reserved_musical_buttons = 0;
  settings.reserved_mouse_analog = 0;
  settings.reserved_mouse_sensitivity = 5;
  settings.reserved_mouse_stick = 0;
  if (settings.jogcon_mode > 3) settings.jogcon_mode = 0;
  if (settings.jogcon_force != 1 && settings.jogcon_force != 3 &&
      settings.jogcon_force != 7 && settings.jogcon_force != 15) {
    settings.jogcon_force = 1;
  }
  if (settings.wheel_sensitivity > 2) settings.wheel_sensitivity = 1;
  if (settings.jogcon_digital_map > 3) settings.jogcon_digital_map = 0;
  if (settings.jogcon_wheel_axis > 1) settings.jogcon_wheel_axis = 0;

  bool analog_valid = settings.analog_cal_lx_min < settings.analog_cal_lx_max &&
                      settings.analog_cal_ly_min < settings.analog_cal_ly_max &&
                      settings.analog_cal_rx_min < settings.analog_cal_rx_max &&
                      settings.analog_cal_ry_min < settings.analog_cal_ry_max;
  if (!analog_valid) {
    settings.analog_cal_lx_min = -128;
    settings.analog_cal_lx_max = 127;
    settings.analog_cal_ly_min = -128;
    settings.analog_cal_ly_max = 127;
    settings.analog_cal_rx_min = -128;
    settings.analog_cal_rx_max = 127;
    settings.analog_cal_ry_min = -128;
    settings.analog_cal_ry_max = 127;
    settings.analog_cal_enabled = 0;
  } else if (settings.analog_cal_enabled > 1) {
    settings.analog_cal_enabled = 0;
  }

  sanitizeTurboRatesForMode(mode, settings.turbo_rates);

  for (uint8_t i = 0; i < PER_MODE_REMAP_SIZE; ++i) {
    if (settings.remaps[i] >= PER_MODE_REMAP_SIZE) {
      settings.remaps[i] = i;
    }
  }
}

TurboInputMode getTurboInputModeForDeviceMode(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:       return TURBO_MODE_NES;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:      return TURBO_MODE_SNES;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:      return TURBO_MODE_VBOY;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:       return TURBO_MODE_N64;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:  return TURBO_MODE_GAMECUBE;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:       return TURBO_MODE_PSX;
    case RZORD_PSX_JOG:   return TURBO_MODE_PSX_JOG;
    #endif
    #ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE: return TURBO_MODE_PSX;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:    return TURBO_MODE_SATURN;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE: return TURBO_MODE_MEGADRIVE;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST: return TURBO_MODE_DREAMCAST;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:       return TURBO_MODE_PCE;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:    return TURBO_MODE_NEOGEO;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:       return TURBO_MODE_3DO;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:       return TURBO_MODE_WII;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:    return TURBO_MODE_JAGUAR;
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:       return TURBO_MODE_SMS;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:       return TURBO_MODE_JPC;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:   return TURBO_MODE_DRIVING;
    #endif
    default:              return TURBO_MODE_GENERIC;
  }
}
