#pragma once

#include <stddef.h>
#include <stdint.h>

#include "device_mode.h"
#include "controller_state.h"
#include "../input/shared/input_button_bits.h"

enum class HotkeyBindingId : uint8_t {
  Menu = 0,
  SystemMenu,
  Home,
  Capture,
  Count,
};

struct HotkeyBindingSet {
  uint32_t menu;
  uint32_t system_menu;
  uint32_t home;
  uint32_t capture;
};

constexpr uint32_t HOTKEY_ALLOWED_BUTTONS =
  INPUT_A | INPUT_B | INPUT_X | INPUT_Y |
  INPUT_L1 | INPUT_R1 | INPUT_L2 | INPUT_R2 |
  INPUT_L3 | INPUT_R3 | INPUT_START | INPUT_SELECT |
  INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R;

uint32_t defaultHotkeyCombo(HotkeyBindingId id);
uint32_t sanitizeHotkeyCombo(HotkeyBindingId id, uint32_t combo);
void setDefaultHotkeyBindings(HotkeyBindingSet& bindings);
void sanitizeHotkeyBindings(HotkeyBindingSet& bindings);
uint32_t* hotkeyBindingSlot(HotkeyBindingSet& bindings, HotkeyBindingId id);
uint32_t hotkeyBindingValue(const HotkeyBindingSet& bindings, HotkeyBindingId id);
bool hotkeyComboMatches(uint32_t buttons, uint32_t combo);
bool hotkeyComboConflicts(const HotkeyBindingSet& bindings, HotkeyBindingId id, uint32_t combo);
bool isValidHotkeyCombo(uint32_t combo);
uint32_t singleHotkeyButton(uint32_t buttons);
uint32_t hotkeyButtonsForControllerFrame(const controller_state_t& frame);
uint16_t hotkeyHoldMsForSetting(uint8_t value);
void formatHotkeyCombo(char* buffer, size_t size, uint32_t combo, DeviceEnum mode);
bool readHotkeyBindings(HotkeyBindingSet& bindings);
void writeHotkeyBindings(const HotkeyBindingSet& bindings);
uint16_t hotkeyStorageRequiredEnd();
