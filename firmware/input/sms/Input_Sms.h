#pragma once

/*******************************************************************************
 * SMS / Atari / C64 / Japanese PC input module for Reflex-Adapt.
 * Based on ReflexMPG by Matheus Fraguas (sonik-br)
 *
 * Supports:
 * - SMS mode: SMS, Atari 2600/7800, C64, Amiga (TL + TR buttons)
 * - JPC mode: FM Towns, MSX, X68000, PC-88/98 (TL + TH buttons, TR as ground)
 */

#include "../base/RZInputModule.h"

extern uint8_t spinner_speed;
extern const uint8_t spinner_speed_mult[5];
extern const uint8_t driving_speed_mult[5];

typedef struct {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t tl;
  uint8_t th;
  uint8_t tr;
} input_sms_config_t;

extern const input_sms_config_t input_sms_config[];

class RZInputSms : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    static constexpr int16_t DRIVING_FALLBACK_CENTER = 128;
    bool isJPC = false;
    uint8_t lastState[input_ports] = { 0xFF, 0xFF };
    uint8_t lastDrivingState[input_ports] = { 0, 0 };
    int16_t drivingPosition[input_ports] = { DRIVING_FALLBACK_CENTER, DRIVING_FALLBACK_CENTER };
    bool drivingActive[input_ports] = { false, false };
    uint8_t pendingWebhidRaw[8] = {};
    bool pendingWebhidRawDirty = false;

    static const int8_t drivingQuadTable[16];
    void stageWebhidRawData(uint8_t state, uint8_t select, uint8_t runCombo,
                            uint8_t port, uint8_t currentDrivingState,
                            int8_t drivingDelta);
    void flushPendingWebhidRawData();

  public:
    RZInputSms();

    uint8_t getInternalMode();
    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    void afterOutputFrameSent(bool polled, bool updated) override;
};
