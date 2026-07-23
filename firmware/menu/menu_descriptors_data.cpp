#include "../product_config.h"

#include "menu_descriptors.h"
#include "../core/settings_registry.h"

const char* const socd_names[] = { "OFF", "NEUTRAL", "SECOND", "FIRST" };
const char* const dpad_mode_names[] = { "DPad", "Left Stick", "Right Stick", "Buttons" };
const char* const stick_invert_names[] = { "Off", "Left", "Right", "Both" };
const char* const trigger_mode_names[] = { "Analog", "Digital", "RStick", "Both" };
const char* const rumble_names[] = { "Off", "Low", "Medium", "High" };
const char* const jogcon_mode_names[] = { "Spinner", "Paddle", "Steering", "Digital" };
const char* const jogcon_digital_names[] = { "L3/R3", "Left/Right", "L/R", "Up/Down" };
const char* const jogcon_wheel_axis_names[] = { "X", "Y" };
const char* const latency_host_names[] = { "Internal", "PC", "MiSTer" };
const char* const win_output_names[] = { "DInput", "XInput", "Keyboard" };
const char* const psx_periph_names[] = { "MiSTer" };
const char* const n64_cstick_mode_names[] = { "Auto", "Buttons", "Stick" };
const char* const classic_analog_range_names[] = { "Raw", "Norm", "Cal" };
const char* const home_jvs_view_names[] = { "Single", "In/Out" };
const char* const hotkey_hold_time_names[] = { "Press", "0.5s", "1.0s", "1.5s", "2.0s", "2.5s", "3.0s" };

const MenuItemDescriptor menu_descriptors[] = {
  {
    .id = menu_item_trigger_mode,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_trigger_mode,
    .runtime_var = &trigger_mode,
    .setting_id = SettingId::TriggerMode,
    .enum_cfg = { trigger_mode_names, 4, trigger_mode_get_max },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_nso_special,
    .type = MENU_TYPE_BOOL,
    .menu_var = (uint8_t*)&menu_nso_special,
    .runtime_var = (uint8_t*)&nso_special,
    .setting_id = SettingId::NsoSpecial,
    .bool_cfg = { "No", "Yes" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_latency_test,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_latency_test,
    .runtime_var = nullptr,
    .setting_id = SettingId::LatencyTest,
    .bool_cfg = { "Off", "On" },
    .on_change = latency_test_apply,
    .on_apply = latency_test_apply
  },
  {
    .id = menu_item_latency_controller,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_latency_controller_in_loop,
    .runtime_var = nullptr,
    .setting_id = SettingId::LatencyControllerInLoop,
    .bool_cfg = { "Internal", "Controller" },
    .on_change = latency_test_apply,
    .on_apply = latency_test_apply
  },
  {
    .id = menu_item_latency_host,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_latency_host_type,
    .runtime_var = nullptr,
    .setting_id = SettingId::LatencyHostType,
    .enum_cfg = { latency_host_names, 3, nullptr },
    .on_change = latency_test_apply,
    .on_apply = latency_test_apply
  },
  {
    .id = menu_item_powerpad,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_powerpad_mode,
    .runtime_var = nullptr,
    .setting_id = SettingId::PowerpadMode,
    .bool_cfg = { "Off", "On" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_menu_hotkey,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_menu_hotkey,
    .runtime_var = nullptr,
    .setting_id = SettingId::MenuHotkey,
    .bool_cfg = { "Off", "On" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_system_menu_hotkey,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_system_menu_hotkey,
    .runtime_var = nullptr,
    .setting_id = SettingId::SystemMenuHotkey,
    .bool_cfg = { "Off", "On" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_home_hotkey,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_home_hotkey,
    .runtime_var = nullptr,
    .setting_id = SettingId::HomeHotkey,
    .bool_cfg = { "Off", "On" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_capture_hotkey,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_capture_hotkey,
    .runtime_var = nullptr,
    .setting_id = SettingId::CaptureHotkey,
    .bool_cfg = { "Off", "On" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_kiosk_mode,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_kiosk_mode,
    .runtime_var = nullptr,
    .setting_id = SettingId::KioskMode,
    .bool_cfg = { "Off", "On" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_home_screen,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_home_screen_debug,
    .runtime_var = nullptr,
    .setting_id = SettingId::HomeScreenDebug,
    .bool_cfg = { "Pad View", "Debug" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_switch_rtrig_stick,
    .type = MENU_TYPE_CUSTOM,
    .menu_var = &menu_trigger_mode,
    .runtime_var = &trigger_mode,
    .setting_id = SettingId::TriggerMode,
    .bool_cfg = { "Off", "On" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_home_button_labels,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_home_button_labels,
    .runtime_var = nullptr,
    .setting_id = SettingId::HomeButtonLabels,
    .bool_cfg = { "Round", "Name" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_home_jvs_view,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_home_jvs_view,
    .runtime_var = nullptr,
    .setting_id = SettingId::HomeJvsView,
    .enum_cfg = { home_jvs_view_names, 2, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_hotkey_hold_time,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_hotkey_hold_time,
    .runtime_var = nullptr,
    .setting_id = SettingId::HotkeyHoldTime,
    .enum_cfg = { hotkey_hold_time_names, 7, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_button_map,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_button_map,
    .runtime_var = &button_map_mode,
    .setting_id = SettingId::ButtonMapMode,
    .bool_cfg = { "Name", "Position" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_n64_z_mode,
    .type = MENU_TYPE_BOOL,
    .menu_var = &menu_n64_z_mode,
    .runtime_var = &n64_z_mode,
    .setting_id = SettingId::N64ZMode,
    .bool_cfg = { "L1", "L2" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_n64_cstick_mode,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_n64_cstick_mode,
    .runtime_var = &n64_cstick_mode,
    .setting_id = SettingId::N64CStickMode,
    .enum_cfg = { n64_cstick_mode_names, 3, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_wii_analog_range,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_wii_analog_range,
    .runtime_var = &wii_analog_range,
    .setting_id = SettingId::WiiAnalogRange,
    .enum_cfg = { classic_analog_range_names, 3, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_n64_analog_range,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_n64_analog_range,
    .runtime_var = &n64_analog_range,
    .setting_id = SettingId::N64AnalogRange,
    .enum_cfg = { classic_analog_range_names, 3, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_socd,
    .type = MENU_TYPE_ENUM,
    .menu_var = (uint8_t*)&menu_socdMode,
    .runtime_var = (uint8_t*)&socdMode,
    .setting_id = SettingId::Socd,
    .enum_cfg = { socd_names, 4, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_dpad_as_buttons,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_dpad_mode,
    .runtime_var = &dpad_mode,
    .setting_id = SettingId::DpadMode,
    .enum_cfg = { dpad_mode_names, 4, dpad_get_max },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_rumble,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_rumble_level,
    .runtime_var = &rumble_level,
    .setting_id = SettingId::RumbleLevel,
    .enum_cfg = { rumble_names, 4, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_stick_invert,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_stick_invert,
    .runtime_var = &stick_invert,
    .setting_id = SettingId::StickInvert,
    .enum_cfg = { stick_invert_names, 4, stick_invert_get_max },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_jogcon_mode,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_jogcon_mode,
    .runtime_var = nullptr,
    .setting_id = SettingId::JogconMode,
    .enum_cfg = { jogcon_mode_names, 4, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_jogcon_digital,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_jogcon_digital_map,
    .runtime_var = nullptr,
    .setting_id = SettingId::JogconDigitalMap,
    .enum_cfg = { jogcon_digital_names, 4, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_jogcon_wheel_axis,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_jogcon_wheel_axis,
    .runtime_var = nullptr,
    .setting_id = SettingId::JogconWheelAxis,
    .enum_cfg = { jogcon_wheel_axis_names, 2, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_win_output,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_win_output,
    .runtime_var = nullptr,
    .setting_id = SettingId::WinOutput,
    .enum_cfg = { win_output_names, 3, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_psx_periph,
    .type = MENU_TYPE_ENUM,
    .menu_var = &menu_psx_periph,
    .runtime_var = nullptr,
    .setting_id = SettingId::PsxPeriph,
    .enum_cfg = { psx_periph_names, 1, nullptr },
    .on_change = nullptr,
    .on_apply = nullptr
  },
  {
    .id = menu_item_analog_deadzone,
    .type = MENU_TYPE_RANGE,
    .menu_var = &menu_deadzone_percent,
    .runtime_var = &deadzone_percent,
    .setting_id = SettingId::Deadzone,
    .range_cfg = { 0, 30, 5, "%" },
    .on_change = nullptr,
    .on_apply = nullptr
  },
};

const uint8_t MENU_DESCRIPTOR_COUNT = sizeof(menu_descriptors) / sizeof(MenuItemDescriptor);
