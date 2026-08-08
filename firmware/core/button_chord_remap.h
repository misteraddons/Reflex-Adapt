#pragma once

#include <stdint.h>

#include "controller_state.h"
#include "settings_store.h"
#include "../input/shared/input_button_bits.h"

constexpr uint8_t BUTTON_CHORD_REMAP_SLOT_COUNT = 4;
constexpr uint32_t BUTTON_CHORD_ALLOWED_BUTTONS =
  INPUT_A | INPUT_B | INPUT_X | INPUT_Y |
  INPUT_L1 | INPUT_R1 | INPUT_L2 | INPUT_R2 |
  INPUT_L3 | INPUT_R3 | INPUT_START | INPUT_SELECT |
  INPUT_HOME | INPUT_CAPTURE |
  INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R;

struct ButtonChordRemapSlot {
  uint32_t source_combo;
  uint32_t output_combo;
};

struct ButtonChordRemapSet {
  ButtonChordRemapSlot slots[BUTTON_CHORD_REMAP_SLOT_COUNT];
};

extern ButtonChordRemapSet active_chord_remaps;

bool isValidButtonChordSource(uint32_t combo);
bool isValidButtonChordOutput(uint32_t combo);
bool hasActiveButtonChordRemap();
void loadButtonChordRemaps();
void writeButtonChordRemaps(const ButtonChordRemapSet& remaps);
void clearButtonChordRemaps();
bool setButtonChordRemap(uint8_t slot, uint32_t source_combo, uint32_t output_combo);
void applyButtonChordRemapsToFrame(controller_state_t& frame);
uint16_t buttonChordStorageRequiredEnd();
