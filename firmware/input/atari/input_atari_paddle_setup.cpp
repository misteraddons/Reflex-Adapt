#include "Input_AtariPaddle.h"

#define PADDLE_DISCHARGE_US     500
#define PADDLE_TIMEOUT_US       120000
#define PADDLE_ADC_THRESHOLD    2048
#define PADDLE_SAMPLES          2
#define PADDLE_MIN_TIME         100
#define PADDLE_MAX_TIME         100000

const input_paddle_config_t input_paddle_config = {
  .pot_a = HDMI_1_03,
  .pot_b = HDMI_1_04,
  .button_a = HDMI_1_06,
  .button_b = HDMI_1_05,
};

RZInputPaddle::RZInputPaddle() : RZInputModule() {}

uint32_t RZInputPaddle::readPaddleRC(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(PADDLE_DISCHARGE_US);
  uint32_t start = micros();
  pinMode(pin, INPUT);
  uint32_t elapsed;
  while ((elapsed = micros() - start) < PADDLE_TIMEOUT_US) {
    if (analogRead(pin) >= PADDLE_ADC_THRESHOLD) {
      break;
    }
  }
  return elapsed;
}

uint32_t RZInputPaddle::getSmoothedValue(uint32_t* samples) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < PADDLE_SAMPLES; i++) {
    sum += samples[i];
  }
  return sum / PADDLE_SAMPLES;
}

int16_t RZInputPaddle::timeToAxis(uint32_t time, uint32_t& cal_min, uint32_t& cal_max) {
  if (time > PADDLE_MIN_TIME && time < cal_min) cal_min = time;
  if (time < PADDLE_MAX_TIME && time > cal_max) cal_max = time;
  if (time < cal_min) time = cal_min;
  if (time > cal_max) time = cal_max;
  uint32_t range = cal_max - cal_min;
  if (range < 100) range = 100;
  int32_t normalized = ((int32_t)(time - cal_min) * 65535 / range) - 32768;
  if (normalized < -32768) normalized = -32768;
  if (normalized > 32767) normalized = 32767;
  return (int16_t)normalized;
}

const char* RZInputPaddle::getUsbId() { return "RZRPadl"; }

void RZInputPaddle::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_PADDLE;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputPaddle::getDescription() { return "ATARI PADDLE"; }

void RZInputPaddle::setup() {
  pollInterval = 250000;
  setInputPortCount(input_ports);
  analogReadResolution(12);
  pinMode(input_paddle_config.button_a, INPUT_PULLUP);
  pinMode(input_paddle_config.button_b, INPUT_PULLUP);

  for (uint8_t i = 0; i < PADDLE_SAMPLES; i++) {
    samples_a[i] = (PADDLE_MIN_TIME + PADDLE_MAX_TIME) / 2;
    samples_b[i] = (PADDLE_MIN_TIME + PADDLE_MAX_TIME) / 2;
  }

  for (uint8_t p = 0; p < input_ports && p < MAX_USB_OUT; ++p) {
    controller_state_t& frame = inputFrame(p);
    setInputFrameConnected(frame, true);
    setInputFrameTypeName(frame, "Paddle");
  }
}

void RZInputPaddle::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}

uint32_t RZInputPaddle::getCalMinA() { return cal_min_a; }
uint32_t RZInputPaddle::getCalMaxA() { return cal_max_a; }
uint32_t RZInputPaddle::getCalMinB() { return cal_min_b; }
uint32_t RZInputPaddle::getCalMaxB() { return cal_max_b; }
uint8_t RZInputPaddle::getPotAPin() { return input_paddle_config.pot_a; }
uint8_t RZInputPaddle::getPotBPin() { return input_paddle_config.pot_b; }
uint8_t RZInputPaddle::getBtnAPin() { return input_paddle_config.button_a; }
uint8_t RZInputPaddle::getBtnBPin() { return input_paddle_config.button_b; }

void RZInputPaddle::resetCalibration() {
  cal_min_a = PADDLE_MAX_TIME;
  cal_max_a = PADDLE_MIN_TIME;
  cal_min_b = PADDLE_MAX_TIME;
  cal_max_b = PADDLE_MIN_TIME;
}
