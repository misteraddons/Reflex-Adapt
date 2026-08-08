#pragma once

#include "../base/RZInputModule.h"

typedef struct {
  uint8_t up_pin;
  uint8_t down_pin;
  uint8_t left_pin;
  uint8_t right_pin;
  uint8_t trigger_pin;
  uint8_t paddle_a_pin;
  uint8_t paddle_b_pin;
  uint8_t paddle_a_btn;
  uint8_t paddle_b_btn;
} input_atari_config_t;

extern const input_atari_config_t input_atari_config[2];

class RZInputAtari : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    static constexpr uint32_t RC_TIMEOUT_MICROS = 2000;
    static constexpr uint32_t RC_MIN_TIME = 10;
    static constexpr uint32_t RC_MAX_TIME = 1500;

    bool joystick_state[input_ports][5] = {{false}};
    bool old_joystick_state[input_ports][5] = {{false}};
    uint16_t paddle_values[input_ports][2] = {{0}};
    bool paddle_buttons[input_ports][2] = {{false}};
    uint16_t old_paddle_values[input_ports][2] = {{0}};
    bool old_paddle_buttons[input_ports][2] = {{false}};

    uint16_t measurePaddleRC(uint8_t pin);
    uint8_t rcTimeToPaddleValue(uint16_t rc_time);

  public:
    RZInputAtari();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
};
