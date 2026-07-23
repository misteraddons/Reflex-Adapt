#include "neutral_frame_packet.h"

#include <string.h>

const char kNeutralFrameButtonOrder[] =
  "A,B,X,Y,L1,R1,L2,R2,L3,R3,START,SELECT,HOME,CAPTURE,"
  "EXTRA0-13,PAD_U,PAD_D,PAD_L,PAD_R";

const char kNeutralFrameArcadeOverlayOrder[] =
  "U,D,L,R,P1,P2,P3,P4,K1,K2,K3,K4,START,COIN,SERVICE,TEST";

namespace {

void setButton(uint32_t& mask, NeutralFrameButtonBit bit, bool pressed) {
  if (pressed) {
    mask |= (uint32_t)1u << (uint8_t)bit;
  }
}

void setArcade(uint16_t& mask, NeutralFrameArcadeBit bit, bool pressed) {
  if (pressed) {
    mask |= (uint16_t)1u << (uint8_t)bit;
  }
}

}  // namespace

uint32_t neutralFrameButtonMask(const controller_state_t& frame) {
  if (!frame.connected) {
    return 0;
  }

  uint32_t mask = 0;
  setButton(mask, NEUTRAL_BTN_A, frame.A);
  setButton(mask, NEUTRAL_BTN_B, frame.B);
  setButton(mask, NEUTRAL_BTN_X, frame.X);
  setButton(mask, NEUTRAL_BTN_Y, frame.Y);
  setButton(mask, NEUTRAL_BTN_L1, frame.L1);
  setButton(mask, NEUTRAL_BTN_R1, frame.R1);
  setButton(mask, NEUTRAL_BTN_L2, frame.L2);
  setButton(mask, NEUTRAL_BTN_R2, frame.R2);
  setButton(mask, NEUTRAL_BTN_L3, frame.L3);
  setButton(mask, NEUTRAL_BTN_R3, frame.R3);
  setButton(mask, NEUTRAL_BTN_START, frame.START);
  setButton(mask, NEUTRAL_BTN_SELECT, frame.SELECT);
  setButton(mask, NEUTRAL_BTN_HOME, frame.HOME);
  setButton(mask, NEUTRAL_BTN_CAPTURE, frame.CAPTURE);

  const uint16_t extra = frame.EXTRA;
  for (uint8_t i = 0; i < 14; ++i) {
    if ((extra & ((uint16_t)1u << i)) != 0) {
      mask |= (uint32_t)1u << ((uint8_t)NEUTRAL_BTN_EXTRA0 + i);
    }
  }

  setButton(mask, NEUTRAL_BTN_PAD_U, frame.PAD_U);
  setButton(mask, NEUTRAL_BTN_PAD_D, frame.PAD_D);
  setButton(mask, NEUTRAL_BTN_PAD_L, frame.PAD_L);
  setButton(mask, NEUTRAL_BTN_PAD_R, frame.PAD_R);
  return mask;
}

uint16_t neutralFrameArcadeOverlayMask(const controller_state_t& frame) {
  if (!frame.connected) {
    return 0;
  }

  uint16_t mask = 0;
  setArcade(mask, NEUTRAL_ARCADE_U, frame.PAD_U);
  setArcade(mask, NEUTRAL_ARCADE_D, frame.PAD_D);
  setArcade(mask, NEUTRAL_ARCADE_L, frame.PAD_L);
  setArcade(mask, NEUTRAL_ARCADE_R, frame.PAD_R);
  setArcade(mask, NEUTRAL_ARCADE_P1, frame.X);
  setArcade(mask, NEUTRAL_ARCADE_P2, frame.Y);
  setArcade(mask, NEUTRAL_ARCADE_P3, frame.R1);
  setArcade(mask, NEUTRAL_ARCADE_P4, frame.L1);
  setArcade(mask, NEUTRAL_ARCADE_K1, frame.A);
  setArcade(mask, NEUTRAL_ARCADE_K2, frame.B);
  setArcade(mask, NEUTRAL_ARCADE_K3, frame.R2);
  setArcade(mask, NEUTRAL_ARCADE_K4, frame.L2);
  setArcade(mask, NEUTRAL_ARCADE_START, frame.START);
  setArcade(mask, NEUTRAL_ARCADE_COIN, frame.CAPTURE);
  setArcade(mask, NEUTRAL_ARCADE_SERVICE, frame.SELECT);
  setArcade(mask, NEUTRAL_ARCADE_TEST, frame.HOME);
  return mask;
}

void packNeutralFramePacket(const controller_state_t& frame, NeutralFramePacket& packet) {
  memset(&packet, 0, sizeof(packet));
  packet.flags = frame.connected ? 0x01 : 0x00;
  packet.config = frame.config;
  packet.precision = (uint8_t)frame.sticks_precision_bits;
  packet.buttons = neutralFrameButtonMask(frame);
  packet.lx = frame.LX;
  packet.ly = frame.LY;
  packet.rx = frame.RX;
  packet.ry = frame.RY;
  packet.analog_l2 = frame.ANALOG_L2;
  packet.analog_r2 = frame.ANALOG_R2;
  packet.analog_a = frame.ANALOG_A;
  packet.analog_b = frame.ANALOG_B;
  packet.analog_x = frame.ANALOG_X;
  packet.analog_y = frame.ANALOG_Y;
  packet.analog_l1 = frame.ANALOG_L1;
  packet.analog_r1 = frame.ANALOG_R1;
  packet.analog_pad_u = frame.ANALOG_PAD_U;
  packet.analog_pad_d = frame.ANALOG_PAD_D;
  packet.analog_pad_l = frame.ANALOG_PAD_L;
  packet.analog_pad_r = frame.ANALOG_PAD_R;
  packet.paddle = frame.paddle;
  packet.spinner = frame.spinner;
  packet.mouse_x = frame.mouse_x;
  packet.mouse_y = frame.mouse_y;
  packet.mouse_wheel_x = frame.mouse_wheel_x;
  packet.mouse_wheel_y = frame.mouse_wheel_y;
  strncpy(packet.controller_type, frame.controller_type_name, sizeof(packet.controller_type));
}
