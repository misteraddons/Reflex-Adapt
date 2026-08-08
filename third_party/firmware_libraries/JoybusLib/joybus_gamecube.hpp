#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <pico/platform/sections.h>

#include "joybus_pio.hpp"

typedef enum GCButton {
  GCB_DPAD_LEFT  = 0b0000000000000001,
  GCB_DPAD_RIGHT = 0b0000000000000010,
  GCB_DPAD_DOWN  = 0b0000000000000100,
  GCB_DPAD_UP    = 0b0000000000001000,
  GCB_Z_TRIGGER  = 0b0000000000010000,
  GCB_R_TRIGGER  = 0b0000000000100000,
  GCB_L_TRIGGER  = 0b0000000001000000,
  GCB_ONE_1      = 0b0000000010000000,
  GCB_A_BUTTON   = 0b0000000100000000,
  GCB_B_BUTTON   = 0b0000001000000000,
  GCB_X_BUTTON   = 0b0000010000000000,
  GCB_Y_BUTTON   = 0b0000100000000000,
  GCB_START      = 0b0001000000000000,
  GCB_ZERO_1     = 0b0010000000000000,
  GCB_ZERO_2     = 0b0100000000000000,
  GCB_ZERO_3     = 0b1000000000000000,
} GCButton;

typedef struct __attribute__((packed)) GCControllerState {
  uint16_t buttons;
  uint8_t joystick_x;
  uint8_t joystick_y;
  uint8_t cstick_x;
  uint8_t cstick_y;
  uint8_t analog_l;
  uint8_t analog_r;
  uint8_t reserved[2];
  bool valid;
} GCControllerState;

typedef struct NormalizedGCControllerState {
  uint16_t buttons;
  int joystick_x;
  int joystick_y;
  int cstick_x;
  int cstick_y;
  int analog_l;
  int analog_r;
  bool valid;
} NormalizedGCControllerState;

typedef struct __attribute__((packed)) GCWheelState {
  uint16_t buttons;
  union {
    uint8_t status; // joystick_x
    struct __attribute__((packed)) {
      uint8_t : 1; // always 0??
      uint8_t : 1; // always 1?
      uint8_t power_connected: 1;
      uint8_t pedal_connected: 1;
      uint8_t : 4; // always 0?
    };
  };

  int wheel; // joystick_y
  uint8_t gas_pedal; // cstick_x (optional)
  uint8_t brake_pedal; // cstick_y (optional)
  uint8_t analog_l; // left paddle
  uint8_t analog_r; // right paddle
  bool valid;

//   gc_report_x.stick_x = 6; // power connected
//   gc_report_x.stick_x = 14; // power and pedal connected
} GCWheelState;

#define GC_POLL_FLAG_RUMBLE 0b00000001
#define GC_POLL_FLAG_RUMBLE_BEFORE 0b00000010

// Returns current origin
GCControllerState __not_in_flash_func(joybus_gc_probe_origin)(JoybusPIOInstance instance);

// Recalibrates origin and returns origin
GCControllerState __not_in_flash_func(joybus_gc_recalibrate)(JoybusPIOInstance instance);

// Returns current position
GCControllerState __not_in_flash_func(joybus_gc_short_poll)(JoybusPIOInstance instance,
                                                            uint8_t flags);

// Returns current position
GCControllerState __not_in_flash_func(joybus_gc_wheel_poll)(JoybusPIOInstance instance,
                                                            bool motor, uint8_t ffb);

GCWheelState __not_in_flash_func(gc_parse_wheel)(GCControllerState state);

NormalizedGCControllerState __not_in_flash_func(gc_normalize)(GCControllerState state,
                                                              GCControllerState origin);
