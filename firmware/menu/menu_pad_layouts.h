#pragma once

#include <stdint.h>

#include "../core/device_mode.h"

// Pad display character constants (from ReflexPad5x7 font)
#define PAD_UP_ON 34
#define PAD_UP_OFF 35
#define PAD_DOWN_ON 36
#define PAD_DOWN_OFF 37
#define PAD_LEFT_ON 38
#define PAD_LEFT_OFF 39
#define PAD_RIGHT_ON 40
#define PAD_RIGHT_OFF 41
#define PAD_DPAD_CENTER '*'
#define PAD_FACE_ON 59
#define PAD_FACE_OFF 60
#define PAD_SHOULDER_ON 62
#define PAD_SHOULDER_OFF 63
#define PAD_RECT_ON '{'
#define PAD_RECT_OFF '}'
#define PAD_DASH_ON '^'
#define PAD_DASH_OFF '`'
#define PAD_PS_TRIANGLE_ON 131
#define PAD_PS_TRIANGLE_OFF 132
#define PAD_PS_SQUARE_ON 133
#define PAD_PS_SQUARE_OFF 134
#define PAD_WIDE_CIRCLE_LEFT_ON 135
#define PAD_WIDE_CIRCLE_RIGHT_ON 136
#define PAD_WIDE_CIRCLE_LEFT_OFF 137
#define PAD_WIDE_CIRCLE_RIGHT_OFF 138
#define PAD_VERTICAL_RECT_ON 139
#define PAD_VERTICAL_RECT_OFF 140
#define PAD_WIDE_RECT_LEFT_ON 141
#define PAD_WIDE_RECT_RIGHT_ON 142
#define PAD_WIDE_RECT_LEFT_OFF 143
#define PAD_WIDE_RECT_RIGHT_OFF 144
#define PAD_WIDE_TRIANGLE_LEFT_ON 145
#define PAD_WIDE_TRIANGLE_RIGHT_ON 146
#define PAD_WIDE_TRIANGLE_LEFT_OFF 147
#define PAD_WIDE_TRIANGLE_RIGHT_OFF 148
#define PAD_SATURN_START_LEFT_ON 149
#define PAD_SATURN_START_RIGHT_ON 150
#define PAD_SATURN_START_LEFT_OFF 151
#define PAD_SATURN_START_RIGHT_OFF 152

// Generic pad layout bitmasks
#define GPAD_UP      0x0001
#define GPAD_DOWN    0x0002
#define GPAD_LEFT    0x0004
#define GPAD_RIGHT   0x0008
#define GPAD_A       0x0010
#define GPAD_B       0x0020
#define GPAD_X       0x0040
#define GPAD_Y       0x0080
#define GPAD_L1      0x0100
#define GPAD_R1      0x0200
#define GPAD_L2      0x0400
#define GPAD_R2      0x0800
#define GPAD_START   0x1000
#define GPAD_SELECT  0x2000
#define GPAD_L3      0x4000
#define GPAD_R3      0x8000
#define GPAD_HOME    0x10000
#define GPAD_EXTRA0  0x20000
#define GPAD_EXTRA1  0x40000
#define GPAD_EXTRA2  0x80000
#define GPAD_EXTRA3  0x100000
#define GPAD_EXTRA4  0x200000

struct PadButton {
  uint32_t mask;
  uint8_t row;
  uint8_t col;
  char on;
  char off;
};

void drawDpadCentersForLayout(const PadButton* layout, uint8_t count, uint8_t colOffset, uint8_t rowOffset);
void drawDpadCentersForLayoutState(const PadButton* layout, uint8_t count, uint32_t state, uint8_t colOffset, uint8_t rowOffset);
bool isDrivingFallbackActive();
bool getSharedControllerTypePadLayout(const char* typeName, const PadButton** layout, uint8_t* layoutCount);
void getPadLayoutForMode(DeviceEnum mode, const PadButton** layout, uint8_t* count, const char** name);
void getLayoutForPlayer(uint8_t player, const PadButton** layout, uint8_t* layoutCount);
