#pragma once

#include "../base/RZInputModule.h"

typedef struct {
  int8_t pinBtn1;
  int8_t pinBtn2;
  int8_t pinBtn3;
  int8_t pinBtn4;
  int8_t pinAxisX;
  int8_t pinAxisY;
} input_gameport_config_t;

extern const input_gameport_config_t input_gameport_config[2];

class RZInputGameport : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    bool wasConnected[MAX_USB_OUT] = { false };

    uint32_t readAxisTiming(uint8_t pin);
    int16_t timingToAxis(uint32_t timing);
    bool checkConnected(uint8_t port);

  public:
    RZInputGameport();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
};
