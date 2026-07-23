#pragma once

#include <stdint.h>

#include "controller_state.h"

// Shared analog stick calibration data used by the menu, WebHID, and settings
// persistence code without forcing extra translation units to include menu.h.

struct AnalogAxisCalibration {
  int8_t raw_min;
  int8_t raw_max;
  int8_t raw_center;
};

struct AnalogCalibrationData {
  AnalogAxisCalibration lx;
  AnalogAxisCalibration ly;
  AnalogAxisCalibration rx;
  AnalogAxisCalibration ry;
  bool enabled;
};

extern AnalogCalibrationData analogCalibration;
int8_t applyAnalogCalibration(int8_t raw, int8_t raw_min, int8_t raw_max);
void applyAnalogCalibrationToReport(controller_state_t& report);
void loadAnalogCalibration();
void saveAnalogCalibration();
void clearAnalogCalibration();
