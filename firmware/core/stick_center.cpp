#include "stick_center.h"

#include <Arduino.h>

StickCenter::StickCenter() {
  reset();
}

void StickCenter::reset() {
  for (uint8_t i = 0; i < MAX_CONTROLLERS; i++) {
    offsets[i] = { 0, 0, 0, 0, false };
    was_connected[i] = false;
  }
}

bool __not_in_flash_func(StickCenter::update)(uint8_t index, bool connected, int16_t lx, int16_t ly,
                                              int16_t rx, int16_t ry, int16_t maxCalibrationOffset) {
  if (index >= MAX_CONTROLLERS) return false;

  bool just_calibrated = false;

  if (connected && !offsets[index].calibrated) {
    if (axisWithin(lx, maxCalibrationOffset) &&
        axisWithin(ly, maxCalibrationOffset) &&
        axisWithin(rx, maxCalibrationOffset) &&
        axisWithin(ry, maxCalibrationOffset)) {
      offsets[index].lx = lx;
      offsets[index].ly = ly;
      offsets[index].rx = rx;
      offsets[index].ry = ry;
      offsets[index].calibrated = true;
      just_calibrated = true;
    }
  } else if (!connected && was_connected[index]) {
    offsets[index] = { 0, 0, 0, 0, false };
  }

  was_connected[index] = connected;
  return just_calibrated;
}

void __not_in_flash_func(StickCenter::apply)(uint8_t index, int16_t& lx, int16_t& ly,
                                             int16_t& rx, int16_t& ry, int16_t restSnapThreshold) {
  if (index >= MAX_CONTROLLERS || !offsets[index].calibrated) return;

  lx = clampInt16(static_cast<int32_t>(lx) - offsets[index].lx);
  ly = clampInt16(static_cast<int32_t>(ly) - offsets[index].ly);
  rx = clampInt16(static_cast<int32_t>(rx) - offsets[index].rx);
  ry = clampInt16(static_cast<int32_t>(ry) - offsets[index].ry);

  snapAxisToCenter(lx, restSnapThreshold);
  snapAxisToCenter(ly, restSnapThreshold);
  snapAxisToCenter(rx, restSnapThreshold);
  snapAxisToCenter(ry, restSnapThreshold);
}

bool StickCenter::hasDrift(uint8_t index) const {
  if (index >= MAX_CONTROLLERS || !offsets[index].calibrated) return false;
  return absInt16(offsets[index].lx) > DEFAULT_DRIFT_WARNING_THRESHOLD ||
         absInt16(offsets[index].ly) > DEFAULT_DRIFT_WARNING_THRESHOLD ||
         absInt16(offsets[index].rx) > DEFAULT_DRIFT_WARNING_THRESHOLD ||
         absInt16(offsets[index].ry) > DEFAULT_DRIFT_WARNING_THRESHOLD;
}

int16_t StickCenter::getMaxDrift(uint8_t index) const {
  if (index >= MAX_CONTROLLERS || !offsets[index].calibrated) return 0;
  int32_t maxDrift = absInt16(offsets[index].lx);
  maxDrift = max(maxDrift, absInt16(offsets[index].ly));
  maxDrift = max(maxDrift, absInt16(offsets[index].rx));
  maxDrift = max(maxDrift, absInt16(offsets[index].ry));
  return (int16_t)min(maxDrift, (int32_t)INT16_MAX);
}

uint8_t StickCenter::getDriftPercent(uint8_t index) const {
  int16_t maxDrift = getMaxDrift(index);
  uint32_t percent = ((uint32_t)maxDrift * 100u) / INT8_MAX;
  return (uint8_t)min(percent, 100u);
}

bool StickCenter::isCalibrated(uint8_t index) const {
  return index < MAX_CONTROLLERS && offsets[index].calibrated;
}

const StickOffsets& StickCenter::getOffsets(uint8_t index) const {
  static const StickOffsets empty = { 0, 0, 0, 0, false };
  return index < MAX_CONTROLLERS ? offsets[index] : empty;
}

int16_t StickCenter::clampInt16(int32_t value) {
  if (value > INT16_MAX) return INT16_MAX;
  if (value < INT16_MIN) return INT16_MIN;
  return static_cast<int16_t>(value);
}

int32_t StickCenter::absInt16(int16_t value) {
  return value < 0 ? -static_cast<int32_t>(value) : static_cast<int32_t>(value);
}

bool __not_in_flash_func(StickCenter::axisWithin)(int16_t value, int16_t threshold) {
  return absInt16(value) <= absInt16(threshold);
}

void StickCenter::snapAxisToCenter(int16_t& value, int16_t threshold) {
  if (axisWithin(value, threshold)) {
    value = 0;
  }
}

StickCenter stickCenter;
