#include "../../product_config.h"

#include "platform_menu_runtime.h"

#include <Arduino.h>

#include "../../core/controller_frame_state.h"
#include "../../core/controller_output_cache_state.h"
#include "../../core/device_runtime_state.h"
#include "../../core/firmware_support.h"
#include "../../core/hotkey_combo.h"
#include "../../core/settings_store.h"
#include "../../firmware_platform_config.h"
#include "../../input/shared/input_button_bits.h"
#include "../../input/runtime/input_mode_runtime.h"
#include "../../menu/menu_bridge.h"
#include "../../menu/menu_idle_runtime.h"
#include "../../menu/menu_input.h"
#include "../../menu/menu_runtime_state.h"
#include "../../menu/menu_ui_state.h"
#include "../../menu/quick_config.h"
#include "../buzzer.h"
#include "../display_runtime_state.h"

namespace {

constexpr uint16_t kMenuEntryHotkeyMinHoldMs = 1000;
constexpr uint16_t kMenuEntryInputSuppressMs = 500;
constexpr uint16_t kKioskTapWindowMs = 900;
constexpr uint8_t kKioskMenuTapCount = 2;
constexpr uint8_t kKioskResetTapCount = 2;

enum class SerialMenuAction : uint8_t {
  None,
  Menu,
  Up,
  Down,
  Left,
  Right,
  Ok,
  Back
};

SerialMenuAction pendingSerialMenuAction = SerialMenuAction::None;

struct KioskTapGate {
  uint8_t count = 0;
  uint32_t firstTapMs = 0;
};

KioskTapGate kioskMenuGate;
KioskTapGate kioskResetGate;

bool productUsesSingleSettingsMenu() {
  return false;
}

void maybeTriggerHeldMenuHotkey(
    uint32_t combo,
    uint32_t& startTime,
    bool& active,
    void (*action)()) {
  if (combo == 0) {
    active = false;
    return;
  }

  uint16_t holdMs = hotkeyHoldMsForSetting(menu_hotkey_hold_time);
  if (holdMs < kMenuEntryHotkeyMinHoldMs) {
    holdMs = kMenuEntryHotkeyMinHoldMs;
  }
  const bool held = hotkeyComboMatches(pre_transform_hotkey_buttons[0], combo);
  if (!held) {
    active = false;
    return;
  }

  if (!active) {
    active = true;
    startTime = millis();
    return;
  }

  if (millis() - startTime >= holdMs) {
    active = false;
    action();
  }
}

void openQuickConfigMenu() {
  suppressMenuControllerInputForMs(kMenuEntryInputSuppressMs);
  isQuickConfigOpen = true;
  quickConfig.open();
  buzzer.playMenuEnter();
}

void openQuickConfigMenuFromHotkey() {
  openQuickConfigMenu();
}

void openFullMenu() {
  suppressMenuControllerInputForMs(kMenuEntryInputSuppressMs);
  system_menu_only = false;
  isMenuOpen = true;
  buzzer.playMenuEnter();
}

void openFullMenuFromHotkey() {
  openFullMenu();
}

void openSystemOnlyMenu() {
  suppressMenuControllerInputForMs(kMenuEntryInputSuppressMs);
  system_menu_only = true;
  isMenuOpen = true;
  buzzer.playMenuEnter();
}

void openSystemOnlyMenuFromHotkey() {
  openSystemOnlyMenu();
}

bool actionEquals(const char* action, const char* expected) {
  if (action == nullptr) {
    return false;
  }
  while (*action == ' ' || *action == '\t') {
    ++action;
  }
  while (*expected != '\0') {
    char c = *action++;
    if (c >= 'a' && c <= 'z') {
      c = (char)(c - ('a' - 'A'));
    }
    if (c != *expected++) {
      return false;
    }
  }
  return *action == '\0' || *action == ' ' || *action == '\t';
}

void resetKioskTapGate(KioskTapGate& gate) {
  gate.count = 0;
  gate.firstTapMs = 0;
}

ButtonEvent kioskGateTapPress(ButtonEvent event, KioskTapGate& gate, uint8_t requiredTaps) {
  if (event == BTN_EVENT_NONE) {
    if (gate.count != 0 && millis() - gate.firstTapMs > kKioskTapWindowMs) {
      resetKioskTapGate(gate);
    }
    return BTN_EVENT_NONE;
  }

  if (event == BTN_EVENT_LONG || event == BTN_EVENT_LONG_RELEASE) {
    resetKioskTapGate(gate);
    return event;
  }

  if (event != BTN_EVENT_SINGLE && event != BTN_EVENT_DOUBLE) {
    resetKioskTapGate(gate);
    return event;
  }

  const uint8_t tapIncrement = event == BTN_EVENT_DOUBLE ? 2 : 1;
  const uint32_t now = millis();
  if (gate.count == 0 || now - gate.firstTapMs > kKioskTapWindowMs) {
    gate.count = tapIncrement;
    gate.firstTapMs = now;
  } else {
    gate.count += tapIncrement;
  }

  if (gate.count >= requiredTaps) {
    resetKioskTapGate(gate);
    return BTN_EVENT_SINGLE;
  }
  return BTN_EVENT_NONE;
}

void applyKioskButtonGate(ButtonEvent& modeEvent, ButtonEvent& resetEvent) {
  if (menu_kiosk_mode != 0) {
    modeEvent = kioskGateTapPress(modeEvent, kioskMenuGate, kKioskMenuTapCount);
    resetEvent = kioskGateTapPress(resetEvent, kioskResetGate, kKioskResetTapCount);
    return;
  }

  {
    resetKioskTapGate(kioskMenuGate);
    resetKioskTapGate(kioskResetGate);
  }
}

uint32_t serialMenuActionToButtons(SerialMenuAction action) {
  switch (action) {
    case SerialMenuAction::Up: return 0x01;
    case SerialMenuAction::Down: return 0x02;
    case SerialMenuAction::Left: return 0x04;
    case SerialMenuAction::Right: return 0x08;
    case SerialMenuAction::Ok: return 0x10;
    case SerialMenuAction::Back: return 0x20;
    default: return 0;
  }
}

SerialMenuAction takeSerialMenuAction() {
  const SerialMenuAction action = pendingSerialMenuAction;
  pendingSerialMenuAction = SerialMenuAction::None;
  return action;
}

#ifdef ENABLE_INPUT_AUTODETECT
void activateAutoDetectModeFromResetHold() {
  #ifdef USE_I2C_DISPLAY
    beginDisplayWire();
    display.clear();
    display.setCol(0);
    display.set2X();
    display.println(F("AUTO DETECT"));
    display.set1X();
    display.println(F("Release RESET"));
    Wire.end();
  #endif

  buzzer.playSave();
  armAutoDetectReboot();
}
#endif

void handleHomeButtonEvents(ButtonEvent modeEvent, ButtonEvent resetEvent) {
  applyKioskButtonGate(modeEvent, resetEvent);

  switch (modeEvent) {
    case BTN_EVENT_SINGLE:
      if (productUsesSingleSettingsMenu()) {
        openFullMenu();
      } else {
        openQuickConfigMenu();
      }
      break;

    case BTN_EVENT_LONG:
      if (productUsesSingleSettingsMenu()) {
        openFullMenu();
      } else {
        openSystemOnlyMenu();
      }
      break;

    case BTN_EVENT_LONG_RELEASE:
      break;

    default:
      break;
  }

  switch (resetEvent) {
    case BTN_EVENT_SINGLE:
      buzzer.playSave();
      waitWithBuzzerUpdates(100);
      reboot();
      break;

    case BTN_EVENT_LONG:
      #ifdef ENABLE_INPUT_AUTODETECT
      activateAutoDetectModeFromResetHold();
      #endif
      break;

    default:
      break;
  }
}

void handleQuickConfigUi(ButtonEvent modeEvent, ButtonEvent resetEvent, SerialMenuAction serialAction) {
  #ifdef USE_I2C_DISPLAY
    beginDisplayWire();

    static bool prev_up = false, prev_down = false, prev_left = false, prev_right = false;
    static bool prev_a = false, prev_b = false, prev_start = false;

    static uint32_t last_nav_time = 0;
    static uint32_t hold_start_time = 0;
    static uint8_t last_direction = 0;
    const uint32_t DEBOUNCE_MS = 50;
    const uint32_t REPEAT_DELAY_MS = 400;
    const uint32_t REPEAT_RATE_MS = 100;

    controller_state_t& primary = controllerFrame(0);
    bool cur_up = primary.PAD_U;
    bool cur_down = primary.PAD_D;
    bool cur_left = primary.PAD_L;
    bool cur_right = primary.PAD_R;
    bool cur_a = menuControllerConfirmPressed(primary, deviceMode);
    bool cur_b = menuControllerBackPressed(primary, deviceMode);
    bool cur_start = (primary.digital_buttons & INPUT_START) != 0;

    const bool suppressControllerInput = isMenuControllerInputSuppressed();
    if (suppressControllerInput) {
      prev_up = cur_up;
      prev_down = cur_down;
      prev_left = cur_left;
      prev_right = cur_right;
      prev_a = cur_a;
      prev_b = cur_b;
      prev_start = cur_start;
      cur_up = false;
      cur_down = false;
      cur_left = false;
      cur_right = false;
      cur_a = false;
      cur_b = false;
      cur_start = false;
      if (serialAction == SerialMenuAction::None) {
        quickConfig.render(display);
        Wire.end();
        return;
      }
    }

    bool up_pressed = cur_up && !prev_up;
    bool down_pressed = cur_down && !prev_down;
    bool left_pressed = cur_left && !prev_left;
    bool right_pressed = cur_right && !prev_right;
    bool a_pressed = cur_a && !prev_a;
    bool b_pressed = cur_b && !prev_b;
    bool start_pressed = cur_start && !prev_start;

    up_pressed = up_pressed || serialAction == SerialMenuAction::Up;
    down_pressed = down_pressed || serialAction == SerialMenuAction::Down;
    left_pressed = left_pressed || serialAction == SerialMenuAction::Left;
    right_pressed = right_pressed || serialAction == SerialMenuAction::Right;
    a_pressed = a_pressed || serialAction == SerialMenuAction::Ok;
    b_pressed = b_pressed || serialAction == SerialMenuAction::Back;

    uint32_t now = millis();
    bool up_repeat = false, down_repeat = false, left_repeat = false, right_repeat = false;
    if (cur_up && last_direction == 1 && (now - hold_start_time > REPEAT_DELAY_MS)) {
      if (now - last_nav_time >= REPEAT_RATE_MS) up_repeat = true;
    }
    if (cur_down && last_direction == 2 && (now - hold_start_time > REPEAT_DELAY_MS)) {
      if (now - last_nav_time >= REPEAT_RATE_MS) down_repeat = true;
    }
    if (cur_left && last_direction == 3 && (now - hold_start_time > REPEAT_DELAY_MS)) {
      if (now - last_nav_time >= REPEAT_RATE_MS) left_repeat = true;
    }
    if (cur_right && last_direction == 4 && (now - hold_start_time > REPEAT_DELAY_MS)) {
      if (now - last_nav_time >= REPEAT_RATE_MS) right_repeat = true;
    }

    if (up_pressed) { last_direction = 1; hold_start_time = now; }
    else if (down_pressed) { last_direction = 2; hold_start_time = now; }
    else if (left_pressed) { last_direction = 3; hold_start_time = now; }
    else if (right_pressed) { last_direction = 4; hold_start_time = now; }
    else if (!cur_up && !cur_down && !cur_left && !cur_right) { last_direction = 0; }

    prev_up = cur_up;
    prev_down = cur_down;
    prev_left = cur_left;
    prev_right = cur_right;
    prev_a = cur_a;
    prev_b = cur_b;
    prev_start = cur_start;

    handleIdleUiActivity(
      modeEvent != BTN_EVENT_NONE ||
      resetEvent != BTN_EVENT_NONE ||
      serialAction != SerialMenuAction::None ||
      up_pressed || down_pressed || left_pressed || right_pressed ||
      a_pressed || b_pressed || start_pressed ||
      up_repeat || down_repeat || left_repeat || right_repeat
    );

    bool can_nav = (now - last_nav_time >= DEBOUNCE_MS);
    bool start_consumed = false;

    if (can_nav && start_pressed) {
      const bool stayedOpen = quickConfig.isOpen();
      if (quickConfig.saveShortcut()) {
        if (stayedOpen && quickConfig.isOpen()) {
          buzzer.playSave();
        }
        last_nav_time = now;
        start_consumed = true;
      }
    }

    if (!start_consumed && can_nav && (resetEvent == BTN_EVENT_SINGLE || down_pressed || down_repeat)) {
      quickConfig.navigateDown();
      buzzer.playMenuNav();
      last_nav_time = now;
    }
    if (!start_consumed && can_nav && (up_pressed || up_repeat)) {
      quickConfig.navigateUp();
      buzzer.playMenuNav();
      last_nav_time = now;
    }
    if (!start_consumed && can_nav && (right_pressed || right_repeat)) {
      quickConfig.cycleValueNext(false);
      buzzer.playMenuNav();
      last_nav_time = now;
    }
    if (!start_consumed && can_nav && (modeEvent == BTN_EVENT_SINGLE || a_pressed)) {
      quickConfig.cycleValueNext(true);
      buzzer.playMenuNav();
      last_nav_time = now;
    }
    if (!start_consumed && can_nav && b_pressed) {
      quickConfig.back();
      buzzer.playMenuNav();
      last_nav_time = now;
    }
    if (!start_consumed && can_nav && (left_pressed || left_repeat)) {
      quickConfig.cycleValuePrev();
      buzzer.playMenuNav();
      last_nav_time = now;
    }

    if (!quickConfig.isOpen()) {
      isQuickConfigOpen = false;
      if (quickConfig.shouldSave()) {
        saveModeSettings();
        buzzer.playSave();
      } else {
        buzzer.playMenuNav();
      }
      display.clear();
      forceMainDisplayRefresh();
      prev_up = prev_down = prev_left = prev_right = false;
      prev_a = prev_b = prev_start = false;
    } else {
      quickConfig.render(display);
    }

    Wire.end();
  #else
    (void)modeEvent;
    (void)resetEvent;
    (void)serialAction;
  #endif
}

void handleSystemMenuUi(ButtonEvent modeEvent, ButtonEvent resetEvent, SerialMenuAction serialAction) {
  #ifdef USE_I2C_DISPLAY
    beginDisplayWire();
    const uint32_t injectedButtons = serialMenuActionToButtons(serialAction);
    if (injectedButtons != 0) {
      queueMenuControllerButtons(injectedButtons);
    }
    bool modeBtnJustPressed = (modeEvent == BTN_EVENT_SINGLE);
    bool resetBtnJustPressed = (resetEvent == BTN_EVENT_SINGLE);
    showMenu(modeBtnJustPressed, resetBtnJustPressed);
    Wire.end();
  #else
    (void)modeEvent;
    (void)resetEvent;
    (void)serialAction;
  #endif
}

}  // namespace

bool wakeFromVisibleScreensaver(ButtonEvent modeEvent, ButtonEvent resetEvent) {
  return handleIdleUiActivity(modeEvent != BTN_EVENT_NONE || resetEvent != BTN_EVENT_NONE);
}

void handleMenuAndButtonUi(ButtonEvent modeEvent, ButtonEvent resetEvent) {
  const SerialMenuAction serialAction = takeSerialMenuAction();
  if (serialAction == SerialMenuAction::Menu && !isMenuOpen && !isQuickConfigOpen) {
    modeEvent = BTN_EVENT_SINGLE;
  }

  if (!isMenuOpen && !isQuickConfigOpen) {
    handleHomeButtonEvents(modeEvent, resetEvent);
  } else if (isQuickConfigOpen) {
    handleQuickConfigUi(modeEvent, resetEvent, serialAction);
  } else if (isMenuOpen) {
    handleSystemMenuUi(modeEvent, resetEvent, serialAction);
  }
}

bool queuePlatformMenuControlAction(const char* action) {
  SerialMenuAction parsed = SerialMenuAction::None;
  if (actionEquals(action, "MENU")) parsed = SerialMenuAction::Menu;
  else if (actionEquals(action, "UP")) parsed = SerialMenuAction::Up;
  else if (actionEquals(action, "DOWN")) parsed = SerialMenuAction::Down;
  else if (actionEquals(action, "LEFT")) parsed = SerialMenuAction::Left;
  else if (actionEquals(action, "RIGHT")) parsed = SerialMenuAction::Right;
  else if (actionEquals(action, "OK")) parsed = SerialMenuAction::Ok;
  else if (actionEquals(action, "BACK")) parsed = SerialMenuAction::Back;

  if (parsed == SerialMenuAction::None) {
    return false;
  }

  pendingSerialMenuAction = parsed;
  handleIdleUiActivity(true);
  return true;
}

void runPlatformMenuHotkeys() {
  const uint32_t quickCombo = menu_menu_hotkey != 0 ? menu_hotkey_combo : 0;
  const uint32_t systemCombo = menu_system_menu_hotkey != 0 ? menu_system_hotkey_combo : 0;
  const bool singleSettingsMenu = productUsesSingleSettingsMenu();
  const uint32_t activeSystemCombo = singleSettingsMenu ? 0 : systemCombo;

  if ((quickCombo == 0 && activeSystemCombo == 0) || isMenuOpen || isQuickConfigOpen) {
    return;
  }

  static uint32_t menuHotkeyStartTime = 0;
  static uint32_t systemHotkeyStartTime = 0;
  static bool menuHotkeyActive = false;
  static bool systemHotkeyActive = false;

  if (!(max_devices > 0 && controllerFrameConst(0).connected)) {
    menuHotkeyActive = false;
    systemHotkeyActive = false;
    return;
  }

  const uint32_t physicalHotkeyButtons = pre_transform_hotkey_buttons[0] & HOTKEY_ALLOWED_BUTTONS;
  const bool canTriggerQuick = quickCombo != 0 && physicalHotkeyButtons == quickCombo;
  const bool canTriggerSystem = activeSystemCombo != 0 && physicalHotkeyButtons == activeSystemCombo;
  if (!menuHotkeyActive && !systemHotkeyActive && !canTriggerQuick && !canTriggerSystem) {
    return;
  }

  if (singleSettingsMenu) {
    maybeTriggerHeldMenuHotkey(quickCombo, menuHotkeyStartTime, menuHotkeyActive, openFullMenuFromHotkey);
    systemHotkeyActive = false;
  } else {
    maybeTriggerHeldMenuHotkey(quickCombo, menuHotkeyStartTime, menuHotkeyActive, openQuickConfigMenuFromHotkey);
    maybeTriggerHeldMenuHotkey(activeSystemCombo, systemHotkeyStartTime, systemHotkeyActive, openSystemOnlyMenuFromHotkey);
  }
}

void saveModeSettings() {
  uint8_t tempRates[TURBO_BTN_COUNT];
  PerModeQuickSettings selectedSettings;
  DeviceEnum newInputMode = quickConfig.getTempInputMode();
  outputMode_t newOutputMode = quickConfig.getTempOutputMode();

  quickConfig.getTempRates(tempRates);
  quickConfig.getTempQuickSettings(newInputMode, selectedSettings);
  saveModeSettings(newInputMode, newOutputMode, tempRates, selectedSettings);
}
