#include "../platform/display_runtime_state.h"
#include "menu_mode_state.h"
#include "menu_output_mode.h"
#include "../output/output_runtime_state.h"
#include "../core/settings_store.h"

void handle_item_output_mode(menu_item_action action) {
  switch (action) {
    case item_action_display:
#ifdef USE_I2C_DISPLAY
      display.print(get_mode_name(canonicalizeOutputMode(menu_output)));
#endif
      break;
    case item_action_change:
      menu_output = cycle_visible_output_mode(menu_output, true);
      break;
    case item_action_change_prev:
      menu_output = cycle_visible_output_mode(menu_output, false);
      break;
    case item_action_reset:
      menu_output = configuredOutputMode;
      break;
    case item_action_apply:
      configuredOutputMode = canonicalizeOutputMode(menu_output);
      outputMode = sanitizeRuntimeOutputMode(configuredOutputMode);
      autoDetectState = AUTO_STATE_IDLE;
      auto_detect_clear_scratch_state();
      break;
    case item_action_save:
      break;
  }
}
