#include "controller_graphics.h"

void drawControllerPSX(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry, bool l3, bool r3) {
  u8g2.drawRFrame(x + 5, y + 5, 40, 16, 3);
  u8g2.drawRFrame(x, y + 12, 8, 14, 2);
  u8g2.drawRFrame(x + 42, y + 12, 8, 14, 2);

  drawShoulderButtons(x + 5, y + 2, 8, state, GFX_BTN_L1, GFX_BTN_R1, 32);

  if (state & GFX_BTN_L2) u8g2.drawBox(x + 5, y, 5, 2);
  else u8g2.drawFrame(x + 5, y, 5, 2);
  if (state & GFX_BTN_R2) u8g2.drawBox(x + 39, y, 5, 2);
  else u8g2.drawFrame(x + 39, y, 5, 2);

  drawDpad(x + 7, y + 8, state);
  drawFaceButtonsDiamond(x + 34, y + 7, state);
  drawSmallButton(x + 18, y + 4, state, GFX_BTN_SELECT);
  drawSmallButton(x + 27, y + 6, state, GFX_BTN_START);

  if (lx != 0 || ly != 0 || rx != 0 || ry != 0 || l3 || r3) {
    drawAnalogStick(x + 16, y + 20, lx, ly, l3);
    drawAnalogStick(x + 34, y + 20, rx, ry, r3);
  }
}

void drawControllerSNES(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawRFrame(x, y + 4, 50, 16, 6);
  drawShoulderButtons(x + 4, y + 1, 10, state, GFX_BTN_L1, GFX_BTN_R1, 32);
  drawDpad(x + 5, y + 8, state);
  drawSmallButton(x + 18, y + 11, state, GFX_BTN_SELECT);
  drawSmallButton(x + 27, y + 11, state, GFX_BTN_START);
  drawFaceButtons4(x + 35, y + 6, state);
}

void drawControllerNES(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawRFrame(x, y + 6, 50, 14, 2);
  drawDpad(x + 5, y + 9, state);
  drawFaceButtons2(x + 37, y + 12, state);
  drawSmallButton(x + 18, y + 11, state, GFX_BTN_SELECT);
  drawSmallButton(x + 27, y + 11, state, GFX_BTN_START);
}

void drawControllerN64(int16_t x, int16_t y, uint32_t state, int8_t ax, int8_t ay) {
  u8g2.drawRFrame(x + 19, y, 12, 10, 2);
  u8g2.drawRFrame(x + 2, y + 8, 14, 16, 2);
  u8g2.drawRFrame(x + 34, y + 8, 14, 16, 2);
  u8g2.drawRFrame(x + 18, y + 10, 14, 16, 2);

  if (state & GFX_BTN_L1) u8g2.drawBox(x + 8, y, 8, 2);
  else u8g2.drawFrame(x + 8, y, 8, 2);

  if (state & GFX_BTN_R1) u8g2.drawBox(x + 34, y, 8, 2);
  else u8g2.drawFrame(x + 34, y, 8, 2);

  if (state & GFX_BTN_L2) u8g2.drawBox(x + 22, y + 13, 6, 2);
  else u8g2.drawFrame(x + 22, y + 13, 6, 2);

  drawDpad(x + 4, y + 12, state);
  drawAnalogStick(x + 25, y + 20, ax, ay, false);

  uint8_t cx = x + 37;
  uint8_t cy = y + 12;
  uint8_t r = 1;
  if (state & GFX_BTN_Y) u8g2.drawDisc(cx + 3, cy, r);
  else u8g2.drawCircle(cx + 3, cy, r);
  if (state & GFX_BTN_A) u8g2.drawDisc(cx + 3, cy + 6, r);
  else u8g2.drawCircle(cx + 3, cy + 6, r);
  if (state & GFX_BTN_X) u8g2.drawDisc(cx, cy + 3, r);
  else u8g2.drawCircle(cx, cy + 3, r);
  if (state & GFX_BTN_B) u8g2.drawDisc(cx + 6, cy + 3, r);
  else u8g2.drawCircle(cx + 6, cy + 3, r);

  if (state & GFX_BTN_A) u8g2.drawDisc(x + 43, y + 12, 2);
  else u8g2.drawCircle(x + 43, y + 12, 2);
  if (state & GFX_BTN_B) u8g2.drawDisc(x + 37, y + 10, 1);
  else u8g2.drawCircle(x + 37, y + 10, 1);

  drawSmallButton(x + 22, y + 6, state, GFX_BTN_START);
}

void drawControllerGC(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry) {
  u8g2.drawRFrame(x + 4, y + 4, 42, 18, 4);
  u8g2.drawRFrame(x, y + 12, 8, 12, 2);
  u8g2.drawRFrame(x + 42, y + 12, 8, 12, 2);

  if (state & GFX_BTN_L2) u8g2.drawBox(x + 5, y + 1, 10, 3);
  else u8g2.drawFrame(x + 5, y + 1, 10, 3);
  if (state & GFX_BTN_R2) u8g2.drawBox(x + 35, y + 1, 10, 3);
  else u8g2.drawFrame(x + 35, y + 1, 10, 3);

  if (state & GFX_BTN_R1) u8g2.drawBox(x + 38, y + 5, 6, 2);
  else u8g2.drawFrame(x + 38, y + 5, 6, 2);

  drawAnalogStick(x + 13, y + 13, lx, ly, false);
  drawDpad(x + 7, y + 17, state);
  drawAnalogStick(x + 28, y + 18, rx, ry, false);

  if (state & GFX_BTN_A) u8g2.drawDisc(x + 38, y + 14, 3);
  else u8g2.drawCircle(x + 38, y + 14, 3);
  if (state & GFX_BTN_B) u8g2.drawDisc(x + 32, y + 12, 1);
  else u8g2.drawCircle(x + 32, y + 12, 1);
  if (state & GFX_BTN_X) u8g2.drawDisc(x + 42, y + 9, 2);
  else u8g2.drawCircle(x + 42, y + 9, 2);
  if (state & GFX_BTN_Y) u8g2.drawDisc(x + 32, y + 17, 2);
  else u8g2.drawCircle(x + 32, y + 17, 2);

  drawSmallButton(x + 22, y + 10, state, GFX_BTN_START);
}

void drawControllerSaturn(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawRFrame(x, y + 4, 50, 16, 5);
  drawShoulderButtons(x + 4, y + 1, 8, state, GFX_BTN_L2, GFX_BTN_R2, 34);
  drawDpad(x + 5, y + 8, state);
  drawFaceButtons6(x + 32, y + 7, state);
  drawSmallButton(x + 22, y + 11, state, GFX_BTN_START);
}

void drawControllerPCE(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawRFrame(x, y + 6, 50, 14, 3);
  drawDpad(x + 5, y + 9, state);
  drawFaceButtons2(x + 37, y + 12, state);
  drawSmallButton(x + 18, y + 11, state, GFX_BTN_SELECT);
  drawSmallButton(x + 27, y + 11, state, GFX_BTN_START);
}

void drawControllerNeoGeo(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawCircle(x + 10, y + 12, 8);
  int8_t bx = 0;
  int8_t by = 0;
  if (state & GFX_BTN_LEFT) bx = -3;
  if (state & GFX_BTN_RIGHT) bx = 3;
  if (state & GFX_BTN_UP) by = -3;
  if (state & GFX_BTN_DOWN) by = 3;
  u8g2.drawDisc(x + 10 + bx, y + 12 + by, 2);
  drawSmallButton(x + 22, y + 18, state, GFX_BTN_SELECT);
  drawSmallButton(x + 30, y + 18, state, GFX_BTN_START);
  uint8_t r = 2;
  if (state & GFX_BTN_A) u8g2.drawDisc(x + 28, y + 10, r);
  else u8g2.drawCircle(x + 28, y + 10, r);
  if (state & GFX_BTN_B) u8g2.drawDisc(x + 34, y + 10, r);
  else u8g2.drawCircle(x + 34, y + 10, r);
  if (state & GFX_BTN_X) u8g2.drawDisc(x + 40, y + 10, r);
  else u8g2.drawCircle(x + 40, y + 10, r);
  if (state & GFX_BTN_Y) u8g2.drawDisc(x + 46, y + 10, r);
  else u8g2.drawCircle(x + 46, y + 10, r);
}

void drawController3DO(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawRFrame(x, y + 4, 50, 16, 5);
  drawShoulderButtons(x + 4, y + 2, 10, state, GFX_BTN_L1, GFX_BTN_R1, 32);
  drawDpad(x + 5, y + 8, state);
  drawSmallButton(x + 18, y + 11, state, GFX_BTN_SELECT);
  drawSmallButton(x + 25, y + 11, state, GFX_BTN_START);
  uint8_t r = 2;
  if (state & GFX_BTN_A) u8g2.drawDisc(x + 35, y + 12, r);
  else u8g2.drawCircle(x + 35, y + 12, r);
  if (state & GFX_BTN_B) u8g2.drawDisc(x + 41, y + 12, r);
  else u8g2.drawCircle(x + 41, y + 12, r);
  if (state & GFX_BTN_X) u8g2.drawDisc(x + 47, y + 12, r);
  else u8g2.drawCircle(x + 47, y + 12, r);
}

void drawControllerJaguar(int16_t x, int16_t y, uint32_t state) {
  u8g2.drawRFrame(x, y + 4, 22, 16, 3);
  drawDpad(x + 6, y + 8, state);
  u8g2.drawRFrame(x + 26, y + 4, 24, 20, 2);
  if (state & GFX_BTN_A) u8g2.drawDisc(x + 38, y + 18, 2);
  else u8g2.drawCircle(x + 38, y + 18, 2);
  if (state & GFX_BTN_B) u8g2.drawDisc(x + 42, y + 13, 2);
  else u8g2.drawCircle(x + 42, y + 13, 2);
  if (state & GFX_BTN_X) u8g2.drawDisc(x + 46, y + 8, 2);
  else u8g2.drawCircle(x + 46, y + 8, 2);
  drawSmallButton(x + 36, y + 4, state, GFX_BTN_START);
  drawSmallButton(x + 44, y + 4, state, GFX_BTN_SELECT);
}

void drawControllerWiiClassic(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry) {
  u8g2.drawRFrame(x + 2, y + 4, 46, 18, 6);
  if (state & GFX_BTN_L1) u8g2.drawBox(x + 4, y + 1, 8, 3);
  else u8g2.drawFrame(x + 4, y + 1, 8, 3);
  if (state & GFX_BTN_L2) u8g2.drawBox(x + 4, y, 6, 1);
  else u8g2.drawFrame(x + 4, y, 6, 1);
  if (state & GFX_BTN_R1) u8g2.drawBox(x + 38, y + 1, 8, 3);
  else u8g2.drawFrame(x + 38, y + 1, 8, 3);
  if (state & GFX_BTN_R2) u8g2.drawBox(x + 40, y, 6, 1);
  else u8g2.drawFrame(x + 40, y, 6, 1);
  drawAnalogStick(x + 13, y + 13, lx, ly, false);
  drawDpad(x + 7, y + 17, state);
  drawFaceButtonsDiamond(x + 35, y + 8, state);
  drawAnalogStick(x + 31, y + 18, rx, ry, false);
  drawSmallButton(x + 18, y + 8, state, GFX_BTN_SELECT);
  if (state & GFX_BTN_HOME) u8g2.drawDisc(x + 25, y + 12, 2);
  else u8g2.drawCircle(x + 25, y + 12, 2);
  drawSmallButton(x + 29, y + 8, state, GFX_BTN_START);
}

void drawControllerDreamcast(int16_t x, int16_t y, uint32_t state, int8_t ax, int8_t ay) {
  u8g2.drawRFrame(x + 4, y + 4, 42, 20, 4);
  if (state & GFX_BTN_L1) u8g2.drawBox(x + 8, y + 1, 10, 3);
  else u8g2.drawFrame(x + 8, y + 1, 10, 3);
  if (state & GFX_BTN_R1) u8g2.drawBox(x + 32, y + 1, 10, 3);
  else u8g2.drawFrame(x + 32, y + 1, 10, 3);
  drawDpad(x + 7, y + 10, state);
  drawAnalogStick(x + 21, y + 16, ax, ay, false);
  drawFaceButtons4(x + 34, y + 8, state);
  drawSmallButton(x + 22, y + 8, state, GFX_BTN_START);
}

void drawControllerGenericHID(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry) {
  u8g2.drawRFrame(x + 4, y + 4, 42, 18, 4);
  drawShoulderButtons(x + 5, y + 1, 8, state, GFX_BTN_L1, GFX_BTN_R1, 32);
  drawAnalogStick(x + 12, y + 12, lx, ly, state & GFX_BTN_L3);
  drawAnalogStick(x + 38, y + 12, rx, ry, state & GFX_BTN_R3);
  drawDpad(x + 5, y + 8, state);
  drawFaceButtonsDiamond(x + 34, y + 7, state);
  drawSmallButton(x + 18, y + 10, state, GFX_BTN_SELECT);
  drawSmallButton(x + 27, y + 10, state, GFX_BTN_START);
}

void drawControllerXbox(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry) {
  u8g2.drawRFrame(x + 4, y + 4, 42, 18, 4);
  if (state & GFX_BTN_L1) u8g2.drawBox(x + 6, y, 10, 3);
  else u8g2.drawFrame(x + 6, y, 10, 3);
  if (state & GFX_BTN_R1) u8g2.drawBox(x + 34, y, 10, 3);
  else u8g2.drawFrame(x + 34, y, 10, 3);
  drawAnalogStick(x + 12, y + 10, lx, ly, state & GFX_BTN_L3);
  drawDpad(x + 5, y + 8, state);
  drawFaceButtonsDiamond(x + 34, y + 7, state);
  drawAnalogStick(x + 38, y + 16, rx, ry, state & GFX_BTN_R3);
  drawSmallButton(x + 18, y + 10, state, GFX_BTN_SELECT);
  drawSmallButton(x + 27, y + 10, state, GFX_BTN_START);
  if (state & GFX_BTN_HOME) u8g2.drawDisc(x + 25, y + 6, 2);
  else u8g2.drawCircle(x + 25, y + 6, 2);
}

void drawControllerSwitchPro(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry) {
  u8g2.drawRFrame(x + 2, y + 4, 46, 18, 4);
  u8g2.drawRFrame(x, y + 12, 6, 14, 2);
  u8g2.drawRFrame(x + 44, y + 12, 6, 14, 2);

  if (state & GFX_BTN_L2) u8g2.drawBox(x + 4, y, 8, 2);
  if (state & GFX_BTN_R2) u8g2.drawBox(x + 38, y, 8, 2);

  if (state & GFX_BTN_L1) u8g2.drawBox(x + 4, y + 2, 10, 2);
  else u8g2.drawFrame(x + 4, y + 2, 10, 2);
  if (state & GFX_BTN_R1) u8g2.drawBox(x + 36, y + 2, 10, 2);
  else u8g2.drawFrame(x + 36, y + 2, 10, 2);

  drawAnalogStick(x + 12, y + 10, lx, ly, state & GFX_BTN_L3);
  drawDpad(x + 6, y + 16, state);

  uint8_t r = 2;
  if (state & GFX_BTN_B) u8g2.drawDisc(x + 38, y + 16, r);
  else u8g2.drawCircle(x + 38, y + 16, r);
  if (state & GFX_BTN_A) u8g2.drawDisc(x + 43, y + 11, r);
  else u8g2.drawCircle(x + 43, y + 11, r);
  if (state & GFX_BTN_Y) u8g2.drawDisc(x + 33, y + 11, r);
  else u8g2.drawCircle(x + 33, y + 11, r);
  if (state & GFX_BTN_X) u8g2.drawDisc(x + 38, y + 6, r);
  else u8g2.drawCircle(x + 38, y + 6, r);

  drawAnalogStick(x + 28, y + 18, rx, ry, state & GFX_BTN_R3);

  drawSmallButton(x + 16, y + 8, state, GFX_BTN_SELECT);
  if (state & GFX_BTN_HOME) u8g2.drawDisc(x + 24, y + 14, 2);
  else u8g2.drawCircle(x + 24, y + 14, 2);
  drawSmallButton(x + 29, y + 8, state, GFX_BTN_START);
}
