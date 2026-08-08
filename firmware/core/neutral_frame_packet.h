#pragma once

#include <stdint.h>

#include "controller_state.h"

// Shared compact neutral-frame packet for CDC/browser/debug consumers.
// Keep this ordering stable: input overlay, Adapt state stream, and host-side
// viewers all key off these bit positions.

constexpr uint8_t NEUTRAL_FRAME_PACKET_VERSION = 1;
constexpr uint8_t NEUTRAL_FRAME_TYPE_NAME_BYTES = 12;

enum NeutralFrameButtonBit : uint8_t {
  NEUTRAL_BTN_A = 0,
  NEUTRAL_BTN_B,
  NEUTRAL_BTN_X,
  NEUTRAL_BTN_Y,
  NEUTRAL_BTN_L1,
  NEUTRAL_BTN_R1,
  NEUTRAL_BTN_L2,
  NEUTRAL_BTN_R2,
  NEUTRAL_BTN_L3,
  NEUTRAL_BTN_R3,
  NEUTRAL_BTN_START,
  NEUTRAL_BTN_SELECT,
  NEUTRAL_BTN_HOME,
  NEUTRAL_BTN_CAPTURE,
  NEUTRAL_BTN_EXTRA0,
  NEUTRAL_BTN_EXTRA1,
  NEUTRAL_BTN_EXTRA2,
  NEUTRAL_BTN_EXTRA3,
  NEUTRAL_BTN_EXTRA4,
  NEUTRAL_BTN_EXTRA5,
  NEUTRAL_BTN_EXTRA6,
  NEUTRAL_BTN_EXTRA7,
  NEUTRAL_BTN_EXTRA8,
  NEUTRAL_BTN_EXTRA9,
  NEUTRAL_BTN_EXTRA10,
  NEUTRAL_BTN_EXTRA11,
  NEUTRAL_BTN_EXTRA12,
  NEUTRAL_BTN_EXTRA13,
  NEUTRAL_BTN_PAD_U,
  NEUTRAL_BTN_PAD_D,
  NEUTRAL_BTN_PAD_L,
  NEUTRAL_BTN_PAD_R,
};

enum NeutralFrameArcadeBit : uint8_t {
  NEUTRAL_ARCADE_U = 0,
  NEUTRAL_ARCADE_D,
  NEUTRAL_ARCADE_L,
  NEUTRAL_ARCADE_R,
  NEUTRAL_ARCADE_P1,
  NEUTRAL_ARCADE_P2,
  NEUTRAL_ARCADE_P3,
  NEUTRAL_ARCADE_P4,
  NEUTRAL_ARCADE_K1,
  NEUTRAL_ARCADE_K2,
  NEUTRAL_ARCADE_K3,
  NEUTRAL_ARCADE_K4,
  NEUTRAL_ARCADE_START,
  NEUTRAL_ARCADE_COIN,
  NEUTRAL_ARCADE_SERVICE,
  NEUTRAL_ARCADE_TEST,
};

extern const char kNeutralFrameButtonOrder[];
extern const char kNeutralFrameArcadeOverlayOrder[];

typedef struct __attribute((packed, aligned(1))) {
  uint8_t flags;       // bit0=connected
  uint8_t config;      // controller_state_t config byte
  uint8_t precision;   // analog_stick_precision
  uint8_t reserved;
  uint32_t buttons;    // NeutralFrameButtonBit ordering
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;
  uint8_t analog_l2;
  uint8_t analog_r2;
  uint8_t analog_a;
  uint8_t analog_b;
  uint8_t analog_x;
  uint8_t analog_y;
  uint8_t analog_l1;
  uint8_t analog_r1;
  uint8_t analog_pad_u;
  uint8_t analog_pad_d;
  uint8_t analog_pad_l;
  uint8_t analog_pad_r;
  uint8_t paddle;
  int8_t spinner;
  int8_t mouse_x;
  int8_t mouse_y;
  int8_t mouse_wheel_x;
  int8_t mouse_wheel_y;
  char controller_type[NEUTRAL_FRAME_TYPE_NAME_BYTES];
} NeutralFramePacket;

uint32_t neutralFrameButtonMask(const controller_state_t& frame);
uint16_t neutralFrameArcadeOverlayMask(const controller_state_t& frame);
void packNeutralFramePacket(const controller_state_t& frame, NeutralFramePacket& packet);
