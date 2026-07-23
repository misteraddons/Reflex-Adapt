#pragma once

#include "pad_display.h"

// N64 controller pad layout
// Layout:
//   Row 0:     [L]               [R]       [Z]
//   Row 1:         [^]       [Cup]
//   Row 2:     [<]   [>]  [St] [Cl][Cr]  [B]
//   Row 3:         [v]       [Cdn]       [A]

// N64 button bit positions (from joybus_n64.hpp)
#define N64_DPAD_RIGHT  0x0001
#define N64_DPAD_LEFT   0x0002
#define N64_DPAD_DOWN   0x0004
#define N64_DPAD_UP     0x0008
#define N64_START       0x0010
#define N64_Z_TRIGGER   0x0020
#define N64_B_BUTTON    0x0040
#define N64_A_BUTTON    0x0080
#define N64_C_RIGHT     0x0100
#define N64_C_LEFT      0x0200
#define N64_C_DOWN      0x0400
#define N64_C_UP        0x0800
#define N64_R_TRIGGER   0x1000
#define N64_L_TRIGGER   0x2000

const Pad padN64[] = {
  // D-pad (left side)
  { N64_DPAD_UP,    1, 1*6, PAD_UP_ON, PAD_UP_OFF },
  { N64_DPAD_DOWN,  3, 1*6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { N64_DPAD_LEFT,  2, 0,   PAD_LEFT_ON, PAD_LEFT_OFF },
  { N64_DPAD_RIGHT, 2, 2*6, PAD_RIGHT_ON, PAD_RIGHT_OFF },

  // Shoulder buttons
  { N64_L_TRIGGER,  0, 0*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { N64_R_TRIGGER,  0, 6*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { N64_Z_TRIGGER,  0, 8*6, PAD_RECT_ON, PAD_RECT_OFF },

  // Start
  { N64_START,      2, 4*6, PAD_RECT_ON, PAD_RECT_OFF },

  // C buttons (yellow, arranged like second d-pad)
  { N64_C_UP,       1, 6*6, PAD_FACE_ON, PAD_FACE_OFF },
  { N64_C_DOWN,     3, 6*6, PAD_FACE_ON, PAD_FACE_OFF },
  { N64_C_LEFT,     2, 5*6, PAD_FACE_ON, PAD_FACE_OFF },
  { N64_C_RIGHT,    2, 7*6, PAD_FACE_ON, PAD_FACE_OFF },

  // A and B (right side, B is to the left of A)
  { N64_B_BUTTON,   2, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
  { N64_A_BUTTON,   3, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
};

const uint8_t PAD_N64_COUNT = sizeof(padN64) / sizeof(Pad);

// GameCube controller pad layout
#define GC_DPAD_LEFT   0x0001
#define GC_DPAD_RIGHT  0x0002
#define GC_DPAD_DOWN   0x0004
#define GC_DPAD_UP     0x0008
#define GC_Z_TRIGGER   0x0010
#define GC_R_TRIGGER   0x0020
#define GC_L_TRIGGER   0x0040
#define GC_A_BUTTON    0x0100
#define GC_B_BUTTON    0x0200
#define GC_X_BUTTON    0x0400
#define GC_Y_BUTTON    0x0800
#define GC_START       0x1000

const Pad padGC[] = {
  // D-pad
  { GC_DPAD_UP,    1, 1*6, PAD_UP_ON, PAD_UP_OFF },
  { GC_DPAD_DOWN,  3, 1*6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GC_DPAD_LEFT,  2, 0,   PAD_LEFT_ON, PAD_LEFT_OFF },
  { GC_DPAD_RIGHT, 2, 2*6, PAD_RIGHT_ON, PAD_RIGHT_OFF },

  // Shoulder buttons
  { GC_L_TRIGGER,  0, 1*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GC_R_TRIGGER,  0, 8*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GC_Z_TRIGGER,  0, 7*6, PAD_RECT_ON, PAD_RECT_OFF },

  // Start
  { GC_START,      2, 4*6, PAD_FACE_ON, PAD_FACE_OFF },

  // Face buttons (GC diamond layout)
  { GC_A_BUTTON,   3, 8*6, PAD_FACE_ON, PAD_FACE_OFF },  // A is big/center
  { GC_B_BUTTON,   3, 7*6, PAD_FACE_ON, PAD_FACE_OFF },  // B left of A
  { GC_X_BUTTON,   2, 8*6, PAD_FACE_ON, PAD_FACE_OFF },  // X above A
  { GC_Y_BUTTON,   3, 9*6, PAD_FACE_ON, PAD_FACE_OFF },  // Y right of A
};

const uint8_t PAD_GC_COUNT = sizeof(padGC) / sizeof(Pad);

// SNES controller pad layout
#define SNES_B       0x0001
#define SNES_Y       0x0002
#define SNES_SELECT  0x0004
#define SNES_START   0x0008
#define SNES_UP      0x0010
#define SNES_DOWN    0x0020
#define SNES_LEFT    0x0040
#define SNES_RIGHT   0x0080
#define SNES_A       0x0100
#define SNES_X       0x0200
#define SNES_L       0x0400
#define SNES_R       0x0800

const Pad padSNES[] = {
  // D-pad
  { SNES_UP,     1, 1*6, PAD_UP_ON, PAD_UP_OFF },
  { SNES_DOWN,   3, 1*6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { SNES_LEFT,   2, 0,   PAD_LEFT_ON, PAD_LEFT_OFF },
  { SNES_RIGHT,  2, 2*6, PAD_RIGHT_ON, PAD_RIGHT_OFF },

  // Shoulder buttons
  { SNES_L,      0, 1*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { SNES_R,      0, 8*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },

  // Select/Start
  { SNES_SELECT, 2, 4*6, PAD_RECT_ON, PAD_RECT_OFF },
  { SNES_START,  2, 5*6, PAD_RECT_ON, PAD_RECT_OFF },

  // Face buttons (diamond layout)
  { SNES_A,      2, 9*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SNES_B,      3, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SNES_X,      1, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SNES_Y,      2, 7*6, PAD_FACE_ON, PAD_FACE_OFF },
};

const uint8_t PAD_SNES_COUNT = sizeof(padSNES) / sizeof(Pad);

// PSX controller pad layout (DualShock)
#define PSX_SELECT    0x0001
#define PSX_L3        0x0002
#define PSX_R3        0x0004
#define PSX_START     0x0008
#define PSX_UP        0x0010
#define PSX_RIGHT     0x0020
#define PSX_DOWN      0x0040
#define PSX_LEFT      0x0080
#define PSX_L2        0x0100
#define PSX_R2        0x0200
#define PSX_L1        0x0400
#define PSX_R1        0x0800
#define PSX_TRIANGLE  0x1000
#define PSX_CIRCLE    0x2000
#define PSX_CROSS     0x4000
#define PSX_SQUARE    0x8000
#define PSX_HOME      0x10000

const Pad padPSX[] = {
  // D-pad
  { PSX_UP,       1, 1*6, PAD_UP_ON, PAD_UP_OFF },
  { PSX_DOWN,     3, 1*6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { PSX_LEFT,     2, 0,   PAD_LEFT_ON, PAD_LEFT_OFF },
  { PSX_RIGHT,    2, 2*6, PAD_RIGHT_ON, PAD_RIGHT_OFF },

  // Shoulder buttons
  { PSX_L1,       0, 0*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { PSX_L2,       0, 1*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { PSX_R1,       0, 8*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { PSX_R2,       0, 9*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },

  // Select/Start/L3/R3
  { PSX_SELECT,   2, 3*6, PAD_RECT_ON, PAD_RECT_OFF },
  { PSX_HOME,     3, (4*6)+1, PAD_WIDE_CIRCLE_LEFT_ON, PAD_WIDE_CIRCLE_LEFT_OFF },
  { PSX_START,    2, 6*6, PAD_RECT_ON, PAD_RECT_OFF },
  { PSX_L3,       4, 2*6, PAD_FACE_ON, PAD_FACE_OFF },
  { PSX_R3,       4, 7*6, PAD_FACE_ON, PAD_FACE_OFF },

  // Face buttons (diamond layout)
  { PSX_TRIANGLE, 1, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
  { PSX_CIRCLE,   2, 9*6, PAD_FACE_ON, PAD_FACE_OFF },
  { PSX_CROSS,    3, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
  { PSX_SQUARE,   2, 7*6, PAD_FACE_ON, PAD_FACE_OFF },
};

const uint8_t PAD_PSX_COUNT = sizeof(padPSX) / sizeof(Pad);

// Saturn controller pad layout
#define SATURN_UP     0x0001
#define SATURN_DOWN   0x0002
#define SATURN_LEFT   0x0004
#define SATURN_RIGHT  0x0008
#define SATURN_B      0x0010
#define SATURN_C      0x0020
#define SATURN_A      0x0040
#define SATURN_START  0x0080
#define SATURN_Z      0x0100
#define SATURN_Y      0x0200
#define SATURN_X      0x0400
#define SATURN_R      0x0800
#define SATURN_L      0x1000

const Pad padSaturn[] = {
  // D-pad
  { SATURN_UP,    1, 1*6, PAD_UP_ON, PAD_UP_OFF },
  { SATURN_DOWN,  3, 1*6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { SATURN_LEFT,  2, 0,   PAD_LEFT_ON, PAD_LEFT_OFF },
  { SATURN_RIGHT, 2, 2*6, PAD_RIGHT_ON, PAD_RIGHT_OFF },

  // Shoulder buttons
  { SATURN_L,     0, 1*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { SATURN_R,     0, 8*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },

  // Start
  { SATURN_START, 2, 4*6, PAD_RECT_ON, PAD_RECT_OFF },

  // Face buttons (6-button layout: X Y Z on top, A B C on bottom)
  { SATURN_A,     3, 6*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SATURN_B,     3, 7*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SATURN_C,     3, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SATURN_X,     2, 6*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SATURN_Y,     2, 7*6, PAD_FACE_ON, PAD_FACE_OFF },
  { SATURN_Z,     2, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
};

const uint8_t PAD_SATURN_COUNT = sizeof(padSaturn) / sizeof(Pad);

// Generic 2-button pad layout (for NES, PCE, etc.)
#define GENERIC_UP     0x0001
#define GENERIC_DOWN   0x0002
#define GENERIC_LEFT   0x0004
#define GENERIC_RIGHT  0x0008
#define GENERIC_B1     0x0010
#define GENERIC_B2     0x0020
#define GENERIC_SELECT 0x0040
#define GENERIC_START  0x0080

const Pad padGeneric2[] = {
  // D-pad
  { GENERIC_UP,     1, 1*6, PAD_UP_ON, PAD_UP_OFF },
  { GENERIC_DOWN,   3, 1*6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GENERIC_LEFT,   2, 0,   PAD_LEFT_ON, PAD_LEFT_OFF },
  { GENERIC_RIGHT,  2, 2*6, PAD_RIGHT_ON, PAD_RIGHT_OFF },

  // Select/Start
  { GENERIC_SELECT, 2, 4*6, PAD_RECT_ON, PAD_RECT_OFF },
  { GENERIC_START,  2, 5*6, PAD_RECT_ON, PAD_RECT_OFF },

  // Face buttons
  { GENERIC_B1,     3, 7*6, PAD_FACE_ON, PAD_FACE_OFF },
  { GENERIC_B2,     3, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
};

const uint8_t PAD_GENERIC2_COUNT = sizeof(padGeneric2) / sizeof(Pad);
