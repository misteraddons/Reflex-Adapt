#include "../product_config.h"

#include "menu_screensaver_bounce.h"
#include "screensaver_rflx_bitmap.h"

#include <Arduino.h>

#include "../platform/display_runtime_state.h"

#ifndef USE_I2C_DISPLAY

void renderIdleAnimation() {}
void resetBounceScreensaverState() {}

#else

namespace {

// Bouncing logo animation state (using 8.8 fixed-point for smooth sub-pixel movement)
// Velocities are slightly different to create varied bounce patterns.
int16_t logoX_fp = 0;
int16_t logoY_fp = 0;
int16_t logoVX_fp = 180;
int16_t logoVY_fp = 140;
int8_t lastLogoX = 0;
int8_t lastLogoY = 0;
uint32_t lastAnimFrame = 0;

constexpr uint32_t ANIM_FRAME_MS = 13;    // ~75 FPS for smooth motion

bool logoInitialized = false;

}  // namespace

void renderIdleAnimation() {
  uint32_t now = millis();

  if (!logoInitialized) {
    logoInitialized = true;
    lastLogoX = (int8_t)((128 - kRflxScreensaverLogoWidth) / 2);
    lastLogoY = (int8_t)((64 - kRflxScreensaverLogoHeight) / 2);
    logoX_fp = lastLogoX << 8;
    logoY_fp = lastLogoY << 8;

    uint8_t seed = (micros() >> 4) & 0x3F;
    logoVX_fp = 140 + seed;
    logoVY_fp = 200 - seed;
    if (micros() & 0x100) logoVX_fp = -logoVX_fp;
    if (micros() & 0x200) logoVY_fp = -logoVY_fp;

    lastAnimFrame = now;

    u8g2.clearBuffer();
    u8g2.drawXBMP(
      lastLogoX,
      lastLogoY,
      kRflxScreensaverLogoWidth,
      kRflxScreensaverLogoHeight,
      kRflxScreensaverLogoBitmap);
    u8g2.sendBuffer();
  }

  if (now - lastAnimFrame < ANIM_FRAME_MS) {
    return;
  }
  lastAnimFrame = now;

  logoX_fp += logoVX_fp;
  logoY_fp += logoVY_fp;

  int16_t maxX = (128 - kRflxScreensaverLogoWidth) << 8;
  int16_t maxY = (64 - kRflxScreensaverLogoHeight) << 8;

  bool hitX = false;
  bool hitY = false;

  if (logoX_fp <= 0) {
    logoX_fp = 0;
    logoVX_fp = -logoVX_fp;
    logoVY_fp += ((micros() & 0x1F) - 16);
    hitX = true;
  }
  if (logoX_fp >= maxX) {
    logoX_fp = maxX;
    logoVX_fp = -logoVX_fp;
    logoVY_fp += ((micros() & 0x1F) - 16);
    hitX = true;
  }
  if (logoY_fp <= 0) {
    logoY_fp = 0;
    logoVY_fp = -logoVY_fp;
    logoVX_fp += ((micros() & 0x1F) - 16);
    hitY = true;
  }
  if (logoY_fp >= maxY) {
    logoY_fp = maxY;
    logoVY_fp = -logoVY_fp;
    logoVX_fp += ((micros() & 0x1F) - 16);
    hitY = true;
  }

  if (hitX && hitY) {
    // Avoid XOR/invert flashes here. If the screensaver is interrupted mid-flash
    // the next home/menu frame can briefly inherit an inverted buffer.
    u8g2.setDrawColor(1);
  }

  if (logoVX_fp > 0 && logoVX_fp < 80) logoVX_fp = 80;
  if (logoVX_fp < 0 && logoVX_fp > -80) logoVX_fp = -80;
  if (logoVX_fp > 250) logoVX_fp = 250;
  if (logoVX_fp < -250) logoVX_fp = -250;
  if (logoVY_fp > 0 && logoVY_fp < 80) logoVY_fp = 80;
  if (logoVY_fp < 0 && logoVY_fp > -80) logoVY_fp = -80;
  if (logoVY_fp > 250) logoVY_fp = 250;
  if (logoVY_fp < -250) logoVY_fp = -250;

  int8_t newLogoX = logoX_fp >> 8;
  int8_t newLogoY = logoY_fp >> 8;
  if (newLogoX == lastLogoX && newLogoY == lastLogoY) {
    return;
  }

  u8g2.clearBuffer();
  u8g2.drawXBMP(
    newLogoX,
    newLogoY,
    kRflxScreensaverLogoWidth,
    kRflxScreensaverLogoHeight,
    kRflxScreensaverLogoBitmap);
  u8g2.sendBuffer();

  lastLogoX = newLogoX;
  lastLogoY = newLogoY;
}

void resetBounceScreensaverState() {
  logoInitialized = false;
}

#endif
