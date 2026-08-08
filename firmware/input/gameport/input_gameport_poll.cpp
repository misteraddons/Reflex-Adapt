#include "Input_Gameport.h"

bool RZInputGameport::poll() {
  beginPollCycle();

  for (uint8_t i = 0; i < input_ports && i < MAX_USB_OUT; ++i) {
    const input_gameport_config_t& cfg = input_gameport_config[i];

    bool isConnected = checkConnected(i);
    controller_state_t& frame = inputFrame(i);

    if (isConnected != wasConnected[i]) {
      wasConnected[i] = isConnected;
      setInputFrameConnected(i, isConnected);

      if (isConnected) {
        setInputFrameTypeName(i, "Gameport");
      } else {
        clearInputFrameTypeName(i);
      }

      setUpdated(i);
    }

    if (!isConnected) continue;

    resetState(i);

    frame.A = !gpio_get(cfg.pinBtn1);
    frame.B = !gpio_get(cfg.pinBtn2);
    frame.X = !gpio_get(cfg.pinBtn3);
    frame.Y = !gpio_get(cfg.pinBtn4);

    uint32_t xTime = readAxisTiming(cfg.pinAxisX);
    uint32_t yTime = readAxisTiming(cfg.pinAxisY);

    frame.LX = timingToAxis(xTime);
    frame.LY = timingToAxis(yTime);

    setUpdated(i);

    if (i == 0) {
      uint8_t raw[16] = {0};
      raw[0] = 1;
      raw[1] = frame.A | (frame.B << 1) | (frame.X << 2) | (frame.Y << 3);
      raw[2] = (xTime >> 8) & 0xFF;
      raw[3] = xTime & 0xFF;
      raw[4] = (yTime >> 8) & 0xFF;
      raw[5] = yTime & 0xFF;
      raw[6] = i;
      webhid_store_raw_data(raw, 16);
    }
  }

  return endPollCycle();
}
