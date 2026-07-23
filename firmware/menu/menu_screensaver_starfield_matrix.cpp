#include "../product_config.h"

#include "menu_screensaver_starfield_matrix.h"

#include <Arduino.h>

#include "../platform/display_runtime_state.h"

#ifndef USE_I2C_DISPLAY

void renderStarfield() {}
void renderMatrixRain() {}
void resetStarfieldMatrixScreensaverState() {}

#else

namespace {

constexpr uint32_t AMBIENT_FRAME_MS = 33;

constexpr uint8_t NUM_STARS = 40;
int16_t starX[NUM_STARS];
int16_t starY[NUM_STARS];
int8_t starZ[NUM_STARS];
bool starfieldInitialized = false;

constexpr uint8_t MATRIX_COLS = 16;
int8_t matrixY[MATRIX_COLS];
int8_t matrixLen[MATRIX_COLS];
int8_t matrixSpeed[MATRIX_COLS];
uint8_t matrixChar[MATRIX_COLS];
bool matrixInitialized = false;

void initStarfield() {
  uint16_t seed = micros();
  for (uint8_t i = 0; i < NUM_STARS; i++) {
    seed = seed * 1103515245 + 12345;
    starX[i] = (seed % 128) << 8;
    seed = seed * 1103515245 + 12345;
    starY[i] = seed % 64;
    seed = seed * 1103515245 + 12345;
    starZ[i] = 1 + (seed % 8);
  }
  starfieldInitialized = true;
}

void initMatrix() {
  uint16_t seed = micros();
  for (uint8_t i = 0; i < MATRIX_COLS; i++) {
    seed = seed * 1103515245 + 12345;
    matrixY[i] = -(seed % 40);
    seed = seed * 1103515245 + 12345;
    matrixLen[i] = 4 + (seed % 8);
    seed = seed * 1103515245 + 12345;
    matrixSpeed[i] = 1 + (seed % 3);
    matrixChar[i] = 0x30 + (seed % 42);
  }
  matrixInitialized = true;
}

}  // namespace

void renderStarfield() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();

  if (!starfieldInitialized) {
    initStarfield();
    lastFrame = now;
  }

  if (now - lastFrame < 20) {
    return;
  }
  lastFrame = now;

  u8g2.clearBuffer();

  for (uint8_t i = 0; i < NUM_STARS; i++) {
    starX[i] -= starZ[i] * 40;

    if (starX[i] < 0) {
      starX[i] = 127 << 8;
      starY[i] = (micros() + i * 17) % 64;
      starZ[i] = 1 + ((micros() >> 2) + i) % 8;
    }

    int8_t x = starX[i] >> 8;
    if (starZ[i] > 5) {
      u8g2.drawPixel(x, starY[i]);
      u8g2.drawPixel(x + 1, starY[i]);
      u8g2.drawPixel(x, starY[i] + 1);
      u8g2.drawPixel(x + 1, starY[i] + 1);
    } else if (starZ[i] > 2) {
      u8g2.drawPixel(x, starY[i]);
      u8g2.drawPixel(x + 1, starY[i]);
    } else {
      u8g2.drawPixel(x, starY[i]);
    }
  }

  u8g2.sendBuffer();
}

void renderMatrixRain() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();

  if (!matrixInitialized) {
    initMatrix();
    lastFrame = now;
  }

  if (now - lastFrame < AMBIENT_FRAME_MS) {
    return;
  }
  lastFrame = now;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);

  for (uint8_t col = 0; col < MATRIX_COLS; col++) {
    int8_t x = col * 8;

    for (int8_t t = 0; t < matrixLen[col]; t++) {
      int8_t y = matrixY[col] - t * 8;
      if (y >= 0 && y < 64) {
        if (t == 0) {
          char c = matrixChar[col];
          u8g2.drawGlyph(x, y + 7, c);
        } else if (t < matrixLen[col] / 2) {
          char c = 0x30 + ((matrixChar[col] + t * 7) % 42);
          u8g2.drawGlyph(x, y + 7, c);
        } else {
          u8g2.drawPixel(x + 2, y + 3);
        }
      }
    }

    matrixY[col] += matrixSpeed[col];

    if (matrixY[col] - matrixLen[col] * 8 > 64) {
      matrixY[col] = -(micros() % 20);
      matrixLen[col] = 4 + (micros() % 8);
      matrixSpeed[col] = 1 + (micros() % 3);
      matrixChar[col] = 0x30 + (micros() % 42);
    }

    if ((now + col * 100) % 200 < 50) {
      matrixChar[col] = 0x30 + ((micros() + col) % 42);
    }
  }

  u8g2.sendBuffer();
}

void resetStarfieldMatrixScreensaverState() {
  starfieldInitialized = false;
  matrixInitialized = false;
}

#endif
