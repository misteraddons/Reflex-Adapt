#pragma once

// Menu Item Descriptor Table
// Data-driven approach to replace repetitive handler functions
// ~21 handlers converted to descriptors, ~12 kept as custom handlers

#include "menu_action.h"
#include "menu_item_defs.h"
#include "menu_runtime_state.h"
#include "menu_working_state.h"
#include "../core/controller_settings_state.h"
#include "../output/output_runtime_state.h"
#include "../core/settings_store.h"

// Menu item types for data-driven handling
enum MenuItemType : uint8_t {
  MENU_TYPE_BOOL,      // On/Off toggle
  MENU_TYPE_ENUM,      // Cycle through named options
  MENU_TYPE_RANGE,     // Numeric with min/max/step
  MENU_TYPE_CUSTOM     // Uses custom handler function (not in table)
};

// Descriptor for data-driven menu items
struct MenuItemDescriptor {
  menu_item_enum id;
  MenuItemType type;

  // Variable pointers
  uint8_t* menu_var;           // Menu working copy
  uint8_t* runtime_var;        // Runtime variable (nullptr if N/A)
  SettingId setting_id;        // Typed persisted setting id

  // Type-specific config
  union {
    struct {  // MENU_TYPE_BOOL
      const char* off_text;    // "Off", "No", "Analog", "Left"
      const char* on_text;     // "On", "Yes", "Digital", "Right"
    } bool_cfg;

    struct {  // MENU_TYPE_ENUM
      const char* const* names;  // Array of option names
      uint8_t count;             // Number of options
      uint8_t (*get_max)();      // Dynamic max (nullptr = use count-1)
    } enum_cfg;

    struct {  // MENU_TYPE_RANGE
      uint8_t min_val;
      uint8_t max_val;
      uint8_t step;
      const char* suffix;      // "%", "", etc.
    } range_cfg;
  };

  // Optional callbacks
  void (*on_change)();         // Called after value change (e.g., immediate preview)
  void (*on_apply)();          // Called after apply (e.g., display.setContrast)
};

void latency_test_apply();
void apply_latency_menu_configuration();
uint8_t dpad_get_max();
uint8_t stick_invert_get_max();
uint8_t trigger_mode_get_max();

extern const char* const socd_names[];
extern const char* const dpad_mode_names[];
extern const char* const stick_invert_names[];
extern const char* const trigger_mode_names[];
extern const char* const rumble_names[];
extern const char* const jogcon_mode_names[];
extern const char* const jogcon_digital_names[];
extern const char* const jogcon_wheel_axis_names[];
extern const char* const latency_host_names[];
extern const char* const win_output_names[];
extern const char* const psx_periph_names[];
extern const char* const n64_cstick_mode_names[];
extern const char* const hotkey_hold_time_names[];
extern const MenuItemDescriptor menu_descriptors[];
extern const uint8_t MENU_DESCRIPTOR_COUNT;

const MenuItemDescriptor* get_menu_descriptor(menu_item_enum item);
void menu_item_handle_generic(const MenuItemDescriptor& desc, menu_item_action action);
