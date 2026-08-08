#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "joybus_pio.hpp"

#define JOYBUS_BLOCK_SIZE 32

typedef struct __attribute__((__packed__)) JoybusControllerInfo {
  uint16_t type;
  uint8_t aux;
} JoybusControllerInfo;

typedef struct JoybusMemoryWriteDiag {
  uint16_t address;
  uint16_t checksummed_address;
  uint8_t command;
  uint8_t expected_checksum;
  uint8_t response_checksum;
  int transport_result;
  int result;
} JoybusMemoryWriteDiag;

JoybusControllerInfo joybus_handshake(JoybusPIOInstance instance, bool reset);

int joybus_read_memory(JoybusPIOInstance instance, uint8_t command, uint address,
                       uint8_t response[]);
int joybus_write_memory(JoybusPIOInstance instance, uint8_t command, uint address,
                        uint8_t data[]);
int joybus_write_memory_diag(JoybusPIOInstance instance, uint8_t command, uint address,
                             uint8_t data[], JoybusMemoryWriteDiag* diag);
