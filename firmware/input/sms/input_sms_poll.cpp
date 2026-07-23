#include "Input_Sms.h"

#include <cstring>

#include "../shared/input_button_bits.h"

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include "hardware/gpio.h"
#endif

namespace {

__force_inline uint8_t __not_in_flash_func(readSmsPin)(uint8_t pin) {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  return gpio_get(pin) ? HIGH : LOW;
#else
  return digitalRead(pin);
#endif
}

}  // namespace

void __not_in_flash_func(RZInputSms::stageWebhidRawData)(uint8_t state, uint8_t select, uint8_t runCombo,
                                                         uint8_t port, uint8_t currentDrivingState,
                                                         int8_t drivingDelta) {
  pendingWebhidRaw[0] = isJPC ? 2 : (drivingActive[port] ? 3 : 1);
  pendingWebhidRaw[1] = state;
  pendingWebhidRaw[2] = select;
  pendingWebhidRaw[3] = runCombo;
  pendingWebhidRaw[4] = port;
  pendingWebhidRaw[5] = currentDrivingState;
  pendingWebhidRaw[6] = (uint8_t)drivingDelta;
  pendingWebhidRaw[7] = (uint8_t)drivingPosition[port];
  pendingWebhidRawDirty = true;
}

void RZInputSms::flushPendingWebhidRawData() {
  if (!pendingWebhidRawDirty) {
    return;
  }
  uint8_t raw[16] = {0};
  memcpy(raw, pendingWebhidRaw, sizeof(pendingWebhidRaw));
  webhid_store_raw_data(raw, 16);
  pendingWebhidRawDirty = false;
}

bool __not_in_flash_func(RZInputSms::poll)() {
  beginPollCycle();

  for (uint8_t port = 0; port < input_ports && port < MAX_USB_OUT; ++port) {
    const input_sms_config_t& pins = input_sms_config[port];

    uint8_t up    = readSmsPin(pins.up);
    uint8_t down  = readSmsPin(pins.down);
    uint8_t left  = readSmsPin(pins.left);
    uint8_t right = readSmsPin(pins.right);
    uint8_t btn1  = readSmsPin(pins.tl);
    uint8_t btn2  = HIGH;
    if (isJPC) {
      btn2 = readSmsPin(pins.th);
    } else {
      btn2 = readSmsPin(pins.tr);
    }

    uint8_t currentDrivingState = ((up == HIGH) ? 1 : 0) << 1 | ((down == HIGH) ? 1 : 0);
    int8_t drivingDelta = 0;
    bool drivingActivatedThisPoll = false;
    controller_state_t& frame = inputFrame(port);
    if (!isJPC && !drivingActive[port] && up == LOW && down == LOW) {
      drivingActive[port] = true;
      drivingActivatedThisPoll = true;
      frame.HAS_ANALOG_STICK_MAIN = 1;
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_16;
      setInputFrameTypeName(frame, "Driving");
    }
    if (!isJPC && currentDrivingState != lastDrivingState[port]) {
      uint8_t drivingTableIndex = (lastDrivingState[port] << 2) | currentDrivingState;
      drivingDelta = drivingQuadTable[drivingTableIndex];

      if (!drivingActive[port] &&
          (currentDrivingState == 0 || lastDrivingState[port] == 0) &&
          drivingDelta != 0) {
        drivingActive[port] = true;
        drivingActivatedThisPoll = true;
        frame.HAS_ANALOG_STICK_MAIN = 1;
        frame.sticks_precision_bits = ANALOG_STICK_PRECISION_16;
        setInputFrameTypeName(frame, "Driving");
      }

      if (drivingActive[port] && drivingDelta != 0) {
        int16_t step = (int16_t)drivingDelta * driving_speed_mult[spinner_speed];
        drivingPosition[port] = constrain(drivingPosition[port] + step, 0, 255);
      }
    }
    lastDrivingState[port] = currentDrivingState;

    uint8_t state = (HIGH << 6) | (btn2 << 5) | (btn1 << 4) | (right << 3) | (left << 2) | (down << 1) | up;

    uint8_t select = 0, runCombo = 0;
    if (isJPC) {
      if ((state & 0x03) == 0x00) select = 1;
      if ((state & 0x0C) == 0x00) runCombo = 1;
    }

    if (state != lastState[port] || drivingActivatedThisPoll) {
      lastState[port] = state;

      if (!drivingActive[port]) {
        uint32_t buttons = 0;
        buttons |= (up == LOW) ? INPUT_PAD_U : 0;
        buttons |= (down == LOW) ? INPUT_PAD_D : 0;
        buttons |= (left == LOW) ? INPUT_PAD_L : 0;
        buttons |= (right == LOW) ? INPUT_PAD_R : 0;

        if (isJPC) {
          buttons |= (btn2 == LOW) ? INPUT_A : 0;
          buttons |= (btn1 == LOW) ? INPUT_B : 0;
          if (select) {
            buttons = (buttons & ~(INPUT_PAD_U | INPUT_PAD_D)) | INPUT_SELECT;
          }
          if (runCombo) {
            buttons = (buttons & ~(INPUT_PAD_L | INPUT_PAD_R)) | INPUT_START;
          }
        } else {
          buttons |= (btn1 == LOW) ? INPUT_A : 0;
          buttons |= (btn2 == LOW) ? INPUT_B : 0;
        }

        frame.digital_buttons = buttons;
      } else {
        resetState(port);

        frame.PAD_U = (up == LOW) ? 1 : 0;
        frame.PAD_D = (down == LOW) ? 1 : 0;
        frame.PAD_L = (left == LOW) ? 1 : 0;
        frame.PAD_R = (right == LOW) ? 1 : 0;
      }

      if (drivingActive[port]) {
        frame.A = (btn1 == LOW) ? 1 : 0;
        frame.B = (btn2 == LOW) ? 1 : 0;

        int16_t axisValue = ((int32_t)drivingPosition[port] * 256) - 32768;
        frame.HAS_ANALOG_STICK_MAIN = 1;
        frame.sticks_precision_bits = ANALOG_STICK_PRECISION_16;
        frame.LX = axisValue;
        frame.LY = 0;
        frame.RX = 0;
        frame.RY = 0;
        frame.paddle = (uint8_t)drivingPosition[port];
        frame.PAD_U = 0;
        frame.PAD_D = 0;
        setInputFrameTypeName(frame, "Driving");
      }

      setUpdated(port);

      if (port == 0) {
        stageWebhidRawData(state, select, runCombo, port, currentDrivingState, drivingDelta);
      }
    }
  }

  return endPollCycle();
}

void RZInputSms::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  flushPendingWebhidRawData();
}
