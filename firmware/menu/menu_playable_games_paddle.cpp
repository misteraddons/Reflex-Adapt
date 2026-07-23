#include "../product_config.h"

#include "menu_playable_games.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>

#include "../core/controller_frame_state.h"
#include "../input/runtime/input_poll_runtime.h"
#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"
#include "menu_playable_games_internal.h"

void runPlayPong() {
#ifndef USE_I2C_DISPLAY
  playable_games_internal::finishPlayableGame();
  return;
#else
  bool gameActive = true;
  int16_t ballX = 64 << 4;
  int16_t ballY = 32 << 4;
  int8_t ballVX = 32;
  int8_t ballVY = 16;
  int8_t paddle1Y = 24;
  int8_t paddle2Y = 24;
  uint8_t score1 = 0;
  uint8_t score2 = 0;
  uint32_t lastFrame = millis();
  bool lastB = false;

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
    if (now - lastFrame >= 25) {
      lastFrame = now;
      if (frame.PAD_U && paddle1Y > 0) paddle1Y -= 4;
      if (frame.PAD_D && paddle1Y < 48) paddle1Y += 4;

      int16_t aiTargetY = (ballVX > 0 || (ballX >> 4) > 72) ? (ballY >> 4) : 32;
      if (paddle2Y + 8 < aiTargetY - 4) paddle2Y += 2;
      else if (paddle2Y + 8 > aiTargetY + 4) paddle2Y -= 2;
      if (paddle2Y < 0) paddle2Y = 0;
      if (paddle2Y > 48) paddle2Y = 48;

      ballX += ballVX;
      ballY += ballVY;
      int bx = ballX >> 4;
      int by = ballY >> 4;
      if (by <= 2 || by >= 60) ballVY = -ballVY;
      if (bx <= 8 && by >= paddle1Y && by <= paddle1Y + 16) {
        ballVX = abs(ballVX);
        ballVY += (by - paddle1Y - 8) / 3;
      }
      if (bx >= 118 && by >= paddle2Y && by <= paddle2Y + 16) {
        ballVX = -abs(ballVX);
        ballVY += (by - paddle2Y - 8) / 3;
      }
      if (bx < 0) {
        score2++;
        ballX = 64 << 4;
        ballVX = 32;
      }
      if (bx > 128) {
        score1++;
        ballX = 64 << 4;
        ballVX = -32;
      }
      if (score1 >= 5 || score2 >= 5) {
        score1 = 0;
        score2 = 0;
      }

      u8g2.clearBuffer();
      u8g2.drawVLine(64, 0, 64);
      u8g2.drawBox(4, paddle1Y, 3, 16);
      u8g2.drawBox(121, paddle2Y, 3, 16);
      u8g2.drawBox((ballX >> 4) - 2, (ballY >> 4) - 2, 4, 4);
      u8g2.setFont(u8g2_font_5x7_tr);
      char s[4];
      snprintf(s, sizeof(s), "%d", score1);
      u8g2.drawStr(50, 10, s);
      snprintf(s, sizeof(s), "%d", score2);
      u8g2.drawStr(74, 10, s);
      u8g2.sendBuffer();
    }
    delay(5);
  }

  playable_games_internal::finishPlayableGame();
#endif
}

void runPlayBreakout() {
#ifndef USE_I2C_DISPLAY
  playable_games_internal::finishPlayableGame();
  return;
#else
  bool gameActive = true;
  int16_t ballX = 64 << 4;
  int16_t ballY = 50 << 4;
  int8_t ballVX = 16;
  int8_t ballVY = -20;
  int8_t paddleX = 54;
  uint8_t bricks[13];
  for (int i = 0; i < 13; i++) bricks[i] = 0x1F;
  uint32_t lastFrame = millis();
  bool lastB = false;

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
    if (now - lastFrame >= 25) {
      lastFrame = now;
      if (frame.PAD_L && paddleX > 0) paddleX -= 4;
      if (frame.PAD_R && paddleX < 108) paddleX += 4;

      ballX += ballVX;
      ballY += ballVY;
      int bx = ballX >> 4;
      int by = ballY >> 4;
      if (bx <= 2 || bx >= 126) ballVX = -ballVX;
      if (by <= 2) ballVY = -ballVY;
      if (by >= 58 && bx >= paddleX && bx <= paddleX + 20) {
        ballVY = -abs(ballVY);
        ballVX += (bx - paddleX - 10) / 2;
      }
      if (by < 25 && by >= 5) {
        int row = (by - 5) / 4;
        int col = (bx - 6) / 9;
        if (col >= 0 && col < 13 && (bricks[col] & (1 << row))) {
          bricks[col] &= ~(1 << row);
          ballVY = -ballVY;
        }
      }

      bool anyBricks = false;
      for (int i = 0; i < 13; i++) if (bricks[i]) anyBricks = true;
      if (by > 64) {
        ballX = 64 << 4;
        ballY = 50 << 4;
        ballVY = -20;
      }
      if (!anyBricks) {
        for (int i = 0; i < 13; i++) bricks[i] = 0x1F;
      }

      u8g2.clearBuffer();
      u8g2.drawFrame(0, 0, 128, 64);
      u8g2.drawBox(paddleX, 60, 20, 3);
      u8g2.drawBox(bx - 1, by - 1, 3, 3);
      for (int col = 0; col < 13; col++) {
        for (int row = 0; row < 5; row++) {
          if (bricks[col] & (1 << row)) u8g2.drawBox(6 + col * 9, 5 + row * 4, 8, 3);
        }
      }
      u8g2.sendBuffer();
    }
    delay(5);
  }

  playable_games_internal::finishPlayableGame();
#endif
}
