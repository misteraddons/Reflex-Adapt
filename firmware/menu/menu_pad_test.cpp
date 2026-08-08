#include "../product_config.h"

#include "menu_pad_test.h"

#include "../core/controller_frame_state.h"
#include "../core/device_runtime_state.h"
#include "../platform/display_runtime_state.h"

#include "menu_mode_state.h"
#include "menu_pad_button_masks.h"
#include "menu_pad_layouts.h"
#include "menu_paddle_debug.h"
#include "menu_ui_state.h"

void renderPadTest(bool modeBtnJustPressed) {
#ifndef USE_I2C_DISPLAY
  (void)modeBtnJustPressed;
  padTestActive = false;
  padTestInitialized = false;
  return;
#else
  static uint32_t prevState[2] = { 0, 0 };
  static const PadButton* currentLayout = nullptr;
  static uint8_t currentLayoutCount = 0;

  #ifdef ENABLE_INPUT_PADDLE
  if (deviceMode == RZORD_PADDLE) {
    renderPaddleDebug(modeBtnJustPressed);
    return;
  }
  #endif

  const uint8_t P2_COL_OFFSET = 66;

  if (!padTestInitialized) {
    padTestInitialized = true;
    display.clear();

    const char* layoutName = nullptr;
    getPadLayoutForMode(menu_input, &currentLayout, &currentLayoutCount, &layoutName);

    if (getSharedControllerTypePadLayout(controllerFrameConst(0).controller_type_name,
        &currentLayout, &currentLayoutCount)) {
      layoutName = controllerFrameConst(0).controller_type_name;
    }

    display.setFont(System5x7);
    display.setRow(0);
    display.setCol(0);
    display.print("P1");
    if (max_devices > 1) {
      display.setCol(P2_COL_OFFSET);
      display.print("P2");
    }

    display.setFont(ReflexPad5x7);
    for (uint8_t i = 0; i < currentLayoutCount; ++i) {
      display.setCursor(currentLayout[i].col, currentLayout[i].row);
      display.print(currentLayout[i].off);
    }
    drawDpadCentersForLayout(currentLayout, currentLayoutCount, 0, 0);

    if (max_devices > 1) {
      for (uint8_t i = 0; i < currentLayoutCount; ++i) {
        display.setCursor(currentLayout[i].col + P2_COL_OFFSET, currentLayout[i].row);
        display.print(currentLayout[i].off);
      }
      drawDpadCentersForLayout(currentLayout, currentLayoutCount, P2_COL_OFFSET, 0);
    }

    display.setFont(System5x7);
    display.setCursor(0, 7);
    display.print("Mode btn to exit");
    display.setFont(ReflexPad5x7);

    prevState[0] = 0;
    prevState[1] = 0;
  }

  uint32_t state0 = buildButtonMaskFromReport(0);
  if (state0 != prevState[0]) {
    for (uint8_t i = 0; i < currentLayoutCount; ++i) {
      uint32_t mask = currentLayout[i].mask;
      if ((state0 & mask) != (prevState[0] & mask)) {
        display.setCursor(currentLayout[i].col, currentLayout[i].row);
        display.print((state0 & mask) ? currentLayout[i].on : currentLayout[i].off);
      }
    }
    prevState[0] = state0;
  }

  if (max_devices > 1) {
    uint32_t state1 = buildButtonMaskFromReport(1);
    if (state1 != prevState[1]) {
      for (uint8_t i = 0; i < currentLayoutCount; ++i) {
        uint32_t mask = currentLayout[i].mask;
        if ((state1 & mask) != (prevState[1] & mask)) {
          display.setCursor(currentLayout[i].col + P2_COL_OFFSET, currentLayout[i].row);
          display.print((state1 & mask) ? currentLayout[i].on : currentLayout[i].off);
        }
      }
      prevState[1] = state1;
    }
  }

  if (modeBtnJustPressed) {
    padTestActive = false;
    padTestInitialized = false;
    display.setFont(System5x7);
  }
#endif
}
