#include "controller_output_cache_state.h"

#include "controller_frame_state.h"

namespace {

RawAnalogInputSnapshot rawAnalogInputSnapshots[MAX_USB_OUT] = {};

}  // namespace

void captureRawAnalogInputSnapshots(DeviceEnum mode) {
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    RawAnalogInputSnapshot& snapshot = rawAnalogInputSnapshots[i];
    snapshot = {};
    snapshot.mode = mode;
    if (i >= max_devices) {
      continue;
    }

    const controller_state_t& frame = controllerFrameConst(i);
    snapshot.connected = frame.connected;
    snapshot.has_main_stick = frame.HAS_ANALOG_STICK_MAIN;
    snapshot.has_aux_stick = frame.HAS_ANALOG_STICK_AUX;
    snapshot.precision = frame.sticks_precision_bits;
    if (frame.connected) {
      snapshot.lx = frame.LX;
      snapshot.ly = frame.LY;
      snapshot.rx = frame.RX;
      snapshot.ry = frame.RY;
    }
  }
}

bool getRawAnalogInputSnapshot(DeviceEnum mode, uint8_t port,
                               RawAnalogInputSnapshot& snapshot) {
  if (port >= MAX_USB_OUT) {
    return false;
  }
  const RawAnalogInputSnapshot& cached = rawAnalogInputSnapshots[port];
  if (cached.mode != mode || !cached.connected) {
    return false;
  }
  snapshot = cached;
  return true;
}

uint32_t raw_input_buttons[MAX_USB_OUT] = { 0 };
uint32_t pre_remap_buttons[MAX_USB_OUT] = { 0 };
uint32_t pre_transform_hotkey_buttons[MAX_USB_OUT] = { 0 };
uint32_t post_remap_buttons[MAX_USB_OUT] = { 0 };
int16_t post_output_lx[MAX_USB_OUT] = { 0 };
int16_t post_output_ly[MAX_USB_OUT] = { 0 };
int16_t post_output_rx[MAX_USB_OUT] = { 0 };
int16_t post_output_ry[MAX_USB_OUT] = { 0 };
uint8_t post_output_l2[MAX_USB_OUT] = { 0 };
uint8_t post_output_r2[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_x[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_y[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_z[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_rx[MAX_USB_OUT] = { 0 };
