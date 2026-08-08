#include "../product_config.h"

#include "menu_input.h"
#include "menu.h"

#include <Arduino.h>

#include "../platform/buzzer.h"
#include "../input/shared/input_button_bits.h"

namespace {

uint32_t menu_controller_input_suppress_until_ms = 0;
bool menu_controller_input_suppress_until_release = false;
uint32_t menu_controller_input_release_since_ms = 0;
uint32_t pending_menu_action_buttons = 0;
uint32_t queued_menu_controller_buttons = 0;
constexpr uint8_t kAnalogButtonWakeThreshold = 16;
constexpr uint32_t kMenuInputReleaseStableMs = 100;
constexpr uint32_t kCtrlUp = 0x01;
constexpr uint32_t kCtrlDown = 0x02;
constexpr uint32_t kCtrlLeft = 0x04;
constexpr uint32_t kCtrlRight = 0x08;
constexpr uint32_t kCtrlConfirm = 0x10;
constexpr uint32_t kCtrlBack = 0x20;
constexpr uint32_t kCtrlQuickHotkey = 0x40;
constexpr uint32_t kCtrlSystemHotkey = 0x80;
constexpr uint32_t kCtrlStart = 0x100;
constexpr uint32_t kCtrlDirectionMask = kCtrlUp | kCtrlDown | kCtrlLeft | kCtrlRight;
constexpr uint32_t kCtrlMenuActionMask =
    kCtrlDirectionMask | kCtrlConfirm | kCtrlBack | kCtrlStart;
constexpr uint32_t kCtrlDeferredActionMask = kCtrlConfirm | kCtrlBack | kCtrlStart;

}

bool menuControllerConfirmPressed(const controller_state_t& report, DeviceEnum mode) {
  const uint32_t buttons = report.digital_buttons;

  switch (mode) {
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      return (buttons & (INPUT_A | INPUT_START)) != 0;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return (buttons & INPUT_B) != 0;
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG:
      return (buttons & INPUT_A) != 0;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return (buttons & INPUT_B) != 0;
    #endif
    default:
      return (buttons & INPUT_A) != 0;
  }
}

bool menuControllerBackPressed(const controller_state_t& report, DeviceEnum mode) {
  const uint32_t buttons = report.digital_buttons;

  switch (mode) {
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      return (buttons & (INPUT_B | INPUT_SELECT)) != 0;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return (buttons & INPUT_A) != 0;
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG:
      return (buttons & INPUT_B) != 0;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return (buttons & INPUT_A) != 0;
    #endif
    default:
      return (buttons & INPUT_B) != 0;
  }
}

const char* getMenuConfirmButtonName(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      return "K1/Start";
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return "I";
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG:
      return "X";
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return "A";
    #endif
    default:
      return getRemapButtonName(mode, 0);
  }
}

const char* getMenuBackButtonName(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      return "K2/Svc";
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return "II";
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG:
      return "O";
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return "B";
    #endif
    default:
      if (getRemapButtonCount(mode) < 2) return "--";
      return getRemapButtonName(mode, 1);
  }
}

uint32_t getMenuWakeButtons(const controller_state_t& report, DeviceEnum mode) {
  uint32_t ctrlButtons = 0;

  if (report.PAD_U) ctrlButtons |= kCtrlUp;
  if (report.PAD_D) ctrlButtons |= kCtrlDown;
  if (report.PAD_L) ctrlButtons |= kCtrlLeft;
  if (report.PAD_R) ctrlButtons |= kCtrlRight;
  if (menuControllerConfirmPressed(report, mode)) ctrlButtons |= kCtrlConfirm;
  if (menuControllerBackPressed(report, mode)) ctrlButtons |= kCtrlBack;

  const uint32_t buttons = report.digital_buttons;
  if (buttons & INPUT_START) ctrlButtons |= kCtrlStart;
  if ((buttons & (INPUT_PAD_L | INPUT_START)) == (INPUT_PAD_L | INPUT_START)) ctrlButtons |= kCtrlQuickHotkey;
  if ((buttons & (INPUT_PAD_R | INPUT_START)) == (INPUT_PAD_R | INPUT_START)) ctrlButtons |= kCtrlSystemHotkey;

  return ctrlButtons;
}
uint32_t currentMenuActionButtons() {
  const controller_state_t& frame = controllerFrameConst(0);
  if (!frame.connected) {
    return 0;
  }
  return getMenuWakeButtons(frame, deviceMode) & kCtrlMenuActionMask;
}

uint32_t getScreensaverWakeButtons(const controller_state_t& report) {
  uint32_t buttons = report.digital_buttons;

  if (report.ANALOG_A > kAnalogButtonWakeThreshold) buttons |= INPUT_A;
  if (report.ANALOG_B > kAnalogButtonWakeThreshold) buttons |= INPUT_B;
  if (report.ANALOG_X > kAnalogButtonWakeThreshold) buttons |= INPUT_X;
  if (report.ANALOG_Y > kAnalogButtonWakeThreshold) buttons |= INPUT_Y;
  if (report.ANALOG_L1 > kAnalogButtonWakeThreshold) buttons |= INPUT_L1;
  if (report.ANALOG_R1 > kAnalogButtonWakeThreshold) buttons |= INPUT_R1;
  if (report.ANALOG_L2 > kAnalogButtonWakeThreshold) buttons |= INPUT_L2;
  if (report.ANALOG_R2 > kAnalogButtonWakeThreshold) buttons |= INPUT_R2;
  if (report.ANALOG_PAD_U > kAnalogButtonWakeThreshold) buttons |= INPUT_PAD_U;
  if (report.ANALOG_PAD_D > kAnalogButtonWakeThreshold) buttons |= INPUT_PAD_D;
  if (report.ANALOG_PAD_L > kAnalogButtonWakeThreshold) buttons |= INPUT_PAD_L;
  if (report.ANALOG_PAD_R > kAnalogButtonWakeThreshold) buttons |= INPUT_PAD_R;

  return buttons;
}

void suppressMenuControllerInputForMs(uint16_t ms) {
  menu_controller_input_suppress_until_ms = millis() + ms;
  menu_controller_input_suppress_until_release = true;
  menu_controller_input_release_since_ms = 0;
  pending_menu_action_buttons = 0;
}

bool isMenuControllerInputSuppressed() {
  const uint32_t now = millis();
  if (menu_controller_input_suppress_until_ms != 0 &&
      (int32_t)(now - menu_controller_input_suppress_until_ms) < 0) {
    return true;
  }
  menu_controller_input_suppress_until_ms = 0;
  if (menu_controller_input_suppress_until_release) {
    if (currentMenuActionButtons() != 0) {
      menu_controller_input_release_since_ms = 0;
      return true;
    }
    if (menu_controller_input_release_since_ms == 0) {
      menu_controller_input_release_since_ms = now;
      return true;
    }
    if ((uint32_t)(now - menu_controller_input_release_since_ms) < kMenuInputReleaseStableMs) {
      return true;
    }
    menu_controller_input_suppress_until_release = false;
    menu_controller_input_release_since_ms = 0;
  }
  return false;
}

void queueMenuControllerButtons(uint32_t buttons) {
  queued_menu_controller_buttons |= buttons & kCtrlMenuActionMask;
}

MenuControllerInput readMenuControllerInput() {
  MenuControllerInput input = {false, false, false, false, false, false, false, 0};

  static uint32_t lastCtrlButtons = 0;
  static uint32_t ctrlRepeatTimer = 0;
  static uint32_t lastActionTime = 0;

  const uint32_t CTRL_REPEAT_DELAY = 400;
  const uint32_t CTRL_REPEAT_RATE = 100;
  const uint32_t CTRL_DEBOUNCE_MS = 50;

  uint32_t ctrlButtons = 0;
  const controller_state_t& frame = controllerFrameConst(0);
  if (frame.connected) {
    ctrlButtons = getMenuWakeButtons(frame, deviceMode) & kCtrlMenuActionMask;
  }
  const uint32_t injectedButtons = queued_menu_controller_buttons & kCtrlMenuActionMask;
  queued_menu_controller_buttons = 0;

  input.buttons = ctrlButtons | injectedButtons;

  uint32_t now = millis();
  if (isMenuControllerInputSuppressed()) {
    lastCtrlButtons = ctrlButtons;
    ctrlRepeatTimer = now + CTRL_REPEAT_DELAY;
    lastActionTime = now;
    pending_menu_action_buttons = 0;
    input.buttons = injectedButtons;
    return input;
  }

  bool canAct = (now - lastActionTime >= CTRL_DEBOUNCE_MS);

  uint32_t ctrlPressed = ctrlButtons & ~lastCtrlButtons;
  uint32_t ctrlReleased = ~ctrlButtons & lastCtrlButtons;

  if (ctrlPressed) {
    ctrlRepeatTimer = now + CTRL_REPEAT_DELAY;
  }
  if (ctrlReleased) {
  }

  bool ctrlRepeat = false;
  if (canAct && (ctrlButtons & kCtrlDirectionMask)) {
    if (now >= ctrlRepeatTimer) {
      ctrlRepeat = true;
      ctrlRepeatTimer = now + CTRL_REPEAT_RATE;
    }
  }

  if (!canAct) {
    // Queue non-repeat action edges through the short debounce window.
    // Otherwise a quick confirm/save after moving to Save/Default can be
    // pressed and released before the menu is allowed to act on it.
    pending_menu_action_buttons |=
        ((ctrlPressed | injectedButtons) & kCtrlDeferredActionMask);
    lastCtrlButtons = ctrlButtons;
    return input;
  }

  const uint32_t actionPressed = ctrlPressed | injectedButtons;
  input.upJust    = (actionPressed & kCtrlUp) || ((ctrlButtons & kCtrlUp) && ctrlRepeat);
  input.downJust  = (actionPressed & kCtrlDown) || ((ctrlButtons & kCtrlDown) && ctrlRepeat);
  input.leftJust  = (actionPressed & kCtrlLeft) || ((ctrlButtons & kCtrlLeft) && ctrlRepeat);
  input.rightJust = (actionPressed & kCtrlRight) || ((ctrlButtons & kCtrlRight) && ctrlRepeat);
  input.aJust     = ((actionPressed | pending_menu_action_buttons) & kCtrlConfirm) != 0;
  input.bJust     = ((actionPressed | pending_menu_action_buttons) & kCtrlBack) != 0;
  input.startJust = ((actionPressed | pending_menu_action_buttons) & kCtrlStart) != 0;
  pending_menu_action_buttons = 0;

  if (input.upJust || input.downJust || input.leftJust || input.rightJust ||
      input.aJust || input.bJust || input.startJust) {
    lastActionTime = now;
  }

  lastCtrlButtons = ctrlButtons;

  return input;
}
