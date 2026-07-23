#pragma once

// Button Remapping System
// Per-input-mode button remapping with hybrid UI:
// - Left side: Scrollable text list of mappings
// - Right side: Static pad display
// - Bottom row: Cancel, Clear, and Save options

#include <Arduino.h>
#include <EEPROM.h>

#include "controller_frame_state.h"
#include "device_runtime_state.h"
#include "controller_state.h"
#include <SSD1306Ascii/SSD1306AsciiWire.h>

extern SSD1306AsciiWire display;

#define REMAP_MAX_BUTTONS 19

extern const char* const remap_button_names[REMAP_MAX_BUTTONS];

bool psxHasL3R3();

#define BTN_BIT_A      (1UL << 0)
#define BTN_BIT_B      (1UL << 1)
#define BTN_BIT_X      (1UL << 2)
#define BTN_BIT_Y      (1UL << 3)
#define BTN_BIT_L1     (1UL << 4)
#define BTN_BIT_R1     (1UL << 5)
#define BTN_BIT_L2     (1UL << 6)
#define BTN_BIT_R2     (1UL << 7)
#define BTN_BIT_L3     (1UL << 8)
#define BTN_BIT_R3     (1UL << 9)
#define BTN_BIT_START  (1UL << 10)
#define BTN_BIT_SELECT (1UL << 11)
#define BTN_BIT_HOME   (1UL << 12)
#define BTN_BIT_CAPTURE (1UL << 13)
#define BTN_BIT_EXTRA0 (1UL << 14)
#define BTN_BIT_EXTRA1 (1UL << 15)
#define BTN_BIT_EXTRA2 (1UL << 16)
#define BTN_BIT_EXTRA3 (1UL << 17)
#define BTN_BIT_EXTRA4 (1UL << 18)

uint32_t getInputButtonMask(DeviceEnum mode);
uint8_t getRemapButtonCount(DeviceEnum mode);
uint8_t getRemapButtonSlot(DeviceEnum mode, uint8_t display_index);
uint8_t getRemapButtonDisplayIndex(DeviceEnum mode, uint8_t button_slot);
const char* getRemapButtonName(DeviceEnum mode, uint8_t index);
const char* getRemapButtonNameForSlot(DeviceEnum mode, uint8_t button_slot);

enum RemapMenuState : uint8_t {
  REMAP_CLOSED = 0,
  REMAP_NAVIGATE,
  REMAP_EDIT_SOURCE,
};

extern uint8_t active_remaps[REMAP_MAX_BUTTONS];

class ButtonRemapMenu {
private:
  RemapMenuState state;
  uint8_t selected_index;
  uint8_t scroll_offset;
  uint8_t button_count;
  bool needs_redraw;
  uint8_t temp_remap[REMAP_MAX_BUTTONS];
  uint8_t edit_source_index;
  DeviceEnum active_mode;

  static const uint8_t LIST_ROWS = 6;
  static const uint8_t PAD_START_COL = 66;
  static const uint8_t BOTTOM_ROW = 7;

  void renderButtonList(DeviceEnum mode);
  void renderPadDisplay(DeviceEnum mode);
  void renderBottomRow();

public:
  ButtonRemapMenu();

  bool isOpen();
  bool isEditMode();
  void open(DeviceEnum mode);
  void close();
  void loadRemaps(const uint8_t* data);
  void getRemaps(uint8_t* data);
  void clearRemaps();
  void navigate();
  uint8_t select();
  void setDestination(uint8_t button_index);
  uint8_t getFirstPressedButton();
  static void applyRemap(controller_state_t& report, const uint8_t* remap);
  void render(DeviceEnum mode);
};

extern ButtonRemapMenu remapMenu;
extern uint16_t g_eeprom_remap_index;

bool hasActiveRemap();
void loadRemapsFromEEPROM();
void writeRemapsToEEPROM();
void saveRemapsToEEPROM();
void clearRemapsToDefault();
