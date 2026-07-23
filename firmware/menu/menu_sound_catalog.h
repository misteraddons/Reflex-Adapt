#pragma once

#include <stdint.h>

struct SoundEventItem {
  uint16_t mask;
  const char* name;
};

extern const SoundEventItem sound_event_items[];
extern const uint8_t SOUND_EVENT_COUNT;
