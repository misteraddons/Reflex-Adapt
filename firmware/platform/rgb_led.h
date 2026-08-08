#pragma once

// RGB LED Array Module
// Uses PIO-based WS2812/NeoPixel driver for addressable LEDs

#include "../product_config.h"
#include <Arduino.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"

#ifndef RGB_LED_PIN
  #ifdef PIN_RGB_LED
    #define RGB_LED_PIN PIN_RGB_LED
  #elif defined(PIN_STATUS_LED)
    #define RGB_LED_PIN PIN_STATUS_LED
  #else
    #define RGB_LED_PIN 8
  #endif
#endif

#ifndef RGB_LED_COUNT
  #ifdef PIN_RGB_LED_COUNT
    #define RGB_LED_COUNT PIN_RGB_LED_COUNT
  #else
    #define RGB_LED_COUNT 4
  #endif
#endif

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb_color_t;

#define COLOR_OFF       (rgb_color_t){0, 0, 0}
#define COLOR_WHITE     (rgb_color_t){255, 255, 255}
#define COLOR_RED       (rgb_color_t){255, 0, 0}
#define COLOR_GREEN     (rgb_color_t){0, 255, 0}
#define COLOR_BLUE      (rgb_color_t){0, 0, 255}
#define COLOR_YELLOW    (rgb_color_t){255, 255, 0}
#define COLOR_CYAN      (rgb_color_t){0, 255, 255}
#define COLOR_MAGENTA   (rgb_color_t){255, 0, 255}
#define COLOR_ORANGE    (rgb_color_t){255, 128, 0}
#define COLOR_PURPLE    (rgb_color_t){128, 0, 255}
#define COLOR_PINK      (rgb_color_t){255, 64, 128}
#define COLOR_LIME      (rgb_color_t){128, 255, 0}

#define COLOR_P1        COLOR_BLUE
#define COLOR_P2        COLOR_RED
#define COLOR_P3        COLOR_GREEN
#define COLOR_P4        COLOR_YELLOW

#define COLOR_BTN_A     COLOR_GREEN
#define COLOR_BTN_B     COLOR_RED
#define COLOR_BTN_X     COLOR_BLUE
#define COLOR_BTN_Y     COLOR_YELLOW
#define COLOR_BTN_L     COLOR_ORANGE
#define COLOR_BTN_R     COLOR_PURPLE
#define COLOR_BTN_START COLOR_WHITE
#define COLOR_BTN_DPAD  COLOR_CYAN

typedef enum {
  LED_MODE_OFF = 0,
  LED_MODE_STATIC,
  LED_MODE_BREATHING,
  LED_MODE_RAINBOW,
  LED_MODE_REACTIVE,
  LED_MODE_RUMBLE,
  LED_MODE_ANALOG,
  LED_MODE_LAST
} led_mode_t;

inline const char* getLedModeName(led_mode_t mode) {
  switch (mode) {
    case LED_MODE_OFF:       return "Off";
    case LED_MODE_STATIC:    return "Static";
    case LED_MODE_BREATHING: return "Breathing";
    case LED_MODE_RAINBOW:   return "Rainbow";
    case LED_MODE_REACTIVE:  return "Reactive";
    case LED_MODE_RUMBLE:    return "Rumble";
    case LED_MODE_ANALOG:    return "Analog";
    default:                 return "Unknown";
  }
}

class RgbLed {
private:
  uint8_t pin;
  uint8_t led_count;
  uint8_t brightness;
  led_mode_t mode;
  bool enabled;

  PIO pio;
  uint sm;
  bool pio_initialized;

  rgb_color_t leds[RGB_LED_COUNT];

  uint32_t last_update;
  uint16_t animation_step;

  uint8_t button_brightness[16];
  uint8_t rumble_intensity;
  int16_t stick_lx, stick_ly;

  rgb_color_t blendColors(rgb_color_t c1, rgb_color_t c2, uint8_t blend);
  rgb_color_t applyBrightness(rgb_color_t c);
  rgb_color_t hsvToRgb(uint8_t h, uint8_t s, uint8_t v);
  void initPio();
  void show();
  void setAll(rgb_color_t color);
  void updateBreathing();
  void updateRainbow();
  void updateReactive();
  void updateRumble();
  void updateAnalog();

public:
  RgbLed();

  void begin();
  void setEnabled(bool en);
  bool isEnabled();
  void setMode(led_mode_t m);
  led_mode_t getMode();
  void setBrightness(uint8_t b);
  uint8_t getBrightness();
  void update();
  void setColor(rgb_color_t color);
  void setLed(uint8_t index, rgb_color_t color);
  void triggerButton(uint8_t button);
  void triggerButtons(uint32_t buttons);
  void setRumble(uint8_t intensity);
  void setAnalog(int16_t lx, int16_t ly);
  void setPlayerColor(uint8_t player);
  void flash(rgb_color_t color, uint8_t times, uint16_t duration_ms);
  void showConnect();
  void showDisconnect();
  void showError();
  void showSuccess();
};

extern RgbLed rgbLed;
