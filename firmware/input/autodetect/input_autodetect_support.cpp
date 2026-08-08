#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include "input_autodetect_support.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
const autodetect_pins_t autodetect_pins[kAutoDetectPortCount] = {
  {
    .snes_clk = HDMI_1_01, .snes_lat = HDMI_1_02, .snes_dat = HDMI_1_03,
    .sat_d0 = HDMI_1_01, .sat_d1 = HDMI_1_02, .sat_d2 = HDMI_1_03, .sat_d3 = HDMI_1_04,
    .sat_th = HDMI_1_06, .sat_tr = HDMI_1_07, .sat_tl = HDMI_1_05,
    .joybus_dat = HDMI_1_13, .joybus_pio = pio0, .joybus_sm = 0,
    .psx_att = HDMI_1_13, .psx_cmd = HDMI_1_11, .psx_dat = HDMI_1_12, .psx_clk = HDMI_1_10, .psx_ack = HDMI_1_08,
    .dc_a = HDMI_1_08, .dc_b = HDMI_1_09,
    .tdo_clk = HDMI_1_06, .tdo_out = HDMI_1_05, .tdo_in = HDMI_1_07,
    .wii_sda = HDMI_1_10, .wii_scl = HDMI_1_11,
    .wii_alt_sda = HDMI_1_02, .wii_alt_scl = HDMI_1_01,
    .jag_j0 = HDMI_1_04, .jag_j1 = HDMI_1_03, .jag_j2 = HDMI_1_08, .jag_j3 = HDMI_1_07,
    .jag_b0 = HDMI_1_05, .jag_b1 = HDMI_1_06, .jag_j8 = HDMI_1_12, .jag_j9 = HDMI_1_09, .jag_j10 = HDMI_1_02, .jag_j11 = HDMI_1_01,
    .neo_up = HDMI_1_10, .neo_down = HDMI_1_05, .neo_left = HDMI_1_07, .neo_right = HDMI_1_04,
    .neo_a = HDMI_1_11, .neo_b = HDMI_1_03, .neo_c = HDMI_1_06, .neo_d = HDMI_1_02,
    .neo_sel = HDMI_1_01, .neo_start = HDMI_1_12,
    .drv_enc_a = HDMI_1_01, .drv_enc_b = HDMI_1_02, .drv_btn = HDMI_1_05,
    .pdl_pot_a = HDMI_1_03, .pdl_pot_b = HDMI_1_04, .pdl_btn_a = HDMI_1_01, .pdl_btn_b = HDMI_1_02,
    .sms_up = HDMI_1_01, .sms_down = HDMI_1_02, .sms_left = HDMI_1_03, .sms_right = HDMI_1_04,
    .sms_tl = HDMI_1_05, .sms_th = HDMI_1_06, .sms_tr = HDMI_1_07
  },
  {
    .snes_clk = HDMI_2_01, .snes_lat = HDMI_2_02, .snes_dat = HDMI_2_03,
    .sat_d0 = HDMI_2_01, .sat_d1 = HDMI_2_02, .sat_d2 = HDMI_2_03, .sat_d3 = HDMI_2_04,
    .sat_th = HDMI_2_06, .sat_tr = HDMI_2_07, .sat_tl = HDMI_2_05,
    .joybus_dat = HDMI_2_13, .joybus_pio = pio0, .joybus_sm = 1,
    .psx_att = HDMI_2_13, .psx_cmd = HDMI_2_11, .psx_dat = HDMI_2_12, .psx_clk = HDMI_2_10, .psx_ack = HDMI_2_08,
    .dc_a = HDMI_2_08, .dc_b = HDMI_2_09,
    .tdo_clk = HDMI_2_06, .tdo_out = HDMI_2_05, .tdo_in = HDMI_2_07,
    .wii_sda = HDMI_2_10, .wii_scl = HDMI_2_11,
    .wii_alt_sda = HDMI_2_02, .wii_alt_scl = HDMI_2_01,
    .jag_j0 = HDMI_2_04, .jag_j1 = HDMI_2_03, .jag_j2 = HDMI_2_08, .jag_j3 = HDMI_2_07,
    .jag_b0 = HDMI_2_05, .jag_b1 = HDMI_2_06, .jag_j8 = HDMI_2_12, .jag_j9 = HDMI_2_09, .jag_j10 = HDMI_2_02, .jag_j11 = HDMI_2_01,
    .neo_up = HDMI_2_10, .neo_down = HDMI_2_05, .neo_left = HDMI_2_07, .neo_right = HDMI_2_04,
    .neo_a = HDMI_2_11, .neo_b = HDMI_2_03, .neo_c = HDMI_2_06, .neo_d = HDMI_2_02,
    .neo_sel = HDMI_2_01, .neo_start = HDMI_2_12,
    .drv_enc_a = HDMI_2_01, .drv_enc_b = HDMI_2_02, .drv_btn = HDMI_2_05,
    .pdl_pot_a = HDMI_2_03, .pdl_pot_b = HDMI_2_04, .pdl_btn_a = HDMI_2_01, .pdl_btn_b = HDMI_2_02,
    .sms_up = HDMI_2_01, .sms_down = HDMI_2_02, .sms_left = HDMI_2_03, .sms_right = HDMI_2_04,
    .sms_tl = HDMI_2_05, .sms_th = HDMI_2_06, .sms_tr = HDMI_2_07
  },
};

AutoDetectResult detectAutoInputPort(uint8_t port, bool is_hotswap) {
  return AutoDetector::detectPort(port, is_hotswap);
}

AutoDetectResult detectAutoInputPortPsxOnly(uint8_t port, bool is_hotswap) {
  return AutoDetector::detectPortPsxOnly(port, is_hotswap);
}

AutoDetectResult detectAutoInputPortDreamcastOnly(uint8_t port, bool is_hotswap) {
  return AutoDetector::detectPortDreamcastOnly(port, is_hotswap);
}

AutoDetectResult detectAutoInputPortShiftRegisterOnly(uint8_t port, bool is_hotswap) {
  return AutoDetector::detectPortShiftRegisterOnly(port, is_hotswap);
}

void AutoDetector::resetSharedPins(const autodetect_pins_t& pins, bool is_hotswap) {
  uint8_t sharedPins[] = {
    pins.snes_clk, pins.snes_lat, pins.snes_dat,
    pins.sat_d0, pins.sat_d1, pins.sat_d2, pins.sat_d3,
    pins.sat_th, pins.sat_tr, pins.sat_tl,
    pins.dc_a, pins.dc_b
  };
  for (uint8_t i = 0; i < sizeof(sharedPins); i++) {
    gpio_set_oeover(sharedPins[i], GPIO_OVERRIDE_NORMAL);
    gpio_set_outover(sharedPins[i], GPIO_OVERRIDE_NORMAL);
    gpio_set_function(sharedPins[i], GPIO_FUNC_SIO);
    gpio_init(sharedPins[i]);
    gpio_set_dir(sharedPins[i], GPIO_IN);
    gpio_pull_up(sharedPins[i]);
  }
  autoDetectDelay(is_hotswap ? 8 : 50);
}

#endif
