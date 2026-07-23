#include "Input_Custom.h"
#include "input_custom_runtime_state.h"

bool RZInputCustom::poll() {
  beginPollCycle();

  for (uint8_t port = 0; port < input_ports; ++port) {
    uint8_t i = port;
    bool port_updated = false;
    if (debouncer[port]->update()) {
      resetState(i);
      controller_state_t& frame = inputFrame(i);
      frame.PAD_U  = debouncer[port]->digitalPressed(input_custom_config[port].buttons.up);
      frame.PAD_D  = debouncer[port]->digitalPressed(input_custom_config[port].buttons.down);
      frame.PAD_L  = debouncer[port]->digitalPressed(input_custom_config[port].buttons.left);
      frame.PAD_R  = debouncer[port]->digitalPressed(input_custom_config[port].buttons.right);
      frame.A      = debouncer[port]->digitalPressed(input_custom_config[port].buttons.a);
      frame.B      = debouncer[port]->digitalPressed(input_custom_config[port].buttons.b);
      frame.X      = debouncer[port]->digitalPressed(input_custom_config[port].buttons.x);
      frame.Y      = debouncer[port]->digitalPressed(input_custom_config[port].buttons.y);
      frame.L1     = debouncer[port]->digitalPressed(input_custom_config[port].buttons.l1);
      frame.R1     = debouncer[port]->digitalPressed(input_custom_config[port].buttons.r1);
      frame.L2     = debouncer[port]->digitalPressed(input_custom_config[port].buttons.l2);
      frame.R2     = debouncer[port]->digitalPressed(input_custom_config[port].buttons.r2);
      frame.L3     = debouncer[port]->digitalPressed(input_custom_config[port].buttons.l3);
      frame.R3     = debouncer[port]->digitalPressed(input_custom_config[port].buttons.r3);
      frame.SELECT = debouncer[port]->digitalPressed(input_custom_config[port].buttons.select);
      frame.START  = debouncer[port]->digitalPressed(input_custom_config[port].buttons.start);
      frame.HOME   = debouncer[port]->digitalPressed(input_custom_config[port].buttons.home);

      port_updated = true;
    }

    uint16_t stick_lx = tryReadAnalog(input_custom_config[port].sticks.stick_lx);
    uint16_t stick_ly = tryReadAnalog(input_custom_config[port].sticks.stick_ly);
    uint16_t stick_rx = tryReadAnalog(input_custom_config[port].sticks.stick_lx);
    uint16_t stick_ry = tryReadAnalog(input_custom_config[port].sticks.stick_ly);

    if (stick_lx != custom_last_lx || stick_ly != custom_last_ly || stick_rx != custom_last_rx || stick_ry != custom_last_ry) {
      controller_state_t& frame = inputFrame(i);
      frame.LX = stick_lx - 0x800;
      frame.LY = stick_ly - 0x800;
      frame.RX = stick_rx - 0x800;
      frame.RY = stick_ry - 0x800;
      custom_last_lx = stick_lx;
      custom_last_ly = stick_ly;
      custom_last_rx = stick_rx;
      custom_last_ry = stick_ry;
      port_updated = true;
    }

    if (port_updated)
      setUpdated(i);
  }

  return endPollCycle();
}
