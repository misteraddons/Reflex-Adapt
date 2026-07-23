#include "../product_config.h"
#include "controller_runtime_internal.h"

#include <Arduino.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include "button_remap.h"
#include "button_chord_remap.h"
#include "classic_dual_merge_config.h"
#include "controller_delivery_state.h"
#include "controller_frame_state.h"
#include "controller_output_cache_state.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "hotkey_combo.h"
#include "../input/jaguar/input_jaguar_runtime_state.h"
#include "../input/shared/input_button_bits.h"
#include "../menu/menu_runtime_state.h"
#include "../menu/menu_ui_state.h"
#include "../platform/rgb_led.h"
#include "../output/output_runtime_state.h"
#include "stick_center.h"
#include "turbo.h"
#include "../input/runtime/input_frame_runtime.h"

namespace {

struct Vector2i16 {
  int16_t x;
  int16_t y;
};

Vector2i16 __not_in_flash_func(hardRadialDeadzone)(int16_t rawX, int16_t rawY, uint16_t deadZone) {
  const float dx = static_cast<float>(rawX);
  const float dy = static_cast<float>(rawY);

  float magnitude = std::sqrt(dx * dx + dy * dy);

  if (magnitude <= static_cast<float>(deadZone)) {
    return { 0, 0 };
  }

  return { rawX, rawY };
}

int16_t __not_in_flash_func(stickPrecisionFullScale)(analog_stick_precision precision) {
  switch (precision) {
    case ANALOG_STICK_PRECISION_12:
      return INT16_MAX >> 4;
    case ANALOG_STICK_PRECISION_16:
      return INT16_MAX;
    case ANALOG_STICK_PRECISION_8:
    default:
      return INT8_MAX;
  }
}

int16_t __not_in_flash_func(scaleStickThresholdFrom8Bit)(analog_stick_precision precision, uint8_t threshold8) {
  const int32_t scaled =
      ((int32_t)threshold8 * stickPrecisionFullScale(precision)) / INT8_MAX;
  return (int16_t)(scaled < 1 ? 1 : scaled);
}

bool __not_in_flash_func(remapRuntimeStateChanged)() {
  static uint8_t last_runtime_remaps[REMAP_MAX_BUTTONS] = {};
  static bool initialized = false;

  if (initialized &&
      std::memcmp(last_runtime_remaps, active_remaps, REMAP_MAX_BUTTONS) == 0) {
    return false;
  }

  std::memcpy(last_runtime_remaps, active_remaps, REMAP_MAX_BUTTONS);
  initialized = true;
  return true;
}

void __not_in_flash_func(apply_stick_deadzone)(controller_state_t& frame) {
  if (deadzone_percent == 0) {
    return;
  }

  uint16_t maxvalue = 0;
  uint16_t deadzone = 0;

  switch (frame.sticks_precision_bits) {
    case ANALOG_STICK_PRECISION_8:
      maxvalue = static_cast<uint16_t>(INT8_MAX);
      break;
    case ANALOG_STICK_PRECISION_12:
      maxvalue = static_cast<uint16_t>(INT16_MAX >> 4);
      break;
    case ANALOG_STICK_PRECISION_16:
      maxvalue = static_cast<uint16_t>(INT16_MAX);
      break;
  }
  deadzone = (maxvalue * deadzone_percent) / 100UL;

  if (frame.LX || frame.LY) {
    auto lstick = hardRadialDeadzone(frame.LX, frame.LY, deadzone);
    frame.LX = lstick.x;
    frame.LY = lstick.y;
  }
  if (frame.RX || frame.RY) {
    auto rstick = hardRadialDeadzone(frame.RX, frame.RY, deadzone);
    frame.RX = rstick.x;
    frame.RY = rstick.y;
  }
}

bool __not_in_flash_func(wiiClassicAnalogTriggersStayAnalog)(const controller_state_t& frame) {
  return std::strcmp(frame.controller_type_name, "Classic") == 0;
}

void __not_in_flash_func(apply_deadzone)(uint8_t i) {
  controller_state_t& frame = controllerFrame(i);
  apply_stick_deadzone(frame);

  if (frame.HAS_ANALOG_TRIGGERS &&
      !wiiClassicAnalogTriggersStayAnalog(frame) &&
      (trigger_mode == TRIGGER_MODE_DIGITAL || trigger_mode == TRIGGER_MODE_BOTH)) {
    frame.L2 |= (frame.ANALOG_L2 > TRIGGER_DIGITAL_THRESHOLD);
    frame.R2 |= (frame.ANALOG_R2 > TRIGGER_DIGITAL_THRESHOLD);
  }

  if (trigger_mode == TRIGGER_MODE_DIGITAL) {
    frame.ANALOG_L2 = 0;
    frame.ANALOG_R2 = 0;
  }
}

}  // namespace

namespace controller_runtime_internal {

namespace {


bool __not_in_flash_func(frameHasAnalogStickStateForCentering)(const controller_state_t& frame) {
  return frame.HAS_ANALOG_STICK_MAIN || frame.HAS_ANALOG_STICK_AUX ||
         frame.LX != 0 || frame.LY != 0 || frame.RX != 0 || frame.RY != 0;
}

}  // namespace

void __not_in_flash_func(updateStickCenteringFromPoll)(bool updated) {
  (void)updated;
  // Input pollers rebuild analog values every completed poll. The updated flag
  // only means the stable state changed, so centering must still be re-applied
  // while the controller is resting at the same raw offset.

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (!frame.connected || !frameHasAnalogStickStateForCentering(frame)) {
      stickCenter.update(i, false, 0, 0, 0, 0);
      continue;
    }

    const int16_t autoCenterThreshold =
        scaleStickThresholdFrom8Bit(frame.sticks_precision_bits, 10);
    const int16_t restSnapThreshold =
        scaleStickThresholdFrom8Bit(frame.sticks_precision_bits, 2);
    int16_t sc_lx = frame.LX;
    int16_t sc_ly = frame.LY;
    int16_t sc_rx = frame.RX;
    int16_t sc_ry = frame.RY;

    stickCenter.update(i, frame.connected, sc_lx, sc_ly, sc_rx, sc_ry,
                       autoCenterThreshold);
    if (frame.connected) {
      stickCenter.apply(i, sc_lx, sc_ly, sc_rx, sc_ry, restSnapThreshold);
      frame.LX = sc_lx;
      frame.LY = sc_ly;
      frame.RX = sc_rx;
      frame.RY = sc_ry;
    }
  }
}

void __not_in_flash_func(applyDeadzoneAndTriggerTransforms)(bool updated) {
  (void)updated;
  // Input pollers rebuild raw frame values even when the stable controller
  // state is unchanged, so these transforms must run on every completed poll.
  const bool hasActiveFrameTransforms =
      deadzone_percent != 0 || stick_invert != 0 || trigger_mode != 0;
  if (!hasActiveFrameTransforms) {
    return;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    if (controllerFrameConst(i).connected) {
      apply_deadzone(i);
    }
  }
}

void __not_in_flash_func(applyFinalStickDeadzoneToConnectedInputs)() {
  if (deadzone_percent == 0) {
    return;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    if (controllerFrameConst(i).connected) {
      apply_stick_deadzone(controllerFrame(i));
    }
  }
}

void __not_in_flash_func(snapshotRawInputButtonsBeforeTransforms)() {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    raw_input_buttons[i] = frame.connected ? frame.digital_buttons : 0;
  }
}

void __not_in_flash_func(snapshotPhysicalButtonsBeforeTransforms)() {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (frame.connected) {
      pre_remap_buttons[i] = frame.digital_buttons;
      pre_transform_hotkey_buttons[i] = hotkeyButtonsForControllerFrame(frame);
    } else {
      pre_remap_buttons[i] = 0;
      pre_transform_hotkey_buttons[i] = 0;
    }
  }
}

void __not_in_flash_func(applyButtonRemapTransforms)() {
  if (remapRuntimeStateChanged()) {
    turbo.resetRuntimeState();
  }

  if (!hasActiveRemap() || menuOwnsControllerInput()) {
    return;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (frame.connected) {
      ButtonRemapMenu::applyRemap(frame, active_remaps);
    }
  }
}

void __not_in_flash_func(applyButtonChordRemapTransforms)() {
  if (!hasActiveButtonChordRemap()) {
    return;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    applyButtonChordRemapsToFrame(frame);
  }
}

void __not_in_flash_func(applyTurboTransforms)() {
  if (!turbo.hasAnyEnabled() || menuOwnsControllerInput()) {
    return;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (!frame.connected) {
      continue;
    }

    turbo.updateRawButtons(i, frame.digital_buttons);
    frame.digital_buttons = turbo.applyTurbo(i, frame.digital_buttons);
    if (turbo.isHoldingTurboButton(i)) {
      requestControllerFrameDelivery(i);
    }
  }
}

void __not_in_flash_func(applyClassicDualControllerMergeTransform)() {
#if defined(PRODUCT_CLASSIC2USB)
  static uint32_t lastMergedPlayer2Actions = 0;
  static uint8_t lastMergedPlayer2AnalogL2 = 0;
  static uint8_t lastMergedPlayer2AnalogR2 = 0;
  static int16_t lastMergedPlayer2Rx = 0;
  static int16_t lastMergedPlayer2Ry = 0;
  static uint32_t lastMergedMask = 0;

  if (!classic_dual_merge_enabled || inputPortCount() < 2 || MAX_USB_OUT < 2) {
    lastMergedPlayer2Actions = 0;
    lastMergedPlayer2AnalogL2 = 0;
    lastMergedPlayer2AnalogR2 = 0;
    lastMergedPlayer2Rx = 0;
    lastMergedPlayer2Ry = 0;
    lastMergedMask = 0;
    return;
  }

  controller_state_t& player1 = controllerFrame(0);
  controller_state_t& player2 = controllerFrame(1);
  if (deviceMode == RZORD_PSX &&
      std::strcmp(player1.controller_type_name, "NeGcon") == 0) {
    lastMergedPlayer2Actions = 0;
    lastMergedPlayer2AnalogL2 = 0;
    lastMergedPlayer2AnalogR2 = 0;
    lastMergedPlayer2Rx = 0;
    lastMergedPlayer2Ry = 0;
    lastMergedMask = 0;
    return;
  }
  if (!player1.connected) {
    return;
  }

  const uint32_t actionButtonMask =
    sanitizeClassicDualMergeP2Mask(deviceMode, classic_dual_merge_p2_mask);
  const bool n64Merge = (deviceMode == RZORD_N64);
  const uint32_t n64CButtonMask =
    INPUT_X | INPUT_Y | INPUT_L2 | INPUT_R2 | INPUT_SELECT;

  const uint32_t player2Actions = player2.connected
    ? (player2.digital_buttons & actionButtonMask)
    : 0;
  const uint8_t player2AnalogL2 = player2.connected ? player2.ANALOG_L2 : 0;
  const uint8_t player2AnalogR2 = player2.connected ? player2.ANALOG_R2 : 0;
  const int16_t player2Rx = (player2.connected && n64Merge) ? player2.RX : 0;
  const int16_t player2Ry = (player2.connected && n64Merge) ? player2.RY : 0;

  player1.digital_buttons = (player1.digital_buttons & ~actionButtonMask) | player2Actions;
  if (player2.connected && (actionButtonMask & (INPUT_L2 | INPUT_R2)) != 0) {
    player1.HAS_ANALOG_TRIGGERS |= player2.HAS_ANALOG_TRIGGERS;
    if ((actionButtonMask & INPUT_L2) != 0) {
      player1.ANALOG_L2 = player2AnalogL2;
    }
    if ((actionButtonMask & INPUT_R2) != 0) {
      player1.ANALOG_R2 = player2AnalogR2;
    }
  } else if ((actionButtonMask & (INPUT_L2 | INPUT_R2)) != 0) {
    if ((actionButtonMask & INPUT_L2) != 0) {
      player1.ANALOG_L2 = 0;
    }
    if ((actionButtonMask & INPUT_R2) != 0) {
      player1.ANALOG_R2 = 0;
    }
  }
  if (n64Merge && (actionButtonMask & n64CButtonMask) != 0) {
    player1.RX = player2Rx;
    player1.RY = player2Ry;
    if (player2.connected) {
      player1.HAS_ANALOG_STICK_AUX |= player2.HAS_ANALOG_STICK_AUX;
    }
  }
  if (lastMergedPlayer2Actions != player2Actions ||
      lastMergedPlayer2AnalogL2 != player2AnalogL2 ||
      lastMergedPlayer2AnalogR2 != player2AnalogR2 ||
      lastMergedPlayer2Rx != player2Rx ||
      lastMergedPlayer2Ry != player2Ry ||
      lastMergedMask != actionButtonMask) {
    requestControllerFrameDelivery(0);
  }
  lastMergedPlayer2Actions = player2Actions;
  lastMergedPlayer2AnalogL2 = player2AnalogL2;
  lastMergedPlayer2AnalogR2 = player2AnalogR2;
  lastMergedPlayer2Rx = player2Rx;
  lastMergedPlayer2Ry = player2Ry;
  lastMergedMask = actionButtonMask;
#endif
}

void __not_in_flash_func(updateJaguarRotaryStateFromInputs)() {
  #ifdef ENABLE_INPUT_JAGUAR
  if (deviceMode != RZORD_JAGUAR) {
    return;
  }

  static const int8_t quadTable[16] = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0
  };

  static uint8_t lastState[MAX_USB_OUT] = {0};
  static int32_t counter[MAX_USB_OUT] = {0};

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    bool curL = frame.ANALOG_PAD_L > 0;
    bool curR = frame.ANALOG_PAD_R > 0;
    uint8_t curState = (curL << 1) | curR;

    if (curL && curR) {
      jaguarRotaryActivePorts[i] = true;
    }

    int8_t delta = 0;
    if (curState != lastState[i]) {
      uint8_t tableIndex = (lastState[i] << 2) | curState;
      delta = quadTable[tableIndex];
    }

    if (jaguarRotaryActivePorts[i]) {
      if (delta != 0) {
        int16_t scaledDelta = (int16_t)delta * spinner_speed_mult[spinner_speed];
        counter[i] = constrain(counter[i] - scaledDelta, 0, 255);
      }

      int16_t rotaryPosition = constrain(counter[i], 0, 255);
      frame.LX = rotaryPosition - 128;
      frame.paddle = rotaryPosition;
    }

    lastState[i] = curState;
  }

  bool anyJaguarRotary = false;
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    if (jaguarRotaryActivePorts[i]) {
      anyJaguarRotary = true;
      break;
    }
  }
  jaguarRotaryActive = anyJaguarRotary;
  #endif
}

void __not_in_flash_func(applySocdCleaningToInputs)() {
#if defined(PRODUCT_CLASSIC2USB)
  return;
#endif
  if (!socdMode) {
    return;
  }

  static struct __attribute((packed, aligned(1))) {
    uint8_t PAD_U : 1;
    uint8_t PAD_D : 1;
    uint8_t PAD_L : 1;
    uint8_t PAD_R : 1;
    uint8_t : 4;
  } last_direction_state[MAX_USB_OUT] { 0 };

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    bool isSpecialLR = false;
    bool isSpecialUD = false;
    #ifdef ENABLE_INPUT_PSX
    if (deviceMode == RZORD_PSX) {
      isSpecialLR = frame.PAD_L && frame.PAD_R;
      isSpecialUD = frame.PAD_U && frame.PAD_D && !isSpecialLR;
    }
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    if (deviceMode == RZORD_JAGUAR) {
      isSpecialLR = frame.PAD_L && frame.PAD_R;
    }
    #endif
    #ifdef ENABLE_INPUT_JPC
    if (deviceMode == RZORD_JPC) {
      isSpecialLR = true;
      isSpecialUD = true;
    }
    #endif

    if (frame.PAD_U && frame.PAD_D && !isSpecialUD) {
      if (socdMode == SOCD_NEUTRAL) {
        frame.PAD_U = 0;
        frame.PAD_D = 0;
      } else if (socdMode == SOCD_SECOND && (last_direction_state[i].PAD_U || last_direction_state[i].PAD_D)) {
        frame.PAD_U = last_direction_state[i].PAD_D;
        frame.PAD_D = last_direction_state[i].PAD_U;
      } else if (socdMode == SOCD_FIRST && (last_direction_state[i].PAD_U || last_direction_state[i].PAD_D)) {
        frame.PAD_U = last_direction_state[i].PAD_U;
        frame.PAD_D = last_direction_state[i].PAD_D;
      } else {
        last_direction_state[i].PAD_U = 0;
        last_direction_state[i].PAD_D = 0;
      }
    } else {
      last_direction_state[i].PAD_U = frame.PAD_U;
      last_direction_state[i].PAD_D = frame.PAD_D;
    }

    if (frame.PAD_L && frame.PAD_R && !isSpecialLR) {
      if (socdMode == SOCD_NEUTRAL) {
        frame.PAD_L = 0;
        frame.PAD_R = 0;
      } else if (socdMode == SOCD_SECOND && (last_direction_state[i].PAD_L || last_direction_state[i].PAD_R)) {
        frame.PAD_L = last_direction_state[i].PAD_R;
        frame.PAD_R = last_direction_state[i].PAD_L;
      } else if (socdMode == SOCD_FIRST && (last_direction_state[i].PAD_L || last_direction_state[i].PAD_R)) {
        frame.PAD_L = last_direction_state[i].PAD_L;
        frame.PAD_R = last_direction_state[i].PAD_R;
      } else {
        last_direction_state[i].PAD_L = 0;
        last_direction_state[i].PAD_R = 0;
      }
    } else {
      last_direction_state[i].PAD_L = frame.PAD_L;
      last_direction_state[i].PAD_R = frame.PAD_R;
    }
  }
}

}  // namespace controller_runtime_internal
