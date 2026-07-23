#include "../../product_config.h"

#include "input_poll_runtime_state.h"

namespace {

bool input_poll_cycle_updated = false;
uint32_t input_last_poll_at_us = 0;

}  // namespace

uint32_t inputLastPollAtUs() {
  return input_last_poll_at_us;
}

void resetInputLastPollAtUs() {
  input_last_poll_at_us = 0;
}

void setInputLastPollAtUs(uint32_t poll_at_us) {
  input_last_poll_at_us = poll_at_us;
}

bool inputPollCycleUpdated() {
  return input_poll_cycle_updated;
}

void resetInputPollCycleUpdated() {
  input_poll_cycle_updated = false;
}

void markInputPollCycleUpdated() {
  input_poll_cycle_updated = true;
}
