#pragma once

#include "../base/RZInputModule.h"

class RZInputDummy : public RZInputModule {
  public:
    RZInputDummy();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
};
