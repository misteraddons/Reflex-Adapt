#pragma once

#include <stdint.h>

#include "device_mode.h"

// Shared Classic2USB stick range policy for GameCube, Wii-family, and
// Dreamcast analog inputs. N64 normal output always uses learned normalization;
// the raw N64 stick path is only exposed through the diagnostic range test.
static constexpr uint8_t CLASSIC_ANALOG_RANGE_RAW = 0;
static constexpr uint8_t CLASSIC_ANALOG_RANGE_NORMALIZED = 1;
static constexpr uint8_t CLASSIC_ANALOG_RANGE_CALIBRATED = 2;
static constexpr uint8_t CLASSIC_ANALOG_RANGE_LEARN = 3;
static constexpr uint8_t CLASSIC_ANALOG_RANGE_COUNT = 4;
static constexpr uint8_t CLASSIC_ANALOG_RANGE_DEFAULT = CLASSIC_ANALOG_RANGE_LEARN;

enum ClassicAnalogAxis : uint8_t {
  CLASSIC_ANALOG_AXIS_LX = 0,
  CLASSIC_ANALOG_AXIS_LY,
  CLASSIC_ANALOG_AXIS_RX,
  CLASSIC_ANALOG_AXIS_RY,
  CLASSIC_ANALOG_AXIS_COUNT,
};

enum ClassicAnalogTrigger : uint8_t {
  CLASSIC_ANALOG_TRIGGER_L2 = 0,
  CLASSIC_ANALOG_TRIGGER_R2,
  CLASSIC_ANALOG_TRIGGER_COUNT,
};

struct ClassicAnalogRangeSnapshot {
  int8_t raw[CLASSIC_ANALOG_AXIS_COUNT];
  int8_t calibrated[CLASSIC_ANALOG_AXIS_COUNT];
  int8_t learned_min[CLASSIC_ANALOG_AXIS_COUNT];
  int8_t learned_max[CLASSIC_ANALOG_AXIS_COUNT];
  bool valid[CLASSIC_ANALOG_AXIS_COUNT];
  uint8_t trigger_raw[CLASSIC_ANALOG_TRIGGER_COUNT];
  uint8_t trigger_calibrated[CLASSIC_ANALOG_TRIGGER_COUNT];
  uint8_t trigger_learned_min[CLASSIC_ANALOG_TRIGGER_COUNT];
  uint8_t trigger_learned_max[CLASSIC_ANALOG_TRIGGER_COUNT];
  bool trigger_valid[CLASSIC_ANALOG_TRIGGER_COUNT];
};

static inline uint8_t sanitizeClassicAnalogRange(uint8_t value) {
  return (value < CLASSIC_ANALOG_RANGE_COUNT)
           ? value
           : CLASSIC_ANALOG_RANGE_DEFAULT;
}

static inline bool classicModeHasRangeSetting(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_GAMECUBE
  if (mode == RZORD_GAMECUBE) return true;
  #endif
  #ifdef ENABLE_INPUT_WII
  if (mode == RZORD_WII) return true;
  #endif
  #ifdef ENABLE_INPUT_DREAMCAST
  if (mode == RZORD_DREAMCAST) return true;
  #endif
  return false;
}

void resetClassicAnalogLearnState(DeviceEnum mode, uint8_t port);
void recordClassicAnalogRangeAxis(DeviceEnum mode, uint8_t port, ClassicAnalogAxis axis,
                                  int16_t raw, int16_t calibrated);
int8_t applyClassicAnalogLearnAxis(DeviceEnum mode, uint8_t port, ClassicAnalogAxis axis,
                                   int16_t raw);
void recordClassicAnalogRangeTrigger(DeviceEnum mode, uint8_t port, ClassicAnalogTrigger trigger,
                                     uint8_t raw, uint8_t calibrated);
uint8_t applyClassicAnalogLearnTrigger(DeviceEnum mode, uint8_t port, ClassicAnalogTrigger trigger,
                                       uint8_t raw);
bool getClassicAnalogRangeSnapshot(DeviceEnum mode, uint8_t port,
                                   ClassicAnalogRangeSnapshot& snapshot);
