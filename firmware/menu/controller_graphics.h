#pragma once

// Controller Graphics - Vector drawing functions for controller visualization
// Uses U8g2 library for pixel-level graphics on 128x64 OLED.

#include <Arduino.h>

#include "../core/controller_frame_state.h"
#include "../core/device_mode.h"
#include "../core/controller_state.h"
#include "../output/output_mode.h"
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

#define GFX_BTN_A       0x0001
#define GFX_BTN_B       0x0002
#define GFX_BTN_X       0x0004
#define GFX_BTN_Y       0x0008
#define GFX_BTN_L1      0x0010
#define GFX_BTN_R1      0x0020
#define GFX_BTN_L2      0x0040
#define GFX_BTN_R2      0x0080
#define GFX_BTN_SELECT  0x0100
#define GFX_BTN_START   0x0200
#define GFX_BTN_L3      0x0400
#define GFX_BTN_R3      0x0800
#define GFX_BTN_HOME    0x1000
#define GFX_BTN_UP      0x2000
#define GFX_BTN_DOWN    0x4000
#define GFX_BTN_LEFT    0x8000
#define GFX_BTN_RIGHT   0x10000

enum ControllerGfxType : uint8_t {
  GFX_CTRL_GENERIC = 0,
  GFX_CTRL_PSX,
  GFX_CTRL_SNES,
  GFX_CTRL_NES,
  GFX_CTRL_N64,
  GFX_CTRL_GC,
  GFX_CTRL_SATURN,
  GFX_CTRL_PCE,
  GFX_CTRL_NEOGEO,
  GFX_CTRL_3DO,
  GFX_CTRL_JAGUAR,
  GFX_CTRL_WII,
  GFX_CTRL_DC,
  GFX_CTRL_XBOX,
  GFX_CTRL_SWITCH,
};

uint32_t buildGfxButtonMask(uint8_t player = 0);

void drawDpad(int16_t x, int16_t y, uint32_t state);
void drawFaceButtonsDiamond(int16_t x, int16_t y, uint32_t state);
void drawFaceButtons6(int16_t x, int16_t y, uint32_t state);
void drawFaceButtons2(int16_t x, int16_t y, uint32_t state);
void drawFaceButtons4(int16_t x, int16_t y, uint32_t state);
void drawShoulderButtons(int16_t x, int16_t y, uint8_t width, uint32_t state, uint32_t l_mask, uint32_t r_mask, uint8_t spacing);
void drawAnalogStick(int16_t cx, int16_t cy, int8_t posX, int8_t posY, bool pressed = false);
void drawSmallButton(int16_t x, int16_t y, uint32_t state, uint32_t mask);

void drawControllerPSX(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry, bool l3, bool r3);
void drawControllerSNES(int16_t x, int16_t y, uint32_t state);
void drawControllerNES(int16_t x, int16_t y, uint32_t state);
void drawControllerN64(int16_t x, int16_t y, uint32_t state, int8_t ax, int8_t ay);
void drawControllerGC(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry);
void drawControllerSaturn(int16_t x, int16_t y, uint32_t state);
void drawControllerPCE(int16_t x, int16_t y, uint32_t state);
void drawControllerNeoGeo(int16_t x, int16_t y, uint32_t state);
void drawController3DO(int16_t x, int16_t y, uint32_t state);
void drawControllerJaguar(int16_t x, int16_t y, uint32_t state);
void drawControllerWiiClassic(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry);
void drawControllerDreamcast(int16_t x, int16_t y, uint32_t state, int8_t ax, int8_t ay);
void drawControllerGenericHID(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry);
void drawControllerXbox(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry);
void drawControllerSwitchPro(int16_t x, int16_t y, uint32_t state, int8_t lx, int8_t ly, int8_t rx, int8_t ry);

ControllerGfxType getInputGfxType(DeviceEnum mode);
ControllerGfxType getOutputGfxType(outputMode_t mode);
void drawController(ControllerGfxType type, int16_t x, int16_t y, uint32_t state,
                   int8_t lx = 0, int8_t ly = 0, int8_t rx = 0, int8_t ry = 0,
                   bool l3 = false, bool r3 = false);
