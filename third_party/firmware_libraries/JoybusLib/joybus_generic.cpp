#include "joybus_generic.hpp"
#include "joybus_checksum.hpp"

#include <string.h>

JoybusControllerInfo joybus_handshake(JoybusPIOInstance instance, bool reset) {
  JoybusControllerInfo info;
  info.type = 0x0000;
  uint8_t payload[] = {reset ? (uint8_t)0xFF : (uint8_t)0x00};
  int len = joybus_pio_transmit_receive(instance, payload, 1, (uint8_t *)&info, 3);
  if (len != 3) {
    info.type = 0;
    info.aux = 0;
    return info;
  }
  info.type = UINT16_FIX_ENDIAN(info.type);
  return info;
}

int joybus_read_memory(JoybusPIOInstance instance, uint8_t command, uint address,
                           uint8_t response[]) {
  uint16_t address_checksummed = joybus_checksummed_address(address);
  uint8_t payload[3] = {command, (uint8_t)(address_checksummed >> 8),
                        (uint8_t)(address_checksummed & 0xFF)};
  uint8_t raw_response[JOYBUS_BLOCK_SIZE + 1];
  int result = joybus_pio_transmit_receive(instance, payload, 3, raw_response,
                                           JOYBUS_BLOCK_SIZE + 1);
  memcpy(response, raw_response, JOYBUS_BLOCK_SIZE);
  if (result != JOYBUS_BLOCK_SIZE + 1) {
    if (result < 0) {
      return result;
    }
    return -20;
  }
  if (joybus_data_checksum(raw_response, JOYBUS_BLOCK_SIZE) !=
      raw_response[JOYBUS_BLOCK_SIZE]) {
    return -30;
  }
  //memcpy(response, raw_response, JOYBUS_BLOCK_SIZE);
  return JOYBUS_BLOCK_SIZE;
}

int joybus_write_memory(JoybusPIOInstance instance, uint8_t command, uint address,
                            uint8_t data[]) {
  return joybus_write_memory_diag(instance, command, address, data, nullptr);
}

int joybus_write_memory_diag(JoybusPIOInstance instance, uint8_t command, uint address,
                             uint8_t data[], JoybusMemoryWriteDiag* diag) {
  uint16_t address_checksummed = joybus_checksummed_address(address);
  uint8_t payload[JOYBUS_BLOCK_SIZE + 3] = {command,
                                         (uint8_t)(address_checksummed >> 8),
                                         (uint8_t)(address_checksummed & 0xFF)};
  memcpy(payload + 3, data, JOYBUS_BLOCK_SIZE);
  uint8_t checksum = joybus_data_checksum(data, JOYBUS_BLOCK_SIZE);
  uint8_t response = 0;
  int result = joybus_pio_transmit_receive(instance, payload,
                                           JOYBUS_BLOCK_SIZE + 3, &response, 1);
  if (diag != nullptr) {
    diag->address = (uint16_t)address;
    diag->checksummed_address = address_checksummed;
    diag->command = command;
    diag->expected_checksum = checksum;
    diag->response_checksum = response;
    diag->transport_result = result;
    diag->result = result;
  }
  if (result != 1) {
    if (result < 0) {
      if (diag != nullptr) {
        diag->result = result;
      }
      return result;
    }
    if (diag != nullptr) {
      diag->result = -20;
    }
    return -20;
  }

  if (response != checksum) {
    if (diag != nullptr) {
      diag->result = -30;
    }
    return -30;
  }
  if (diag != nullptr) {
    diag->result = result;
  }
  return result;
}





//int joybus_read_memory(JoybusPIOInstance instance, uint8_t command, uint address,
//                           uint8_t response[]) {
//  uint32_t address_checksummed = joybus_checksummed_address(address);
//  uint8_t payload[3] = {command, (uint8_t)(address_checksummed >> 8),
//                        (uint8_t)(address_checksummed & 0xFF)};
//  uint8_t raw_response[JOYBUS_BLOCK_SIZE + 1];
//  int result = joybus_pio_transmit_receive(instance, payload, 3, raw_response,
//                                           JOYBUS_BLOCK_SIZE + 1);
////                                           printf("\n0x%02x\n", result);
////                                           printf("\n0x%02x\n", response);
////                                           printf("\n0x%02x\n", joybus_data_checksum(response, JOYBUS_BLOCK_SIZE));
//memcpy(response, raw_response, JOYBUS_BLOCK_SIZE);
//  for (uint8_t i = 0; i < sizeof(raw_response); ++i)
//  printf("0x%02x, ", raw_response[i]);
//  printf("READ----\n");
//  if (result != JOYBUS_BLOCK_SIZE + 1) {
//    if (result < 0) {
//      return result;
//    }
//    return -20;
//  }
//  if (joybus_data_checksum(raw_response, JOYBUS_BLOCK_SIZE) !=
//      raw_response[JOYBUS_BLOCK_SIZE]) {
//    return -30;
//  }
//  //memcpy(response, raw_response, JOYBUS_BLOCK_SIZE);
//  return JOYBUS_BLOCK_SIZE;
//}
//
//int joybus_write_memory(JoybusPIOInstance instance, uint8_t command, uint address,
//                            uint8_t data[]) {
//  uint32_t address_checksummed = joybus_checksummed_address(address);
//  uint8_t payload[JOYBUS_BLOCK_SIZE + 3] = {command,
//                                         (uint8_t)(address_checksummed >> 8),
//                                         (uint8_t)(address_checksummed & 0xFF)};
//  memcpy(payload + 3, data, JOYBUS_BLOCK_SIZE);
//
//  for (uint8_t i = 0; i < sizeof(payload); ++i)
//  printf("0x%02x, ", payload[i]);
//  printf("--PAYLOAD\n");
//
//  uint8_t checksum = joybus_data_checksum(data, JOYBUS_BLOCK_SIZE);
//  uint8_t response;
//  int result = joybus_pio_transmit_receive(instance, payload,
//                                           JOYBUS_BLOCK_SIZE + 3, &response, 1);
//
//  if (result != 1) {
//    if (result < 0) {
//      return result;
//    }
//    return -20;
//  }
//
//  if (response != checksum) {
//    return -30;
//  }
//  return result;
//}
