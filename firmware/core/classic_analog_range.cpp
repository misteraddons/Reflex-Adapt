#include "classic_analog_range.h"

#include <Arduino.h>

namespace {

constexpr uint8_t kClassicAnalogLearnPorts = 4;
constexpr uint8_t kClassicAnalogLearnInitialPercent = 90;
constexpr int8_t kClassicAnalogAutoCenterMaxOffset = 10;
constexpr int8_t kClassicAnalogAutoCenterSnapThreshold = 2;
constexpr uint8_t kN64AnalogLearnInitialMax = 80;
constexpr uint8_t kGcAnalogLearnFullMax = 100;
constexpr uint8_t kWiiAnalogLearnFullMax = 100;
constexpr uint8_t kGcTriggerLearnFullMax = 160;
constexpr uint16_t kFullTriggerLearnMax = 255;

struct ClassicAnalogLearnAxisState {
  int8_t min_value;
  int8_t max_value;
  int8_t raw;
  int8_t calibrated;
  int8_t center_value;
  bool valid;
  bool center_valid;
};

struct ClassicAnalogLearnTriggerState {
  uint8_t min_value;
  uint8_t max_value;
  uint8_t raw;
  uint8_t calibrated;
  bool valid;
};

ClassicAnalogLearnAxisState learnState[kClassicAnalogLearnPorts][CLASSIC_ANALOG_AXIS_COUNT];
ClassicAnalogLearnTriggerState triggerLearnState[kClassicAnalogLearnPorts][CLASSIC_ANALOG_TRIGGER_COUNT];
DeviceEnum learnMode[kClassicAnalogLearnPorts] = {};

int8_t clampInt8(int16_t value) {
  if (value < INT8_MIN) return INT8_MIN;
  if (value > INT8_MAX) return INT8_MAX;
  return (int8_t)value;
}

constexpr uint8_t learnInitialFromFullMax(uint16_t fullMax) {
  return (uint8_t)((fullMax * kClassicAnalogLearnInitialPercent) / 100u);
}

int8_t initialLearnMagnitude(DeviceEnum mode, ClassicAnalogAxis axis) {
  (void)axis;
  #ifdef ENABLE_INPUT_N64
  if (mode == RZORD_N64) {
    return (int8_t)kN64AnalogLearnInitialMax;
  }
  #endif
  #ifdef ENABLE_INPUT_GAMECUBE
  if (mode == RZORD_GAMECUBE) {
    return (int8_t)learnInitialFromFullMax(kGcAnalogLearnFullMax);
  }
  #endif
  #ifdef ENABLE_INPUT_WII
  if (mode == RZORD_WII) {
    return (int8_t)learnInitialFromFullMax(kWiiAnalogLearnFullMax);
  }
  #endif
  return (int8_t)learnInitialFromFullMax(kGcAnalogLearnFullMax);
}

uint8_t initialTriggerLearnMax(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_GAMECUBE
  if (mode == RZORD_GAMECUBE) {
    return learnInitialFromFullMax(kGcTriggerLearnFullMax);
  }
  #endif
  (void)mode;
  return learnInitialFromFullMax(kFullTriggerLearnMax);
}

void initializeAxis(DeviceEnum mode, uint8_t port, ClassicAnalogAxis axis) {
  if (port >= kClassicAnalogLearnPorts || axis >= CLASSIC_ANALOG_AXIS_COUNT) {
    return;
  }

  const int8_t magnitude = initialLearnMagnitude(mode, axis);
  ClassicAnalogLearnAxisState& axisState = learnState[port][axis];
  axisState.min_value = -magnitude;
  axisState.max_value = magnitude;
  axisState.raw = 0;
  axisState.calibrated = 0;
  axisState.center_value = 0;
  axisState.valid = false;
  axisState.center_valid = false;
}

void ensureAxis(DeviceEnum mode, uint8_t port, ClassicAnalogAxis axis) {
  if (port >= kClassicAnalogLearnPorts || axis >= CLASSIC_ANALOG_AXIS_COUNT) {
    return;
  }
  if (learnMode[port] != mode) {
    resetClassicAnalogLearnState(mode, port);
  }
}

void initializeTrigger(DeviceEnum mode, uint8_t port, ClassicAnalogTrigger trigger) {
  if (port >= kClassicAnalogLearnPorts || trigger >= CLASSIC_ANALOG_TRIGGER_COUNT) {
    return;
  }

  ClassicAnalogLearnTriggerState& triggerState = triggerLearnState[port][trigger];
  triggerState.min_value = 0;
  triggerState.max_value = initialTriggerLearnMax(mode);
  triggerState.raw = 0;
  triggerState.calibrated = 0;
  triggerState.valid = false;
}

void ensureTrigger(DeviceEnum mode, uint8_t port, ClassicAnalogTrigger trigger) {
  if (port >= kClassicAnalogLearnPorts || trigger >= CLASSIC_ANALOG_TRIGGER_COUNT) {
    return;
  }
  if (learnMode[port] != mode) {
    resetClassicAnalogLearnState(mode, port);
  }
}

int8_t scaleSignedAxis(int16_t raw, int8_t rawMin, int8_t rawMax) {
  if (rawMin >= 0 || rawMax <= 0) {
    return clampInt8(raw);
  }

  int32_t normalized = 0;
  if (raw <= 0) {
    if (raw < rawMin) raw = rawMin;
    normalized = ((int32_t)raw * 127) / (-rawMin);
  } else {
    if (raw > rawMax) raw = rawMax;
    normalized = ((int32_t)raw * 127) / rawMax;
  }

  if (normalized > -4 && normalized < 4) {
    return 0;
  }
  return (int8_t)constrain(normalized, -127, 127);
}

int16_t absInt16(int16_t value) {
  return value < 0 ? -value : value;
}

int8_t centerClassicAnalogLearnAxisRaw(ClassicAnalogLearnAxisState& axisState,
                                       int16_t raw) {
  const int8_t raw8 = clampInt8(raw);
  if (!axisState.center_valid &&
      absInt16(raw8) <= kClassicAnalogAutoCenterMaxOffset) {
    axisState.center_value = raw8;
    axisState.center_valid = true;
  }

  if (!axisState.center_valid) {
    return raw8;
  }

  int16_t centered = (int16_t)raw8 - (int16_t)axisState.center_value;
  if (absInt16(centered) <= kClassicAnalogAutoCenterSnapThreshold) {
    centered = 0;
  }
  return clampInt8(centered);
}

uint8_t scaleUnsignedTrigger(uint8_t raw, uint8_t rawMin, uint8_t rawMax) {
  if (rawMax <= rawMin) {
    return raw;
  }
  if (raw <= rawMin) {
    return 0;
  }
  if (raw >= rawMax) {
    return 255;
  }
  return (uint8_t)(((uint32_t)(raw - rawMin) * 255u) / (rawMax - rawMin));
}

}  // namespace

void resetClassicAnalogLearnState(DeviceEnum mode, uint8_t port) {
  if (port >= kClassicAnalogLearnPorts) {
    return;
  }
  learnMode[port] = mode;
  for (uint8_t axis = 0; axis < CLASSIC_ANALOG_AXIS_COUNT; ++axis) {
    initializeAxis(mode, port, (ClassicAnalogAxis)axis);
  }
  for (uint8_t trigger = 0; trigger < CLASSIC_ANALOG_TRIGGER_COUNT; ++trigger) {
    initializeTrigger(mode, port, (ClassicAnalogTrigger)trigger);
  }
}

void recordClassicAnalogRangeAxis(DeviceEnum mode, uint8_t port, ClassicAnalogAxis axis,
                                  int16_t raw, int16_t calibrated) {
  if (port >= kClassicAnalogLearnPorts || axis >= CLASSIC_ANALOG_AXIS_COUNT) {
    return;
  }
  ensureAxis(mode, port, axis);

  ClassicAnalogLearnAxisState& axisState = learnState[port][axis];
  axisState.raw = clampInt8(raw);
  axisState.calibrated = clampInt8(calibrated);
  axisState.valid = true;
}

int8_t applyClassicAnalogLearnAxis(DeviceEnum mode, uint8_t port, ClassicAnalogAxis axis,
                                   int16_t raw) {
  if (port >= kClassicAnalogLearnPorts || axis >= CLASSIC_ANALOG_AXIS_COUNT) {
    return clampInt8(raw);
  }
  ensureAxis(mode, port, axis);

  ClassicAnalogLearnAxisState& axisState = learnState[port][axis];
  const int8_t raw8 = centerClassicAnalogLearnAxisRaw(axisState, raw);

  if (raw8 < axisState.min_value) {
    axisState.min_value = raw8;
  }
  if (raw8 > axisState.max_value) {
    axisState.max_value = raw8;
  }

  const int8_t calibrated = scaleSignedAxis(raw8, axisState.min_value, axisState.max_value);

  axisState.raw = raw8;
  axisState.calibrated = calibrated;
  axisState.valid = true;
  return calibrated;
}

void recordClassicAnalogRangeTrigger(DeviceEnum mode, uint8_t port, ClassicAnalogTrigger trigger,
                                     uint8_t raw, uint8_t calibrated) {
  if (port >= kClassicAnalogLearnPorts || trigger >= CLASSIC_ANALOG_TRIGGER_COUNT) {
    return;
  }
  ensureTrigger(mode, port, trigger);

  ClassicAnalogLearnTriggerState& triggerState = triggerLearnState[port][trigger];
  triggerState.raw = raw;
  triggerState.calibrated = calibrated;
  triggerState.valid = true;
}

uint8_t applyClassicAnalogLearnTrigger(DeviceEnum mode, uint8_t port, ClassicAnalogTrigger trigger,
                                       uint8_t raw) {
  if (port >= kClassicAnalogLearnPorts || trigger >= CLASSIC_ANALOG_TRIGGER_COUNT) {
    return raw;
  }
  ensureTrigger(mode, port, trigger);

  ClassicAnalogLearnTriggerState& triggerState = triggerLearnState[port][trigger];
  if (raw < triggerState.min_value) {
    triggerState.min_value = raw;
  } else if (raw > triggerState.max_value) {
    triggerState.max_value = raw;
  }

  const uint8_t calibrated = scaleUnsignedTrigger(
      raw, triggerState.min_value, triggerState.max_value);
  triggerState.raw = raw;
  triggerState.calibrated = calibrated;
  triggerState.valid = true;
  return calibrated;
}

bool getClassicAnalogRangeSnapshot(DeviceEnum mode, uint8_t port,
                                   ClassicAnalogRangeSnapshot& snapshot) {
  if (port >= kClassicAnalogLearnPorts) {
    return false;
  }
  ensureAxis(mode, port, CLASSIC_ANALOG_AXIS_LX);

  bool anyValid = false;
  for (uint8_t axis = 0; axis < CLASSIC_ANALOG_AXIS_COUNT; ++axis) {
    const ClassicAnalogLearnAxisState& axisState = learnState[port][axis];
    snapshot.raw[axis] = axisState.raw;
    snapshot.calibrated[axis] = axisState.calibrated;
    snapshot.learned_min[axis] = axisState.min_value;
    snapshot.learned_max[axis] = axisState.max_value;
    snapshot.valid[axis] = axisState.valid;
    anyValid = anyValid || axisState.valid;
  }
  for (uint8_t trigger = 0; trigger < CLASSIC_ANALOG_TRIGGER_COUNT; ++trigger) {
    const ClassicAnalogLearnTriggerState& triggerState = triggerLearnState[port][trigger];
    snapshot.trigger_raw[trigger] = triggerState.raw;
    snapshot.trigger_calibrated[trigger] = triggerState.calibrated;
    snapshot.trigger_learned_min[trigger] = triggerState.min_value;
    snapshot.trigger_learned_max[trigger] = triggerState.max_value;
    snapshot.trigger_valid[trigger] = triggerState.valid;
    anyValid = anyValid || triggerState.valid;
  }
  return anyValid;
}
