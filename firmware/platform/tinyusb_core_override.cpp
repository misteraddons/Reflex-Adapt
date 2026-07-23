#include <stdint.h>
#include "../product_config.h"
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED) && defined(ADAPT_OUTPUT_USB_DEVICE)
extern "C" bool tud_disconnect(void);

extern "C" void TinyUSB_Device_Init(uint8_t rhport) {
  (void)rhport;
  // Defer USB device init until configure_usb_output() has restored the saved
  // output mode and registered any raw descriptor hooks. Xbox 360 auth depends
  // on those hooks being in place before tud_init() captures descriptors and
  // application class drivers.
  Serial.end();
}
#endif
