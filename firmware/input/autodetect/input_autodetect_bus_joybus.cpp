#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../core/firmware_support.h"
#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
#if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
AutoDetectResult AutoDetector::probeJoybus(const autodetect_pins_t& pins, uint8_t port, bool is_hotswap) {
  AutoDetectResult result = AUTODETECT_NONE;
  constexpr uint16_t kGcWirelessBitmask = 0x8900;

  gpio_set_oeover(pins.joybus_dat, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(pins.joybus_dat, GPIO_OVERRIDE_NORMAL);
  gpio_set_function(pins.joybus_dat, GPIO_FUNC_SIO);
  gpio_init(pins.joybus_dat);
  gpio_set_dir(pins.joybus_dat, GPIO_IN);
  gpio_pull_up(pins.joybus_dat);

  autoDetectDelay(10);

  pio_sm_set_enabled(pins.joybus_pio, pins.joybus_sm, false);
  pio_sm_clear_fifos(pins.joybus_pio, pins.joybus_sm);
  joybus_pio_set_timeout_ms(is_hotswap ? 18 : 50);

  JoybusPIOInstance instance = joybus_pio_program_init(pins.joybus_pio, pins.joybus_sm, pins.joybus_dat);
  autoDetectDelay(is_hotswap ? 12 : 50);

  const uint8_t attempts = is_hotswap ? 1 : 5;
  for (uint8_t attempt = 0; attempt < attempts && result == AUTODETECT_NONE; attempt++) {
    if (attempt > 0) {
      joybus_pio_reset(instance);
      autoDetectDelay(20);
    }

    JoybusControllerInfo info = joybus_handshake(instance, attempt == 0);
    uint16_t type = info.type;

    if (type == 0x0000 || type == 0xA800) {
      autoDetectDelay(6);
      JoybusControllerInfo followup = joybus_handshake(instance, false);
      if (followup.type != 0) {
        info = followup;
        type = info.type;
      }
    }

    #ifdef AUTODETECT_DEBUG
    if (type != 0) {
      lastDebug[port].joybus_type = type;
    }
    #endif

    // 0x0200 is N64 mouse; mouse input is intentionally not promoted by AUTO.
    if (type == 0x0500 || type == 0x0001 || type == 0x0002) {
      result = AUTODETECT_JOYBUS_N64;
    } else if (type == 0x0900 || type == 0x0800 || type == 0x0820 || type == 0xA800 ||
               (type & kGcWirelessBitmask) == kGcWirelessBitmask) {
      result = AUTODETECT_JOYBUS_GC;
    } else if (type == 0x0004) {
      result = AUTODETECT_JOYBUS_GBA;
    }
  }

  pio_sm_set_enabled(pins.joybus_pio, pins.joybus_sm, false);
  pio_sm_clear_fifos(pins.joybus_pio, pins.joybus_sm);
  gpio_set_oeover(pins.joybus_dat, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(pins.joybus_dat, GPIO_OVERRIDE_NORMAL);
  gpio_set_function(pins.joybus_dat, GPIO_FUNC_SIO);
  gpio_init(pins.joybus_dat);
  gpio_set_dir(pins.joybus_dat, GPIO_IN);
  gpio_pull_up(pins.joybus_dat);
  delay(5);
  joybus_pio_unload(pins.joybus_pio);

  return result;
}
#endif
#endif
