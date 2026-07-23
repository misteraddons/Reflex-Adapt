#include "screensaver_animation.h"

const char* const screensaver_anim_names[] = {
  "Bouncing Logo",
  "Starfield",
  "Matrix Rain",
  "Auto Snake",
  "Pong AI",
  "Game of Life",
  "Fireworks",
  "Analog Clock",
  "Maze Generator",
  "Breakout AI",
  "Plasma Effect",
  "Tetris AI",
  "Rain Drops",
  "Spirograph"
};

const uint8_t SCREENSAVER_ANIM_COUNT = 14;
const uint8_t screensaver_enabled_anim_ids[] = {
  0, 1, 2, 3, 4, 6, 8, 9, 10, 12, 13
};
const uint8_t SCREENSAVER_ENABLED_ANIM_COUNT =
  sizeof(screensaver_enabled_anim_ids) / sizeof(screensaver_enabled_anim_ids[0]);

bool isScreensaverAnimationEnabled(uint8_t anim) {
  for (uint8_t i = 0; i < SCREENSAVER_ENABLED_ANIM_COUNT; ++i) {
    if (screensaver_enabled_anim_ids[i] == anim) {
      return true;
    }
  }
  return false;
}

uint8_t sanitizeScreensaverAnimation(uint8_t anim) {
  return isScreensaverAnimationEnabled(anim) ? anim : 0;
}
