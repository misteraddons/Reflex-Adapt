#pragma once

#include <stdbool.h>
#include <stdint.h>

enum analog_stick_precision : uint8_t {
  ANALOG_STICK_PRECISION_8 = 0, // common
  ANALOG_STICK_PRECISION_12,    // switch pro
  ANALOG_STICK_PRECISION_16,    // xbox
};

typedef struct __attribute((packed, aligned(1))) {
  union {
    struct __attribute((packed, aligned(1))) {
      //config
      uint8_t HAS_BTN_HOME: 1;
      uint8_t HAS_BTN_SELECT : 1;
      uint8_t HAS_BTN_START : 1;
      uint8_t HAS_ANALOG_STICK_MAIN : 1;
      uint8_t HAS_ANALOG_STICK_AUX : 1;
      uint8_t HAS_ANALOG_TRIGGERS : 1;      //contains analog L2 and R2
      uint8_t HAS_ANALOG_MAIN_BUTTONS : 1;  //contains analog buttons (except L2 and R2)
      uint8_t HAS_ANALOG_DPAD : 1;          //contains analog DPAD
    };
    uint8_t config;
  };

  bool connected;
  // Controller type name for display (null-terminated, max 10 chars + null)
  char controller_type_name[12];

  analog_stick_precision sticks_precision_bits;

  //digital data. buttons with "xbox label"
  union {
    struct __attribute((packed, aligned(1))) {
      uint8_t A : 1;
      uint8_t B : 1;
      uint8_t X : 1;
      uint8_t Y : 1;
      uint8_t L1 : 1; // sega Z, ogXbox white
      uint8_t R1 : 1; // sega C, ogXbox black
      uint8_t L2 : 1;
      uint8_t R2 : 1;

      uint8_t L3 : 1;
      uint8_t R3 : 1;
      uint8_t START : 1;
      uint8_t SELECT : 1;
      uint8_t HOME : 1;
      uint8_t CAPTURE : 1;
      uint16_t EXTRA : 14;

      uint8_t PAD_U : 1;
      uint8_t PAD_D : 1;
      uint8_t PAD_L : 1;
      uint8_t PAD_R : 1;
    };
    struct __attribute((packed, aligned(1))) {
      uint32_t digital_buttons;
    };
  };

  union {
    struct __attribute((packed, aligned(1))) {
      //analog axis
      int16_t LX;
      int16_t LY;
      int16_t RX;
      int16_t RY;
    };
    uint64_t analog_sticks;
  };

  union {
    struct __attribute((packed, aligned(1))) {
      //analog buttons
      uint8_t ANALOG_L2;
      uint8_t ANALOG_R2;
      uint8_t ANALOG_A;
      uint8_t ANALOG_B;
      uint8_t ANALOG_X;
      uint8_t ANALOG_Y;
      uint8_t ANALOG_L1;
      uint8_t ANALOG_R1;
    };
    uint64_t analog_buttons;
  };

  union {
    struct __attribute((packed, aligned(1))) {
      //analog dpad
      uint8_t ANALOG_PAD_U;
      uint8_t ANALOG_PAD_D;
      uint8_t ANALOG_PAD_L;
      uint8_t ANALOG_PAD_R;
    };
    uint32_t analog_pad;
  };

  //todo add paddle, spinner and mouse?
  //never apply deadzone to those
  uint8_t paddle;
  int8_t spinner;
  int8_t mouse_x;
  int8_t mouse_y;
  int8_t mouse_wheel_x;
  int8_t mouse_wheel_y;
} controller_state_t;
