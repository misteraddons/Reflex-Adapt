#pragma once

// Stick Auto-Centering Module
// Captures analog stick baseline when controller connects and compensates for drift
// This ensures sticks read as centered (0) when at their physical rest position

#include <Arduino.h>

#define MAX_CONTROLLERS 4

struct StickOffsets {
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;
  bool calibrated;
};

class StickCenter {
private:
  StickOffsets offsets[MAX_CONTROLLERS];
  bool was_connected[MAX_CONTROLLERS];

  // Default thresholds are in 8-bit stick units. Callers with wider precision
  // should scale these before calling update/apply.
  static constexpr int16_t DEFAULT_MAX_CALIBRATION_OFFSET = 10;
  static constexpr int16_t DEFAULT_REST_SNAP_THRESHOLD = 2;
  static constexpr int16_t DEFAULT_DRIFT_WARNING_THRESHOLD = 12;

public:
  StickCenter();

  void reset();

  // Call this for each controller on every poll
  // Returns true if we just calibrated (newly connected)
  bool update(uint8_t index, bool connected, int16_t lx, int16_t ly, int16_t rx, int16_t ry,
              int16_t maxCalibrationOffset = DEFAULT_MAX_CALIBRATION_OFFSET);

  // Apply centering offset to analog values
  // Call this BEFORE deadzone processing
  void apply(uint8_t index, int16_t& lx, int16_t& ly, int16_t& rx, int16_t& ry,
             int16_t restSnapThreshold = DEFAULT_REST_SNAP_THRESHOLD);

  // Check if a controller has significant drift
  bool hasDrift(uint8_t index) const;

  // Get max drift amount for display purposes
  int16_t getMaxDrift(uint8_t index) const;

  // Get percentage of drift (0-100)
  uint8_t getDriftPercent(uint8_t index) const;

  bool isCalibrated(uint8_t index) const;

  // Get raw offsets for debugging/display
  const StickOffsets& getOffsets(uint8_t index) const;

private:
  static int16_t clampInt16(int32_t value);
  static int32_t absInt16(int16_t value);
  static bool axisWithin(int16_t value, int16_t threshold);
  static void snapAxisToCenter(int16_t& value, int16_t threshold);
};

// Global instance
extern StickCenter stickCenter;
