#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../core/firmware_support.h"
#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
#ifdef ENABLE_INPUT_NEOGEO
AutoDetectResult AutoDetector::probeNeoGeo(const autodetect_pins_t& pins) {
  uint8_t neoPins[] = {
    pins.neo_up, pins.neo_down, pins.neo_left, pins.neo_right,
    pins.neo_a, pins.neo_b, pins.neo_c, pins.neo_d,
    pins.neo_sel, pins.neo_start
  };

  for (uint8_t i = 0; i < sizeof(neoPins); i++) {
    gpio_init(neoPins[i]);
    gpio_set_dir(neoPins[i], GPIO_IN);
    gpio_pull_up(neoPins[i]);
  }

  delayMicroseconds(50);

  uint8_t readings[10];
  for (uint8_t i = 0; i < sizeof(neoPins); i++) {
    readings[i] = gpio_get(neoPins[i]);
  }

  bool neoSpecificActive = readings[9] == 0;

  if (neoSpecificActive) {
    autoDetectDelay(20);
    bool neoSpecificStable = gpio_get(pins.neo_start) == 0;
    if (neoSpecificStable) {
      return AUTODETECT_NEOGEO;
    }
  }

  return AUTODETECT_NONE;
}
#endif
#endif
