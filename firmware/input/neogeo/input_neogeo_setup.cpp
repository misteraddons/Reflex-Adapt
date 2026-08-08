#include "Input_Neogeo.h"

#include <hardware/gpio.h>

#include "../shared/input_button_bits.h"

namespace {

uint8_t outputButtonShift(uint32_t outputButton) {
  return (uint8_t)(__builtin_ctz(outputButton) + 1U);
}

}  // namespace

const input_neogeo_config_t input_neogeo_config[] = {
  { .buttons = { .up = HDMI_1_10, .down = HDMI_1_05, .left = HDMI_1_07, .right = HDMI_1_04, .a = HDMI_1_11, .b = HDMI_1_03, .c = HDMI_1_06, .d = HDMI_1_02, .select = HDMI_1_01, .start = HDMI_1_12 }, .debounceMs = 4 },
  { .buttons = { .up = HDMI_2_10, .down = HDMI_2_05, .left = HDMI_2_07, .right = HDMI_2_04, .a = HDMI_2_11, .b = HDMI_2_03, .c = HDMI_2_06, .d = HDMI_2_02, .select = HDMI_2_01, .start = HDMI_2_12 }, .debounceMs = 4 },
};

RZInputNeoGeo::RZInputNeoGeo() : RZInputModule() {}

const char* RZInputNeoGeo::getUsbId() {
  return "RZRNEOGEO";
}

void RZInputNeoGeo::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_NEOGEO;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputNeoGeo::getDescription() {
  return "NEO-GEO";
}

void RZInputNeoGeo::setup() {
  pollInterval = 0;
  setInputPortCount(input_ports);

  for (uint8_t i = 0; i < input_ports; ++i) {
    input_neogeo_config_t c = input_neogeo_config[i];
    uint32_t buttonsMask = 0;
    for (uint8_t j = 0; j < sizeof(c.btnArray); ++j) {
      buttonsMask |= (1UL << c.btnArray[j]);
    }
    buttonMask[i] = buttonsMask;
    outputButtonShiftByPin[i][c.buttons.up] = outputButtonShift(INPUT_PAD_U);
    outputButtonShiftByPin[i][c.buttons.down] = outputButtonShift(INPUT_PAD_D);
    outputButtonShiftByPin[i][c.buttons.left] = outputButtonShift(INPUT_PAD_L);
    outputButtonShiftByPin[i][c.buttons.right] = outputButtonShift(INPUT_PAD_R);
    outputButtonShiftByPin[i][c.buttons.a] = outputButtonShift(INPUT_A);
    outputButtonShiftByPin[i][c.buttons.b] = outputButtonShift(INPUT_B);
    outputButtonShiftByPin[i][c.buttons.c] = outputButtonShift(INPUT_X);
    outputButtonShiftByPin[i][c.buttons.d] = outputButtonShift(INPUT_Y);
    outputButtonShiftByPin[i][c.buttons.select] = outputButtonShift(INPUT_SELECT);
    outputButtonShiftByPin[i][c.buttons.start] = outputButtonShift(INPUT_START);
    acceptedRawState[i] = 0;
    lastRawState[i] = 0;
    acceptedButtons[i] = 0;
    for (uint8_t pin = 0; pin < 32; ++pin) {
      debounceBlockedUntilMs[i][pin] = 0;
    }
    gpio_init_mask(buttonsMask);
    gpio_set_dir_in_masked(buttonsMask);
    for (uint8_t j = 0; j < sizeof(c.btnArray); ++j) {
      gpio_pull_up(c.btnArray[j]);
    }

    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      frame.HAS_BTN_SELECT = 1;
      frame.HAS_BTN_START = 1;
      setInputFrameConnected(frame, true);
    }
  }
}

void RZInputNeoGeo::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}
