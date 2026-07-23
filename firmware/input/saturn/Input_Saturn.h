#pragma once

//Enable usage of HOME button on 8bidto M30 2.4G.
//It will report as saturn's L button.
#define SATLIB_ENABLE_8BITDO_HOME_BTN

//Dual Mission Stick (6-axis) support
#define SATLIB_ENABLE_MISSION6

//Multitap support
#define SATLIB_ENABLE_MEGATAP //suport for 4p megatap
#define SATLIB_ENABLE_SATTAP //support for 6p multitap

#include "../base/RZInputModule.h"
#include "input_saturn_runtime_state.h"
#include <SaturnLib/SaturnLib.h>

typedef struct {
  uint8_t d0;
  uint8_t d1;
  uint8_t d2;
  uint8_t d3;
  uint8_t th;
  uint8_t tr;
  uint8_t tl;
} input_saturn_config_t;

const input_saturn_config_t input_saturn_config[] = {
  { .d0 = HDMI_1_01, .d1 = HDMI_1_02, .d2 = HDMI_1_03, .d3 = HDMI_1_04, .th = HDMI_1_06, .tr = HDMI_1_07, .tl = HDMI_1_05 },
  { .d0 = HDMI_2_01, .d1 = HDMI_2_02, .d2 = HDMI_2_03, .d3 = HDMI_2_04, .th = HDMI_2_06, .tr = HDMI_2_07, .tl = HDMI_2_05 },
};

class RZInputSaturn : public RZInputModule {
  private:
    static const uint8_t input_ports { sizeof(input_saturn_config) / sizeof(input_saturn_config_t) };
    static constexpr uint32_t MEGADRIVE_CONNECTED_POLL_INTERVAL_US = 125;
    static constexpr uint32_t SATURN_CONNECTED_POLL_INTERVAL_US = 125;
    SaturnPort* saturn[input_ports] = {};
    uint8_t portSlotCapacity[input_ports] = {};
    uint8_t lastTapPorts[input_ports] = {};
    uint32_t tapLastSeenMs[input_ports] = {};
    bool tapLatchedFromAutoDetect[input_ports] = {};
    SatDeviceType_Enum dtype[MAX_USB_OUT] = {};
    SatDeviceType_Enum stableMegadriveType[MAX_USB_OUT] = {};
    uint32_t megadriveTypeDowngradeStartedMs[MAX_USB_OUT] = {};
    uint32_t slotLastSeenMs[MAX_USB_OUT] = {};
    static const uint16_t SLOT_DISCONNECT_DEBOUNCE_MS = 120;
    static const uint16_t MEGA6_DOWNGRADE_DEBOUNCE_MS = 250;
    static const uint16_t TAP_DISCONNECT_DEBOUNCE_MS = 1000;
    static const uint16_t HOTSWAP_RECENT_SLOT_GRACE_MS = 1500;

    bool isMegadriveType(SatDeviceType_Enum type) const;
    bool recentControllerSlotPresent() const;
    void updateTapPresence(uint8_t port, uint8_t tap_ports, uint32_t now_ms);
    uint8_t refreshTapPresence(uint8_t port, uint8_t observed_tap_ports, uint32_t now_ms);
    void resetMegadriveTypeStability(uint8_t slot);
    SatDeviceType_Enum stabilizeMegadriveType(uint8_t slot, SatDeviceType_Enum detectedType, uint32_t now);

  public:
    RZInputSaturn();

    uint8_t getInternalMode();
    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    void setPhysicalPortMask(uint8_t mask) override;
    bool hasPhysicalConnectionForHotSwap() const override;
    const char* physicalConnectionDisplayName() const override;
};
