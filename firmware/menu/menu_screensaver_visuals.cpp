#include "../product_config.h"

#include "menu_screensaver_visuals.h"

#include <Arduino.h>
#include <math.h>

#include "../platform/display_runtime_state.h"

#ifndef USE_I2C_DISPLAY

void renderClock() {}
void renderPlasma() {}
void renderSpirograph() {}
void resetVisualScreensaverState() {}

#else

namespace {

constexpr uint32_t AMBIENT_FRAME_MS = 33;

bool clockInitialized = false;
bool plasmaInitialized = false;
uint8_t plasmaOffset = 0;
uint8_t plasmaSinTable[64];
bool spiroInitialized = false;
float spiroAngle = 0.0f;
int16_t spiroLastX = -1;
int16_t spiroLastY = -1;

void initPlasma() {
  for (int i = 0; i < 64; i++) {
    plasmaSinTable[i] = (uint8_t)(32 + 31 * sin(i * 3.14159 / 32));
  }
  plasmaInitialized = true;
}

}  // namespace

void renderClock() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!clockInitialized) {
    clockInitialized = true;
    lastFrame = now;
  }

  if (now - lastFrame < 1000) {
    return;
  }
  lastFrame = now;

  uint32_t secs = (now / 1000) % 43200;
  uint8_t h = (secs / 3600) % 12;
  uint8_t m = (secs / 60) % 60;
  uint8_t s = secs % 60;

  u8g2.clearBuffer();

  int cx = 64;
  int cy = 32;
  int r = 28;
  u8g2.drawCircle(cx, cy, r);

  for (int i = 0; i < 12; i++) {
    float angle = i * 30 * 3.14159f / 180 - 1.5708f;
    int x1 = cx + cos(angle) * (r - 2);
    int y1 = cy + sin(angle) * (r - 2);
    int x2 = cx + cos(angle) * (r - 5);
    int y2 = cy + sin(angle) * (r - 5);
    u8g2.drawLine(x1, y1, x2, y2);
  }

  float hAngle = (h * 30 + m * 0.5f) * 3.14159f / 180 - 1.5708f;
  u8g2.drawLine(cx, cy, cx + cos(hAngle) * 14, cy + sin(hAngle) * 14);

  float mAngle = m * 6 * 3.14159f / 180 - 1.5708f;
  u8g2.drawLine(cx, cy, cx + cos(mAngle) * 20, cy + sin(mAngle) * 20);

  float sAngle = s * 6 * 3.14159f / 180 - 1.5708f;
  u8g2.drawLine(cx, cy, cx + cos(sAngle) * 24, cy + sin(sAngle) * 24);

  u8g2.sendBuffer();
}

void renderPlasma() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!plasmaInitialized) {
    initPlasma();
    lastFrame = now;
  }

  if (now - lastFrame < AMBIENT_FRAME_MS) {
    return;
  }
  lastFrame = now;
  plasmaOffset++;

  u8g2.clearBuffer();
  for (int y = 0; y < 64; y += 2) {
    for (int x = 0; x < 128; x += 2) {
      uint8_t v1 = plasmaSinTable[(x + plasmaOffset) & 63];
      uint8_t v2 = plasmaSinTable[(y + plasmaOffset * 2) & 63];
      uint8_t v3 = plasmaSinTable[((x + y) / 2 + plasmaOffset) & 63];
      uint8_t val = (v1 + v2 + v3) / 3;
      if (val > 40) {
        u8g2.drawPixel(x, y);
      }
    }
  }
  u8g2.sendBuffer();
}

void renderSpirograph() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!spiroInitialized) {
    spiroInitialized = true;
    spiroAngle = 0.0f;
    spiroLastX = -1;
    u8g2.clearBuffer();
    lastFrame = now;
  }

  if (now - lastFrame < 10) {
    return;
  }
  lastFrame = now;

  float R = 24;
  float r = 7;
  float d = 15;
  float x = 64 + (R - r) * cos(spiroAngle) + d * cos((R - r) / r * spiroAngle);
  float y = 32 + (R - r) * sin(spiroAngle) - d * sin((R - r) / r * spiroAngle);

  if (spiroLastX >= 0) {
    u8g2.drawLine(spiroLastX, spiroLastY, (int)x, (int)y);
  }
  spiroLastX = (int)x;
  spiroLastY = (int)y;

  spiroAngle += 0.05f;
  if (spiroAngle > 62.83f) {
    spiroAngle = 0.0f;
    spiroLastX = -1;
    u8g2.clearBuffer();
  }

  u8g2.sendBuffer();
}

void resetVisualScreensaverState() {
  clockInitialized = false;
  plasmaInitialized = false;
  spiroInitialized = false;
}

#endif
