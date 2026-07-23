#include "firmware_support.h"

#include "../platform/buzzer.h"
#include "hardware/watchdog.h"
#include "settings_store.h"
#include "turbo.h"

BootselButtonHandler resetButton;
ButtonHandler modeButton(pinBtn, true);

ps4_device_type_emum ps4_type = PS4_DEVICE_TYPE_GAMEPAD;

void reboot(void) {
  watchdog_enable(1, false);
  for (;;) {
  }
}

void setLed(uint8_t state) {
  #ifdef LED_BUILTIN
    gpio_put(LED_BUILTIN, state);
  #endif
}

void delayWithButtonPolling(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    modeButton.poll();
    resetButton.poll();
    buzzer.update();
    delay(1);
  }
}

void waitWithBuzzerUpdates(uint32_t durationMs) {
  uint32_t waitStart = millis();
  while (millis() - waitStart < durationMs) {
    buzzer.update();
    delay(5);
  }
}

void webhid_set_turbo_rate(uint8_t btn_index, uint8_t rate) {
  if (btn_index < TURBO_BTN_COUNT && rate < TURBO_RATE_LAST) {
    turbo.setButtonRate((TurboButton)btn_index, (TurboRate)rate);
  }
}

