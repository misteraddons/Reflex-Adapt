#pragma once

#include <cstdint>

#include "../firmware_platform_config.h"
#include "controller_state.h"
#include "device_mode.h"

// Cached snapshots used by menu/debug/output inspection paths.
struct RawAnalogInputSnapshot {
  DeviceEnum mode;
  bool connected;
  bool has_main_stick;
  bool has_aux_stick;
  analog_stick_precision precision;
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;
};

void captureRawAnalogInputSnapshots(DeviceEnum mode);
bool getRawAnalogInputSnapshot(DeviceEnum mode, uint8_t port,
                               RawAnalogInputSnapshot& snapshot);

extern uint32_t raw_input_buttons[MAX_USB_OUT];
extern uint32_t pre_remap_buttons[MAX_USB_OUT];
extern uint32_t pre_transform_hotkey_buttons[MAX_USB_OUT];
extern uint32_t post_remap_buttons[MAX_USB_OUT];
extern int16_t post_output_lx[MAX_USB_OUT];
extern int16_t post_output_ly[MAX_USB_OUT];
extern int16_t post_output_rx[MAX_USB_OUT];
extern int16_t post_output_ry[MAX_USB_OUT];
extern uint8_t post_output_l2[MAX_USB_OUT];
extern uint8_t post_output_r2[MAX_USB_OUT];
extern uint8_t debug_hid_x[MAX_USB_OUT];
extern uint8_t debug_hid_y[MAX_USB_OUT];
extern uint8_t debug_hid_z[MAX_USB_OUT];
extern uint8_t debug_hid_rx[MAX_USB_OUT];
