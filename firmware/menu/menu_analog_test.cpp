#include "../product_config.h"

#include "menu_analog_test.h"

#include "../core/analog_calibration_state.h"
#include "../platform/buzzer.h"
#include "../core/controller_frame_state.h"
#include "../platform/display_runtime_state.h"
#include "menu_mode_state.h"
#include "menu_ui_state.h"

namespace {

bool analogCalibrationMode = false;
int8_t cal_live_lx_min, cal_live_lx_max;
int8_t cal_live_ly_min, cal_live_ly_max;
int8_t cal_live_rx_min, cal_live_rx_max;
int8_t cal_live_ry_min, cal_live_ry_max;

void startAnalogCalibration() {
  analogCalibrationMode = true;
  cal_live_lx_min = cal_live_lx_max = 0;
  cal_live_ly_min = cal_live_ly_max = 0;
  cal_live_rx_min = cal_live_rx_max = 0;
  cal_live_ry_min = cal_live_ry_max = 0;
}

void updateAnalogCalibration(int8_t lx, int8_t ly, int8_t rx, int8_t ry) {
  if (!analogCalibrationMode) return;

  if (lx < cal_live_lx_min) cal_live_lx_min = lx;
  if (lx > cal_live_lx_max) cal_live_lx_max = lx;
  if (ly < cal_live_ly_min) cal_live_ly_min = ly;
  if (ly > cal_live_ly_max) cal_live_ly_max = ly;
  if (rx < cal_live_rx_min) cal_live_rx_min = rx;
  if (rx > cal_live_rx_max) cal_live_rx_max = rx;
  if (ry < cal_live_ry_min) cal_live_ry_min = ry;
  if (ry > cal_live_ry_max) cal_live_ry_max = ry;
}

void finishAnalogCalibration() {
  analogCalibration.lx.raw_min = cal_live_lx_min;
  analogCalibration.lx.raw_max = cal_live_lx_max;
  analogCalibration.ly.raw_min = cal_live_ly_min;
  analogCalibration.ly.raw_max = cal_live_ly_max;
  analogCalibration.rx.raw_min = cal_live_rx_min;
  analogCalibration.rx.raw_max = cal_live_rx_max;
  analogCalibration.ry.raw_min = cal_live_ry_min;
  analogCalibration.ry.raw_max = cal_live_ry_max;
  analogCalibration.enabled = true;
  analogCalibrationMode = false;
  saveAnalogCalibration();
}

void cancelAnalogCalibration() {
  analogCalibrationMode = false;
}

}  // namespace

void renderAnalogTest(bool modeBtnJustPressed) {
#ifndef USE_I2C_DISPLAY
  (void)modeBtnJustPressed;
  analogTestActive = false;
  analogTestInitialized = false;
  return;
#else
  static int8_t prevLX = 127, prevLY = 127, prevRX = 127, prevRY = 127;
  static uint8_t prevL2 = 255, prevR2 = 255;
  static bool prevCalMode = false;
  static uint8_t menuSelection = 0;
  const controller_state_t& frame = controllerFrameConst(0);

  static bool prev_up = false, prev_down = false, prev_a = false;
  bool cur_up = frame.PAD_U;
  bool cur_down = frame.PAD_D;
  bool cur_a = frame.A || frame.START;
  bool up_just = cur_up && !prev_up;
  bool down_just = cur_down && !prev_down;
  bool a_just = cur_a && !prev_a;
  prev_up = cur_up;
  prev_down = cur_down;
  prev_a = cur_a;

  if (!analogTestInitialized) {
    analogTestInitialized = true;
    display.clear();
    menuSelection = 0;
    prevCalMode = false;
    prevLX = prevLY = prevRX = prevRY = 127;
    prevL2 = prevR2 = 255;
  }

  int8_t lx = frame.LX;
  int8_t ly = frame.LY;
  int8_t rx = frame.RX;
  int8_t ry = frame.RY;

  if (analogCalibrationMode) {
    updateAnalogCalibration(lx, ly, rx, ry);
  }

  if (prevCalMode != analogCalibrationMode) {
    display.clear();
    prevCalMode = analogCalibrationMode;
    prevLX = prevLY = prevRX = prevRY = 127;
  }

  display.setFont(System5x7);

  if (analogCalibrationMode) {
    display.setRow(0);
    display.setCol(0);
    display.print("CALIBRATING - Move sticks");

    display.setRow(2);
    display.setCol(0);
    display.print("LX:");
    display.print(cal_live_lx_min);
    display.print("/");
    display.print(cal_live_lx_max);
    display.print("  LY:");
    display.print(cal_live_ly_min);
    display.print("/");
    display.print(cal_live_ly_max);

    display.setRow(3);
    display.setCol(0);
    display.print("RX:");
    display.print(cal_live_rx_min);
    display.print("/");
    display.print(cal_live_rx_max);
    display.print("  RY:");
    display.print(cal_live_ry_min);
    display.print("/");
    display.print(cal_live_ry_max);

    display.setRow(5);
    display.setCol(0);
    display.print("Now: L(");
    display.print(lx);
    display.print(",");
    display.print(ly);
    display.print(") R(");
    display.print(rx);
    display.print(",");
    display.print(ry);
    display.print(")   ");

    display.setRow(7);
    display.setCol(0);
    display.print("A=Save  Mode=Cancel");

    if (a_just) {
      finishAnalogCalibration();
      buzzer.playSave();
      display.clear();
      prevLX = prevLY = prevRX = prevRY = 127;
    }

    if (modeBtnJustPressed) {
      cancelAnalogCalibration();
      buzzer.playMenuNav();
      display.clear();
      prevLX = prevLY = prevRX = prevRY = 127;
    }
  } else {
    display.setRow(0);
    display.setCol(0);
    if (analogCalibration.enabled) {
      display.print("Analog Test [CAL ON]");
    } else {
      display.print("Analog Test          ");
    }

    if (lx != prevLX || ly != prevLY) {
      display.setRow(2);
      display.setCol(0);
      display.print("LX:");
      display.setCol(18);
      display.print("     ");
      display.setCol(18);
      display.print(lx);
      display.setCol(54);
      display.print("LY:");
      display.setCol(72);
      display.print("     ");
      display.setCol(72);
      display.print(ly);
      prevLX = lx;
      prevLY = ly;
    }

    bool hasRightStick = input_has_right_stick();
    if (hasRightStick && (rx != prevRX || ry != prevRY)) {
      display.setRow(3);
      display.setCol(0);
      display.print("RX:");
      display.setCol(18);
      display.print("     ");
      display.setCol(18);
      display.print(rx);
      display.setCol(54);
      display.print("RY:");
      display.setCol(72);
      display.print("     ");
      display.setCol(72);
      display.print(ry);
      prevRX = rx;
      prevRY = ry;
    }

    uint8_t infoRow = hasRightStick ? 4 : 3;

    if (frame.HAS_ANALOG_TRIGGERS) {
      uint8_t l2 = frame.ANALOG_L2;
      uint8_t r2 = frame.ANALOG_R2;
      if (l2 != prevL2 || r2 != prevR2) {
        display.setRow(infoRow);
        display.setCol(0);
        display.print("L2:");
        display.setCol(18);
        display.print("     ");
        display.setCol(18);
        display.print(l2);
        display.setCol(54);
        display.print("R2:");
        display.setCol(72);
        display.print("     ");
        display.setCol(72);
        display.print(r2);
        prevL2 = l2;
        prevR2 = r2;
      }
    } else if (analogCalibration.enabled) {
      display.setRow(infoRow);
      display.setCol(0);
      display.print("Cal: ");
      display.print(analogCalibration.lx.raw_min);
      display.print("-");
      display.print(analogCalibration.lx.raw_max);
      display.print(" / ");
      display.print(analogCalibration.ly.raw_min);
      display.print("-");
      display.print(analogCalibration.ly.raw_max);
    }

    display.setRow(5);
    display.setCol(0);
    display.print(menuSelection == 0 ? ">" : " ");
    display.print("Calibrate");

    display.setRow(6);
    display.setCol(0);
    display.print(menuSelection == 1 ? ">" : " ");
    display.print("Clear Cal");

    display.setRow(7);
    display.setCol(0);
    display.print(menuSelection == 2 ? ">" : " ");
    display.print("[Back]");

    if (down_just && menuSelection < 2) {
      menuSelection++;
      buzzer.playMenuNav();
    }
    if (up_just && menuSelection > 0) {
      menuSelection--;
      buzzer.playMenuNav();
    }

    if (a_just || modeBtnJustPressed) {
      switch (menuSelection) {
        case 0:
          startAnalogCalibration();
          buzzer.playMenuEnter();
          display.clear();
          break;
        case 1:
          clearAnalogCalibration();
          buzzer.playSave();
          display.clear();
          prevLX = prevLY = prevRX = prevRY = 127;
          break;
        case 2:
          analogTestActive = false;
          analogTestInitialized = false;
          break;
      }
    }
  }
#endif
}
