#include "../product_config.h"

#include "menu_helpers.h"

#include <cstring>

#include "../core/controller_frame_state.h"
#include "../core/device_runtime_state.h"
#include "../features/feature_module.h"
#include "../output/output_runtime_state.h"
#include "menu_catalog.h"
#include "menu_capabilities.h"
#include "menu_mode_state.h"
#include "menu_runtime_state.h"
#include "menu_ui_state.h"
#include "menu_working_state.h"

namespace {

bool menuInputIsPsx() {
#ifdef ENABLE_INPUT_PSX
  return menu_input == RZORD_PSX;
#else
  return false;
#endif
}

bool menuInputIsNes() {
#ifdef ENABLE_INPUT_NES
  return menu_input == RZORD_NES;
#else
  return false;
#endif
}

bool menuInputIsSnes() {
#ifdef ENABLE_INPUT_SNES
  return menu_input == RZORD_SNES;
#else
  return false;
#endif
}

bool menuInputIsN64() {
#ifdef ENABLE_INPUT_N64
  return menu_input == RZORD_N64;
#else
  return false;
#endif
}

bool menuInputIsGameCube() {
#ifdef ENABLE_INPUT_GAMECUBE
  return menu_input == RZORD_GAMECUBE;
#else
  return false;
#endif
}

bool menuInputIsWii() {
#ifdef ENABLE_INPUT_WII
  return menu_input == RZORD_WII;
#else
  return false;
#endif
}

bool menuInputSupportsButtonMapMode() {
  return menuInputIsSnes() || menuInputIsNes() || menuInputIsN64() ||
         menuInputIsGameCube() || menuInputIsWii();
}

}  // namespace

bool should_hide_menu_item(menu_item_enum item) {
  bool featureHidden = false;
  if (featureModulesShouldHideMenuItem(item, &featureHidden)) {
    return featureHidden;
  }

  if (system_menu_only &&
      (item == menu_item_input_mode ||
       item == menu_item_output_mode ||
       is_mode_specific_setting(item))) {
    return true;
  }

  if (menu_input != deviceMode && is_mode_specific_setting(item)) {
    return true;
  }

  if (item == menu_item_home_button_labels) {
    return true;
  }

  switch (item) {
    case menu_item_home_screen:
      return true;
    case menu_item_hotkey_hold_time:
    case menu_item_menu_hotkey:
    case menu_item_system_menu_hotkey:
    case menu_item_home_hotkey:
    case menu_item_capture_hotkey:
      return true;
#if !defined(ENABLE_BLUETOOTH_PAIRING)
    case menu_item_bluetooth_pairing:
    case menu_item_bluetooth_forget:
      return true;
#endif
#if !defined(ADAPT_PRIMARY_EGRESS_USB_HOST)
    case menu_item_home_jvs_view:
      return true;
#endif
    case menu_item_nso_special:
      return menu_output != OUTPUT_SWITCHPRO;
    case menu_item_analog_deadzone:
    case menu_item_analog_test:
    case menu_item_stick_invert:
      return !input_has_analog();
    case menu_item_trigger_mode:
      return !input_has_analog_triggers() || menu_output == OUTPUT_SWITCHPRO;
    case menu_item_switch_rtrig_stick:
      return !(menu_output == OUTPUT_SWITCHPRO &&
               input_has_analog_triggers() &&
               menuModeSupportsTriggerAxisMode(menu_input));
    case menu_item_guncon_offset:
      return !menuInputIsPsx();
    case menu_item_powerpad:
      return !menuInputIsNes();
    case menu_item_jogcon_force:
      if (menuInputIsPsx()) return false;
      if (strncmp(controllerFrameConst(0).controller_type_name, "JogCon", 6) == 0) return false;
      if (outputMode == OUTPUT_MISTER_JOGCON) return false;
      return true;
    case menu_item_jogcon_mode:
      return true;
    case menu_item_wheel_sensitivity:
      if (strncmp(controllerFrameConst(0).controller_type_name, "JogCon", 6) == 0) return false;
      if (outputMode == OUTPUT_MISTER_JOGCON) return false;
      return true;
    case menu_item_jogcon_digital:
    case menu_item_jogcon_wheel_axis:
      if (menuInputIsPsx()) return false;
      if (strncmp(controllerFrameConst(0).controller_type_name, "JogCon", 6) == 0) return false;
      if (outputMode == OUTPUT_MISTER_JOGCON) return false;
      return true;
    case menu_item_button_map:
      if (is_nso_special_active()) return true;
      return !menuInputSupportsButtonMapMode();
    case menu_item_n64_z_mode:
      if (is_nso_special_active()) return true;
      return !(menuInputIsN64() || menuInputIsGameCube());
    case menu_item_n64_cstick_mode:
      if (is_nso_special_active()) return true;
      return !menuInputIsN64();
    case menu_item_wii_analog_range:
      return !(menuInputIsWii() || menuInputIsGameCube());
    case menu_item_n64_analog_range:
      return true;
    case menu_item_latency_test:
    #if defined(ADAPT_ENABLE_LATENCY_TEST)
      return false;
    #else
      return true;
    #endif
    case menu_item_latency_controller:
    case menu_item_latency_host:
    case menu_item_latency_run:
    #if defined(ADAPT_ENABLE_LATENCY_TEST)
      return menu_latency_test == 0;
    #else
      return true;
    #endif
    case menu_item_win_output:
      return false;
    case menu_item_psx_periph:
      return true;
    case menu_item_socd:
#ifdef PRODUCT_CLASSIC2USB
      return true;
#else
      return false;
#endif
    case menu_item_dpad_as_buttons:
      if (is_nso_special_active()) return true;
      return false;
    case menu_item_rumble:
      return false;
    case menu_item_hotkeys:
    case menu_item_kiosk_mode:
      return false;
    default:
      return false;
  }
}

uint8_t getVisibleItemCount() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MENU_TOTAL_ITEMS; ++i) {
    if (!should_hide_menu_item(menu_items[i].item))
      ++count;
  }
  return count;
}

uint8_t getActualIndex(uint8_t visible_index) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MENU_TOTAL_ITEMS; ++i) {
    if (!should_hide_menu_item(menu_items[i].item)) {
      if (count == visible_index)
        return i;
      ++count;
    }
  }
  return 0;
}

uint8_t getVisibleIndex(uint8_t actual_index) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < actual_index; ++i) {
    if (!should_hide_menu_item(menu_items[i].item))
      ++count;
  }
  return count;
}

bool is_mode_specific_setting(menu_item_enum item) {
  switch (item) {
    case menu_item_analog_deadzone:
    case menu_item_analog_test:
    case menu_item_dpad_as_buttons:
    case menu_item_socd:
    case menu_item_nso_special:
    case menu_item_stick_invert:
    case menu_item_rumble:
    case menu_item_trigger_mode:
    case menu_item_switch_rtrig_stick:
    case menu_item_button_remap:
    case menu_item_mapping_view:
    case menu_item_guncon_offset:
    case menu_item_powerpad:
    case menu_item_jogcon_force:
    case menu_item_wheel_sensitivity:
    case menu_item_jogcon_digital:
    case menu_item_jogcon_wheel_axis:
    case menu_item_button_map:
    case menu_item_n64_z_mode:
    case menu_item_n64_cstick_mode:
    case menu_item_wii_analog_range:
      return true;
    default:
      return false;
  }
}

bool is_per_input_quick_setting(menu_item_enum item) {
  switch (item) {
    case menu_item_analog_deadzone:
    case menu_item_dpad_as_buttons:
    case menu_item_socd:
    case menu_item_nso_special:
    case menu_item_stick_invert:
    case menu_item_rumble:
    case menu_item_trigger_mode:
    case menu_item_switch_rtrig_stick:
    case menu_item_button_map:
    case menu_item_n64_z_mode:
    case menu_item_n64_cstick_mode:
    case menu_item_wii_analog_range:
    case menu_item_guncon_offset:
    case menu_item_powerpad:
    case menu_item_jogcon_force:
    case menu_item_wheel_sensitivity:
    case menu_item_jogcon_digital:
    case menu_item_jogcon_wheel_axis:
      return true;
    default:
      return false;
  }
}

bool is_system_setting(menu_item_enum item) {
  switch (item) {
    case menu_item_display_contrast:
    case menu_item_home_screen:
    case menu_item_home_button_labels:
    case menu_item_home_jvs_view:
    case menu_item_hotkey_hold_time:
    case menu_item_buzzer:
    case menu_item_latency_test:
    case menu_item_latency_controller:
    case menu_item_latency_host:
    case menu_item_screensaver:
    case menu_item_win_output:
    case menu_item_psx_periph:
    case menu_item_hotkeys:
    case menu_item_kiosk_mode:
    case menu_item_menu_hotkey:
    case menu_item_system_menu_hotkey:
    case menu_item_home_hotkey:
    case menu_item_capture_hotkey:
    case menu_item_bluetooth_pairing:
    case menu_item_bluetooth_forget:
      return true;
    default:
      return false;
  }
}
