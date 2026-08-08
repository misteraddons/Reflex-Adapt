#include "../product_config.h"

#include "menu_playable_games.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>

#include "../core/controller_frame_state.h"
#include "../input/runtime/input_poll_runtime.h"
#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"
#include "menu_playable_games_internal.h"

void runPlayFlappy() {
#ifndef USE_I2C_DISPLAY
  playable_games_internal::finishPlayableGame();
  return;
#else
  bool gameActive = true;
  int16_t birdY = 32 << 4;
  int8_t birdVY = 0;
  int16_t pipes[3][2];
  for (int i = 0; i < 3; i++) {
    pipes[i][0] = 128 + i * 60;
    pipes[i][1] = 10 + (micros() % 28);
  }
  uint16_t score = 0;
  uint32_t lastFrame = millis();
  bool lastA = false;
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
    if (now - lastFrame >= 40) {
      lastFrame = now;
      bool curA = frame.A || frame.PAD_U;
      if (curA && !lastA) birdVY = -35;
      lastA = curA;

      birdVY += 3;
      birdY += birdVY;
      int by = birdY >> 4;
      if (by < 0) {
        by = 0;
        birdY = 0;
      }
      if (by > 58) {
        by = 58;
        birdY = 58 << 4;
        birdVY = 0;
      }

      bool collision = false;
      for (int i = 0; i < 3; i++) {
        pipes[i][0] -= 1;
        if (pipes[i][0] < -10) {
          pipes[i][0] = 128;
          pipes[i][1] = 10 + (micros() % 28);
          score++;
        }
        if (pipes[i][0] > 0 && pipes[i][0] < 20) {
          if (by < pipes[i][1] || by > pipes[i][1] + 26) collision = true;
        }
      }
      if (collision || by >= 58) {
        birdY = 32 << 4;
        birdVY = 0;
        score = 0;
        for (int i = 0; i < 3; i++) pipes[i][0] = 128 + i * 60;
      }

      u8g2.clearBuffer();
      u8g2.drawBox(10, by, 6, 6);
      for (int i = 0; i < 3; i++) {
        if (pipes[i][0] > -10 && pipes[i][0] < 128) {
          u8g2.drawBox(pipes[i][0], 0, 10, pipes[i][1]);
          u8g2.drawBox(pipes[i][0], pipes[i][1] + 26, 10, 64 - pipes[i][1] - 26);
        }
      }
      u8g2.setFont(u8g2_font_5x7_tr);
      char s[8];
      snprintf(s, sizeof(s), "%d", score);
      u8g2.drawStr(60, 10, s);
      u8g2.sendBuffer();
    }
    delay(5);
  }

  playable_games_internal::finishPlayableGame();
#endif
}

void runPlayDino() {
#ifndef USE_I2C_DISPLAY
  playable_games_internal::finishPlayableGame();
  return;
#else
  bool gameActive = true;
  int8_t dinoY = 48;
  int8_t dinoVY = 0;
  int16_t obstacles[3];
  for (int i = 0; i < 3; i++) obstacles[i] = 128 + i * 70;
  uint16_t score = 0;
  uint8_t speed = 2;
  uint32_t lastFrame = millis();
  bool lastA = false;
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
    if (now - lastFrame >= 40) {
      lastFrame = now;
      bool curA = frame.A || frame.PAD_U;
      if (curA && !lastA && dinoY >= 48) dinoVY = -8;
      lastA = curA;

      dinoVY++;
      dinoY += dinoVY;
      if (dinoY > 48) {
        dinoY = 48;
        dinoVY = 0;
      }

      bool collision = false;
      for (int i = 0; i < 3; i++) {
        obstacles[i] -= speed;
        if (obstacles[i] < -10) {
          obstacles[i] = 128 + (micros() % 40);
          score++;
        }
        if (obstacles[i] > 5 && obstacles[i] < 25 && dinoY > 40) collision = true;
      }
      if (collision) {
        dinoY = 48;
        score = 0;
        for (int i = 0; i < 3; i++) obstacles[i] = 128 + i * 70;
        speed = 2;
      }
      if (score > 0 && score % 10 == 0 && speed < 5) speed++;

      u8g2.clearBuffer();
      u8g2.drawLine(0, 56, 128, 56);
      u8g2.drawBox(10, dinoY, 10, 12);
      for (int i = 0; i < 3; i++) {
        if (obstacles[i] > -10 && obstacles[i] < 128) {
          u8g2.drawBox(obstacles[i], 48, 8, 8);
        }
      }
      u8g2.setFont(u8g2_font_5x7_tr);
      char s[8];
      snprintf(s, sizeof(s), "%d", score);
      u8g2.drawStr(100, 10, s);
      u8g2.sendBuffer();
    }
    delay(5);
  }

  playable_games_internal::finishPlayableGame();
#endif
}

void runPlayInvaders() {
#ifndef USE_I2C_DISPLAY
  playable_games_internal::finishPlayableGame();
  return;
#else
  bool gameActive = true;
  int8_t playerX = 60;
  int16_t bulletY = -1;
  int16_t bulletX = 0;
  uint8_t invaders[5];
  for (int i = 0; i < 5; i++) invaders[i] = 0xFF;
  int8_t invaderX = 0;
  int8_t invaderDir = 1;
  uint8_t invaderY = 5;
  uint16_t score = 0;
  uint32_t lastFrame = millis();
  uint32_t lastInvMove = millis();
  bool lastA = false;
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
    if (now - lastFrame >= 33) {
      lastFrame = now;
      if (frame.PAD_L && playerX > 5) playerX -= 3;
      if (frame.PAD_R && playerX < 115) playerX += 3;

      bool curA = frame.A;
      if (curA && !lastA && bulletY < 0) {
        bulletY = 54;
        bulletX = playerX + 4;
      }
      lastA = curA;

      if (bulletY >= 0) {
        bulletY -= 4;
        int row = (bulletY - invaderY) / 8;
        int col = (bulletX - invaderX) / 14;
        if (row >= 0 && row < 5 && col >= 0 && col < 8 && (invaders[row] & (1 << col))) {
          invaders[row] &= ~(1 << col);
          bulletY = -1;
          score += 10;
        }
        if (bulletY < 0) bulletY = -1;
      }

      if (now - lastInvMove > 700) {
        lastInvMove = now;
        invaderX += invaderDir * 3;
        if (invaderX < 0 || invaderX > 128 - 8 * 14) {
          invaderDir = -invaderDir;
          invaderY += 2;
        }
      }

      bool anyInvaders = false;
      for (int i = 0; i < 5; i++) if (invaders[i]) anyInvaders = true;
      if (!anyInvaders) {
        for (int i = 0; i < 5; i++) invaders[i] = 0xFF;
        invaderX = 0;
        invaderY = 5;
      }
      if (invaderY > 45) {
        for (int i = 0; i < 5; i++) invaders[i] = 0xFF;
        invaderX = 0;
        invaderY = 5;
        score = 0;
      }

      u8g2.clearBuffer();
      u8g2.drawBox(playerX, 56, 10, 6);
      if (bulletY >= 0) u8g2.drawBox(bulletX, bulletY, 2, 4);
      for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
          if (invaders[row] & (1 << col)) {
            u8g2.drawBox(invaderX + col * 14, invaderY + row * 8, 10, 6);
          }
        }
      }
      u8g2.setFont(u8g2_font_5x7_tr);
      char s[8];
      snprintf(s, sizeof(s), "%d", score);
      u8g2.drawStr(5, 10, s);
      u8g2.sendBuffer();
    }
    delay(5);
  }

  playable_games_internal::finishPlayableGame();
#endif
}

void runPlayAsteroids() {
#ifndef USE_I2C_DISPLAY
  playable_games_internal::finishPlayableGame();
  return;
#else
  bool gameActive = true;
  float shipX = 64;
  float shipY = 32;
  float shipAngle = 0;
  float shipVX = 0;
  float shipVY = 0;
  int16_t bulletX = -1;
  int16_t bulletY = 0;
  int16_t bulletVX = 0;
  int16_t bulletVY = 0;
  struct Asteroid { int16_t x, y, vx, vy; uint8_t size; };
  Asteroid asteroids[8];
  for (int i = 0; i < 4; i++) {
    asteroids[i].x = micros() % 128;
    asteroids[i].y = micros() % 64;
    asteroids[i].vx = (micros() % 3) - 1;
    asteroids[i].vy = (micros() % 3) - 1;
    asteroids[i].size = 8;
  }
  for (int i = 4; i < 8; i++) asteroids[i].size = 0;
  uint16_t score = 0;
  uint32_t lastFrame = millis();
  bool lastA = false;
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
    if (now - lastFrame >= 50) {
      lastFrame = now;
      if (frame.PAD_L) shipAngle -= 0.1f;
      if (frame.PAD_R) shipAngle += 0.1f;
      if (frame.PAD_U) {
        shipVX += cos(shipAngle) * 0.2f;
        shipVY += sin(shipAngle) * 0.2f;
      }

      bool curA = frame.A;
      if (curA && !lastA && bulletX < 0) {
        bulletX = shipX;
        bulletY = shipY;
        bulletVX = cos(shipAngle) * 4;
        bulletVY = sin(shipAngle) * 4;
      }
      lastA = curA;

      shipX += shipVX;
      shipY += shipVY;
      shipVX *= 0.98f;
      shipVY *= 0.98f;
      if (shipX < 0) shipX += 128;
      if (shipX >= 128) shipX -= 128;
      if (shipY < 0) shipY += 64;
      if (shipY >= 64) shipY -= 64;

      if (bulletX >= 0) {
        bulletX += bulletVX;
        bulletY += bulletVY;
        if (bulletX < 0 || bulletX >= 128 || bulletY < 0 || bulletY >= 64) bulletX = -1;
      }

      for (int i = 0; i < 8; i++) {
        if (asteroids[i].size > 0) {
          asteroids[i].x += asteroids[i].vx;
          asteroids[i].y += asteroids[i].vy;
          if (asteroids[i].x < 0) asteroids[i].x += 128;
          if (asteroids[i].x >= 128) asteroids[i].x -= 128;
          if (asteroids[i].y < 0) asteroids[i].y += 64;
          if (asteroids[i].y >= 64) asteroids[i].y -= 64;

          if (bulletX >= 0 && abs(bulletX - asteroids[i].x) < asteroids[i].size &&
              abs(bulletY - asteroids[i].y) < asteroids[i].size) {
            if (asteroids[i].size > 4) {
              asteroids[i].size = 4;
              for (int j = 0; j < 8; j++) {
                if (asteroids[j].size == 0) {
                  asteroids[j] = asteroids[i];
                  asteroids[j].vx = -asteroids[i].vx;
                  break;
                }
              }
            } else {
              asteroids[i].size = 0;
            }
            bulletX = -1;
            score += 10;
          }

          if (abs(shipX - asteroids[i].x) < asteroids[i].size + 3 &&
              abs(shipY - asteroids[i].y) < asteroids[i].size + 3) {
            shipX = 64;
            shipY = 32;
            shipVX = 0;
            shipVY = 0;
            score = 0;
          }
        }
      }

      bool anyAsteroids = false;
      for (int i = 0; i < 8; i++) if (asteroids[i].size > 0) anyAsteroids = true;
      if (!anyAsteroids) {
        for (int i = 0; i < 4; i++) {
          asteroids[i].x = micros() % 128;
          asteroids[i].y = micros() % 64;
          asteroids[i].size = 8;
        }
      }

      u8g2.clearBuffer();
      int sx = (int)shipX;
      int sy = (int)shipY;
      u8g2.drawLine(sx + cos(shipAngle) * 5, sy + sin(shipAngle) * 5,
                    sx + cos(shipAngle + 2.5f) * 4, sy + sin(shipAngle + 2.5f) * 4);
      u8g2.drawLine(sx + cos(shipAngle) * 5, sy + sin(shipAngle) * 5,
                    sx + cos(shipAngle - 2.5f) * 4, sy + sin(shipAngle - 2.5f) * 4);
      u8g2.drawLine(sx + cos(shipAngle + 2.5f) * 4, sy + sin(shipAngle + 2.5f) * 4,
                    sx + cos(shipAngle - 2.5f) * 4, sy + sin(shipAngle - 2.5f) * 4);
      if (bulletX >= 0) u8g2.drawPixel(bulletX, bulletY);
      for (int i = 0; i < 8; i++) {
        if (asteroids[i].size > 0) {
          u8g2.drawCircle(asteroids[i].x, asteroids[i].y, asteroids[i].size);
        }
      }
      u8g2.setFont(u8g2_font_5x7_tr);
      char s[8];
      snprintf(s, sizeof(s), "%d", score);
      u8g2.drawStr(5, 10, s);
      u8g2.sendBuffer();
    }
    delay(5);
  }

  playable_games_internal::finishPlayableGame();
#endif
}
