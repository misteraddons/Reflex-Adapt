#include "../product_config.h"

#include "menu_system_screens.h"

#include "../platform/display_runtime_state.h"

void drawFactoryResetPrompt(uint8_t selection) {
#ifndef USE_I2C_DISPLAY
  (void)selection;
  return;
#else
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(18, 14, "FACTORY RESET");

  u8g2.drawHLine(0, 18, 128);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(16, 32, "Reset all settings?");

  u8g2.setFont(u8g2_font_6x10_tr);
  const char* options[2] = { "RESET", "CANCEL" };
  const uint8_t optX[2] = { 20, 70 };
  const uint8_t optW[2] = { 40, 40 };

  for (uint8_t i = 0; i < 2; i++) {
    if (i == selection) {
      u8g2.drawBox(optX[i], 36, optW[i], 14);
      u8g2.setDrawColor(0);
      u8g2.drawStr(optX[i] + 3, 46, options[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawFrame(optX[i], 36, optW[i], 14);
      u8g2.drawStr(optX[i] + 3, 46, options[i]);
    }
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(8, 60, "Reset=Navigate Mode=Select");

  u8g2.sendBuffer();
#endif
}

void drawFactoryResetResult() {
#ifndef USE_I2C_DISPLAY
  return;
#else
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(12, 20, "RESET COMPLETE!");

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(15, 38, "All settings and");
  u8g2.drawStr(15, 50, "remaps cleared.");

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(30, 62, "Rebooting...");

  u8g2.sendBuffer();
#endif
}

void drawBootloaderPrompt() {
#ifndef USE_I2C_DISPLAY
  return;
#else
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(29, 14, "BOOTLOADER");

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(4, 28, "Enter firmware update");
  u8g2.drawStr(4, 38, "mode? Device will");
  u8g2.drawStr(4, 48, "appear as USB drive.");

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(4, 62, "MODE=Enter  RESET=Cancel");

  u8g2.sendBuffer();
#endif
}

void drawBootloaderEnteringScreen() {
#ifndef USE_I2C_DISPLAY
  return;
#else
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(36, 30, "ENTERING");
  u8g2.drawStr(29, 46, "BOOTLOADER");
  u8g2.sendBuffer();
#endif
}
