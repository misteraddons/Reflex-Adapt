#include "../product_config.h"

#include "menu_playable_games_internal.h"

#include <Arduino.h>

#include "../core/controller_settings_state.h"
#include "../core/runtime/runtime_loop.h"
#include "../input/runtime/input_poll_runtime.h"
#include "../platform/display_runtime_state.h"
#include "menu_display_state.h"
#include "menu_ui_state.h"

namespace playable_games_internal {

void beginPlayableGameDisplay() {
#ifdef USE_I2C_DISPLAY
  beginDisplayWire();
  u8g2.begin();
  u8g2.setI2CAddress(I2C_ADDRESS * 2);
  u8g2.setFlipMode(0);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
#endif
}

void finishPlayableGame() {
#ifdef USE_I2C_DISPLAY
  beginDisplayWire();
  display.begin(&Adafruit128x64, I2C_ADDRESS);
  display.setContrast(display_contrast);
  display.setFont(System5x7);
  display.set1X();
  display.clear();
  needsU8g2Clear = true;
#endif
  mainDisplayInitialized = false;
}

void servicePlayableGameRuntime() {
  runLoopBackgroundTasks();
  pollInputModuleIfDue();
  yield();
}

}  // namespace playable_games_internal
