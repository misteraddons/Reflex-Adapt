#pragma once

// Quick Config Menu (Mode Menu)
// Accessed via single tap of mode button
// Provides quick access to mode-specific settings for current input mode

#include <Arduino.h>
#include "../core/analog_calibration_state.h"
#include "../core/button_remap.h"
#include "../core/classic_analog_range.h"
#include "../core/classic_dual_merge_config.h"
#include "../core/controller_settings_state.h"
#include "../core/device_runtime_state.h"
#include "../core/dpad_mode.h"
#include "../core/controller_frame_state.h"
#include "../core/firmware_support.h"
#include "menu_input_mode.h"
#include "menu_input.h"
#include "menu_mode_state.h"
#include "menu_runtime_state.h"
#include "menu_working_state.h"
#include "../output/output_mode.h"
#include "../output/output_runtime_state.h"
#include "../core/turbo.h"
#include "../input/runtime/input_adapter_runtime.h"
#include "../input/snes/input_snes_runtime_state.h"
#include "../core/settings_store.h"

// Quick config menu states
enum QuickConfigState : uint8_t {
  QC_CLOSED = 0,
  QC_MAIN_MENU,       // Main mode menu with categories
  QC_CONFIRM_DEFAULT, // Confirm staging current mode defaults
  QC_TURBO_LIST,      // List of buttons for turbo config
  QC_TURBO_RATE,      // Set rate for selected button
  QC_SETTING_EDIT,    // Editing a setting value
  QC_REMAP_LIST,      // Button remap list
  QC_REMAP_SELECT,    // Selecting remap target
  QC_RANGE_TEST,      // Analog range test display
  QC_GUNCON_ADJUST,   // Guncon offset adjustment
  QC_ANALOG_SUBMENU,  // Analog stick/trigger settings submenu
  QC_DUAL_MERGE_MAP,  // Classic2USB 2-player merge ownership map
  QC_RUMBLE_SUBMENU,  // Rumble level and motor test submenu
  QC_STICK_CAL,       // Analog stick calibration
};

// Mode menu items - all possible items
enum QuickConfigItem : uint8_t {
  QCI_INPUT_MODE = 0,
  QCI_OUTPUT_MODE,
  QCI_WIN_OUTPUT,
  QCI_ANALOG_MENU,
  QCI_DEADZONE,
  QCI_RANGE_TEST,
  QCI_DPAD_BUTTONS,
  QCI_SOCD,
  QCI_STICK_INVERT,
  QCI_RUMBLETECH,
  QCI_RUMBLE,
  QCI_TRIGGERS,
  QCI_SWITCH_RTRIG_STICK,
  QCI_REMAP,
  QCI_BTN_MAP,       // Button mapping: Name (A→A) or Position (layout swap)
  QCI_N64_Z,         // Z button policy: N64 L1/L2, GameCube R1/Back
  QCI_N64_CSTICK,    // N64 C buttons: Auto, Buttons, or Stick
  QCI_N64_RANGE,     // Classic analog range: Raw, Norm, Cal, or Learn
  QCI_NSO_SPECIAL,   // NSO special (Switch Pro output only)
  QCI_POWERPAD,      // NES Power Pad mode
  QCI_CLASSIC_DUAL_MERGE, // Classic2USB: merge P1 movement + P2 action buttons
  QCI_GUNCON_OFFSET,
  QCI_TURBO,
  QCI_WHEEL_SENS,
  QCI_JOGCON_DIGITAL,
  QCI_JOGCON_WHEEL_AXIS,
  QCI_SPINNER_SPEED,
  QCI_STICK_CAL,     // Analog stick calibration
  QCI_DISCARD_EXIT,
  QCI_EXIT,
  QCI_COUNT
};

struct QuickConfigSnapshot {
  uint8_t state;
  uint8_t cursor;
  uint8_t top;
  uint8_t item;
  uint8_t count;
  uint8_t flags;
};

enum RumbleMenuItem : uint8_t {
  QCI_RUMBLE_LEVEL = 0,
  QCI_RUMBLE_HEAVY,       // reserved; heavy strength follows Level
  QCI_RUMBLE_LIGHT,       // reserved; light strength follows Level
  QCI_RUMBLE_TEST_HEAVY,
  QCI_RUMBLE_TEST_LIGHT,
  QCI_RUMBLE_TEST_BOTH,
  QCI_RUMBLE_BACK,
  QCI_RUMBLE_COUNT
};

class QuickConfigMenu {
private:
  QuickConfigState state;
  uint8_t selected_index;    // Index in visible items
  uint8_t turbo_index;       // Index in turbo button list
  uint8_t remap_index;       // Index in remap list
  uint8_t analog_index;      // Index in analog submenu
  uint8_t dual_merge_index;  // Index in 2P merge ownership list
  uint8_t rumble_index;      // Index in rumble submenu
  bool needs_redraw;
  QuickConfigItem editing_item;

  // Visible items for current mode
  QuickConfigItem visible_items[QCI_COUNT];
  uint8_t visible_count;
  QuickConfigItem analog_items[QCI_COUNT];
  uint8_t analog_count;

  // Temporary values
  TurboRate temp_rates[TURBO_BTN_COUNT];
  DeviceEnum temp_input_mode;
  outputMode_t temp_output_mode;
  uint8_t temp_win_output;
  uint8_t temp_deadzone;
  socdMode_t temp_socd;
  uint8_t temp_dpad_mode;  // 0=DPad, 1=Left Stick, 2=Right Stick, 3=Buttons
  uint8_t temp_stick_invert;
  uint8_t temp_snes_rumbletech;
  uint8_t temp_rumble;
  uint8_t temp_triggers;
  int8_t temp_guncon_x;
  int8_t temp_guncon_y;
  uint8_t temp_spinner;      // 0=0.25x, 1=0.5x, 2=1x, 3=2x, 4=4x
  uint8_t temp_wheel_sens;   // 0=Fine (0.5x), 1=Normal (1x), 2=Coarse (2x)
  uint8_t temp_jogcon_digital; // 0=L3/R3, 1=Left/Right, 2=L1/R1, 3=Up/Down
  uint8_t temp_jogcon_wheel_axis; // 0=X, 1=Y
  uint8_t temp_btn_map;      // 0=Name (A→A), 1=Position (layout swap)
  uint8_t temp_n64_z;        // N64: 0=L1, 1=L2; GC: 0=R1, 1=Back
  uint8_t temp_n64_cstick;   // 0=Auto, 1=Buttons, 2=Stick
  uint8_t temp_n64_range;    // 0=Raw, 1=Normalized, 2=Calibrated, 3=Learn
  uint8_t temp_wii_range;    // 0=Raw, 1=Normalized, 2=Calibrated, 3=Learn (Wii/GC)
  uint8_t temp_nso_special;  // 0=Off, 1=On
  uint8_t temp_powerpad;     // 0=Off, 1=On
  uint8_t temp_classic_dual_merge; // 0=Off, 1=P1 dpad/start/select + P2 face/shoulders
  uint32_t temp_classic_dual_merge_p2_mask;
  uint8_t guncon_edit_axis;  // 0=X, 1=Y, 2=Done
  bool should_save;          // True if user selected Save, false if Cancel
  uint8_t turbo_rate_selection;  // Selection in turbo rate screen (0-N = rates, N+1 = Back)
  bool range_test_exit_selected;  // True if [Back] is selected in range test
  int16_t range_last_lx, range_last_ly, range_last_rx, range_last_ry;  // Last values for flicker prevention
  int8_t range_n64_min_x, range_n64_max_x;
  int8_t range_n64_min_y, range_n64_max_y;
  bool range_n64_has_data;
  bool on_bottom_row;             // True if selection is on bottom row
  bool bottom_right;              // Legacy mirror: true when [Save] is selected
  uint8_t bottom_index;           // 0=[Cancel], 1=[Default], 2=[Save]
  uint8_t default_confirm_index;  // 0=[Cancel], 1=[Default]
  bool return_to_analog_submenu;

  // Stick calibration state
  int8_t cal_lx_min, cal_lx_max;  // Tracked min/max during calibration
  int8_t cal_ly_min, cal_ly_max;
  int8_t cal_rx_min, cal_rx_max;
  int8_t cal_ry_min, cal_ry_max;
  uint8_t cal_selection;          // 0=Cancel, 1=Save, 2=Clear
  bool cal_has_data;              // True if any movement recorded

  // Check if mode has analog sticks
  bool hasAnalogSticks(DeviceEnum mode);

  // Check if mode has analog triggers
  bool hasAnalogTriggers(DeviceEnum mode);

  // Check if mode supports rumble
  bool hasRumble(DeviceEnum mode);
  bool isSnesMode(DeviceEnum mode);

  // Check if mode can generate relative mouse delta input
  // Check if mode is Guncon
  bool isGuncon(DeviceEnum mode);

  // Check if mode is JogCon
  bool isJogcon(DeviceEnum mode);

  // Check if mode has a spinner/rotary encoder
  bool isDrivingFallbackActive(DeviceEnum mode);
  bool hasSpinner(DeviceEnum mode);

  // Check if mode is a Nintendo controller (for button map mode)
  bool isNintendoController(DeviceEnum mode);

  // Check if mode is N64
  bool isN64(DeviceEnum mode);

  // Check if mode is GameCube
  bool isGameCube(DeviceEnum mode);

  // Check if mode supports Classic stick range policy
  bool hasClassicAnalogRange(DeviceEnum mode);
  uint8_t getTempClassicAnalogRange();
  void cycleTempClassicAnalogRange(bool forward);

  // Check if mode has right analog stick
  bool hasRightStick(DeviceEnum mode);
  uint8_t getDpadModeMax(outputMode_t mode);
  bool isTempDpadModeAllowed(uint8_t mode);
  void cycleTempDpadMode(bool forward);
  uint8_t getStickInvertMax(DeviceEnum mode);
  uint8_t getTriggerModeMax(outputMode_t mode);
  bool isTempTriggerModeAllowed(uint8_t mode);
  bool supportsTempTriggerRightStick();
  void cycleTempTriggerMode(bool forward);
  bool isTurboConfigIndexVisible(uint8_t configIndex);
  uint8_t getVisibleTurboCount();
  uint8_t getTurboConfigIndexForVisibleIndex(uint8_t visibleIndex);
  uint8_t getTurboButtonIndexForVisibleIndex(uint8_t visibleIndex);
  const char* getTurboButtonNameForVisibleIndex(uint8_t visibleIndex);
  uint8_t getDualMergeMenuCount();
  void toggleDualMergeCurrentItem();
  uint8_t rumbleMotorCount();
  bool isRumbleMenuItemVisible(RumbleMenuItem item);
  void advanceRumbleIndex(bool forward);
  void startTempRumbleTest(bool heavy, bool light);

  // Get number of analog sticks for mode
  // Returns 0, 1, or 2
  uint8_t getStickCount(DeviceEnum mode);
  bool canEditModeSpecificSettings();
  bool isLiveInputSelection();
  const TurboButtonConfig& getSelectedTurboConfig();
  void loadTempQuickSettingsRecord(DeviceEnum mode, const PerModeQuickSettings& settings);
  void loadTempQuickSettingsForMode(DeviceEnum mode);
  void resetTempQuickSettingsToDefaults();
  bool isNsoSpecialActiveForSelection(DeviceEnum mode);

  // Build list of visible items based on current mode
  void buildVisibleItems();
  void buildAnalogItems();
  bool renderMarqueeTick(SSD1306AsciiWire& display);
  void addVisibleItem(QuickConfigItem item) {
    if (visible_count < QCI_COUNT) {
      visible_items[visible_count++] = item;
    }
  }
  void addAnalogItem(QuickConfigItem item) {
    if (analog_count < QCI_COUNT) {
      analog_items[analog_count++] = item;
    }
  }
  void clampVisibleSelection() {
    if (visible_count == 0) {
      selected_index = 0;
      on_bottom_row = true;
      bottom_right = false;
      bottom_index = 0;
    } else if (selected_index >= visible_count) {
      selected_index = visible_count - 1;
    }
  }

  const char* getItemName(QuickConfigItem item);
  const char* getItemValue(QuickConfigItem item);

  void setTempRumbleTech(bool enabled);
  void toggleTempRumbleTech() {
    setTempRumbleTech(temp_snes_rumbletech == 0);
  }

  bool isSubmenu(QuickConfigItem item) {
    return item == QCI_TURBO || item == QCI_REMAP || item == QCI_RANGE_TEST ||
           item == QCI_GUNCON_OFFSET ||
           item == QCI_ANALOG_MENU || item == QCI_CLASSIC_DUAL_MERGE ||
           item == QCI_RUMBLE || item == QCI_STICK_CAL;
  }

  void rebuildVisibleAndClampSelection() {
    buildAnalogItems();
    buildVisibleItems();
    clampVisibleSelection();
    // If selected option becomes unsupported, clamp values to valid ranges.
    if (!isTempDpadModeAllowed(temp_dpad_mode)) temp_dpad_mode = DPAD_MODE_DPAD;
    uint8_t invert_max = getStickInvertMax(temp_input_mode);
    if (temp_stick_invert > invert_max) temp_stick_invert = invert_max;
    uint8_t trigger_max = getTriggerModeMax(temp_output_mode);
    if (temp_triggers > trigger_max || !isTempTriggerModeAllowed(temp_triggers)) {
      temp_triggers = TRIGGER_MODE_ANALOG;
    }
  }

public:
  static constexpr DeviceEnum defaultInputMode() {
#ifdef ENABLE_INPUT_SNES
    return RZORD_SNES;
#elif defined(ENABLE_INPUT_JVS)
    return RZORD_JVS;
#elif defined(ENABLE_INPUT_USB)
    return RZORD_USB;
#elif defined(ENABLE_INPUT_SATURN)
    return RZORD_SATURN;
#else
    return RZORD_NONE;
#endif
  }

  QuickConfigMenu()
    : state(QC_CLOSED), selected_index(0), turbo_index(0), remap_index(0),
      analog_index(0), dual_merge_index(0), rumble_index(0),
      needs_redraw(true), visible_count(0),
      analog_count(0),
      temp_input_mode(defaultInputMode()), temp_output_mode(OUTPUT_MISTER),
      temp_win_output(0), temp_snes_rumbletech(0), temp_rumble(3),
      temp_classic_dual_merge_p2_mask(0),
      guncon_edit_axis(0), should_save(false), range_test_exit_selected(false),
      range_last_lx(0), range_last_ly(0), range_last_rx(0), range_last_ry(0),
      range_n64_min_x(0), range_n64_max_x(0), range_n64_min_y(0), range_n64_max_y(0),
      range_n64_has_data(false),
      on_bottom_row(false), bottom_right(false), bottom_index(0),
      default_confirm_index(0), return_to_analog_submenu(false),
      cal_lx_min(0), cal_lx_max(0), cal_ly_min(0), cal_ly_max(0),
      cal_rx_min(0), cal_rx_max(0), cal_ry_min(0), cal_ry_max(0),
      cal_selection(0), cal_has_data(false) {
    for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
      temp_rates[i] = TURBO_OFF;
    }
  }

  bool isOpen() {
    return state != QC_CLOSED;
  }

  void open();

  void close() {
    state = QC_CLOSED;
    needs_redraw = true;
    // Note: should_save is NOT set here - it's set by applyAndClose() before calling close()
    // Only set to false explicitly when canceling
  }

  void discard() {
    should_save = false;  // Cancel - don't save
    close();
  }

  bool shouldSave() {
    return should_save;
  }

  void applyAndClose();
  bool saveShortcut();

  // Get temp rates/settings for saving
  void getTempRates(uint8_t* rates);
  void getTempQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings);
  uint8_t debugBuildVisibleItems(DeviceEnum inputMode, outputMode_t outputMode,
                                 uint8_t winOutput, QuickConfigItem* items,
                                 uint8_t maxItems);
  void getSnapshot(QuickConfigSnapshot& snapshot);

  // Get temp settings for saving
  DeviceEnum getTempInputMode() { return temp_input_mode; }
  outputMode_t getTempOutputMode() { return temp_output_mode; }
  uint8_t getTempDeadzone() { return temp_deadzone * 5; }
  socdMode_t getTempSocd() { return temp_socd; }
  uint8_t getTempDpadMode() { return temp_dpad_mode; }
  uint8_t getTempStickInvert() { return temp_stick_invert; }
  uint8_t getTempRumble() { return temp_rumble; }
  uint8_t getTempTriggers() { return temp_triggers; }
  int8_t getTempGunconX() { return temp_guncon_x; }
  int8_t getTempGunconY() { return temp_guncon_y; }
  uint8_t getTempBtnMap() { return temp_btn_map; }
  uint8_t getTempN64Z() { return temp_n64_z; }
  uint8_t getTempN64CStickMode() { return temp_n64_cstick; }
  uint8_t getTempN64AnalogRange() { return temp_n64_range; }

  // Handle navigation down (reset/boot button or D-Pad Down)
  void navigate() {
    navigateDown();
  }

  void navigateDown();

  // Handle navigation up (D-Pad Up)
  void navigateUp();

  // Cycle value forward (D-Pad Right) or activate/select in submenus
  // isSelectAction = true when triggered by mode/face button (should execute on bottom row)
  // isSelectAction = false when triggered by right dpad (should toggle on bottom row)
  void cycleValueNext(bool isSelectAction = false);

  // Cycle value backward (D-Pad Left) or go back in submenus
  void cycleValuePrev();

  // Check if item is directly editable with left/right (no submenu needed)
  bool isDirectlyEditable(QuickConfigItem item);

  // Check if item needs a submenu (entered with left/right or select)
  bool needsSubmenu(QuickConfigItem item);

  // Enter submenu for current item (called by left/right for submenu items)
  void enterSubmenu(QuickConfigItem item);

  // Cycle current main menu item value forward (or activate exit items)
  void cycleMainMenuItemNext();

  // Cycle current main menu item value backward (or activate exit items)
  void cycleMainMenuItemPrev();

  void cycleSettingValue();
  void cycleSettingValueBack();

  // Back/cancel action (B button semantics)
  // Main menu: cancel and close
  // Submenus: return to main menu without closing
  void back();

  // Handle selection (mode button)
  void select();
  void handleAnalogSelect();
  void handleAnalogSelectBack();
  void handleDualMergeSelect();
  void handleDualMergeSelectBack();
  void handleRumbleSelect(bool forward = true);
  void handleGunconSelect();
  void handleGunconNavigate();
  void handleGunconNavigateBack();

  // Render the menu (called when display is available)
  void render(SSD1306AsciiWire& display);

  // Force redraw on next render (for live updates like range test)
  void forceRedraw() {
    needs_redraw = true;
  }

private:
  void renderMainMenu(SSD1306AsciiWire& display);
  void renderDefaultConfirm(SSD1306AsciiWire& display);
  void renderTurboList(SSD1306AsciiWire& display);
  void renderTurboRate(SSD1306AsciiWire& display);
  void renderSettingEdit(SSD1306AsciiWire& display);
  void renderAnalogSubmenu(SSD1306AsciiWire& display);
  void renderDualMergeMap(SSD1306AsciiWire& display);
  void renderRumbleSubmenu(SSD1306AsciiWire& display);
  void renderGunconAdjust(SSD1306AsciiWire& display);
  bool isWheelController();
  void renderRangeTest(SSD1306AsciiWire& display);
  void renderStickCal(SSD1306AsciiWire& display);
  void renderRemapList(SSD1306AsciiWire& display);
};

// Global quick config menu instance
extern QuickConfigMenu quickConfig;

uint8_t quickConfigItemSettingIds(QuickConfigItem item, DeviceEnum mode,
                                  SettingId* ids, uint8_t maxIds);
