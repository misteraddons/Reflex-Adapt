#pragma once

//Enable or disable multitap support
//With multitap enabled it's impossible to detect if a pad is connected or not.
//It will report devices as PCE_DEVICE_PAD2 or PCE_DEVICE_PAD6
#define PCE_ENABLE_MULTITAP

#include "../base/RZInputModule.h"
#include <PceLib/PceLib.h>

#ifndef PCE_TYPE_UPGRADE_CONFIRM_POLLS
  #define PCE_TYPE_UPGRADE_CONFIRM_POLLS 8
#endif

#ifndef PCE_TYPE_CHANGE_CONFIRM_POLLS
  #define PCE_TYPE_CHANGE_CONFIRM_POLLS 512
#endif

#ifndef PCE_TYPE_DOWNGRADE_CONFIRM_POLLS
  #define PCE_TYPE_DOWNGRADE_CONFIRM_POLLS 512
#endif

typedef struct {
  uint8_t sel;
  uint8_t clr;
  uint8_t d0;
  uint8_t d1;
  uint8_t d2;
  uint8_t d3;
} input_pce_config_t;

extern const input_pce_config_t input_pce_config[];

class RZInputPce : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    PcePort* pce[input_ports];
    uint8_t portSlotCapacity[input_ports] = {};
    PceDeviceType_Enum dtype[MAX_USB_OUT] = {};
    PceDeviceType_Enum pendingDtype[MAX_USB_OUT] = {};
    uint16_t pendingDtypeCount[MAX_USB_OUT] = {};
    uint32_t slotLastSeenMs[MAX_USB_OUT] = {};
    static const uint16_t SLOT_DISCONNECT_DEBOUNCE_MS = 120;
    static const uint16_t TYPE_UPGRADE_CONFIRM_POLLS = PCE_TYPE_UPGRADE_CONFIRM_POLLS;
    static const uint16_t TYPE_CHANGE_CONFIRM_POLLS = PCE_TYPE_CHANGE_CONFIRM_POLLS;
    static const uint16_t TYPE_DOWNGRADE_CONFIRM_POLLS = PCE_TYPE_DOWNGRADE_CONFIRM_POLLS;
    uint8_t pendingWebhidRaw[5] = {};
    bool pendingWebhidRawDirty = false;

    bool rawControllerPresent(uint8_t port) const;
    PceDeviceType_Enum stabilizeDeviceType(uint8_t slot, PceDeviceType_Enum observedType, bool justConnected);
    void clearPendingDeviceType(uint8_t slot);
    uint32_t mapDigitalToButtons(PceDeviceType_Enum type, uint16_t digital) const;
    void stageWebhidRawData(PceDeviceType_Enum stableType, uint16_t digital, uint8_t port, uint8_t sourceIndex);
    void flushPendingWebhidRawData();

  public:
    RZInputPce();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    void afterOutputFrameSent(bool polled, bool updated) override;
};
