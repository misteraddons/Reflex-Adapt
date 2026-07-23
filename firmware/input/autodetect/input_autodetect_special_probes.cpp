#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../core/firmware_support.h"
#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
#ifdef ENABLE_INPUT_PADDLE
AutoDetectResult AutoDetector::probeAtariPaddle(const autodetect_pins_t& pins, uint8_t port) {
  if (port != 1) {
    return AUTODETECT_NONE;
  }

  uint8_t potAGpio = pins.pdl_pot_a;
  uint8_t potBGpio = pins.pdl_pot_b;
  if (potAGpio < 26 || potAGpio > 29 || potBGpio < 26 || potBGpio > 29) {
    return AUTODETECT_NONE;
  }

  analogReadResolution(12);
  uint32_t sumA = 0;
  uint32_t sumB = 0;
  constexpr int kSamples = 6;
  for (int i = 0; i < kSamples; i++) {
    sumA += analogRead(potAGpio);
    sumB += analogRead(potBGpio);
    delayMicroseconds(100);
  }
  uint16_t avgA = sumA / kSamples;
  uint16_t avgB = sumB / kSamples;

  bool validA = (avgA > 100 && avgA < 3900);
  bool validB = (avgB > 100 && avgB < 3900);

  auto measureRc = [](uint8_t pin) -> uint32_t {
    constexpr uint32_t kRcDischargeUs = 12;
    constexpr uint32_t kRcTimeoutUs = 4000;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(kRcDischargeUs);

    uint32_t start = micros();
    pinMode(pin, INPUT);

    uint32_t elapsed = 0;
    while ((elapsed = micros() - start) < kRcTimeoutUs) {
      if (digitalRead(pin) == HIGH) {
        break;
      }
    }
    return elapsed;
  };

  uint32_t rcSumA = 0;
  uint32_t rcSumB = 0;
  for (int i = 0; i < 4; i++) {
    rcSumA += measureRc(potAGpio);
    rcSumB += measureRc(potBGpio);
  }
  uint32_t rcAvgA = rcSumA / 4;
  uint32_t rcAvgB = rcSumB / 4;

  bool rcValidA = (rcAvgA > 4 && rcAvgA < 3950);
  bool rcValidB = (rcAvgB > 4 && rcAvgB < 3950);

  if (validA || validB || rcValidA || rcValidB) {
    return AUTODETECT_ATARI_PADDLE;
  }

  return AUTODETECT_NONE;
}
#endif

#ifdef ENABLE_INPUT_DRIVING
AutoDetectResult AutoDetector::probeAtariDriving(const autodetect_pins_t& pins, bool is_hotswap) {
  gpio_init(pins.drv_enc_a);
  gpio_init(pins.drv_enc_b);
  gpio_init(pins.drv_btn);
  gpio_set_dir(pins.drv_enc_a, GPIO_IN);
  gpio_set_dir(pins.drv_enc_b, GPIO_IN);
  gpio_set_dir(pins.drv_btn, GPIO_IN);
  gpio_pull_up(pins.drv_enc_a);
  gpio_pull_up(pins.drv_enc_b);
  gpio_pull_up(pins.drv_btn);

  delayMicroseconds(50);

  uint8_t initA = gpio_get(pins.drv_enc_a);
  uint8_t initB = gpio_get(pins.drv_enc_b);
  uint8_t initState = (initA << 1) | initB;

  uint16_t sampleCount = is_hotswap ? 720 : 900;
  constexpr uint16_t kSampleDelayUs = 125;
  uint8_t stateChanges = 0;
  uint8_t lastState = initState;

  for (uint16_t i = 0; i < sampleCount; ++i) {
    uint8_t a = gpio_get(pins.drv_enc_a);
    uint8_t b = gpio_get(pins.drv_enc_b);
    uint8_t currState = (a << 1) | b;

    if (currState != lastState) {
      uint8_t diff = currState ^ lastState;
      if (diff == 1 || diff == 2) {
        stateChanges++;
        if (stateChanges >= 2) {
          return AUTODETECT_ATARI_DRIVING;
        }
      }
      lastState = currState;
    }
    delayMicroseconds(kSampleDelayUs);
  }

  (void)gpio_get(pins.drv_btn);
  return AUTODETECT_NONE;
}
#endif

#ifdef ENABLE_INPUT_3DO
AutoDetectResult AutoDetector::probe3DO(const autodetect_pins_t& pins, bool is_hotswap) {
  gpio_init(pins.tdo_clk);
  gpio_init(pins.tdo_out);
  gpio_init(pins.tdo_in);

  gpio_set_dir(pins.tdo_clk, GPIO_OUT);
  gpio_set_dir(pins.tdo_out, GPIO_OUT);
  gpio_set_dir(pins.tdo_in, GPIO_IN);
  gpio_pull_up(pins.tdo_in);

  auto read3doFrame = [&]() -> uint16_t {
    gpio_put(pins.tdo_clk, HIGH);
    gpio_put(pins.tdo_out, HIGH);
    delayMicroseconds(30);

    gpio_put(pins.tdo_clk, LOW);
    delayMicroseconds(4);
    gpio_put(pins.tdo_out, LOW);

    uint16_t data = 0;
    for (uint8_t i = 0; i < 16; i++) {
      if (gpio_get(pins.tdo_in)) {
        data |= (1 << i);
      }

      gpio_put(pins.tdo_clk, HIGH);
      delayMicroseconds(4);
      gpio_put(pins.tdo_clk, LOW);
      delayMicroseconds(4);
    }

    gpio_put(pins.tdo_out, HIGH);
    gpio_put(pins.tdo_clk, HIGH);
    delayMicroseconds(4);
    return data;
  };

  auto isValid3do = [&](uint16_t data) -> bool {
    return (data != 0xFFFF) && (data != 0x0000) && ((data & 0xC007) == 0x0001);
  };

  uint8_t passes = is_hotswap ? 3 : 2;
  uint8_t validCount = 0;
  uint8_t stableValidPairs = 0;
  uint16_t prevData = 0xFFFF;
  bool prevValid = false;
  for (uint8_t pass = 0; pass < passes; ++pass) {
    uint16_t data = read3doFrame();
    bool valid = isValid3do(data);
    if (valid) {
      validCount++;
    }
    if (pass > 0 && valid && prevValid && data == prevData) {
      stableValidPairs++;
    }
    prevData = data;
    prevValid = valid;
    if (pass + 1 < passes) {
      autoDetectDelay(1);
    }
  }

  gpio_set_dir(pins.tdo_clk, GPIO_IN);
  gpio_set_dir(pins.tdo_out, GPIO_IN);

  if (is_hotswap) {
    if (validCount >= 2 && stableValidPairs >= 1) {
      return AUTODETECT_3DO;
    }
    return AUTODETECT_NONE;
  }

  if (validCount >= 1) {
    return AUTODETECT_3DO;
  }

  return AUTODETECT_NONE;
}
#endif
#endif
