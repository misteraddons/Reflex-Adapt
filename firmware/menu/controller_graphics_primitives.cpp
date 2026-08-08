#include "controller_graphics.h"

void drawDpad(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawFrame(x + 3, y, 3, 9);
  u8g2.drawFrame(x, y + 3, 9, 3);

  if (state & GFX_BTN_UP) u8g2.drawBox(x + 3, y, 3, 3);
  if (state & GFX_BTN_DOWN) u8g2.drawBox(x + 3, y + 6, 3, 3);
  if (state & GFX_BTN_LEFT) u8g2.drawBox(x, y + 3, 3, 3);
  if (state & GFX_BTN_RIGHT) u8g2.drawBox(x + 6, y + 3, 3, 3);

  u8g2.setDrawColor(0);
  u8g2.drawBox(x + 3, y + 3, 3, 3);
  u8g2.setDrawColor(1);
}

void drawFaceButtonsDiamond(int16_t x, int16_t y, uint32_t state) {
  uint8_t r = 2;

  if (state & GFX_BTN_Y) u8g2.drawDisc(x + 5, y, r);
  else u8g2.drawCircle(x + 5, y, r);

  if (state & GFX_BTN_B) u8g2.drawDisc(x + 10, y + 5, r);
  else u8g2.drawCircle(x + 10, y + 5, r);

  if (state & GFX_BTN_A) u8g2.drawDisc(x + 5, y + 10, r);
  else u8g2.drawCircle(x + 5, y + 10, r);

  if (state & GFX_BTN_X) u8g2.drawDisc(x, y + 5, r);
  else u8g2.drawCircle(x, y + 5, r);
}

void drawFaceButtons6(int16_t x, int16_t y, uint32_t state) {
  uint8_t r = 2;

  if (state & GFX_BTN_X) u8g2.drawDisc(x, y, r);
  else u8g2.drawCircle(x, y, r);

  if (state & GFX_BTN_Y) u8g2.drawDisc(x + 6, y, r);
  else u8g2.drawCircle(x + 6, y, r);

  if (state & GFX_BTN_L1) u8g2.drawDisc(x + 12, y, r);
  else u8g2.drawCircle(x + 12, y, r);

  if (state & GFX_BTN_A) u8g2.drawDisc(x, y + 6, r);
  else u8g2.drawCircle(x, y + 6, r);

  if (state & GFX_BTN_B) u8g2.drawDisc(x + 6, y + 6, r);
  else u8g2.drawCircle(x + 6, y + 6, r);

  if (state & GFX_BTN_R1) u8g2.drawDisc(x + 12, y + 6, r);
  else u8g2.drawCircle(x + 12, y + 6, r);
}

void drawFaceButtons2(int16_t x, int16_t y, uint32_t state) {
  uint8_t r = 2;

  if (state & GFX_BTN_B) u8g2.drawDisc(x, y, r);
  else u8g2.drawCircle(x, y, r);

  if (state & GFX_BTN_A) u8g2.drawDisc(x + 7, y, r);
  else u8g2.drawCircle(x + 7, y, r);
}

void drawFaceButtons4(int16_t x, int16_t y, uint32_t state) {
  uint8_t r = 2;

  if (state & GFX_BTN_X) u8g2.drawDisc(x + 5, y, r);
  else u8g2.drawCircle(x + 5, y, r);

  if (state & GFX_BTN_A) u8g2.drawDisc(x + 10, y + 5, r);
  else u8g2.drawCircle(x + 10, y + 5, r);

  if (state & GFX_BTN_B) u8g2.drawDisc(x + 5, y + 10, r);
  else u8g2.drawCircle(x + 5, y + 10, r);

  if (state & GFX_BTN_Y) u8g2.drawDisc(x, y + 5, r);
  else u8g2.drawCircle(x, y + 5, r);
}

void drawShoulderButtons(int16_t x, int16_t y, uint8_t width, uint32_t state, uint32_t l_mask, uint32_t r_mask, uint8_t spacing) {
  if (state & l_mask) u8g2.drawBox(x, y, width, 3);
  else u8g2.drawFrame(x, y, width, 3);

  if (state & r_mask) u8g2.drawBox(x + spacing, y, width, 3);
  else u8g2.drawFrame(x + spacing, y, width, 3);
}

void drawAnalogStick(int16_t cx, int16_t cy, int8_t posX, int8_t posY, bool pressed) {
  u8g2.drawCircle(cx, cy, 5);

  int8_t dotX = (posX * 4) / 128;
  int8_t dotY = (posY * 4) / 128;

  if (pressed) {
    u8g2.drawDisc(cx + dotX, cy + dotY, 2);
  } else {
    u8g2.drawDisc(cx + dotX, cy + dotY, 1);
  }
}

void drawSmallButton(int16_t x, int16_t y, uint32_t state, uint32_t mask) {
  if (state & mask) u8g2.drawBox(x, y, 5, 3);
  else u8g2.drawFrame(x, y, 5, 3);
}
