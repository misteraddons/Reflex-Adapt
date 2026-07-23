#pragma once

#include <stdint.h>

extern const char* const screensaver_anim_names[];
extern const uint8_t SCREENSAVER_ANIM_COUNT;
extern const uint8_t screensaver_enabled_anim_ids[];
extern const uint8_t SCREENSAVER_ENABLED_ANIM_COUNT;

bool isScreensaverAnimationEnabled(uint8_t anim);
uint8_t sanitizeScreensaverAnimation(uint8_t anim);
