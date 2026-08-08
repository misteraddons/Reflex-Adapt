#include "../product_config.h"

#include "menu_submenu_kiosk.h"

#include "menu.h"
#include "menu_runtime_state.h"
#include "../platform/buzzer.h"

namespace {

constexpr uint8_t kKioskChoiceCount = 3;

struct KioskRenderSnapshot {
  bool valid;
  uint8_t selection;
};

KioskRenderSnapshot s_kiosk_render_snapshot{};

bool kioskRenderSnapshotMatches(uint8_t selection) {
  return s_kiosk_render_snapshot.valid &&
         s_kiosk_render_snapshot.selection == selection;
}

void captureKioskRenderSnapshot(uint8_t selection) {
  s_kiosk_render_snapshot.valid = true;
  s_kiosk_render_snapshot.selection = selection;
}

void invalidateKioskRenderSnapshot() {
  s_kiosk_render_snapshot.valid = false;
}

void renderKioskChoice(uint8_t selection, uint8_t index, const char* label) {
  display.print(selection == index ? ">" : " ");
  display.print(label);
}

void renderKioskDialog(uint8_t selection) {
  if (kioskRenderSnapshotMatches(selection)) {
    return;
  }
  if (!s_kiosk_render_snapshot.valid) {
    display.clear();
  }

  display.setFont(System5x7);
  display.set1X();
  display.setCursor(0, 0);
  display.print(F("Kiosk Mode"));
  display.clearToEOL();

  display.setCursor(0, 1);
  display.print(F("Mode x2 = Quick Menu"));
  display.clearToEOL();
  display.setCursor(0, 2);
  display.print(F("Mode (hold) = Settings"));
  display.clearToEOL();
  display.setCursor(0, 3);
  display.print(F("Reset x2 = Reboot"));
  display.clearToEOL();
  display.setCursor(0, 4);
  display.print(F("Reset (hold) = Auto Input"));
  display.clearToEOL();

  display.clear(0, 127, 5, 7);
  display.setCursor(0, 5);
  renderKioskChoice(selection, 0, "On");
  display.print(F(" "));
  renderKioskChoice(selection, 1, "Off");
  display.print(F(" "));
  renderKioskChoice(selection, 2, "Cancel");
  display.clearToEOL();

  captureKioskRenderSnapshot(selection);
}

void closeKioskSubmenu(bool& active, uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right) {
  active = false;
  invalidateKioskRenderSnapshot();
  display.clear();
  renderMenu(selected_visible, scroll_offset, bottom_right);
}

}  // namespace

bool handleKioskSubmenu(
    uint8_t& kiosk_submenu_selection,
    bool& kiosk_submenu_active,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlLeftJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed) {
  if (!kiosk_submenu_active) {
    invalidateKioskRenderSnapshot();
    return false;
  }

  if (kiosk_submenu_selection >= kKioskChoiceCount) {
    kiosk_submenu_selection = 2;
  }

  if (ctrlDownJust || ctrlRightJust || btnNavigateJustPressed) {
    kiosk_submenu_selection = (uint8_t)((kiosk_submenu_selection + 1) % kKioskChoiceCount);
    buzzer.playMenuNav();
    renderKioskDialog(kiosk_submenu_selection);
    return true;
  }

  if (ctrlUpJust || ctrlLeftJust) {
    kiosk_submenu_selection = (kiosk_submenu_selection == 0) ? (kKioskChoiceCount - 1) : (uint8_t)(kiosk_submenu_selection - 1);
    buzzer.playMenuNav();
    renderKioskDialog(kiosk_submenu_selection);
    return true;
  }

  if (ctrlBJust) {
    closeKioskSubmenu(kiosk_submenu_active, selected_visible, scroll_offset, bottom_right);
    buzzer.playMenuNav();
    return true;
  }

  if (ctrlAJust || btnChangeJustPressed) {
    if (kiosk_submenu_selection == 0) {
      menu_kiosk_mode = 1;
    } else if (kiosk_submenu_selection == 1) {
      menu_kiosk_mode = 0;
    }
    closeKioskSubmenu(kiosk_submenu_active, selected_visible, scroll_offset, bottom_right);
    buzzer.playMenuNav();
    return true;
  }

  renderKioskDialog(kiosk_submenu_selection);
  return true;
}
