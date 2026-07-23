#include "Input_Intellivision.h"
#include "input_intellivision_runtime_state.h"

namespace {
constexpr uint8_t INTV_KEY_1 = 0x81;
constexpr uint8_t INTV_KEY_2 = 0x41;
constexpr uint8_t INTV_KEY_3 = 0x21;
constexpr uint8_t INTV_KEY_4 = 0x82;
constexpr uint8_t INTV_KEY_5 = 0x42;
constexpr uint8_t INTV_KEY_6 = 0x22;
constexpr uint8_t INTV_KEY_7 = 0x84;
constexpr uint8_t INTV_KEY_8 = 0x44;
constexpr uint8_t INTV_KEY_9 = 0x24;
constexpr uint8_t INTV_KEY_CLEAR = 0x88;
constexpr uint8_t INTV_KEY_0 = 0x48;
constexpr uint8_t INTV_KEY_ENTER = 0x28;

constexpr uint8_t INTV_DISC_N = 0x04;
constexpr uint8_t INTV_DISC_NNE = 0x14;
constexpr uint8_t INTV_DISC_NE = 0x16;
constexpr uint8_t INTV_DISC_ENE = 0x06;
constexpr uint8_t INTV_DISC_E = 0x02;
constexpr uint8_t INTV_DISC_ESE = 0x12;
constexpr uint8_t INTV_DISC_SE = 0x13;
constexpr uint8_t INTV_DISC_SSE = 0x03;
constexpr uint8_t INTV_DISC_S = 0x01;
constexpr uint8_t INTV_DISC_SSW = 0x11;
constexpr uint8_t INTV_DISC_SW = 0x19;
constexpr uint8_t INTV_DISC_WSW = 0x09;
constexpr uint8_t INTV_DISC_W = 0x08;
constexpr uint8_t INTV_DISC_WNW = 0x18;
constexpr uint8_t INTV_DISC_NW = 0x1C;
constexpr uint8_t INTV_DISC_NNW = 0x0C;
}

bool RZInputIntv::poll() {
  beginPollCycle();

  for (uint8_t port = 0; port < input_ports; ++port) {
    uint8_t state = readController(port);

    if (state != intellivision_last_state[port]) {
      resetState(port);
      controller_state_t& frame = inputFrame(port);

      if (state & 0x80) frame.A = 1;
      if (state & 0x40) frame.B = 1;
      if (state & 0x20) frame.X = 1;

      uint8_t keypad = state & 0x8F;
      switch (keypad) {
        case INTV_KEY_1: frame.Y = 1; break;
        case INTV_KEY_2: frame.L1 = 1; break;
        case INTV_KEY_3: frame.R1 = 1; break;
        case INTV_KEY_4: frame.L2 = 1; break;
        case INTV_KEY_5: frame.R2 = 1; break;
        case INTV_KEY_6: frame.L3 = 1; break;
        case INTV_KEY_7: frame.R3 = 1; break;
        case INTV_KEY_8: frame.HOME = 1; break;
        case INTV_KEY_9: frame.CAPTURE = 1; break;
        case INTV_KEY_CLEAR: frame.SELECT = 1; break;
        case INTV_KEY_0: frame.START = 1; break;
        case INTV_KEY_ENTER:
          frame.START = 1;
          frame.SELECT = 1;
          break;
      }

      uint8_t disc = state & 0x1F;
      switch (disc) {
        case INTV_DISC_N:
        case INTV_DISC_NNE:
        case INTV_DISC_NNW:
          frame.PAD_U = 1;
          break;
        case INTV_DISC_S:
        case INTV_DISC_SSE:
        case INTV_DISC_SSW:
          frame.PAD_D = 1;
          break;
        case INTV_DISC_E:
        case INTV_DISC_ENE:
        case INTV_DISC_ESE:
          frame.PAD_R = 1;
          break;
        case INTV_DISC_W:
        case INTV_DISC_WNW:
        case INTV_DISC_WSW:
          frame.PAD_L = 1;
          break;
        case INTV_DISC_NE:
          frame.PAD_U = 1;
          frame.PAD_R = 1;
          break;
        case INTV_DISC_SE:
          frame.PAD_D = 1;
          frame.PAD_R = 1;
          break;
        case INTV_DISC_SW:
          frame.PAD_D = 1;
          frame.PAD_L = 1;
          break;
        case INTV_DISC_NW:
          frame.PAD_U = 1;
          frame.PAD_L = 1;
          break;
      }

      intellivision_last_state[port] = state;
      setUpdated(port);

      if (port == 0) {
        uint8_t raw[16] = {0};
        raw[0] = 1;
        raw[1] = state;
        raw[2] = state & 0x1F;
        raw[3] = state & 0x8F;
        raw[4] = (state >> 5) & 0x07;
        raw[5] = port;
        webhid_store_raw_data(raw, 16);
      }
    }
  }

  return endPollCycle();
}
