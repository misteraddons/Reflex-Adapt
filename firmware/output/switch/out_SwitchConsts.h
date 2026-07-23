#ifndef SwitchConsts_h
#define SwitchConsts_h

#include "pico/stdlib.h"

typedef struct __attribute((packed, aligned(1))) {
  uint8_t l_calibration[9];
  uint8_t r_calibration[9];
} switch_factory_stick_calibration_t;

const switch_factory_stick_calibration_t switch_factory_stick_calibration[] = {
//  { // N64 input range
//    .l_calibration = {0xee, 0xe4, 0x4e, 0x00, 0x08, 0x80, 0xee, 0xe4, 0x4e},
//    .r_calibration = {0x00, 0x08, 0x80, 0xee, 0xe4, 0x4e, 0xee, 0xe4, 0x4e}
//  }
//  { // GC input range
//    .l_calibration = {0x88, 0x85, 0x58, 0x00, 0x08, 0x80, 0x88, 0x85, 0x58},
//    .r_calibration = {0x00, 0x08, 0x80, 0x88, 0x85, 0x58, 0x88, 0x85, 0x58}
//  },
//  { // pro controller range?
//    .l_calibration = {0xee, 0xe5, 0x5e, 0x00, 0x08, 0x80, 0xee, 0xe5, 0x5e},
//    .r_calibration = {0x00, 0x08, 0x80, 0xee, 0xe5, 0x5e, 0xee, 0xe5, 0x5e}
//  },
  { // full range
    .l_calibration = {0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80},
    .r_calibration = {0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80}
  },


//  {
//    .l_calibration = {0xD4, 0x75, 0x61, 0xE5, 0x87, 0x7C, 0xEC, 0x55, 0x61},
//    .r_calibration = {0x5D, 0xD8, 0x7F, 0x18, 0xE6, 0x61, 0x86, 0x65, 0x5D}
//  },
};
//
//  //limited range. good for using n64 and gc controllers?
//  const uint8_t l_calibration[9] = {0xD4, 0x75, 0x61, 0xE5, 0x87,
//                                    0x7C, 0xEC, 0x55, 0x61};
//  const uint8_t r_calibration[9] = {0x5D, 0xD8, 0x7F, 0x18, 0xE6,
//                                    0x61, 0x86, 0x65, 0x5D};
//  //full range.  
////  const uint8_t l_calibration[9] = {0x00, 0x08, 0x80, 0x00, 0x08,
////                                    0x80, 0x00, 0x08, 0x80};
////  const uint8_t r_calibration[9] = {0x00, 0x08, 0x80, 0x00, 0x08,
////                                    0x80, 0x00, 0x08, 0x80};

//Default
//#define SWITCH_COLOR_BODY 0x323232
//#define SWITCH_COLOR_BUTTONS 0xFFFFFF
//#define SWITCH_COLOR_GRIP_LEFT 0xFFFFFF
//#define SWITCH_COLOR_GRIP_RIGHT 0xFFFFFF

//Splatoon
//#define SWITCH_COLOR_BODY 0x313232
//#define SWITCH_COLOR_BUTTONS 0xFFFFFF
//#define SWITCH_COLOR_GRIP_LEFT 0xFFFFFF
//#define SWITCH_COLOR_GRIP_RIGHT 0xFFFFFF

//Splatoon 3 Pro Controller
//#define SWITCH_COLOR_BODY 0x2D2D2D
//#define SWITCH_COLOR_BUTTONS 0xE6E6E6
//#define SWITCH_COLOR_GRIP_LEFT 0x6455F5
//#define SWITCH_COLOR_GRIP_RIGHT 0xC3FA05

//Super Smash Bros. Ultimate Edition
//#define SWITCH_COLOR_BODY 0xD2D2D2
//#define SWITCH_COLOR_BUTTONS 0xE6E6E6
//#define SWITCH_COLOR_GRIP_LEFT 0xFFFFFF
//#define SWITCH_COLOR_GRIP_RIGHT 0xFFFFFF

//Xenoblade Chronicles 2
//#define SWITCH_COLOR_BODY 0x323132
//#define SWITCH_COLOR_BUTTONS 0xFFFFFF
//#define SWITCH_COLOR_GRIP_LEFT 0xFFFFFF
//#define SWITCH_COLOR_GRIP_RIGHT 0xFFFFFF

//Zelda Tears of the Kingdom
#define SWITCH_COLOR_BODY 0x2D2D2D
#define SWITCH_COLOR_BUTTONS 0xE6E6E6
#define SWITCH_COLOR_GRIP_LEFT 0x464646
#define SWITCH_COLOR_GRIP_RIGHT 0xFFFFFF

const uint8_t switch_colors[] {
  (SWITCH_COLOR_BODY >> 16) & 0xFF, // Body R
  (SWITCH_COLOR_BODY >> 8 ) & 0xFF, // Body G
  (SWITCH_COLOR_BODY      ) & 0xFF, // Body B
  (SWITCH_COLOR_BUTTONS >> 16) & 0xFF, //Buttons R
  (SWITCH_COLOR_BUTTONS >> 8) & 0xFF,  //Buttons G
  (SWITCH_COLOR_BUTTONS     ) & 0xFF,  //Buttons B
  (SWITCH_COLOR_GRIP_LEFT >> 16) & 0xFF, // Grip left R
  (SWITCH_COLOR_GRIP_LEFT >>  8) & 0xFF, // Grip left G
  (SWITCH_COLOR_GRIP_LEFT      ) & 0xFF, // Grip left B
  (SWITCH_COLOR_GRIP_RIGHT >> 16) & 0xFF, // Grip right R
  (SWITCH_COLOR_GRIP_RIGHT >>  8) & 0xFF, // Grip right G
  (SWITCH_COLOR_GRIP_RIGHT      ) & 0xFF, // Grip right B
  0xFF // DesignVariation? moved to spi data
};

//SwitchColors switch_colors {
//  .body_r = 0x0A,
//  .body_g = 0xB9,
//  .body_b = 0xE6,
//  .buttons_r = 0x1E,
//  .buttons_g = 0xDC,
//  .buttons_b = 0x00,
//  .grip_l_r = 0xFF,
//  .grip_l_g = 0xFF,
//  .grip_l_b = 0xFF,
//  .grip_r_r = 0xFF,
//  .grip_r_g = 0xFF,
//  .grip_r_b = 0xFF,
//  .grip_unk = 0xFF
//};

typedef struct __attribute((packed, aligned(1))) {
  uint8_t batteryConnection;
  
  union {
    struct __attribute((packed, aligned(1))) {
      uint8_t y : 1;
      uint8_t x : 1;
      uint8_t b : 1;
      uint8_t a : 1;

      uint8_t : 2;
      uint8_t r : 1;
      uint8_t zr : 1;

      uint8_t minus : 1;
      uint8_t plus : 1;
      uint8_t r3 : 1;
      uint8_t l3 : 1;

      uint8_t home : 1;
      uint8_t capture : 1;
      uint8_t : 2;

      uint8_t pad_d : 1;
      uint8_t pad_u : 1;
      uint8_t pad_r : 1;
      uint8_t pad_l : 1;

      uint8_t : 2;
      uint8_t l : 1;
      uint8_t zl : 1;
    };

    uint8_t buttons[3];
  };
  
  union {
    struct __attribute((packed, aligned(1))) {
      uint16_t lx : 12;
      uint16_t ly : 12;
    };
    uint8_t lstick[3];
  };
  
  union {
    struct __attribute((packed, aligned(1))) {
      uint16_t rx : 12;
      uint16_t ry : 12;
    };
    uint8_t rstick[3];
  };
} SwitchReport;

//BUTTONS
//0: //ZR,R,A,B,X,Y
//1: CAPTURE,HOME,L3,R3,PLUS,MINUS
//2: DPAD
//ANALOG
//0: X & 0xFF
//1: ((Y & 0xF) << 4) | (X >> 8)
//2: Y >> 4

// Button report (3 bytes)
#define SWITCH_MASK_ZR (1U << 7)
#define SWITCH_MASK_R (1U << 6)
#define SWITCH_MASK_A (1U << 3)
#define SWITCH_MASK_B (1U << 2)
#define SWITCH_MASK_X (1U << 1)
#define SWITCH_MASK_Y 1U

#define SWITCH_MASK_CAPTURE (1U << 5)
#define SWITCH_MASK_HOME (1U << 4)
#define SWITCH_MASK_L3 (1U << 3)
#define SWITCH_MASK_R3 (1U << 2)
#define SWITCH_MASK_PLUS (1U << 1)
#define SWITCH_MASK_MINUS 1U

#define SWITCH_MASK_ZL (1U << 7)
#define SWITCH_MASK_L (1U << 6)

// HAT report (4 bits)
#define SWITCH_HAT_UP 0x2
#define SWITCH_HAT_UPRIGHT 0x6
#define SWITCH_HAT_RIGHT 0x4
#define SWITCH_HAT_DOWNRIGHT 0x5
#define SWITCH_HAT_DOWN 0x1
#define SWITCH_HAT_DOWNLEFT 0x9
#define SWITCH_HAT_LEFT 0x8
#define SWITCH_HAT_UPLEFT 0xa
#define SWITCH_HAT_NOTHING 0x0

// Switch analog sticks
#define SWITCH_JOYSTICK_MIN 0x000
#define SWITCH_JOYSTICK_MID 0x7FF
#define SWITCH_JOYSTICK_MAX 0xFFF

#include "output_switch_descriptors_runtime.h"

#endif

