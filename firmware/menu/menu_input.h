#pragma once

#include <stdint.h>

#include "../core/controller_state.h"
#include "../core/device_mode.h"
#include "../input/shared/input_button_bits.h"

struct MenuControllerInput {
  bool upJust;
  bool downJust;
  bool leftJust;
  bool rightJust;
  bool aJust;
  bool bJust;
  bool startJust;
  uint32_t buttons;
};

bool menuControllerConfirmPressed(const controller_state_t& report, DeviceEnum mode);
bool menuControllerBackPressed(const controller_state_t& report, DeviceEnum mode);
const char* getMenuConfirmButtonName(DeviceEnum mode);
const char* getMenuBackButtonName(DeviceEnum mode);
uint32_t getMenuWakeButtons(const controller_state_t& report, DeviceEnum mode);
uint32_t getScreensaverWakeButtons(const controller_state_t& report);
void queueMenuControllerButtons(uint32_t buttons);
void suppressMenuControllerInputForMs(uint16_t ms);
bool isMenuControllerInputSuppressed();
MenuControllerInput readMenuControllerInput();
