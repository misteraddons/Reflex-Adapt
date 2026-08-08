#include "../product_config.h"

#include "menu_screensaver_auto_snake.h"

#include <Arduino.h>
#include <cstdio>

#include "../platform/display_runtime_state.h"

#ifndef USE_I2C_DISPLAY

void renderAutoSnake() {}
void resetAutoSnakeScreensaverState() {}

#else

namespace {

constexpr uint8_t SNAKE_MAX_LEN = 30;
int8_t snakeX[SNAKE_MAX_LEN];
int8_t snakeY[SNAKE_MAX_LEN];
uint8_t snakeLen = 5;
int8_t snakeDirX = 1;
int8_t snakeDirY = 0;
int8_t foodX = 0;
int8_t foodY = 0;
bool snakeInitialized = false;

void initAutoSnake() {
  snakeLen = 5;
  for (uint8_t i = 0; i < snakeLen; i++) {
    snakeX[i] = 16 - i;
    snakeY[i] = 8;
  }
  snakeDirX = 1;
  snakeDirY = 0;
  foodX = 20 + (micros() % 8);
  foodY = 2 + (micros() % 12);
  snakeInitialized = true;
}

}  // namespace

void renderAutoSnake() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();

  if (!snakeInitialized) {
    initAutoSnake();
    lastFrame = now;
  }

  if (now - lastFrame < 100) {
    return;
  }
  lastFrame = now;

  int8_t headX = snakeX[0];
  int8_t headY = snakeY[0];

  int8_t bestDirX = snakeDirX;
  int8_t bestDirY = snakeDirY;

  int8_t dx = (foodX > headX) ? 1 : (foodX < headX) ? -1 : 0;
  int8_t dy = (foodY > headY) ? 1 : (foodY < headY) ? -1 : 0;

  int8_t moves[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
  int8_t bestScore = -100;

  for (uint8_t m = 0; m < 4; m++) {
    int8_t mx = moves[m][0];
    int8_t my = moves[m][1];

    if (mx == -snakeDirX && my == -snakeDirY && (snakeDirX != 0 || snakeDirY != 0)) continue;

    int8_t newX = headX + mx;
    int8_t newY = headY + my;

    if (newX < 0 || newX >= 32 || newY < 0 || newY >= 16) continue;

    bool hitSelf = false;
    for (uint8_t s = 0; s < snakeLen - 1; s++) {
      if (snakeX[s] == newX && snakeY[s] == newY) {
        hitSelf = true;
        break;
      }
    }
    if (hitSelf) continue;

    int8_t score = 0;
    if (mx == dx && dx != 0) score += 2;
    if (my == dy && dy != 0) score += 2;
    if ((now + m) % 7 == 0) score += 1;

    if (score > bestScore) {
      bestScore = score;
      bestDirX = mx;
      bestDirY = my;
    }
  }

  if (bestScore < 0) {
    for (uint8_t m = 0; m < 4; m++) {
      int8_t mx = moves[m][0];
      int8_t my = moves[m][1];
      int8_t newX = headX + mx;
      int8_t newY = headY + my;
      if (newX >= 0 && newX < 32 && newY >= 0 && newY < 16) {
        bestDirX = mx;
        bestDirY = my;
        break;
      }
    }
  }

  snakeDirX = bestDirX;
  snakeDirY = bestDirY;

  int8_t newHeadX = headX + snakeDirX;
  int8_t newHeadY = headY + snakeDirY;

  if (newHeadX < 0) newHeadX = 31;
  if (newHeadX >= 32) newHeadX = 0;
  if (newHeadY < 0) newHeadY = 15;
  if (newHeadY >= 16) newHeadY = 0;

  bool ate = (newHeadX == foodX && newHeadY == foodY);
  if (!ate) {
    for (int8_t i = snakeLen - 1; i > 0; i--) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }
  } else {
    if (snakeLen < SNAKE_MAX_LEN) {
      for (int8_t i = snakeLen; i > 0; i--) {
        snakeX[i] = snakeX[i - 1];
        snakeY[i] = snakeY[i - 1];
      }
      snakeLen++;
    } else {
      for (int8_t i = snakeLen - 1; i > 0; i--) {
        snakeX[i] = snakeX[i - 1];
        snakeY[i] = snakeY[i - 1];
      }
    }
    do {
      foodX = (micros() >> 2) % 32;
      foodY = (micros() >> 4) % 16;
    } while (foodX == newHeadX && foodY == newHeadY);
  }

  snakeX[0] = newHeadX;
  snakeY[0] = newHeadY;

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);

  if ((now / 200) % 2 == 0) {
    u8g2.drawBox(foodX * 4, foodY * 4, 4, 4);
  } else {
    u8g2.drawFrame(foodX * 4, foodY * 4, 4, 4);
  }

  for (uint8_t i = 0; i < snakeLen; i++) {
    if (i == 0) {
      u8g2.drawBox(snakeX[i] * 4, snakeY[i] * 4, 4, 4);
    } else {
      u8g2.drawBox(snakeX[i] * 4 + 1, snakeY[i] * 4 + 1, 2, 2);
    }
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  char scoreStr[8];
  snprintf(scoreStr, sizeof(scoreStr), "%d", snakeLen);
  u8g2.drawStr(118, 10, scoreStr);

  u8g2.sendBuffer();
}

void resetAutoSnakeScreensaverState() {
  snakeInitialized = false;
}

#endif
