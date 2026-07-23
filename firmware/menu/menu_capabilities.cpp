#include "../product_config.h"

#include "menu_capabilities.h"

#include "../core/controller_frame_state.h"
#include "../core/device_runtime_state.h"
#include "../input/snes/input_snes_runtime_state.h"
#include "../output/auth/auth_status.h"
#include "../output/output_runtime_state.h"

namespace {

bool menuModeIsCurrentLiveInput(DeviceEnum mode) {
  return mode == deviceMode;
}

bool liveFramesHaveAnalogStick() {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (frame.connected && (frame.HAS_ANALOG_STICK_MAIN || frame.HAS_ANALOG_STICK_AUX)) {
      return true;
    }
  }
  return false;
}

bool liveFramesHaveRightStick() {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (frame.connected && frame.HAS_ANALOG_STICK_AUX) {
      return true;
    }
  }
  return false;
}

bool liveFramesHaveAnalogTriggers() {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (frame.connected && frame.HAS_ANALOG_TRIGGERS) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool menuModeHasAnalog(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
    #endif
    #ifdef ENABLE_INPUT_USB
    case RZORD_USB:
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
    case RZORD_ESP32_SPI:
    #endif
      return true;
    default:
      return false;
  }
}

bool menuModeHasRightStick(DeviceEnum mode) {
  switch (mode) {
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
    #ifdef ENABLE_INPUT_ESP32_SPI
    case RZORD_ESP32_SPI:
    #endif
      return true;
    default:
      return false;
  }
}

bool menuModeHasRumble(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      return true;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return true;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return true;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      return true;
    #endif
    #ifdef ENABLE_INPUT_USB
    case RZORD_USB:
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
    case RZORD_ESP32_SPI:
    #endif
      return true;
    default:
      return false;
  }
}

bool menuModeHasAnalogTriggers(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
    #endif
    #ifdef ENABLE_INPUT_USB
    case RZORD_USB:
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
    case RZORD_ESP32_SPI:
    #endif
      return true;
    default:
      return false;
  }
}

bool menuModeCurrentControllerHasAnalog(DeviceEnum mode) {
  return menuModeCurrentControllerHasAnalogStick(mode) ||
         menuModeCurrentControllerHasAnalogTriggers(mode);
}

bool menuModeCurrentControllerHasAnalogStick(DeviceEnum mode) {
  if (menuModeIsCurrentLiveInput(mode)) {
    return liveFramesHaveAnalogStick();
  }
  return menuModeHasAnalog(mode);
}

bool menuModeCurrentControllerHasRightStick(DeviceEnum mode) {
  if (menuModeIsCurrentLiveInput(mode)) {
    return liveFramesHaveRightStick();
  }
  return menuModeHasRightStick(mode);
}

bool menuModeCurrentControllerHasAnalogTriggers(DeviceEnum mode) {
  if (menuModeIsCurrentLiveInput(mode)) {
    return liveFramesHaveAnalogTriggers();
  }
  return menuModeHasAnalogTriggers(mode);
}

bool menuModeSupportsTriggerAxisMode(DeviceEnum mode) {
  return menuModeHasAnalogTriggers(mode);
}

bool menuModeSupportsTriggerBothMode(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
    #endif
      return true;
    default:
      return false;
  }
}

bool menuModeHasIndependentDigitalTriggerButtons(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    #endif
      return true;
    default:
      return false;
  }
}

bool outputModeSupportsTriggerBothMode(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID:
    case OUTPUT_MISTER:
      return true;
    default:
      return false;
  }
}

bool menuModeIsNintendoController(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
    #endif
      return true;
    default:
      return false;
  }
}

bool menuModeIsN64Controller(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_N64
  return mode == RZORD_N64;
  #else
  (void)mode;
  return false;
  #endif
}

bool shouldHideMenuOutputMode(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID:
    case OUTPUT_RESERVED_JOGCON:
    case OUTPUT_XINPUTW:
    case OUTPUT_XBOXONE:
    case OUTPUT_SWITCH:
    case OUTPUT_PANTHERLORD:
    case OUTPUT_GCWIIU:
    case OUTPUT_MDMINI:
    case OUTPUT_RESERVED_MOUSE:
      return true;
    case OUTPUT_XINPUT2P:
      return !is_xinput2p_output_enabled();
#if defined(ENABLE_OUTPUT_JVS) && !defined(ADAPT_PRIMARY_EGRESS_JVS_BOARD)
    case OUTPUT_JVS:
      return true;
#endif
    default:
      break;
  }

  if (!authOutputModeCanRun(mode)) {
    return true;
  }

  #if defined(ADAPT_PRIMARY_EGRESS_CLASSIC_CONSOLE)
  return mode != OUTPUT_CONSOLE_NES && mode != OUTPUT_CONSOLE_SNES;
  #elif defined(ADAPT_PRIMARY_EGRESS_JVS_BOARD)
  return mode != OUTPUT_JVS;
  #elif defined(ADAPT_PRIMARY_EGRESS_DB15)
  return mode != OUTPUT_DB15_SUPERGUN;
  #else
  return false;
  #endif
}
