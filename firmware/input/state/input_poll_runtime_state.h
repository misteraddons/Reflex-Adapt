#pragma once

#include <stdint.h>

uint32_t inputLastPollAtUs();

void resetInputLastPollAtUs();

void setInputLastPollAtUs(uint32_t poll_at_us);

bool inputPollCycleUpdated();

void resetInputPollCycleUpdated();

void markInputPollCycleUpdated();
