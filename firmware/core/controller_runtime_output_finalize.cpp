#include "../product_config.h"
#include "controller_runtime_internal.h"

#include <Arduino.h>

#include "analog_calibration_state.h"
#include "classic_analog_range.h"
#include "controller_frame_state.h"
#include "controller_output_cache_state.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "button_map_mode.h"
#include "dpad_mode.h"
#include "../input/shared/input_button_bits.h"
#include "../input/jaguar/input_jaguar_runtime_state.h"
#include "../menu/menu_ui_state.h"
#include "../platform/latency_service_mask.h"
#include "n64_output_helpers.h"

#include <cstring>

namespace controller_runtime_internal {

namespace {

controller_state_t menuOutputRestoreFrames[MAX_USB_OUT];
bool menuOutputRestoreFrameValid[MAX_USB_OUT] = {};
bool menuOutputRestoreValid = false;
controller_state_t outputFinalizeRestoreFrames[MAX_USB_OUT];
bool outputFinalizeRestoreFrameValid[MAX_USB_OUT] = {};
bool outputFinalizeRestoreActive = false;

void neutralizeControllerFrameForMenu(controller_state_t& frame) {
  frame.digital_buttons = 0;
  frame.analog_sticks = 0;
  frame.analog_buttons = 0;
  frame.analog_pad = 0;
  frame.paddle = 0;
  frame.spinner = 0;
  frame.mouse_x = 0;
  frame.mouse_y = 0;
  frame.mouse_wheel_x = 0;
  frame.mouse_wheel_y = 0;
}

bool shouldApplyAnalogCalibrationToCurrentMode() {
  if (!analogCalibration.enabled) {
    return false;
  }

  switch (deviceMode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return false;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return wii_analog_range == CLASSIC_ANALOG_RANGE_CALIBRATED;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      return wii_analog_range == CLASSIC_ANALOG_RANGE_CALIBRATED;
    #endif
    default:
      return true;
  }
}

bool outputCacheInputHasIndependentDigitalTriggers() {
  #ifdef ENABLE_INPUT_PSX
  if (deviceMode == RZORD_PSX) {
    return true;
  }
  #endif
  return false;
}

bool outputCacheAnalogTriggersActive(const controller_state_t& frame) {
  return frame.HAS_ANALOG_TRIGGERS &&
         (trigger_mode == TRIGGER_MODE_ANALOG ||
          trigger_mode == TRIGGER_MODE_RSTICK ||
          trigger_mode == TRIGGER_MODE_BOTH);
}

bool wiiClassicAnalogTriggersStayAnalog(const controller_state_t& frame) {
  return std::strcmp(frame.controller_type_name, "Classic") == 0;
}

bool zOutputPolicyAppliesToCurrentMode() {
#ifdef ENABLE_INPUT_N64
  if (deviceMode == RZORD_N64) {
    return true;
  }
#endif
#ifdef ENABLE_INPUT_GAMECUBE
  if (deviceMode == RZORD_GAMECUBE) {
    return true;
  }
#endif
  return false;
}

void applyTriggerModeToCachedButtons(const controller_state_t& frame, uint32_t& buttons) {
  if (!frame.HAS_ANALOG_TRIGGERS) {
    return;
  }
  if (!wiiClassicAnalogTriggersStayAnalog(frame) &&
      (trigger_mode == TRIGGER_MODE_DIGITAL || trigger_mode == TRIGGER_MODE_BOTH)) {
    if (frame.ANALOG_L2 > TRIGGER_DIGITAL_THRESHOLD) buttons |= INPUT_L2;
    if (frame.ANALOG_R2 > TRIGGER_DIGITAL_THRESHOLD) buttons |= INPUT_R2;
  }
  if (trigger_mode == TRIGGER_MODE_ANALOG &&
      !outputCacheInputHasIndependentDigitalTriggers()) {
    buttons &= ~(INPUT_L2 | INPUT_R2);
  }
}

}  // namespace

void cacheProcessedControllerOutputStateInternal() {
  for (uint8_t i = 0; i < max_devices; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    uint32_t buttons = applyNintendoPositionButtonMap(
      applyZButtonOutputMode(frame.digital_buttons, frame),
      buttonMapModeAppliesToInputMode(deviceMode) && !nso_special && button_map_mode == 1);

    int16_t lx = frame.LX;
    int16_t ly = frame.LY;
    int16_t rx = frame.RX;
    int16_t ry = frame.RY;

    const uint8_t effectiveDpadMode = effective_dpad_mode_for_sticks(
      dpad_mode,
      frame.HAS_ANALOG_STICK_MAIN,
      frame.HAS_ANALOG_STICK_AUX);
    if (effectiveDpadMode == DPAD_MODE_LEFT_STICK ||
        effectiveDpadMode == DPAD_MODE_RIGHT_STICK) {
      int16_t dpad_x = 0;
      int16_t dpad_y = 0;
      if (frame.PAD_L) dpad_x = -127;
      if (frame.PAD_R) dpad_x = 127;
      if (frame.PAD_U) dpad_y = -127;
      if (frame.PAD_D) dpad_y = 127;

      if (effectiveDpadMode == DPAD_MODE_LEFT_STICK) {
        lx = dpad_x;
        ly = dpad_y;
      } else {
        rx = dpad_x;
        ry = dpad_y;
      }
      buttons &= ~(INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R);
    }

    applyTriggerModeToCachedButtons(frame, buttons);
    post_remap_buttons[i] = buttons;

    if (stick_invert & 0x01) ly = -ly;
    if (stick_invert & 0x02) ry = -ry;

    post_output_lx[i] = lx;
    post_output_ly[i] = ly;
    post_output_rx[i] = rx;
    post_output_ry[i] = ry;
    post_output_l2[i] = outputCacheAnalogTriggersActive(frame) ? frame.ANALOG_L2 : 0;
    post_output_r2[i] = outputCacheAnalogTriggersActive(frame) ? frame.ANALOG_R2 : 0;
  }
}

void __not_in_flash_func(captureOutputFinalizeRestoreFrames)() {
  outputFinalizeRestoreActive = true;
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    outputFinalizeRestoreFrameValid[i] = false;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (frame.connected) {
      outputFinalizeRestoreFrames[i] = frame;
      outputFinalizeRestoreFrameValid[i] = true;
    }
  }
}

void __not_in_flash_func(rebuildDigitalButtonsForOutputFrame)(bool applyRemap,
                                                              bool applyChordRemap,
                                                              bool applyTurbo,
                                                              bool applyVirtualHotkeys) {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (frame.connected) {
      frame.digital_buttons = pre_remap_buttons[i];
      if (applyVirtualHotkeys) {
        frame.digital_buttons &= ~suppressedControllerHotkeySourceButtons(i);
      }
    }
  }

  if (applyRemap && !latencyServiceDisabled(LATENCY_SERVICE_BUTTON_REMAP)) {
    applyButtonRemapTransforms();
  }
  if (applyChordRemap && !latencyServiceDisabled(LATENCY_SERVICE_CHORD_REMAP)) {
    applyButtonChordRemapTransforms();
  }
  if (applyTurbo && !latencyServiceDisabled(LATENCY_SERVICE_TURBO_TRANSFORM)) {
    applyTurboTransforms();
  }
}

void __not_in_flash_func(applyAnalogCalibrationToConnectedInputs)() {
  if (!shouldApplyAnalogCalibrationToCurrentMode()) {
    return;
  }
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (frame.connected) {
      applyAnalogCalibrationToReport(frame);
    }
  }
}

void __not_in_flash_func(applyOutputButtonModeMappingsToConnectedInputs)() {
  if (!zOutputPolicyAppliesToCurrentMode()) {
    return;
  }
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (frame.connected) {
      frame.digital_buttons = applyZButtonOutputMode(frame.digital_buttons, frame);
    }
  }
}

void __not_in_flash_func(clearJaguarRotaryDpadBeforeUsbSend)() {
  #ifdef ENABLE_INPUT_JAGUAR
  if (deviceMode == RZORD_JAGUAR && jaguarRotaryActive) {
    for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
      if (!jaguarRotaryActivePorts[i]) {
        continue;
      }
      controller_state_t& frame = controllerFrame(i);
      frame.PAD_L = 0;
      frame.PAD_R = 0;
      frame.digital_buttons &= ~((1UL << 30) | (1UL << 31));
    }
  }
  #endif
}

void __not_in_flash_func(consumeMenuCapturedControllerOutput)() {
  if (!menuOwnsControllerInput()) {
    menuOutputRestoreValid = false;
    for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
      menuOutputRestoreFrameValid[i] = false;
    }
    return;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (frame.connected) {
      menuOutputRestoreFrames[i] = frame;
      menuOutputRestoreFrameValid[i] = true;
      neutralizeControllerFrameForMenu(frame);
    } else {
      menuOutputRestoreFrameValid[i] = false;
    }
  }
  menuOutputRestoreValid = true;
}

void __not_in_flash_func(restorePhysicalButtonsAfterUsbSend)() {
  if (outputFinalizeRestoreActive) {
    for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
      if (outputFinalizeRestoreFrameValid[i]) {
        controllerFrame(i) = outputFinalizeRestoreFrames[i];
        // Keep held-button transforms idempotent between polls so remap/turbo never feeds back into itself.
        // Swapped remaps such as A<->B must always start from physical buttons, not the last remapped USB frame.
        controllerFrame(i).digital_buttons = pre_remap_buttons[i];
        outputFinalizeRestoreFrameValid[i] = false;
      }
    }
    outputFinalizeRestoreActive = false;
    menuOutputRestoreValid = false;
    for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
      menuOutputRestoreFrameValid[i] = false;
    }
    return;
  }

  if (menuOutputRestoreValid) {
    for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
      if (menuOutputRestoreFrameValid[i]) {
        controllerFrame(i) = menuOutputRestoreFrames[i];
        menuOutputRestoreFrameValid[i] = false;
      }
    }
    menuOutputRestoreValid = false;
    return;
  }

  for (uint8_t i = 0; i < max_devices; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (frame.connected) {
      frame.digital_buttons = pre_remap_buttons[i];
    }
  }
}

}  // namespace controller_runtime_internal
