#include "../product_config.h"

#include "menu_paddle_debug.h"

#ifdef ENABLE_INPUT_PADDLE

#include "../platform/display_runtime_state.h"
#include "../input/Input_Paddle.h"
#include "../input/runtime/input_adapter_runtime.h"
#include "menu_ui_state.h"

void renderPaddleDebug(bool modeBtnJustPressed) {
  static uint32_t prevTimeA = 0xFFFFFFFF;
  static uint32_t prevTimeB = 0xFFFFFFFF;
  static bool prevBtnA = false;
  static bool prevBtnB = false;
  static bool initialized = false;

  RZInputPaddle* paddle = activePaddleInputAdapter();
  if (paddle == nullptr) {
    return;
  }

  if (!initialized) {
    initialized = true;
    display.clear();
    display.setFont(System5x7);

    display.setCursor(0, 0);
    display.print("Paddle RC Debug");

    display.setCursor(0, 1);
    display.print("(Needs 100nF cap)");

    display.setCursor(0, 2);
    display.print("A: GPIO ");
    display.print(paddle->getPotAPin());

    display.setCursor(0, 3);
    display.print("B: GPIO ");
    display.print(paddle->getPotBPin());

    display.setCursor(0, 5);
    display.print("Btn A: GPIO ");
    display.print(paddle->getBtnAPin());

    display.setCursor(0, 6);
    display.print("Btn B: GPIO ");
    display.print(paddle->getBtnBPin());

    display.setCursor(0, 7);
    display.print("Mode btn to exit");
  }

  if (paddle->debug_smoothed_a != prevTimeA) {
    display.setCursor(60, 2);
    display.print("      ");
    display.setCursor(60, 2);
    display.print(paddle->debug_smoothed_a);
    display.print("us");
    prevTimeA = paddle->debug_smoothed_a;
  }

  if (paddle->debug_smoothed_b != prevTimeB) {
    display.setCursor(60, 3);
    display.print("      ");
    display.setCursor(60, 3);
    display.print(paddle->debug_smoothed_b);
    display.print("us");
    prevTimeB = paddle->debug_smoothed_b;
  }

  if (paddle->debug_btn_a != prevBtnA) {
    display.setCursor(90, 5);
    display.print(paddle->debug_btn_a ? "PRESSED" : "       ");
    prevBtnA = paddle->debug_btn_a;
  }

  if (paddle->debug_btn_b != prevBtnB) {
    display.setCursor(90, 6);
    display.print(paddle->debug_btn_b ? "PRESSED" : "       ");
    prevBtnB = paddle->debug_btn_b;
  }

  if (modeBtnJustPressed) {
    padTestActive = false;
    padTestInitialized = false;
    initialized = false;
    prevTimeA = 0xFFFFFFFF;
    prevTimeB = 0xFFFFFFFF;
  }
}

#endif
