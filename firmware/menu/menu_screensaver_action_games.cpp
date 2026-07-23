#include "../product_config.h"

#include "menu_screensaver_action_games.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>

#include "../platform/display_runtime_state.h"

#ifndef USE_I2C_DISPLAY

void renderPong() {}
void renderFireworks() {}
void renderBreakout() {}
void resetActionGameScreensaverState() {}

#else

namespace {

constexpr uint32_t ACTION_FRAME_MS = 25;

bool pongInitialized = false;
int16_t pongBallX = 0;
int16_t pongBallY = 0;
int8_t pongBallVX = 0;
int8_t pongBallVY = 0;
int8_t pongPaddle1Y = 0;
int8_t pongPaddle2Y = 0;
uint8_t pongScore1 = 0;
uint8_t pongScore2 = 0;

bool fireworksInitialized = false;
struct Particle { int16_t x, y, vx, vy; uint8_t life; };
Particle fwParticles[32];
uint8_t fwActiveCount = 0;

bool breakoutInitialized = false;
int16_t brBallX = 0;
int16_t brBallY = 0;
int16_t brBallVX = 0;
int16_t brBallVY = 0;
int8_t brPaddleX = 0;
uint8_t brBricks[13];

void initPong() {
  pongBallX = 64 << 4;
  pongBallY = 32 << 4;
  pongBallVX = (micros() & 1) ? 20 : -20;
  pongBallVY = ((micros() >> 2) % 21) - 10;
  pongPaddle1Y = 24;
  pongPaddle2Y = 24;
  pongScore1 = 0;
  pongScore2 = 0;
  pongInitialized = true;
}

void initFireworks() {
  fwActiveCount = 0;
  fireworksInitialized = true;
}

void initBreakout() {
  brBallX = 64 << 4;
  brBallY = 50 << 4;
  brBallVX = (micros() & 1) ? 16 : -16;
  brBallVY = -20;
  brPaddleX = 54;
  for (int i = 0; i < 13; i++) {
    brBricks[i] = 0x1F;
  }
  breakoutInitialized = true;
}

}  // namespace

void renderPong() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!pongInitialized) {
    initPong();
    lastFrame = now;
  }

  if (now - lastFrame < ACTION_FRAME_MS) {
    return;
  }
  lastFrame = now;

  pongBallX += pongBallVX;
  pongBallY += pongBallVY;

  int16_t bx = pongBallX >> 4;
  int16_t by = pongBallY >> 4;

  if (by <= 2 || by >= 60) pongBallVY = -pongBallVY;

  if (pongPaddle1Y + 8 < by) pongPaddle1Y += 2;
  else if (pongPaddle1Y + 8 > by) pongPaddle1Y -= 2;
  if (pongPaddle2Y + 8 < by) pongPaddle2Y += 2;
  else if (pongPaddle2Y + 8 > by) pongPaddle2Y -= 2;

  if (pongPaddle1Y < 0) pongPaddle1Y = 0;
  if (pongPaddle1Y > 48) pongPaddle1Y = 48;
  if (pongPaddle2Y < 0) pongPaddle2Y = 0;
  if (pongPaddle2Y > 48) pongPaddle2Y = 48;

  if (bx <= 8 && by >= pongPaddle1Y && by <= pongPaddle1Y + 16) {
    pongBallVX = -pongBallVX;
    pongBallVY += ((by - pongPaddle1Y - 8) / 2);
  }
  if (bx >= 118 && by >= pongPaddle2Y && by <= pongPaddle2Y + 16) {
    pongBallVX = -pongBallVX;
    pongBallVY += ((by - pongPaddle2Y - 8) / 2);
  }

  if (bx < 0) { pongScore2++; pongBallX = 64 << 4; pongBallVX = 20; }
  if (bx > 128) { pongScore1++; pongBallX = 64 << 4; pongBallVX = -20; }
  if (pongScore1 > 9 || pongScore2 > 9) { pongScore1 = 0; pongScore2 = 0; }

  u8g2.clearBuffer();
  u8g2.drawVLine(64, 0, 64);
  u8g2.drawBox(4, pongPaddle1Y, 3, 16);
  u8g2.drawBox(121, pongPaddle2Y, 3, 16);
  u8g2.drawBox(bx - 2, by - 2, 4, 4);

  u8g2.setFont(u8g2_font_5x7_tr);
  char s1[4];
  char s2[4];
  snprintf(s1, sizeof(s1), "%d", pongScore1);
  snprintf(s2, sizeof(s2), "%d", pongScore2);
  u8g2.drawStr(50, 10, s1);
  u8g2.drawStr(74, 10, s2);

  u8g2.sendBuffer();
}

void renderFireworks() {
  static uint32_t lastFrame = 0;
  static uint32_t lastLaunch = 0;
  uint32_t now = millis();
  if (!fireworksInitialized) {
    initFireworks();
    lastFrame = now;
    lastLaunch = now;
  }

  if (now - lastFrame < ACTION_FRAME_MS) {
    return;
  }
  lastFrame = now;

  if (now - lastLaunch > 800 && fwActiveCount < 20) {
    lastLaunch = now;
    int cx = 20 + (micros() % 88);
    int cy = 48;
    for (int i = 0; i < 12; i++) {
      if (fwActiveCount >= 32) break;
      int angle = i * 30;
      fwParticles[fwActiveCount].x = cx << 4;
      fwParticles[fwActiveCount].y = cy << 4;
      fwParticles[fwActiveCount].vx = (int16_t)(cos(angle * 3.14159 / 180) * 40);
      fwParticles[fwActiveCount].vy = (int16_t)(sin(angle * 3.14159 / 180) * 40) - 30;
      fwParticles[fwActiveCount].life = 30 + (micros() % 20);
      fwActiveCount++;
    }
  }

  for (int i = 0; i < fwActiveCount; ) {
    fwParticles[i].x += fwParticles[i].vx;
    fwParticles[i].y += fwParticles[i].vy;
    fwParticles[i].vy += 2;
    fwParticles[i].life--;
    if (fwParticles[i].life == 0 || (fwParticles[i].y >> 4) > 64) {
      fwParticles[i] = fwParticles[--fwActiveCount];
    } else {
      i++;
    }
  }

  u8g2.clearBuffer();
  for (int i = 0; i < fwActiveCount; i++) {
    int px = fwParticles[i].x >> 4;
    int py = fwParticles[i].y >> 4;
    if (px >= 0 && px < 128 && py >= 0 && py < 64) {
      u8g2.drawPixel(px, py);
      if (fwParticles[i].life > 15) u8g2.drawPixel(px + 1, py);
    }
  }
  u8g2.sendBuffer();
}

void renderBreakout() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!breakoutInitialized) {
    initBreakout();
    lastFrame = now;
  }

  if (now - lastFrame < ACTION_FRAME_MS) {
    return;
  }
  lastFrame = now;

  brBallX += brBallVX;
  brBallY += brBallVY;
  int bx = brBallX >> 4;
  int by = brBallY >> 4;

  if (bx <= 2 || bx >= 126) brBallVX = -brBallVX;
  if (by <= 2) brBallVY = -brBallVY;

  if (by >= 58 && bx >= brPaddleX && bx <= brPaddleX + 20) {
    brBallVY = -abs(brBallVY);
    brBallVX += (bx - brPaddleX - 10) / 2;
  }

  if (by < 25 && by >= 5) {
    int row = (by - 5) / 4;
    int col = (bx - 6) / 9;
    if (col >= 0 && col < 13 && (brBricks[col] & (1 << row))) {
      brBricks[col] &= ~(1 << row);
      brBallVY = -brBallVY;
    }
  }

  if (brPaddleX + 10 < bx) brPaddleX += 3;
  else if (brPaddleX + 10 > bx) brPaddleX -= 3;
  if (brPaddleX < 0) brPaddleX = 0;
  if (brPaddleX > 108) brPaddleX = 108;

  bool anyBricks = false;
  for (int i = 0; i < 13; i++) {
    if (brBricks[i]) anyBricks = true;
  }
  if (by > 64 || !anyBricks) initBreakout();

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawBox(brPaddleX, 60, 20, 3);
  u8g2.drawBox(bx - 1, by - 1, 3, 3);
  for (int col = 0; col < 13; col++) {
    for (int row = 0; row < 5; row++) {
      if (brBricks[col] & (1 << row)) {
        u8g2.drawBox(6 + col * 9, 5 + row * 4, 8, 3);
      }
    }
  }
  u8g2.sendBuffer();
}

void resetActionGameScreensaverState() {
  pongInitialized = false;
  fireworksInitialized = false;
  breakoutInitialized = false;
}

#endif
