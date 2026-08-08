#include "rgb_led.h"

namespace {

static const uint16_t ws2812_program_instructions[] = {
    0x6221,
    0x1123,
    0x1400,
    0xa442,
};

static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length = 4,
    .origin = -1,
};

}  // namespace

rgb_color_t RgbLed::blendColors(rgb_color_t c1, rgb_color_t c2, uint8_t blend) {
  return (rgb_color_t){
    (uint8_t)((c1.r * (255 - blend) + c2.r * blend) / 255),
    (uint8_t)((c1.g * (255 - blend) + c2.g * blend) / 255),
    (uint8_t)((c1.b * (255 - blend) + c2.b * blend) / 255)
  };
}

rgb_color_t RgbLed::applyBrightness(rgb_color_t c) {
  return (rgb_color_t){
    (uint8_t)((c.r * brightness) / 255),
    (uint8_t)((c.g * brightness) / 255),
    (uint8_t)((c.b * brightness) / 255)
  };
}

rgb_color_t RgbLed::hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
  if (s == 0) {
    return (rgb_color_t){v, v, v};
  }

  uint8_t region = h / 43;
  uint8_t remainder = (h - (region * 43)) * 6;

  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:  return (rgb_color_t){v, t, p};
    case 1:  return (rgb_color_t){q, v, p};
    case 2:  return (rgb_color_t){p, v, t};
    case 3:  return (rgb_color_t){p, q, v};
    case 4:  return (rgb_color_t){t, p, v};
    default: return (rgb_color_t){v, p, q};
  }
}

void RgbLed::initPio() {
  if (pio_initialized) return;

  pio = pio0;
  sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &ws2812_program);

  pio_gpio_init(pio, pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_wrap(&c, offset, offset + ws2812_program.length - 1);
  sm_config_set_sideset(&c, 1, false, false);
  sm_config_set_sideset_pins(&c, pin);
  sm_config_set_out_shift(&c, false, true, 24);
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

  float div = clock_get_hz(clk_sys) / (800000 * 10);
  sm_config_set_clkdiv(&c, div);

  pio_sm_init(pio, sm, offset, &c);
  pio_sm_set_enabled(pio, sm, true);

  pio_initialized = true;
}

void RgbLed::show() {
  if (!pio_initialized) return;

  for (uint8_t i = 0; i < led_count; i++) {
    rgb_color_t c = applyBrightness(leds[i]);
    uint32_t grb = ((uint32_t)c.g << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.b << 8);
    pio_sm_put_blocking(pio, sm, grb);
  }
}

void RgbLed::setAll(rgb_color_t color) {
  for (uint8_t i = 0; i < led_count; i++) {
    leds[i] = color;
  }
}

void RgbLed::updateBreathing() {
  uint8_t phase = (millis() / 8) & 0xFF;
  int16_t sine = 128 + ((phase < 128) ? (phase * 2 - 128) : (384 - phase * 2));
  if (sine < 0) sine = 0;
  if (sine > 255) sine = 255;

  rgb_color_t c = hsvToRgb(160, 255, sine);
  setAll(c);
}

void RgbLed::updateRainbow() {
  uint8_t base_hue = (millis() / 20) & 0xFF;
  for (uint8_t i = 0; i < led_count; i++) {
    uint8_t hue = base_hue + (i * 256 / led_count);
    leds[i] = hsvToRgb(hue, 255, 200);
  }
}

void RgbLed::updateReactive() {
  for (uint8_t i = 0; i < 16; i++) {
    if (button_brightness[i] > 0) {
      button_brightness[i] = (button_brightness[i] > 8) ? button_brightness[i] - 8 : 0;
    }
  }

  uint8_t max_brightness = 0;
  uint8_t max_button = 0;
  for (uint8_t i = 0; i < 16; i++) {
    if (button_brightness[i] > max_brightness) {
      max_brightness = button_brightness[i];
      max_button = i;
    }
  }

  if (max_brightness > 0) {
    rgb_color_t colors[] = {
      COLOR_BTN_A, COLOR_BTN_B, COLOR_BTN_X, COLOR_BTN_Y,
      COLOR_BTN_L, COLOR_BTN_R, COLOR_BTN_L, COLOR_BTN_R,
      COLOR_BTN_START, COLOR_BTN_START, COLOR_WHITE, COLOR_WHITE,
      COLOR_BTN_DPAD, COLOR_BTN_DPAD, COLOR_BTN_DPAD, COLOR_BTN_DPAD
    };
    rgb_color_t c = colors[max_button];
    c.r = (c.r * max_brightness) / 255;
    c.g = (c.g * max_brightness) / 255;
    c.b = (c.b * max_brightness) / 255;
    setAll(c);
  } else {
    setAll((rgb_color_t){8, 8, 8});
  }
}

void RgbLed::updateRumble() {
  if (rumble_intensity > 0) {
    uint8_t r = rumble_intensity;
    uint8_t g = rumble_intensity / 3;
    setAll((rgb_color_t){r, g, 0});
  } else {
    setAll((rgb_color_t){0, 16, 0});
  }
}

void RgbLed::updateAnalog() {
  int16_t dx = stick_lx - 128;
  int16_t dy = stick_ly - 128;
  uint16_t distance = sqrt(dx * dx + dy * dy);
  if (distance > 127) distance = 127;

  uint8_t hue = 160 - (distance * 160 / 127);
  uint8_t sat = 255;
  uint8_t val = 64 + (distance * 191 / 127);

  setAll(hsvToRgb(hue, sat, val));
}

RgbLed::RgbLed()
  : pin(RGB_LED_PIN),
    led_count(RGB_LED_COUNT),
    brightness(128),
    mode(LED_MODE_OFF),
    enabled(true),
    pio_initialized(false),
    last_update(0),
    animation_step(0),
    rumble_intensity(0),
    stick_lx(128),
    stick_ly(128) {
  memset(leds, 0, sizeof(leds));
  memset(button_brightness, 0, sizeof(button_brightness));
}

void RgbLed::begin() {
  initPio();
  setAll(COLOR_OFF);
  show();
}

void RgbLed::setEnabled(bool en) {
  enabled = en;
  if (!en) {
    setAll(COLOR_OFF);
    show();
  }
}

bool RgbLed::isEnabled() {
  return enabled;
}

void RgbLed::setMode(led_mode_t m) {
  mode = m;
  animation_step = 0;
  if (mode == LED_MODE_OFF) {
    setAll(COLOR_OFF);
    show();
  }
}

led_mode_t RgbLed::getMode() {
  return mode;
}

void RgbLed::setBrightness(uint8_t b) {
  brightness = b;
}

uint8_t RgbLed::getBrightness() {
  return brightness;
}

void RgbLed::update() {
  if (!enabled || mode == LED_MODE_OFF) return;

  uint32_t now = millis();
  if (now - last_update < 16) return;
  last_update = now;

  switch (mode) {
    case LED_MODE_STATIC:
      break;
    case LED_MODE_BREATHING:
      updateBreathing();
      break;
    case LED_MODE_RAINBOW:
      updateRainbow();
      break;
    case LED_MODE_REACTIVE:
      updateReactive();
      break;
    case LED_MODE_RUMBLE:
      updateRumble();
      break;
    case LED_MODE_ANALOG:
      updateAnalog();
      break;
    default:
      break;
  }

  show();
}

void RgbLed::setColor(rgb_color_t color) {
  setAll(color);
  if (mode == LED_MODE_STATIC) {
    show();
  }
}

void RgbLed::setLed(uint8_t index, rgb_color_t color) {
  if (index < led_count) {
    leds[index] = color;
  }
}

void RgbLed::triggerButton(uint8_t button) {
  if (button < 16) {
    button_brightness[button] = 255;
  }
}

void RgbLed::triggerButtons(uint32_t buttons) {
  if (buttons & 0x0001) triggerButton(0);
  if (buttons & 0x0002) triggerButton(1);
  if (buttons & 0x0004) triggerButton(2);
  if (buttons & 0x0008) triggerButton(3);
  if (buttons & 0x0010) triggerButton(4);
  if (buttons & 0x0020) triggerButton(5);
  if (buttons & 0x0040) triggerButton(6);
  if (buttons & 0x0080) triggerButton(7);
  if (buttons & 0x0100) triggerButton(8);
  if (buttons & 0x0200) triggerButton(9);
  if (buttons & 0x0400) triggerButton(10);
  if (buttons & 0x0800) triggerButton(11);
  if (buttons & 0x1000) triggerButton(12);
  if (buttons & 0x2000) triggerButton(13);
  if (buttons & 0x4000) triggerButton(14);
  if (buttons & 0x8000) triggerButton(15);
}

void RgbLed::setRumble(uint8_t intensity) {
  rumble_intensity = intensity;
}

void RgbLed::setAnalog(int16_t lx, int16_t ly) {
  stick_lx = lx;
  stick_ly = ly;
}

void RgbLed::setPlayerColor(uint8_t player) {
  rgb_color_t colors[] = { COLOR_P1, COLOR_P2, COLOR_P3, COLOR_P4 };
  if (player < 4) {
    setColor(colors[player]);
  }
}

void RgbLed::flash(rgb_color_t color, uint8_t times, uint16_t duration_ms) {
  uint32_t phase = (millis() / duration_ms) % (times * 2);
  if (phase < times * 2 && (phase & 1) == 0) {
    setAll(color);
  } else {
    setAll(COLOR_OFF);
  }
  show();
}

void RgbLed::showConnect() {
  setAll(COLOR_GREEN);
  show();
}

void RgbLed::showDisconnect() {
  setAll(COLOR_RED);
  show();
}

void RgbLed::showError() {
  setAll(COLOR_RED);
  show();
}

void RgbLed::showSuccess() {
  setAll(COLOR_GREEN);
  show();
}

RgbLed rgbLed;
