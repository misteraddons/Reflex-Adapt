#include "../../product_config.h"

#include "boot_hardware_runtime.h"

#include <Adafruit_TinyUSB.h>
#include <Arduino.h>

#include "../../core/firmware_support.h"
#include "../../output/runtime/output_boot_bridge.h"

void prepareHardwareForBoot() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  tud_disconnect();
  #if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  rp2040_usb_install_debug_irq();
  #endif
  #endif

  Serial.end();

  modeButton.begin();
  resetButton.begin();

  delay(10);
  if (digitalRead(pinBtn) == LOW) {
    #ifdef LED_BUILTIN
      pinMode(LED_BUILTIN, OUTPUT);
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
      }
    #endif
    rp2040.rebootToBootloader();
  }

  #ifdef LED_BUILTIN
    gpio_init(LED_BUILTIN);
    gpio_set_dir(LED_BUILTIN, 1);
    gpio_put(LED_BUILTIN, LOW);
  #endif
}
