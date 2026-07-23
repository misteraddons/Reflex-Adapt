#include "../product_config.h"

#include "quick_config.h"

void QuickConfigMenu::navigateDown() {
  needs_redraw = true;
  const uint8_t turboCount = getVisibleTurboCount();
  const uint8_t dualMergeCount = getDualMergeMenuCount();

  switch (state) {
    case QC_MAIN_MENU:
      if (visible_count == 0) {
        on_bottom_row = true;
        bottom_index = 0;
        bottom_right = false;
        break;
      }
      if (on_bottom_row) {
        if (bottom_index < 2) {
          bottom_index++;
          bottom_right = bottom_index == 2;
        } else {
          on_bottom_row = false;
          bottom_index = 0;
          bottom_right = false;
          selected_index = 0;
        }
      } else {
        selected_index++;
        if (selected_index >= visible_count) {
          on_bottom_row = true;
          bottom_index = 0;
          bottom_right = false;
        }
      }
      break;

    case QC_CONFIRM_DEFAULT:
      default_confirm_index = !default_confirm_index;
      break;

    case QC_TURBO_LIST:
      turbo_index++;
      if (turbo_index > turboCount) {
        turbo_index = 0;
      }
      break;

    case QC_TURBO_RATE:
      turbo_rate_selection++;
      if (turbo_rate_selection > TURBO_RATE_LAST) {
        turbo_rate_selection = 0;
      }
      break;

    case QC_SETTING_EDIT:
      cycleSettingValue();
      break;

    case QC_ANALOG_SUBMENU:
      buildAnalogItems();
      analog_index++;
      if (analog_index > analog_count) {
        analog_index = 0;
      }
      break;

    case QC_DUAL_MERGE_MAP:
      dual_merge_index++;
      if (dual_merge_index > dualMergeCount) {
        dual_merge_index = 0;
      }
      break;

    case QC_RUMBLE_SUBMENU:
      advanceRumbleIndex(true);
      break;

    case QC_GUNCON_ADJUST:
      handleGunconNavigate();
      break;

    case QC_REMAP_LIST: {
      uint8_t btn_count = getRemapButtonCount(deviceMode);
      remap_index++;
      if (remap_index > btn_count) {
        remap_index = 0;
      }
      break;
    }

    case QC_RANGE_TEST:
      range_test_exit_selected = !range_test_exit_selected;
      break;

    case QC_STICK_CAL:
      break;

    default:
      break;
  }
}

void QuickConfigMenu::navigateUp() {
  needs_redraw = true;
  const uint8_t turboCount = getVisibleTurboCount();
  const uint8_t dualMergeCount = getDualMergeMenuCount();

  switch (state) {
    case QC_MAIN_MENU:
      if (visible_count == 0) {
        on_bottom_row = true;
        bottom_index = 2;
        bottom_right = true;
        break;
      }
      if (on_bottom_row) {
        if (bottom_index > 0) {
          bottom_index--;
          bottom_right = bottom_index == 2;
        } else {
          on_bottom_row = false;
          selected_index = visible_count - 1;
        }
      } else {
        if (selected_index == 0) {
          on_bottom_row = true;
          bottom_index = 2;
          bottom_right = true;
        } else {
          selected_index--;
        }
      }
      break;

    case QC_CONFIRM_DEFAULT:
      default_confirm_index = !default_confirm_index;
      break;

    case QC_TURBO_LIST:
      if (turbo_index == 0) {
        turbo_index = turboCount;
      } else {
        turbo_index--;
      }
      break;

    case QC_TURBO_RATE:
      if (turbo_rate_selection == 0) {
        turbo_rate_selection = TURBO_RATE_LAST;
      } else {
        turbo_rate_selection--;
      }
      break;

    case QC_SETTING_EDIT:
      cycleSettingValueBack();
      break;

    case QC_ANALOG_SUBMENU:
      buildAnalogItems();
      if (analog_index == 0) {
        analog_index = analog_count;
      } else {
        analog_index--;
      }
      break;

    case QC_DUAL_MERGE_MAP:
      if (dual_merge_index == 0) {
        dual_merge_index = dualMergeCount;
      } else {
        dual_merge_index--;
      }
      break;

    case QC_RUMBLE_SUBMENU:
      advanceRumbleIndex(false);
      break;

    case QC_GUNCON_ADJUST:
      handleGunconNavigateBack();
      break;

    case QC_REMAP_LIST: {
      uint8_t btn_count = getRemapButtonCount(deviceMode);
      if (remap_index == 0) {
        remap_index = btn_count;
      } else {
        remap_index--;
      }
      break;
    }

    case QC_RANGE_TEST:
      range_test_exit_selected = !range_test_exit_selected;
      break;

    case QC_STICK_CAL:
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleValueNext(bool isSelectAction) {
  needs_redraw = true;
  const uint8_t turboCount = getVisibleTurboCount();

  switch (state) {
    case QC_MAIN_MENU:
      if (on_bottom_row) {
        if (isSelectAction) {
          select();
          return;
        }
        bottom_index = (bottom_index + 1) % 3;
        bottom_right = bottom_index == 2;
        return;
      }
      cycleMainMenuItemNext();
      break;

    case QC_CONFIRM_DEFAULT:
      if (isSelectAction) {
        select();
      } else {
        default_confirm_index = !default_confirm_index;
      }
      break;

    case QC_TURBO_LIST:
      if (turbo_index >= turboCount) {
        state = QC_MAIN_MENU;
      } else {
        uint8_t btn_index = getTurboButtonIndexForVisibleIndex(turbo_index);
        if (btn_index != 0xFF) {
          const uint8_t rateIndex =
              (getTurboMenuIndexForRate(temp_rates[btn_index]) + 1) % TURBO_RATE_LAST;
          temp_rates[btn_index] = getTurboRateForMenuIndex(rateIndex);
        }
      }
      break;

    case QC_ANALOG_SUBMENU:
      handleAnalogSelect();
      break;

    case QC_DUAL_MERGE_MAP:
      handleDualMergeSelect();
      break;

    case QC_RUMBLE_SUBMENU:
      handleRumbleSelect(true);
      break;

    case QC_GUNCON_ADJUST:
      handleGunconNavigate();
      break;

    case QC_REMAP_LIST: {
      uint8_t btn_count = getRemapButtonCount(deviceMode);
      if (remap_index >= btn_count) {
        state = QC_MAIN_MENU;
      } else {
        const uint8_t source_slot = getRemapButtonSlot(deviceMode, remap_index);
        if (source_slot != 0xFF) {
          uint8_t target_display = getRemapButtonDisplayIndex(deviceMode, active_remaps[source_slot]);
          if (target_display == 0xFF) {
            target_display = remap_index;
          }
          target_display = (target_display + 1) % btn_count;
          active_remaps[source_slot] = getRemapButtonSlot(deviceMode, target_display);
        }
      }
      break;
    }

    case QC_RANGE_TEST:
      if (range_test_exit_selected) {
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      }
      break;

    case QC_STICK_CAL:
      if (isSelectAction) {
        select();
      } else {
        cal_selection = (cal_selection + 1) % 3;
      }
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleValuePrev() {
  needs_redraw = true;
  const uint8_t turboCount = getVisibleTurboCount();

  switch (state) {
    case QC_MAIN_MENU:
      cycleMainMenuItemPrev();
      break;

    case QC_CONFIRM_DEFAULT:
      default_confirm_index = !default_confirm_index;
      break;

    case QC_TURBO_LIST:
      if (turbo_index < turboCount) {
        uint8_t btn_index = getTurboButtonIndexForVisibleIndex(turbo_index);
        if (btn_index != 0xFF) {
          const uint8_t currentIndex = getTurboMenuIndexForRate(temp_rates[btn_index]);
          const uint8_t rateIndex =
              (currentIndex == 0) ? (TURBO_RATE_LAST - 1) : (uint8_t)(currentIndex - 1);
          temp_rates[btn_index] = getTurboRateForMenuIndex(rateIndex);
        }
      }
      break;

    case QC_TURBO_RATE:
      if (turbo_rate_selection >= TURBO_RATE_LAST) {
        state = QC_TURBO_LIST;
      } else {
        uint8_t btn_index = getTurboButtonIndexForVisibleIndex(turbo_index);
        if (btn_index != 0xFF) {
          temp_rates[btn_index] = getTurboRateForMenuIndex(turbo_rate_selection);
        }
        state = QC_TURBO_LIST;
      }
      break;

    case QC_ANALOG_SUBMENU:
      handleAnalogSelectBack();
      break;

    case QC_DUAL_MERGE_MAP:
      handleDualMergeSelectBack();
      break;

    case QC_RUMBLE_SUBMENU:
      handleRumbleSelect(false);
      break;

    case QC_GUNCON_ADJUST:
      handleGunconNavigateBack();
      break;

    case QC_REMAP_LIST: {
      uint8_t btn_count = getRemapButtonCount(deviceMode);
      if (remap_index >= btn_count) {
        state = QC_MAIN_MENU;
      } else {
        const uint8_t source_slot = getRemapButtonSlot(deviceMode, remap_index);
        if (source_slot != 0xFF) {
          uint8_t target_display = getRemapButtonDisplayIndex(deviceMode, active_remaps[source_slot]);
          if (target_display == 0xFF) {
            target_display = remap_index;
          }
          target_display = (target_display == 0) ? (btn_count - 1) : (target_display - 1);
          active_remaps[source_slot] = getRemapButtonSlot(deviceMode, target_display);
        }
      }
      break;
    }

    case QC_RANGE_TEST:
      if (range_test_exit_selected) {
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      }
      break;

    case QC_STICK_CAL:
      cal_selection = (cal_selection == 0) ? 2 : cal_selection - 1;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::back() {
  needs_redraw = true;

  switch (state) {
    case QC_MAIN_MENU:
      discard();
      break;

    case QC_CONFIRM_DEFAULT:
      state = QC_MAIN_MENU;
      break;

    case QC_TURBO_LIST:
    case QC_REMAP_LIST:
    case QC_ANALOG_SUBMENU:
    case QC_DUAL_MERGE_MAP:
    case QC_RUMBLE_SUBMENU:
    case QC_GUNCON_ADJUST:
      state = QC_MAIN_MENU;
      break;

    case QC_RANGE_TEST:
    case QC_STICK_CAL:
      state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      break;

    case QC_TURBO_RATE:
      state = QC_TURBO_LIST;
      break;

    case QC_SETTING_EDIT:
      state = QC_MAIN_MENU;
      break;

    default:
      break;
  }
}
