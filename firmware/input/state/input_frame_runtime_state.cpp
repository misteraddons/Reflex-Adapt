#include "../../product_config.h"

#include "input_frame_runtime_state.h"

namespace {

uint8_t input_frame_port_count = 1;

}  // namespace

uint8_t inputFramePortCount() {
  return input_frame_port_count;
}

void setInputFramePortCount(uint8_t port_count) {
  input_frame_port_count = port_count;
}
