#include "../product_config.h"

#include "menu_playable_games.h"

#include <Arduino.h>
#include <stdio.h>

#include "../core/controller_frame_state.h"
#include "../input/runtime/input_poll_runtime.h"
#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"
#include "menu_playable_games_internal.h"

void runReactionTest() {
#ifndef USE_I2C_DISPLAY
  playable_games_internal::finishPlayableGame();
  return;
#else
  bool gameActive = true;
  uint8_t state = 0;
  uint32_t waitStart = millis();
  uint32_t goTime = 0;
  uint32_t reactionTime = 0;
  uint32_t waitDuration = 2000 + (micros() % 3000);
  bool lastA = false;
  bool lastB = false;
  uint8_t lastRenderedState = 0xFF;
  uint32_t lastRenderedReactionTime = UINT32_MAX;

  while (gameActive) {
    playable_games_internal::servicePlayableGameRuntime();

    const controller_state_t& frame = controllerFrameConst(0);
    bool curB = frame.B || frame.SELECT;
    if (curB && !lastB) {
      gameActive = false;
      break;
    }
    lastB = curB;

    uint32_t now = millis();
    bool curA = frame.A || frame.START;
    if (curA && !lastA) {
      if (state == 0 || state == 3) {
        state = 1;
        waitStart = now;
        waitDuration = 2000 + (micros() % 3000);
      } else if (state == 1) {
        reactionTime = 9999;
        state = 3;
      } else if (state == 2) {
        reactionTime = now - goTime;
        state = 3;
      }
    }
    lastA = curA;

    if (state == 1 && now - waitStart >= waitDuration) {
      state = 2;
      goTime = now;
    }

    if (state != lastRenderedState || reactionTime != lastRenderedReactionTime) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_helvB14_tr);
      switch (state) {
        case 0:
          u8g2.drawStr(15, 30, "Press A");
          u8g2.drawStr(15, 50, "to start");
          break;
        case 1:
          u8g2.drawStr(30, 40, "Wait...");
          break;
        case 2:
          u8g2.drawStr(40, 40, "GO!");
          break;
        case 3:
          if (reactionTime == 9999) {
            u8g2.drawStr(10, 30, "Too early!");
          } else {
            char s[16];
            snprintf(s, sizeof(s), "%d ms", (int)reactionTime);
            u8g2.drawStr(30, 30, s);
          }
          u8g2.setFont(u8g2_font_5x7_tr);
          u8g2.drawStr(25, 55, "Press to retry");
          break;
      }
      u8g2.sendBuffer();
      lastRenderedState = state;
      lastRenderedReactionTime = reactionTime;
    }
    delay(1);
  }

  playable_games_internal::finishPlayableGame();
#endif
}
