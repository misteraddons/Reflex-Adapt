#include "menu_item_handlers_internal.h"

namespace menu_item_handlers_internal {

void handle_item_button_remap(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(hasActiveRemap() ? "Active" : "Enter");
      break;
    case item_action_change:
      buzzer.playMenuEnter();
      buttonRemapActive = true;
      remapMenu.open(deviceMode);
      break;
    default:
      break;
  }
}

void handle_item_mapping_view(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(hasActiveRemap() ? "Active" : "Enter");
      break;
    case item_action_change:
      buzzer.playMenuEnter();
      mappingDisplayActive = true;
      activateMappingDisplay();
      break;
    default:
      break;
  }
}

void handle_item_pad_test(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print("Enter");
      break;
    case item_action_change:
      buzzer.playMenuEnter();
      padTestActive = true;
      padTestInitialized = false;
      break;
    default:
      break;
  }
}

void handle_item_pin_debug(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print("Enter");
      break;
    case item_action_change:
      buzzer.playMenuEnter();
      pinDebugActive = true;
      pinDebugInitialized = false;
      break;
    default:
      break;
  }
}

void handle_item_analog_test(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print("Enter");
      break;
    case item_action_change:
      buzzer.playMenuEnter();
      analogTestActive = true;
      analogTestInitialized = false;
      break;
    default:
      break;
  }
}

void handle_item_latency_run(menu_item_action action) {
  switch (action) {
    case item_action_display:
      apply_latency_menu_configuration();
      if (latencyTest.isBenchRunning()) {
        display.print("Stop");
      } else if (!menu_latency_test) {
        display.print("Off");
      } else if (!latencyTest.isCurrentModeReady()) {
        display.print("No Pins");
      } else if (latencyTest.getCompletedRunSamples() > 0) {
        display.print("Again");
      } else {
        display.print("Start");
      }
      break;
    case item_action_change:
    case item_action_change_prev:
      apply_latency_menu_configuration();
      if (!menu_latency_test) {
        return;
      }
      if (latencyTest.isBenchRunning()) {
        latencyTest.stopBench();
      } else if (latencyTest.isCurrentModeReady()) {
        latencyTest.startBench();
      }
      break;
    default:
      break;
  }
}

}
