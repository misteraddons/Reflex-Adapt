#pragma once

#include "../base/RZInputModule.h"

typedef struct __attribute((packed, aligned(1))) {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t select;
  uint8_t start;
} neogeo_buttons_t;

typedef struct __attribute((packed, aligned(1))) {
  union {
    neogeo_buttons_t buttons;
    uint8_t btnArray[sizeof(neogeo_buttons_t)];
  };
  uint8_t debounceMs;
} input_neogeo_config_t;

extern const input_neogeo_config_t input_neogeo_config[];

class RZInputNeoGeo : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;

    uint32_t buttonMask[input_ports] = {0};
    uint8_t outputButtonShiftByPin[input_ports][32] = {};
    uint32_t lastRawState[input_ports] = {0};
    uint32_t acceptedRawState[input_ports] = {0};
    uint32_t acceptedButtons[input_ports] = {0};
    uint32_t debounceBlockedUntilMs[input_ports][32] = {};
    uint32_t pendingWebhidRawState = 0;
    uint8_t pendingWebhidRawPort = 0;
    bool pendingWebhidRawDirty = false;

    uint32_t filterImmediatePressDebounce(uint8_t port, uint32_t rawState);
    void stageWebhidRawData(uint8_t port, uint32_t rawState);
    void flushPendingWebhidRawData();

  public:
    RZInputNeoGeo();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    void afterOutputFrameSent(bool polled, bool updated) override;
};
