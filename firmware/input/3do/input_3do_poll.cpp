#include "Input_3do.h"

bool __not_in_flash_func(RZInput3do::poll)() {
  beginPollCycle();

  uint8_t i = 0;
  const uint32_t now_us = micros();

  bool port_sample_valid[input_ports] = {true, true};
  for (uint8_t port = 0; port < input_ports; ++port) {
    if (port < MAX_USB_OUT) {
      const bool discovery_due =
          static_cast<int32_t>(now_us - next_discovery_at_us[port]) >= 0;
      const bool recently_active =
          port_controller_count[port] > 0 ||
          (last_active_at_us[port] != 0 &&
           now_us - last_active_at_us[port] < active_port_grace_us);
      if (recently_active || discovery_due) {
        const uint8_t max_controllers =
            (discovery_due || port_controller_count[port] == 0)
                ? THREEDO_MAX_CTRL : port_controller_count[port];
        tdo[port]->update(max_controllers);
        if (discovery_due) {
          next_discovery_at_us[port] = now_us + discovery_interval_us;
        }
      }
      const uint8_t observed_controller_count = tdo[port]->getControllerCount();
      if (observed_controller_count > 0) {
        port_controller_count[port] = observed_controller_count;
        last_active_at_us[port] = now_us;
      } else if (last_active_at_us[port] != 0 &&
                 now_us - last_active_at_us[port] < active_port_grace_us) {
        port_sample_valid[port] = false;
      } else {
        port_controller_count[port] = 0;
      }
    }
  }

  for (uint8_t port = 0; port < input_ports; ++port) {
    if (!port_sample_valid[port]) {
      const uint8_t next_index = i + port_controller_count[port];
      i = next_index < MAX_USB_OUT ? next_index : MAX_USB_OUT;
      continue;
    }

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
