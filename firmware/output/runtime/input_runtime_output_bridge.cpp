#include "../../product_config.h"

#include "input_runtime_output_bridge.h"

#include <cstring>

#include "../output_runtime_state.h"

uint8_t webhid_raw_data[WEBHID_RAW_DATA_SIZE] = {0};
uint8_t webhid_raw_data_len = 0;

void webhid_store_raw_data(const uint8_t* data, uint8_t len) {
  if (len > WEBHID_RAW_DATA_SIZE) {
    len = WEBHID_RAW_DATA_SIZE;
  }
  memcpy(webhid_raw_data, data, len);
  webhid_raw_data_len = len;
}

outputMode_t output_mode_for_effective_n64_cstick(void) {
  return get_effective_output_mode();
}

n64_cstick_mode_enum get_effective_n64_cstick_mode(void) {
  switch (n64_cstick_mode) {
    case N64CSTICK_AS_BUTTONS:
      return N64CSTICK_AS_BUTTONS;
    case N64CSTICK_AS_RS:
      return N64CSTICK_AS_RS;
    default:
      break;
  }

  return N64CSTICK_AS_BUTTONS;
}
