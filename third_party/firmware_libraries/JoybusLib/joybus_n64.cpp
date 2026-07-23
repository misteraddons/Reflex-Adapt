#include <Arduino.h>
#include <string.h>

#include "joybus_n64.hpp"
#include "joybus_pio.hpp"
#include "joybus_generic.hpp"

int joybus_n64_read_memory(JoybusPIOInstance instance, uint address,
                           uint8_t response[]) {
  return joybus_read_memory(instance, 0x02, address, response);
}

int joybus_n64_write_memory(JoybusPIOInstance instance, uint address,
                            uint8_t data[]) {
  return joybus_write_memory(instance, 0x03, address, data);
}

int joybus_n64_write_memory_diag(JoybusPIOInstance instance, uint address,
                                 uint8_t data[], JoybusMemoryWriteDiag* diag) {
  return joybus_write_memory_diag(instance, 0x03, address, data, diag);
}

N64ControllerState __not_in_flash_func(joybus_n64_read_controller)(JoybusPIOInstance instance) {
  N64ControllerState state;
  uint8_t payload[] = {0x01};
  state.valid = joybus_pio_transmit_receive(instance, payload, 1,
                                            (uint8_t *)&state, 4) == 4;
  // N64 returns button bytes in the same low-byte/high-byte order expected by
  // our N64Button bit constants (d-pad/start/Z/A/B in the first byte, C/L/R
  // in the second). Swapping here aliases d-pad to C-buttons and swaps
  // Z/L plus Start/R for every downstream consumer.
  return state;
}
