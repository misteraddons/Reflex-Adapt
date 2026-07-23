#pragma once

// Enable Guitar Hero controller support
#define ENABLE_WII_GUITAR

#include "../base/RZInputModule.h"
#include "input_wii_shared.h"
#include "input_wii_runtime_state.h"

// Wii/GC analog range setting: 0=Raw, 1=Normalized, 2=Calibrated, 3=Learn.
extern uint8_t wii_analog_range;

typedef struct {
  uint8_t sda;
  uint8_t scl;
} input_wii_pin_pair_t;

static constexpr uint8_t INPUT_WII_PIN_PAIR_COUNT = 2;
static constexpr uint8_t INPUT_WII_PIN_PAIR_INVALID = 0xFF;

typedef struct {
  TwoWire* wire;
  input_wii_pin_pair_t pinPairs[INPUT_WII_PIN_PAIR_COUNT];
} input_wii_config_t;

const input_wii_config_t input_wii_config[] = {
  // Support both board routes. Prefer direct 3.3 V I2C, then probe the
  // level-shifted HDMI pins 1/2 route.
  { .wire = &Wire1, .pinPairs = { { .sda = 2, .scl = 7 }, { .sda = 10, .scl = 11 } } },
  { .wire = &Wire1, .pinPairs = { { .sda = 14, .scl = 15 }, { .sda = 22, .scl = 23 } } },
};

class RZInputWii : public RZInputModule {
  private:
    static const uint8_t input_ports { sizeof(input_wii_config) / sizeof(input_wii_config_t) };
    static constexpr uint32_t WII_CONNECTED_POLL_INTERVAL_US = 500;
    static constexpr uint32_t WII_EMPTY_PORT_CONNECT_INTERVAL_US = 500000;
    ExtensionPort* wii[input_ports] = {};
    ClassicController::Shared* wii_classic[input_ports] = {};
    Nunchuk::Shared* wii_nchuk[input_ports] = {};
    #ifdef ENABLE_WII_GUITAR
    GuitarController::Shared* wii_guitar[input_ports] = {};
    #endif

    bool haveController[input_ports] = {};
    uint8_t updateFailCount[input_ports] = {};
    bool settlingAfterBadFrame[input_ports] = {};
    bool cachedClassicPro[input_ports] = {};
    bool cachedClassicProValid[input_ports] = {};
    ExtensionType cachedControllerType[input_ports] = {};
    uint8_t activePinPair[input_ports] = {};
    uint32_t nextConnectAttemptUs[input_ports] = {};
    uint8_t initializedBusPort = INPUT_WII_PIN_PAIR_INVALID;
    uint8_t initializedBusPair = INPUT_WII_PIN_PAIR_INVALID;
    uint8_t activePortMask {0xFF};
    uint8_t pendingWebhidRaw[16] = {};
    bool pendingWebhidRawDirty = false;
    ExtensionType queuedWebhidRawType = ExtensionType::NoController;
    uint8_t queuedWebhidRawPort = 0;
    bool queuedWebhidRawBuild = false;
    // A connected controller is polled every 500 us. Tolerate a short burst of
    // I2C errors without treating it as an unplug, while still disconnecting
    // quickly enough for normal hot-swap behavior.
    static const uint8_t WII_UPDATE_FAIL_DISCONNECT_THRESHOLD = 32;

    bool portEnabled(uint8_t port) const;
    bool beginWiiBus(uint8_t port, uint8_t pair);
    void endWiiBus(uint8_t port);
    bool wii_connect(uint8_t i);
    bool wii_update(uint8_t i);
    void resetControllerCache(uint8_t port);
    bool isCachedClassicPro(uint8_t port);
    void queueWebhidRawData(uint8_t port, ExtensionType conType);
    void buildQueuedWebhidRawData();
    void flushPendingWebhidRawData();

  public:
    RZInputWii();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    void afterOutputFrameSent(bool polled, bool updated) override;
    void setPhysicalPortMask(uint8_t mask) override;
};
