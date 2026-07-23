#pragma once

#include "output_usb_player_count.h"

// Keep HID report routing in descriptor/source-port order. Earlier firmware
// reversed the two Windows DInput interfaces, but current Windows enumeration
// already presents interface 0 first, so the reversal makes physical port 1
// appear as player 2.
inline bool output_usb_windows_dinput_hid_order_swap_active() {
  return false;
}

inline uint8_t output_usb_hid_interface_for_source_port(uint8_t sourcePort) {
  return sourcePort;
}

inline uint8_t output_usb_source_port_for_hid_interface(uint8_t hidInterface) {
  return hidInterface;
}
