#pragma once

#include "../base/RZInputModule.h"
#include "input_mixed_runtime_state.h"

class RZInputMixed : public RZInputModule {
public:
  RZInputMixed();

  const char* getUsbId() override;
  void configureBcdDeviceVersion() override;
  const char* getDescription() override;
  void setup() override;
  void setup2() override;
  bool poll() override;

  RZInputModule* moduleForPhysicalPort(uint8_t port);
  DeviceEnum modeForPhysicalPort(uint8_t port) const;

private:
  RZInputModule* modules[INPUT_MIXED_PORT_COUNT] = { nullptr, nullptr };
  DeviceEnum portModes[INPUT_MIXED_PORT_COUNT] = { RZORD_NONE, RZORD_NONE };

  void ensureModules();
  RZInputModule* createModuleForMode(DeviceEnum mode);
  void configurePrimaryBcd();
};
