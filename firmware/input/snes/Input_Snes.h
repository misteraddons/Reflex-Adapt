#pragma once

//#define SNES_ENABLE_MULTITAP
//#define SNES_MULTI_CONNECTION 2

#include "../base/RZInputModule.h"
#include "input_snes_runtime_state.h"
#include "../../core/controller_settings_state.h"
#include "../../menu/menu_runtime_state.h"
#include <SnesLib/SnesLib.h>

typedef struct {
  int8_t clk;
  int8_t lat;
  int8_t dat0;
  int8_t dat1; // optional. only used for multitap. -1 if not using
  int8_t sel;  // optional. only used for multitap. -1 if not using
} input_snes_config_t;

const input_snes_config_t input_snes_config[] = {
  { .clk = HDMI_1_01, .lat = HDMI_1_02, .dat0 = HDMI_1_03, .dat1 = HDMI_1_05, .sel = HDMI_1_06 },
  { .clk = HDMI_2_01, .lat = HDMI_2_02, .dat0 = HDMI_2_03, .dat1 = HDMI_2_05, .sel = HDMI_2_06 },
};

class RZInputSnes : public RZInputModule {
  private:
    static const uint8_t input_ports { sizeof(input_snes_config) / sizeof(input_snes_config_t) };
    // SNES/NES style shift-register reads can briefly look empty during noisy
    // bus windows. Require a real run of empty reads before dropping the pad.
    static constexpr uint8_t DISCONNECT_CONFIRM_FRAMES = 32;
    static constexpr uint8_t TYPE_CHANGE_CONFIRM_FRAMES = 4;
    static constexpr uint16_t SLOT_DISCONNECT_DEBOUNCE_MS = 120;
    static constexpr uint16_t HOTSWAP_RECENT_SLOT_GRACE_MS = 1500;
    static constexpr uint32_t STANDARD_POLL_INTERVAL_US = 16000;
    static constexpr uint32_t NES_IDLE_POLL_INTERVAL_US = 125;
    static constexpr uint32_t SNES_IDLE_POLL_INTERVAL_US = 125;
    static constexpr uint32_t SNES_RUMBLETECH_ACTIVE_POLL_INTERVAL_US = 1000;
    static constexpr uint32_t SNES_RUMBLETECH_POLL_INTERVAL_US = 3000;
    static constexpr uint16_t SNES_EMPTY_PORT_SCAN_INTERVAL_MS = 16;
    static constexpr uint16_t SNES_SAFE_POLL_HOLD_MS = 1500;
    static constexpr uint8_t NES_FAST_FULL_VALIDATE_POLLS = 32;
    static constexpr uint8_t DIGITAL_PRESS_CONFIRM_FRAMES = 2;
    static constexpr uint8_t RUMBLETECH_PRESS_CONFIRM_FRAMES = 4;
    static constexpr uint16_t SNES_DPAD_DIGITAL_MASK = SNES_UP | SNES_DOWN | SNES_LEFT | SNES_RIGHT;
    static constexpr uint16_t SNES_NON_DPAD_DIGITAL_MASK =
      SNES_B | SNES_Y | SNES_SELECT | SNES_START | SNES_A | SNES_X | SNES_L | SNES_R;
    SnesPort* snes[input_ports]; //SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1, SNESPIN_DATA2, SNESPIN_SELECT> snes;
    SnesDeviceType_Enum dtype[MAX_USB_OUT] = {};
    uint8_t disconnect_confirm_frames[MAX_USB_OUT] = {};
    SnesDeviceType_Enum candidate_type[MAX_USB_OUT] = {};
    uint8_t candidate_type_frames[MAX_USB_OUT] = {};
    uint16_t glitch_frames[MAX_USB_OUT] = {};
    uint32_t slotLastSeenMs[MAX_USB_OUT] = {};
    uint16_t stable_digital[MAX_USB_OUT] = {};
    uint16_t pending_non_dpad_press[MAX_USB_OUT] = {};
    uint8_t pending_non_dpad_press_frames[MAX_USB_OUT] = {};
    bool digital_filter_initialized[MAX_USB_OUT] = {};
    uint8_t last_rumble_data[input_ports] = {};
    uint8_t rumble_detect_confidence[input_ports] = {};
    uint8_t slot_physical_port[MAX_USB_OUT] = {};
    uint32_t safe_poll_until_ms[input_ports] = {};
    uint8_t nes_fast_validate_count[input_ports] = {};
    static constexpr uint8_t RUMBLE_DETECT_THRESHOLD = 2;
    static constexpr uint8_t RUMBLE_DETECT_MAX = 8;

    // Four Score detection - disabled/mothballed for now
    bool fourScoreDetected = false;
    bool fourScorePortsSwapped = false;

    bool confirmDisconnect(uint8_t index);
    bool recentControllerSlotPresent() const;
    void clearDisconnectConfirm(uint8_t index);
    void setDisconnected(uint8_t index, SnesDeviceType_Enum newType = SNES_DEVICE_NONE);
    SnesDeviceType_Enum confirmStableType(uint8_t index, SnesDeviceType_Enum observedType);
    void clearTypeCandidate(uint8_t index);
    void noteGlitchFrame(uint8_t index);
    void requestSafeSnesPoll(uint8_t port, uint32_t now_ms);
    bool snesSafePollActive(uint32_t now_ms) const;
    bool snesRumbleTechEnabled() const;
    void applySnesFrameIdentity(uint8_t port, uint8_t index, controller_state_t& frame);
    SnesDeviceType_Enum effectiveDeviceType(const SnesController& sc);
    bool isStableSnesPadTransientType(uint8_t index, SnesDeviceType_Enum observedType);
    uint16_t stabilizeSnesPadDigital(uint8_t index, uint16_t rawDigital, SnesDeviceType_Enum type);
    SnesDeviceType_Enum normalizeVirtualBoyObservedType(uint8_t index, const SnesController& sc, SnesDeviceType_Enum observedType);
    uint8_t virtualBoyABBits(const SnesController& sc);
    bool isRumbleTechDetected(uint8_t port);
    bool isRumbleTechLatched(uint8_t port) const;
    void updateRumbleTechDetection(uint8_t port, bool detected_now);
    void clearRumbleTechDetection();
    bool standardPadFastPathEligible(uint8_t port, uint8_t index, const SnesController& sc,
                                     uint8_t portControllerCount, bool safePollActive);
    bool tryFastPollStandardNes(uint8_t port, uint8_t index);
    void applyStandardPadFastPathIdentity(controller_state_t& frame, bool nesMode);
    void mapStandardPadFastPath(uint8_t port, uint8_t index, const SnesController& sc, uint32_t nowMs);
    uint8_t toSnesRumbleNibble(uint8_t value) const;
    void detectFourScoreDual();
    bool pollFourScore();
    bool is_port_connected(const uint8_t index) override;

  public:
    RZInputSnes();

    uint8_t getInternalMode();
    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;

    void setup() override;
    void setup2() override;
    bool poll() override;
    bool hasPhysicalConnectionForHotSwap() const override;
    const char* physicalConnectionDisplayName() const override;
};
