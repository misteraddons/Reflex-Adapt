#include "Input_3do.h"

bool RZInput3do::poll() {
  beginPollCycle();

  uint8_t i = 0;
  uint8_t port_controller_count[input_ports] = {0};

  for (uint8_t port = 0; port < input_ports; ++port) {
    if (port < MAX_USB_OUT) {
      tdo[port]->update();
      port_controller_count[port] = tdo[port]->getControllerCount();
    }
  }

  for (uint8_t port = 0; port < input_ports; ++port) {
    if (port_controller_count[port] == 0) {
      if (dtype[i] != THREEDO_DEVICE_NONE) {
        dtype[i] = THREEDO_DEVICE_NONE;
        setInputFrameConnected(i, false);
        setUpdated(i);
      }
      ++i;
    }

    for (uint8_t c = 0; c < port_controller_count[port]; ++c) {
      if (i >= MAX_USB_OUT)
        break;

      const ThreedoController& sc = tdo[port]->get3doController(c);

      if (sc.stateChanged()) {
        resetState(i);
        controller_state_t& frame = inputFrame(i);
        if (sc.deviceJustChanged()) {
          dtype[i] = sc.deviceType();
          setInputFrameConnected(i, dtype[i] != THREEDO_DEVICE_NONE);
        }

        switch (dtype[i]) {
          case THREEDO_DEVICE_NONE:
          case THREEDO_DEVICE_NOTSUPPORTED:
            break;
          case THREEDO_DEVICE_PAD:
          {
            frame.PAD_U   = sc.digitalPressed(THREEDO_UP);
            frame.PAD_D   = sc.digitalPressed(THREEDO_DOWN);
            frame.PAD_L   = sc.digitalPressed(THREEDO_LEFT);
            frame.PAD_R   = sc.digitalPressed(THREEDO_RIGHT);
            frame.A       = sc.digitalPressed(THREEDO_A);
            frame.B       = sc.digitalPressed(THREEDO_B);
            frame.X       = sc.digitalPressed(THREEDO_C);
            frame.L1      = sc.digitalPressed(THREEDO_L);
            frame.R1      = sc.digitalPressed(THREEDO_R);
            frame.SELECT  = sc.digitalPressed(THREEDO_X);
            frame.START   = sc.digitalPressed(THREEDO_P);
            break;
          }
        }
        setUpdated(i);

        if (i == 0) {
          uint8_t raw[16] = {0};
          raw[0] = dtype[0];
          raw[1] = sc.currentState.digital >> 8;
          raw[2] = sc.currentState.digital & 0xFF;
          raw[3] = port;
          raw[4] = c;
          webhid_store_raw_data(raw, 16);
        }
      }
      ++i;
    }
  }

  while (i < MAX_USB_OUT) {
    if (dtype[i] != THREEDO_DEVICE_NONE) {
      dtype[i] = THREEDO_DEVICE_NONE;
      setInputFrameConnected(i, false);
      setUpdated(i);
    }
    ++i;
  }

  return endPollCycle();
}
