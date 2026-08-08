#pragma once

#include "../base/RZInputModule.h"

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include "hardware/gpio.h"
#endif

typedef struct {
  uint8_t enc_a;
  uint8_t enc_b;
  uint8_t button;
} input_driving_config_t;

extern const input_driving_config_t input_driving_config[2];
extern const int8_t driving_quad_table[16];

extern uint8_t spinner_speed;
extern const uint8_t spinner_speed_mult[5];
extern const uint8_t driving_speed_mult[5];

inline uint8_t drivingReadPin(uint8_t pin) {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  return gpio_get(pin) ? 1 : 0;
#else
  return digitalRead(pin) ? 1 : 0;
#endif
}

inline uint8_t drivingReadEncoderState(uint8_t enc_a, uint8_t enc_b) {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  const uint32_t pins = gpio_get_all();
  return (((pins >> enc_a) & 1u) << 1) | ((pins >> enc_b) & 1u);
#else
  return (drivingReadPin(enc_a) << 1) | drivingReadPin(enc_b);
#endif
}

class RZInputDriving : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    uint8_t lastState[input_ports] = { 0 };
    uint8_t lastButton[input_ports] = { 1, 1 };
    int16_t position[input_ports] = { 0 };

  public:
    RZInputDriving();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    int16_t getPosition(uint8_t port);
};
