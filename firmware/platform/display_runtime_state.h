#pragma once

#include "../product_config.h"
#include "../firmware_platform_config.h"

#ifdef USE_I2C_DISPLAY
  #include <Wire.h>
  #include <SSD1306Ascii/SSD1306Ascii.h>
  #include <SSD1306Ascii/SSD1306AsciiWire.h>
  #include <U8g2lib.h>

  #define I2C_ADDRESS 0x3C

  extern SSD1306AsciiWire display;
  extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

  static inline void beginDisplayWire() {
    Wire.setSDA(OLED_I2C_SDA);
    Wire.setSCL(OLED_I2C_SCL);
    Wire.setClock(400000L);
    Wire.setTimeout(2, false);
    Wire.begin();
  }

  static inline void restoreOledPanelOrientation() {
    u8g2.setDisplayRotation(U8G2_R0);
    u8g2.setFlipMode(0);
    display.displayRemap(false);
    #if INCLUDE_SCROLLING
    display.setStartLine(0);
    #endif
  }
#endif
