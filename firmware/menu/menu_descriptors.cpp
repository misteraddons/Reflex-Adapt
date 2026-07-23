#include "../product_config.h"

#include "menu_descriptors.h"

#include "../core/device_runtime_state.h"
#include "../core/controller_settings_state.h"
#include "../core/dpad_mode.h"
#include "../core/settings_registry.h"
#include "../output/output_runtime_state.h"
#include "../platform/display_runtime_state.h"
#include "../platform/rgb_led.h"
#include "../platform/latency_test.h"
#include "menu_capabilities.h"
#include "menu_helpers.h"
#include "menu_mode_state.h"

namespace {

bool dpad_mode_value_allowed(uint8_t value) {
  switch (value) {
    case DPAD_MODE_DPAD:
      return true;
    case DPAD_MODE_LEFT_STICK:
    case DPAD_MODE_RIGHT_STICK:
      return true;
    case DPAD_MODE_BUTTONS:
      return output_supports_dpad_buttons(menu_output);
    default:
      return false;
  }
}

uint8_t cycle_dpad_mode_value(uint8_t current, bool forward) {
  uint8_t value = current;
  for (uint8_t attempts = 0; attempts < 4; ++attempts) {
    if (forward) {
      value = (uint8_t)((value + 1) % 4);
    } else {
      value = (value == 0) ? 3 : (uint8_t)(value - 1);
    }
    if (dpad_mode_value_allowed(value)) {
      return value;
    }
  }
  return DPAD_MODE_DPAD;
}

bool handle_dpad_mode_menu_item(const MenuItemDescriptor& desc, menu_item_action action) {
  if (desc.id != menu_item_dpad_as_buttons) {
    return false;
  }

  switch (action) {
    case item_action_display:
#ifdef USE_I2C_DISPLAY
    {
      uint8_t idx = *desc.menu_var;
      if (!dpad_mode_value_allowed(idx)) {
        idx = DPAD_MODE_DPAD;
      }
      display.print(dpad_mode_names[idx]);
    }
#endif
      return true;
    case item_action_change:
      *desc.menu_var = cycle_dpad_mode_value(*desc.menu_var, true);
      if (desc.on_change) desc.on_change();
      return true;
    case item_action_change_prev:
      *desc.menu_var = cycle_dpad_mode_value(*desc.menu_var, false);
      if (desc.on_change) desc.on_change();
      return true;
    case item_action_apply:
      if (!dpad_mode_value_allowed(*desc.menu_var)) {
        *desc.menu_var = DPAD_MODE_DPAD;
      }
      if (desc.runtime_var) {
        *desc.runtime_var = *desc.menu_var;
      }
      if (desc.on_apply) desc.on_apply();
      return true;
    default:
      return false;
  }
}

bool trigger_mode_value_allowed(uint8_t value) {
  const outputMode_t triggerOutput =
    (menu_output == OUTPUT_AUTO) ? get_effective_output_mode() : menu_output;
  switch (value) {
    case TRIGGER_MODE_ANALOG:
    case TRIGGER_MODE_DIGITAL:
      return input_has_analog_triggers();
    case TRIGGER_MODE_BOTH:
      return input_has_analog_triggers() &&
             outputModeSupportsTriggerBothMode(triggerOutput) &&
             menuModeSupportsTriggerBothMode(menu_input);
    default:
      return false;
  }
}

uint8_t cycle_trigger_mode_value(uint8_t current, bool forward) {
  uint8_t value = current;
  for (uint8_t attempts = 0; attempts < 4; ++attempts) {
    if (forward) {
      value = (uint8_t)((value + 1) % 4);
    } else {
      value = (value == 0) ? 3 : (uint8_t)(value - 1);
    }
    if (value == TRIGGER_MODE_RSTICK) {
      continue;
    }
    if (trigger_mode_value_allowed(value)) {
      return value;
    }
  }
  return TRIGGER_MODE_ANALOG;
}

bool handle_trigger_mode_menu_item(const MenuItemDescriptor& desc, menu_item_action action) {
  if (desc.id != menu_item_trigger_mode) {
    return false;
  }

  switch (action) {
    case item_action_display:
#ifdef USE_I2C_DISPLAY
    {
      uint8_t idx = *desc.menu_var;
      if (!trigger_mode_value_allowed(idx)) {
        idx = TRIGGER_MODE_ANALOG;
      }
      display.print(trigger_mode_names[idx]);
    }
#endif
      return true;
    case item_action_change:
      *desc.menu_var = cycle_trigger_mode_value(*desc.menu_var, true);
      if (desc.on_change) desc.on_change();
      return true;
    case item_action_change_prev:
      *desc.menu_var = cycle_trigger_mode_value(*desc.menu_var, false);
      if (desc.on_change) desc.on_change();
      return true;
    default:
      return false;
  }
}

bool switch_rtrig_stick_allowed() {
  return menu_output == OUTPUT_SWITCHPRO &&
         input_has_analog_triggers() &&
         menuModeSupportsTriggerAxisMode(menu_input);
}

bool handle_switch_rtrig_stick_menu_item(const MenuItemDescriptor& desc, menu_item_action action) {
  if (desc.id != menu_item_switch_rtrig_stick) {
    return false;
  }

  switch (action) {
    case item_action_display:
#ifdef USE_I2C_DISPLAY
      display.print(*desc.menu_var == TRIGGER_MODE_RSTICK ? F("On") : F("Off"));
#endif
      return true;
    case item_action_change:
    case item_action_change_prev:
      *desc.menu_var = (*desc.menu_var == TRIGGER_MODE_RSTICK)
                         ? TRIGGER_MODE_ANALOG
                         : TRIGGER_MODE_RSTICK;
      if (desc.on_change) desc.on_change();
      return true;
    case item_action_apply:
      if (!switch_rtrig_stick_allowed() && *desc.menu_var == TRIGGER_MODE_RSTICK) {
        *desc.menu_var = TRIGGER_MODE_ANALOG;
      }
      if (desc.runtime_var) {
        *desc.runtime_var = *desc.menu_var;
      }
      if (desc.on_apply) desc.on_apply();
      return true;
    default:
      return false;
  }
}

bool handle_z_button_menu_item(const MenuItemDescriptor& desc, menu_item_action action) {
  if (desc.id != menu_item_n64_z_mode || action != item_action_display) {
    return false;
  }

#ifdef USE_I2C_DISPLAY
#ifdef ENABLE_INPUT_GAMECUBE
  if (menu_input == RZORD_GAMECUBE) {
    display.print(*desc.menu_var ? F("Back") : F("R1"));
    return true;
  }
#endif
  display.print(*desc.menu_var ? F("L2") : F("L1"));
#endif
  return true;
}

}  // namespace

void apply_latency_menu_configuration() {
  latencyTest.setEnabled(menu_latency_test != 0);
  latencyTest.setControllerInLoop(menu_latency_controller_in_loop != 0);
  latencyTest.setHostType(menu_latency_host_type);
}

void latency_test_apply() {
  apply_latency_menu_configuration();
}

uint8_t dpad_get_max() {
  return output_supports_dpad_buttons(outputMode) ? 3 : 2;
}

uint8_t stick_invert_get_max() {
  return input_has_right_stick() ? 3 : 1;
}

uint8_t trigger_mode_get_max() {
  const outputMode_t triggerOutput =
    (menu_output == OUTPUT_AUTO) ? get_effective_output_mode() : menu_output;
  if (input_has_analog_triggers() &&
      outputModeSupportsTriggerBothMode(triggerOutput) &&
      menuModeSupportsTriggerBothMode(menu_input)) {
    return TRIGGER_MODE_BOTH;
  }
  return TRIGGER_MODE_DIGITAL;
}

const MenuItemDescriptor* get_menu_descriptor(menu_item_enum item) {
  for (uint8_t i = 0; i < MENU_DESCRIPTOR_COUNT; i++) {
    if (menu_descriptors[i].id == item) {
      return &menu_descriptors[i];
    }
  }
  return nullptr;
}

void menu_item_handle_generic(const MenuItemDescriptor& desc, menu_item_action action) {
  if (handle_dpad_mode_menu_item(desc, action)) {
    return;
  }
  if (handle_switch_rtrig_stick_menu_item(desc, action)) {
    return;
  }
  if (handle_trigger_mode_menu_item(desc, action)) {
    return;
  }
  if (handle_z_button_menu_item(desc, action)) {
    return;
  }

  switch (action) {
    case item_action_display:
#ifdef USE_I2C_DISPLAY
      switch (desc.type) {
        case MENU_TYPE_BOOL:
          display.print(*desc.menu_var ? desc.bool_cfg.on_text : desc.bool_cfg.off_text);
          break;
        case MENU_TYPE_ENUM: {
          uint8_t idx = *desc.menu_var;
          uint8_t max_idx = desc.enum_cfg.get_max ? desc.enum_cfg.get_max() : (desc.enum_cfg.count - 1);
          if (idx > max_idx) idx = max_idx;
          display.print(desc.enum_cfg.names[idx]);
          break;
        }
        case MENU_TYPE_RANGE:
          display.print(*desc.menu_var);
          if (desc.range_cfg.suffix && desc.range_cfg.suffix[0]) {
            display.print(desc.range_cfg.suffix);
          }
          break;
        default:
          break;
      }
#else
      (void)desc;
#endif
      break;

    case item_action_change:
      switch (desc.type) {
        case MENU_TYPE_BOOL:
          *desc.menu_var = !*desc.menu_var;
          break;
        case MENU_TYPE_ENUM: {
          uint8_t max_val = desc.enum_cfg.get_max ? desc.enum_cfg.get_max() : (desc.enum_cfg.count - 1);
          *desc.menu_var = (*desc.menu_var + 1) % (max_val + 1);
          break;
        }
        case MENU_TYPE_RANGE:
          *desc.menu_var += desc.range_cfg.step;
          if (*desc.menu_var > desc.range_cfg.max_val) {
            *desc.menu_var = desc.range_cfg.min_val;
          }
          break;
        default:
          break;
      }
      if (desc.on_change) desc.on_change();
      break;

    case item_action_change_prev:
      switch (desc.type) {
        case MENU_TYPE_BOOL:
          *desc.menu_var = !*desc.menu_var;
          break;
        case MENU_TYPE_ENUM: {
          uint8_t max_val = desc.enum_cfg.get_max ? desc.enum_cfg.get_max() : (desc.enum_cfg.count - 1);
          if (*desc.menu_var == 0) {
            *desc.menu_var = max_val;
          } else {
            *desc.menu_var = *desc.menu_var - 1;
          }
          break;
        }
        case MENU_TYPE_RANGE:
          if (*desc.menu_var <= desc.range_cfg.min_val) {
            *desc.menu_var = desc.range_cfg.max_val;
          } else {
            *desc.menu_var -= desc.range_cfg.step;
          }
          break;
        default:
          break;
      }
      if (desc.on_change) desc.on_change();
      break;

    case item_action_reset:
      if (desc.runtime_var && settingIsPerMode(desc.setting_id)) {
        *desc.menu_var = *desc.runtime_var;
      } else {
        *desc.menu_var = (uint8_t)loadSettingValue(desc.setting_id, deviceMode);
      }
      if (desc.on_change) desc.on_change();
      break;

    case item_action_apply:
      if (desc.runtime_var) {
        *desc.runtime_var = *desc.menu_var;
      }
      if (desc.on_apply) desc.on_apply();
      break;

    case item_action_save:
      if (is_system_setting(desc.id)) {
        const int32_t previousValue = loadSettingValue(desc.setting_id, deviceMode);
        saveSystemSettingByte(desc.setting_id, *desc.menu_var);
        if (desc.setting_id == SettingId::WinOutput &&
            previousValue != (int32_t)*desc.menu_var) {
          auto_detect_clear_scratch_state();
        }
      }
      break;
  }
}
