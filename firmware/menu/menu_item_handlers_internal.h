#pragma once

#include "../product_config.h"

#include <Arduino.h>

#include "../core/button_remap.h"
#include "../core/controller_settings_state.h"
#include "../core/device_runtime_state.h"
#include "../core/firmware_support.h"
#include "../core/settings_store.h"
#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"
#include "../platform/latency_test.h"
#include "../platform/rgb_led.h"
#include "mapping_display.h"
#include "menu_about.h"
#include "menu_bridge.h"
#include "menu_descriptors.h"
#include "menu_input_mode.h"
#include "menu_item_handlers.h"
#include "menu_mode_state.h"
#include "menu_output_mode.h"
#include "menu_runtime_state.h"
#include "menu_system_screens.h"
#include "menu_ui_state.h"
#include "menu_working_state.h"

namespace menu_item_handlers_internal {

void handle_item_screensaver(menu_item_action action);
void handle_item_games(menu_item_action action);
void handle_item_buzzer(menu_item_action action);
void handle_item_hotkeys(menu_item_action action);
void handle_item_kiosk_mode(menu_item_action action);
void handle_item_about(menu_item_action action);

void handle_item_factory_reset(menu_item_action action);
void handle_item_bootloader(menu_item_action action);
void handle_item_display_contrast(menu_item_action action);

void handle_item_button_remap(menu_item_action action);
void handle_item_mapping_view(menu_item_action action);
void handle_item_pad_test(menu_item_action action);
void handle_item_pin_debug(menu_item_action action);
void handle_item_analog_test(menu_item_action action);
void handle_item_latency_run(menu_item_action action);

void handle_item_guncon_offset(menu_item_action action);
void handle_item_jogcon_force(menu_item_action action);
void handle_item_wheel_sensitivity(menu_item_action action);

}
