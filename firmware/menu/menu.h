#pragma once

#include "../core/button_remap.h"
#include "menu_action.h"
#include "menu_analog_test.h"
#include "menu_input_mode.h"
#include "menu_item_defs.h"
#include "menu_navigation.h"
#include "menu_bridge.h"
#include "menu_display_state.h"
#include "menu_idle_runtime.h"
#include "mapping_display.h"
#include "menu_about.h"
#include "menu_catalog.h"
#include "menu_home_debug.h"
#include "menu_home_mode_line.h"
#include "menu_item_handlers.h"
#include "menu_mode_labels.h"
#include "menu_pad_layouts.h"
#include "menu_pad_button_masks.h"
#include "menu_virtual_output_pad.h"
#include "menu_helpers.h"
#include "menu_mode_state.h"
#include "menu_pin_debug.h"
#include "menu_output_mode.h"
#include "menu_paddle_debug.h"
#include "menu_playable_games.h"
#include "menu_screensaver_dispatch.h"
#include "menu_screensaver_action_games.h"
#include "menu_screensaver_auto_snake.h"
#include "menu_screensaver_bounce.h"
#include "menu_screensaver_simulations.h"
#include "menu_screensaver_starfield_matrix.h"
#include "menu_screensaver_visuals.h"
#include "../core/mode_settings_crc.h"
#include "menu_system_screens.h"
#include "menu_runtime_state.h"
#include "menu_sound_catalog.h"
#include "menu_ui_state.h"
#include "menu_working_state.h"
#include "../core/analog_calibration_state.h"
#include "../core/controller_frame_state.h"
#include "../firmware_build_info.h"
#include "../input/jaguar/input_jaguar_runtime_state.h"
#include "../input/autodetect/input_autodetect_runtime_state.h"
#include "../input/runtime/input_runtime_bridge.h"
#include "../input/snes/input_snes_runtime_state.h"
#include "../input/autodetect/input_autodetect_flags.h"
#include "../output/output_mode.h"
#include "../output/output_runtime_state.h"
#include "../core/settings_store.h"
#include "../platform/display_runtime_state.h"
#include "../platform/webhid_runtime.h"

// Selected menu modes now live in menu_mode_state.cpp.
// Input mode menu helpers now live in menu_input_mode.cpp.
// Submenu/navigation/display state now lives in menu_ui_state.cpp.
// Main home display rendering now lives in menu_main_display.cpp.

// void handle_item_template(menu_item_action action) {
//   switch (action) {
//     case item_action_display:
//       break;
//     case item_action_change:
//       break;
//     case item_action_reset:
//       break;
//     case item_action_apply:
//       break;
//     case item_action_save:
//       break;
//   }
// }

// isMenuOpen and mainDisplayInitialized declared earlier for submenu access

// =============================================================================
// Additional Screensaver Animations
// =============================================================================

// Animation type selection (0=Bounce, 1=Starfield, 2=Matrix, 3=Snake)
// screensaver_animation and menu_screensaver_anim declared earlier for submenu access
