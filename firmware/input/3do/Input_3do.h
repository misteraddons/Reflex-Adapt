#pragma once

#define THREEDO_MAX_CTRL 6

#include "../base/RZInputModule.h"
#include <ThreedoLib/ThreedoLib.h>

typedef struct {
  uint8_t clk;
  uint8_t out;
  uint8_t in;
} input_3do_config_t;

extern const input_3do_config_t input_3do_config[];

class RZInput3do : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    ThreedoPort* tdo[input_ports];
    ThreedoDeviceType_Enum dtype[MAX_USB_OUT] = {};

  public:
    RZInput3do();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
};
