#include "../../../product_config.h"

#include <string.h>

#include "../../../core/button_remap.h"
#include "../../../core/classic_analog_range.h"
#include "../../../core/controller_frame_state.h"
#include "../../../core/controller_output_cache_state.h"
#include "../../../core/controller_settings_state.h"
#include "../../../core/device_runtime_state.h"
#include "../../../input/autodetect/input_autodetect_runtime_state.h"
#include "../../runtime/input_runtime_output_bridge.h"
#include "../../output_runtime_state.h"
#include "webhid_debug_reports.h"
#include "webhid_input_modes.h"
#include "webhid_protocol.h"

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  #include "hardware/gpio.h"
#endif

namespace {
uint8_t webhid_input_state_player = 0;
}

void webhid_set_input_state_player(const uint8_t* buffer, uint16_t bufsize) {
  if (bufsize >= 2 && buffer[0] == WEBHID_MAGIC_BYTE) {
    webhid_input_state_player = buffer[1];
  }
}

uint16_t webhid_get_input_state_report(uint8_t* buffer) {
  // Return live controller input state for visualization (split display: input vs output)
  // Format:
  // Byte 0: magic (0xAD)
  // Byte 1: player index (0-5)
  // Byte 2: connected flag
  // Bytes 3-6: OUTPUT buttons (32-bit, WITH remap/turbo applied)
  // Bytes 7-8: INPUT Left X (16-bit signed, raw)
  // Bytes 9-10: INPUT Left Y (16-bit signed, raw)
  // Bytes 11-12: INPUT Right X (16-bit signed, raw)
  // Bytes 13-14: INPUT Right Y (16-bit signed, raw)
  // Byte 15: Left trigger (8-bit)
  // Byte 16: Right trigger (8-bit)
  // Byte 17: Stick precision (0=8-bit, 1=12-bit, 2=16-bit)
  // Bytes 18-29: Controller type name (null-terminated)
  // Byte 30: Button count for current input mode
  // Bytes 31-34: Button mask (32-bit, which bits in digital_buttons are valid)
  // Byte 35: Stable WebHID input mode ID
  // Bytes 36-39: INPUT buttons (32-bit, RAW pre-remap state)
  // Byte 40: Output mode (outputMode_t)
  // Bytes 41-42: OUTPUT Left X (16-bit signed, with dpad_mode applied)
  // Bytes 43-44: OUTPUT Left Y (16-bit signed, with dpad_mode applied)
  // Bytes 45-46: OUTPUT Right X (16-bit signed, with dpad_mode applied)
  // Bytes 47-48: OUTPUT Right Y (16-bit signed, with dpad_mode applied)
  // Byte 49: dpad_mode
  // Byte 50: button_map_mode
  // Byte 51: n64_z_mode
  // Byte 52: configured n64_cstick_mode
  // Byte 53: effective n64_cstick_mode
  // Byte 54: configured output mode
  // Byte 55: effective output mode
  // Byte 56: raw left trigger before analog range learn/normalization
  // Byte 57: raw right trigger before analog range learn/normalization
  // Byte 58: OUTPUT left trigger after the shared output transform/cache path
  // Byte 59: OUTPUT right trigger after the shared output transform/cache path
  // Byte 60: stable WebHID saved input mode ID
  // Byte 61: input auto-detect active flag
  // Byte 62: reserved

  uint8_t player = webhid_input_state_player;
  if (player >= max_devices || player >= MAX_USB_OUT) {
    player = 0;
  }
  const controller_state_t& frame = controllerFrameConst(player);
  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = player;
  buffer[2] = frame.connected ? 1 : 0;

  uint32_t outputButtons = post_remap_buttons[player];
  buffer[3] = outputButtons & 0xFF;
  buffer[4] = (outputButtons >> 8) & 0xFF;
  buffer[5] = (outputButtons >> 16) & 0xFF;
  buffer[6] = (outputButtons >> 24) & 0xFF;

  ClassicAnalogRangeSnapshot analogSnapshot = {};
  const bool hasAnalogSnapshot =
      classicModeHasRangeSetting(deviceMode) &&
      getClassicAnalogRangeSnapshot(deviceMode, player, analogSnapshot);

  int16_t lx = frame.LX;
  int16_t ly = frame.LY;
  int16_t rx = frame.RX;
  int16_t ry = frame.RY;
  if (hasAnalogSnapshot) {
    if (analogSnapshot.valid[CLASSIC_ANALOG_AXIS_LX]) {
      lx = analogSnapshot.raw[CLASSIC_ANALOG_AXIS_LX];
    }
    if (analogSnapshot.valid[CLASSIC_ANALOG_AXIS_LY]) {
      ly = analogSnapshot.raw[CLASSIC_ANALOG_AXIS_LY];
    }
    if (analogSnapshot.valid[CLASSIC_ANALOG_AXIS_RX]) {
      rx = analogSnapshot.raw[CLASSIC_ANALOG_AXIS_RX];
    }
    if (analogSnapshot.valid[CLASSIC_ANALOG_AXIS_RY]) {
      ry = analogSnapshot.raw[CLASSIC_ANALOG_AXIS_RY];
    }
  }

  buffer[7] = lx & 0xFF;
  buffer[8] = (lx >> 8) & 0xFF;
  buffer[9] = ly & 0xFF;
  buffer[10] = (ly >> 8) & 0xFF;
  buffer[11] = rx & 0xFF;
  buffer[12] = (rx >> 8) & 0xFF;
  buffer[13] = ry & 0xFF;
  buffer[14] = (ry >> 8) & 0xFF;

  uint8_t rawAnalogL2 = frame.ANALOG_L2;
  uint8_t rawAnalogR2 = frame.ANALOG_R2;
  uint8_t calibratedAnalogL2 = frame.ANALOG_L2;
  uint8_t calibratedAnalogR2 = frame.ANALOG_R2;
  if (hasAnalogSnapshot) {
    if (analogSnapshot.trigger_valid[CLASSIC_ANALOG_TRIGGER_L2]) {
      rawAnalogL2 = analogSnapshot.trigger_raw[CLASSIC_ANALOG_TRIGGER_L2];
      calibratedAnalogL2 = analogSnapshot.trigger_calibrated[CLASSIC_ANALOG_TRIGGER_L2];
    }
    if (analogSnapshot.trigger_valid[CLASSIC_ANALOG_TRIGGER_R2]) {
      rawAnalogR2 = analogSnapshot.trigger_raw[CLASSIC_ANALOG_TRIGGER_R2];
      calibratedAnalogR2 = analogSnapshot.trigger_calibrated[CLASSIC_ANALOG_TRIGGER_R2];
    }
  }

  buffer[15] = calibratedAnalogL2;
  buffer[16] = calibratedAnalogR2;
  buffer[17] = frame.sticks_precision_bits;

  strncpy((char*)&buffer[18], frame.controller_type_name, 11);
  buffer[29] = 0;

  buffer[30] = getRemapButtonCount(deviceMode);

  uint32_t btnMask = getInputButtonMask(deviceMode);
  buffer[31] = btnMask & 0xFF;
  buffer[32] = (btnMask >> 8) & 0xFF;
  buffer[33] = (btnMask >> 16) & 0xFF;
  buffer[34] = (btnMask >> 24) & 0xFF;

  buffer[35] = webhid_input_mode_from_device(deviceMode);

  uint32_t inputButtons = raw_input_buttons[player];
  buffer[36] = inputButtons & 0xFF;
  buffer[37] = (inputButtons >> 8) & 0xFF;
  buffer[38] = (inputButtons >> 16) & 0xFF;
  buffer[39] = (inputButtons >> 24) & 0xFF;

  buffer[40] = (uint8_t)outputMode;

  int16_t out_lx = post_output_lx[player];
  int16_t out_ly = post_output_ly[player];
  int16_t out_rx = post_output_rx[player];
  int16_t out_ry = post_output_ry[player];
  buffer[41] = out_lx & 0xFF;
  buffer[42] = (out_lx >> 8) & 0xFF;
  buffer[43] = out_ly & 0xFF;
  buffer[44] = (out_ly >> 8) & 0xFF;
  buffer[45] = out_rx & 0xFF;
  buffer[46] = (out_rx >> 8) & 0xFF;
  buffer[47] = out_ry & 0xFF;
  buffer[48] = (out_ry >> 8) & 0xFF;

  buffer[49] = dpad_mode;
  buffer[50] = button_map_mode;
  buffer[51] = n64_z_mode;
  buffer[52] = n64_cstick_mode;
  buffer[53] = (uint8_t)get_effective_n64_cstick_mode();
  buffer[54] = (uint8_t)configuredOutputMode;
  buffer[55] = (uint8_t)get_effective_output_mode();
  buffer[56] = rawAnalogL2;
  buffer[57] = rawAnalogR2;
  buffer[58] = post_output_l2[player];
  buffer[59] = post_output_r2[player];
  buffer[60] = webhid_input_mode_from_device(savedDeviceMode);
  buffer[61] = inputAutoDetectModeActive() ? 1 : 0;
  buffer[62] = 0;

  return WEBHID_REPORT_SIZE;
}

uint16_t webhid_get_gpio_state_report(uint8_t* buffer) {
  // Return GPIO pin states for debugging
  // Format:
  // Byte 0: magic (0xAD)
  // Byte 1: port (0=Port1, 1=Port2)
  // Bytes 2-14: Pin 01-13 states (0=LOW, 1=HIGH)
  // Bytes 15-27: Pin 01-13 GPIO numbers
  // Byte 28: number of pins (13)

  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = 0;

  const uint8_t port1_pins[] = {11, 10, 26, 27, 9, 12, 13, 5, 6, 2, 7, 4, 3};
  const uint8_t port2_pins[] = {22, 21, 28, 29, 20, 23, 24, 16, 17, 15, 18, 19, 14};

  for (uint8_t i = 0; i < 13; i++) {
    buffer[2 + i] = gpio_get(port1_pins[i]) ? 1 : 0;
    buffer[15 + i] = port1_pins[i];
  }
  buffer[28] = 13;

  for (uint8_t i = 0; i < 13; i++) {
    buffer[29 + i] = gpio_get(port2_pins[i]) ? 1 : 0;
    buffer[42 + i] = port2_pins[i];
  }
  buffer[55] = 13;

  return WEBHID_REPORT_SIZE;
}

uint16_t webhid_get_raw_data_report(uint8_t* buffer) {
  // Return raw controller data buffer
  // Format:
  // Byte 0: magic (0xAD)
  // Byte 1: data length
  // Bytes 2-33: raw data (up to 32 bytes)
  // Byte 34: input mode (deviceMode)
  // Bytes 35+: controller type name

  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = webhid_raw_data_len;
  memcpy(&buffer[2], webhid_raw_data, webhid_raw_data_len);
  buffer[34] = deviceMode;
  strncpy((char*)&buffer[35], controllerFrameConst(0).controller_type_name, 12);

  return WEBHID_REPORT_SIZE;
}
