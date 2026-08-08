#pragma once

#include "../base/RZInputModule.h"

// Pin configuration for Intellivision
// Maps DE9 pins to HDMI connector pins
// Note: Intellivision GND is on DE9 pin 5, not pin 8 like Genesis!
typedef struct {
  uint8_t b0;  // DE9 pin 4
  uint8_t b1;  // DE9 pin 3
  uint8_t b2;  // DE9 pin 2
  uint8_t b3;  // DE9 pin 1
  uint8_t b4;  // DE9 pin 9
  uint8_t b5;  // DE9 pin 8
  uint8_t b6;  // DE9 pin 7
  uint8_t b7;  // DE9 pin 6
} input_intv_config_t;

extern const input_intv_config_t input_intv_config[2];

class RZInputIntv : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;

    uint8_t readController(uint8_t port);

  public:
    RZInputIntv();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
};
