#include "Input_Jaguar.h"
#include "input_jaguar_runtime_state.h"

#include "../shared/input_button_bits.h"

uint32_t __not_in_flash_func(RZInputJaguar::mapJaguarPadToButtons)(uint8_t port, const JagController& sc) const {
  const uint32_t raw = sc.currentState.digital;
  uint32_t buttons = 0;
  buttons |= !(raw & JAG_PAD_UP) ? INPUT_PAD_U : 0;
  buttons |= !(raw & JAG_PAD_DOWN) ? INPUT_PAD_D : 0;
  if (!jaguarRotaryActivePorts[port]) {
    buttons |= !(raw & JAG_PAD_LEFT) ? INPUT_PAD_L : 0;
    buttons |= !(raw & JAG_PAD_RIGHT) ? INPUT_PAD_R : 0;
  }
  buttons |= !(raw & JAG_B) ? INPUT_A : 0;
  buttons |= !(raw & JAG_A) ? INPUT_B : 0;
  buttons |= !(raw & JAG_C) ? INPUT_X : 0;
  buttons |= !(raw & JAG_8) ? INPUT_Y : 0;
  buttons |= !(raw & JAG_7) ? INPUT_L1 : 0;
  buttons |= !(raw & JAG_9) ? INPUT_R1 : 0;
  buttons |= !(raw & JAG_4) ? INPUT_L2 : 0;
  buttons |= !(raw & JAG_6) ? INPUT_R2 : 0;
  buttons |= !(raw & JAG_1) ? INPUT_L3 : 0;
  buttons |= !(raw & JAG_3) ? INPUT_R3 : 0;
  buttons |= !(raw & JAG_OPTION) ? INPUT_SELECT : 0;
  buttons |= !(raw & JAG_PAUSE) ? INPUT_START : 0;
  buttons |= !(raw & JAG_0) ? INPUT_EXTRA0 : 0;
  buttons |= !(raw & JAG_2) ? INPUT_EXTRA1 : 0;
  buttons |= !(raw & JAG_STAR) ? INPUT_EXTRA2 : 0;
  buttons |= !(raw & JAG_HASH) ? INPUT_EXTRA3 : 0;
  buttons |= !(raw & JAG_5) ? INPUT_EXTRA4 : 0;
  return buttons;
}

void __not_in_flash_func(RZInputJaguar::stageWebhidRawData)(uint8_t port, JagDeviceType_Enum type, uint32_t digital) {
  if (port != 0) {
    return;
  }
  pendingWebhidRaw[0] = type;
  pendingWebhidRaw[1] = digital >> 24;
  pendingWebhidRaw[2] = (digital >> 16) & 0xFF;
  pendingWebhidRaw[3] = (digital >> 8) & 0xFF;
  pendingWebhidRaw[4] = digital & 0xFF;
  pendingWebhidRaw[5] = port;
  pendingWebhidRawDirty = true;
}

void RZInputJaguar::flushPendingWebhidRawData() {
  if (!pendingWebhidRawDirty) {
    return;
  }
  uint8_t raw[16] = {0};
  for (uint8_t i = 0; i < sizeof(pendingWebhidRaw); ++i) {
    raw[i] = pendingWebhidRaw[i];
  }
  webhid_store_raw_data(raw, 16);
  pendingWebhidRawDirty = false;
}

bool __not_in_flash_func(RZInputJaguar::poll)() {
  beginPollCycle();

  for (uint8_t port = 0; port < input_ports && port < MAX_USB_OUT; ++port) {
    const uint8_t i = port;

    jag[port]->update();

    const JagController& sc = jag[port]->getJagController(0);
    if (!sc.stateChanged())
      continue;

    controller_state_t& frame = inputFrame(i);
    frame.digital_buttons = 0;
    frame.analog_pad = 0;
    dtype[i] = sc.deviceType();

    switch (dtype[i]) {
      case JAG_DEVICE_PAD:
      {
        frame.digital_buttons = mapJaguarPadToButtons(i, sc);
        frame.analog_pad = 0;
        frame.ANALOG_PAD_L = (sc.currentState.digital & JAG_PAD_LEFT) ? 0 : 255;
        frame.ANALOG_PAD_R = (sc.currentState.digital & JAG_PAD_RIGHT) ? 0 : 255;
        break;
      }
      default:
        break;
    }

    setUpdated(i);

    if (i == 0) {
      stageWebhidRawData(port, dtype[0], sc.currentState.digital);
    }
  }

  for (uint8_t i = input_ports; i < MAX_USB_OUT; ++i) {
    if (dtype[i] != JAG_DEVICE_NONE) {
      dtype[i] = JAG_DEVICE_NONE;
      controller_state_t& frame = inputFrame(i);
      setInputFrameConnected(frame, false);
      clearInputFrameTypeName(frame);
      setUpdated(i);
    }
  }

  return endPollCycle();
}

void RZInputJaguar::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  flushPendingWebhidRawData();
}
