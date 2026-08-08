#include "../product_config.h"

#include "menu.h"
#include "menu_playable_games_internal.h"
#include "../platform/buzzer.h"

namespace {

const char* const game_names[] = {
  "Snake",
  "Pong",
  "Breakout",
  "Flappy Bird",
  "Dino Run",
  "Space Invaders",
  "Asteroids",
  "Reaction Test"
};

constexpr uint8_t GAME_COUNT = 8;

}  // namespace

bool handleGamesSubmenu(
    uint8_t& games_submenu_cursor,
    bool& games_submenu_active,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed) {
  if (!games_submenu_active) return false;

  static bool games_submenu_initialized = false;

  if (!games_submenu_initialized) {
    games_submenu_initialized = true;
    display.clear();
  }

  display.setFont(System5x7);
  display.set1X();
  display.setCursor(0, 0);
  display.print(F("Games:"));

  const uint8_t visible_rows = 6;
  const uint8_t scroll_margin = 2;
  uint8_t scroll = 0;
  uint8_t total_items = GAME_COUNT + 1;

  if (games_submenu_cursor >= visible_rows - scroll_margin && total_items > visible_rows) {
    uint8_t new_scroll = games_submenu_cursor - visible_rows + scroll_margin + 1;
    uint8_t max_scroll = total_items - visible_rows;
    scroll = (new_scroll < max_scroll) ? new_scroll : max_scroll;
  }

  for (uint8_t row = 0; row < visible_rows; row++) {
    uint8_t idx = scroll + row;
    display.setCursor(0, row + 1);

    if (idx < GAME_COUNT) {
      bool selected = (idx == games_submenu_cursor);
      display.print(selected ? ">" : " ");
      display.print(game_names[idx]);
      display.clearToEOL();
    } else if (idx == GAME_COUNT) {
      bool selected = (games_submenu_cursor == GAME_COUNT);
      display.print(selected ? ">" : " ");
      display.print(F("[Back]"));
      display.clearToEOL();
    }
  }

  if (ctrlDownJust || btnNavigateJustPressed) {
    if (games_submenu_cursor < GAME_COUNT) {
      games_submenu_cursor++;
      buzzer.playMenuNav();
    }
  }
  if (ctrlUpJust) {
    if (games_submenu_cursor > 0) {
      games_submenu_cursor--;
      buzzer.playMenuNav();
    }
  }

  if (ctrlAJust || ctrlRightJust || btnChangeJustPressed) {
    if (games_submenu_cursor < GAME_COUNT) {
      games_submenu_active = false;
      games_submenu_initialized = false;
      closeMenu();
      playable_games_internal::beginPlayableGameDisplay();
      switch (games_submenu_cursor) {
        case 0: runPlaySnake(); break;
        case 1: runPlayPong(); break;
        case 2: runPlayBreakout(); break;
        case 3: runPlayFlappy(); break;
        case 4: runPlayDino(); break;
        case 5: runPlayInvaders(); break;
        case 6: runPlayAsteroids(); break;
        case 7: runReactionTest(); break;
      }
      isMenuOpen = true;
      mainDisplayInitialized = false;
    } else {
      games_submenu_active = false;
      games_submenu_initialized = false;
      display.clear();
      renderMenu(selected_visible, scroll_offset, bottom_right);
    }
  }

  if (ctrlBJust) {
    games_submenu_active = false;
    games_submenu_initialized = false;
    display.clear();
    renderMenu(selected_visible, scroll_offset, bottom_right);
  }

  return true;
}
