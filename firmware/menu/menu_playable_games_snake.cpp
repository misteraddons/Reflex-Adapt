#include "../product_config.h"

#include "menu_playable_games.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "../core/controller_frame_state.h"
#include "../input/runtime/input_poll_runtime.h"
#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"
#include "menu_playable_games_internal.h"

#ifndef USE_I2C_DISPLAY

void runPlaySnake() {
  playable_games_internal::finishPlayableGame();
}

#else

namespace {

constexpr uint8_t PLAY_SNAKE_MAX_LEN = 64;

int8_t playSnakeX[PLAY_SNAKE_MAX_LEN];
int8_t playSnakeY[PLAY_SNAKE_MAX_LEN];
uint8_t playSnakeLen = 5;
int8_t playSnakeDirX = 1;
int8_t playSnakeDirY = 0;
int8_t playSnakeNextDirX = 1;
int8_t playSnakeNextDirY = 0;
int8_t playFoodX = 0;
int8_t playFoodY = 0;
uint16_t playSnakeScore = 0;
bool playSnakeGameOver = false;
bool playSnakeActive = false;
uint32_t playSnakeLastFrame = 0;
uint32_t playSnakeSpeed = 150;

void initPlaySnake() {
  playSnakeLen = 5;
  for (uint8_t i = 0; i < playSnakeLen; i++) {
    playSnakeX[i] = 16 - i;
    playSnakeY[i] = 8;
  }
  playSnakeDirX = 1;
  playSnakeDirY = 0;
  playSnakeNextDirX = 1;
  playSnakeNextDirY = 0;
  playSnakeScore = 0;
  playSnakeGameOver = false;
  playSnakeSpeed = 150;

  bool validFood = false;
  while (!validFood) {
    playFoodX = 2 + (micros() % 28);
    playFoodY = 1 + (micros() % 14);
    validFood = true;
    for (uint8_t i = 0; i < playSnakeLen; i++) {
      if (playSnakeX[i] == playFoodX && playSnakeY[i] == playFoodY) {
        validFood = false;
        break;
      }
    }
  }
}

void placeNewFood() {
  bool validFood = false;
  uint8_t attempts = 0;
  while (!validFood && attempts < 100) {
    playFoodX = 1 + (micros() % 30);
    playFoodY = 1 + (micros() % 14);
    validFood = true;
    for (uint8_t i = 0; i < playSnakeLen; i++) {
      if (playSnakeX[i] == playFoodX && playSnakeY[i] == playFoodY) {
        validFood = false;
        break;
      }
    }
    attempts++;
  }
}

bool movePlaySnake() {
  playSnakeDirX = playSnakeNextDirX;
  playSnakeDirY = playSnakeNextDirY;

  int8_t newHeadX = playSnakeX[0] + playSnakeDirX;
  int8_t newHeadY = playSnakeY[0] + playSnakeDirY;

  if (newHeadX < 0) {
    newHeadX = 31;
  } else if (newHeadX >= 32) {
    newHeadX = 0;
  }
  if (newHeadY < 0) {
    newHeadY = 15;
  } else if (newHeadY >= 16) {
    newHeadY = 0;
  }

  for (uint8_t i = 0; i < playSnakeLen; i++) {
    if (playSnakeX[i] == newHeadX && playSnakeY[i] == newHeadY) {
      playSnakeGameOver = true;
      buzzer.playError();
      return false;
    }
  }

  bool ateFood = (newHeadX == playFoodX && newHeadY == playFoodY);
  if (ateFood) {
    if (playSnakeLen < PLAY_SNAKE_MAX_LEN) {
      for (int i = playSnakeLen; i > 0; i--) {
        playSnakeX[i] = playSnakeX[i - 1];
        playSnakeY[i] = playSnakeY[i - 1];
      }
      playSnakeLen++;
      playSnakeScore += 10;
      if (playSnakeSpeed > 50) {
        playSnakeSpeed -= 5;
      }
      buzzer.playMenuNav();
      placeNewFood();
    }
  } else {
    for (int i = playSnakeLen - 1; i > 0; i--) {
      playSnakeX[i] = playSnakeX[i - 1];
      playSnakeY[i] = playSnakeY[i - 1];
    }
  }

  playSnakeX[0] = newHeadX;
  playSnakeY[0] = newHeadY;
  return true;
}

void renderPlaySnake() {
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);

  for (uint8_t i = 0; i < playSnakeLen; i++) {
    int16_t px = playSnakeX[i] * 4;
    int16_t py = playSnakeY[i] * 4;
    if (i == 0) {
      u8g2.drawBox(px, py, 4, 4);
    } else {
      u8g2.drawBox(px + 1, py + 1, 2, 2);
    }
  }

  if ((millis() / 200) % 2 == 0) {
    u8g2.drawBox(playFoodX * 4, playFoodY * 4, 4, 4);
  } else {
    u8g2.drawFrame(playFoodX * 4, playFoodY * 4, 4, 4);
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  char scoreStr[8];
  snprintf(scoreStr, sizeof(scoreStr), "%d", playSnakeScore);
  uint8_t scoreWidth = strlen(scoreStr) * 5;
  u8g2.drawStr(125 - scoreWidth, 8, scoreStr);
  u8g2.sendBuffer();
}

void renderPlaySnakeGameOver() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(32, 25, "GAME OVER");

  u8g2.setFont(u8g2_font_6x10_tr);
  char scoreStr[20];
  snprintf(scoreStr, sizeof(scoreStr), "Score: %d", playSnakeScore);
  u8g2.drawStr(40, 40, scoreStr);

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(20, 55, "A=Restart  B=Exit");
  u8g2.sendBuffer();
}

}  // namespace

void runPlaySnake() {
  playSnakeActive = true;
  initPlaySnake();
  playSnakeLastFrame = millis();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(30, 35, "GET READY!");
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(25, 50, "Use D-pad to move");
  u8g2.drawStr(30, 62, "B = Exit game");
  u8g2.sendBuffer();
  delay(1500);

  bool lastA = false;
  bool lastB = false;
  while (playSnakeActive) {
    uint32_t now = millis();
    playable_games_internal::servicePlayableGameRuntime();

    const controller_state_t& frame = controllerFrameConst(0);
    bool curA = frame.A || frame.START;
    bool curB = frame.B || frame.SELECT;

    if (!playSnakeGameOver) {
      if (frame.PAD_U && playSnakeDirY != 1) {
        playSnakeNextDirX = 0;
        playSnakeNextDirY = -1;
      } else if (frame.PAD_D && playSnakeDirY != -1) {
        playSnakeNextDirX = 0;
        playSnakeNextDirY = 1;
      } else if (frame.PAD_L && playSnakeDirX != 1) {
        playSnakeNextDirX = -1;
        playSnakeNextDirY = 0;
      } else if (frame.PAD_R && playSnakeDirX != -1) {
        playSnakeNextDirX = 1;
        playSnakeNextDirY = 0;
      }
    }

    if (playSnakeGameOver) {
      renderPlaySnakeGameOver();
      if (curA && !lastA) {
        initPlaySnake();
        playSnakeLastFrame = now;
      }
      if (curB && !lastB) {
        playSnakeActive = false;
      }
    } else {
      if (curB && !lastB) {
        playSnakeActive = false;
      }
      if (now - playSnakeLastFrame >= playSnakeSpeed) {
        playSnakeLastFrame = now;
        movePlaySnake();
      }
      renderPlaySnake();
    }

    lastA = curA;
    lastB = curB;
    delay(5);
  }

  playSnakeActive = false;
  playable_games_internal::finishPlayableGame();
}

#endif
