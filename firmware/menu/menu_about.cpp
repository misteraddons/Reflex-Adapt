#include "../product_config.h"
#include "../product_display_identity.h"

#include "menu_about.h"

#include "../firmware_build_info.h"
#include "../platform/display_runtime_state.h"
#include "../platform/webhid_runtime.h"

namespace {

uint16_t aboutPollRateHz() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  return poll_rate_hz;
  #else
  return 0;
  #endif
}

}  // namespace

void renderAboutScreen() {
  #ifdef USE_I2C_DISPLAY
  display.clear();
  display.setFont(System5x7);
  display.set1X();
  display.setCursor(0, 0);
  display.print(F("   -- About --"));
  display.setCursor(0, 1);
  display.print(getProductAboutDisplayTitle());
  display.setCursor(0, 2);
  display.print(F("Firmware: " FIRMWARE_VERSION_STRING));
  display.setCursor(0, 3);
  display.print(F("Built: " FIRMWARE_BUILD_DATE));
  display.setCursor(0, 4);
  display.print(F("Hardware: " FIRMWARE_HARDWARE_STRING));
  display.setCursor(0, 5);
  display.print(F("USB poll: "));
  display.print(aboutPollRateHz());
  display.print(F(" Hz"));
  display.setCursor(0, 6);
  display.print(F("misteraddons.com"));
  display.setCursor(0, 7);
  display.print(F("github: Reflex-Adapt"));
  #endif
}
