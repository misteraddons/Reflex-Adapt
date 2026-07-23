#pragma once

#include "../base/RZInputModule.h"
#include <JaguarLib/JaguarLib.h>

typedef struct {
  uint8_t j3_j4;   // de15  1
  uint8_t j2_j5;   // de15  2
  uint8_t j1_j6;   // de15  3
  uint8_t j0_j7;   // de15  4
  uint8_t b0_b2;   // de15  6
  uint8_t b1_b3;   // de15 10
  uint8_t j11_j15; // de15 11
  uint8_t j10_j14; // de15 12
  uint8_t j9_j13;  // de15 13
  uint8_t j8_j12;  // de15 14
} input_jag_config_t;

extern const input_jag_config_t input_jag_config[2];

class RZInputJaguar : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    JagPort* jag[input_ports] = { nullptr };
    JagDeviceType_Enum dtype[MAX_USB_OUT] = {};
    uint8_t pendingWebhidRaw[6] = {};
    bool pendingWebhidRawDirty = false;

    uint32_t mapJaguarPadToButtons(uint8_t port, const JagController& sc) const;
    void stageWebhidRawData(uint8_t port, JagDeviceType_Enum type, uint32_t digital);
    void flushPendingWebhidRawData();

  public:
    RZInputJaguar();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    void afterOutputFrameSent(bool polled, bool updated) override;
};
