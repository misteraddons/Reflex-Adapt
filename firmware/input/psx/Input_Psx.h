#pragma once

#include <EEPROM.h>

#include "../base/RZInputModule.h"
#include "input_psx_guncon_runtime_state.h"
#include "input_psx_jogcon_runtime_state.h"
#include "input_psx_negcon_runtime_state.h"
#include "../../core/settings_store.h"
#include "../../menu/menu_runtime_state.h"
#include "../../platform/display_runtime_state.h"
#include <PsxNewLib/PsxDriver.h>
#include <PsxNewLib/PsxPublicTypes.h>
#include <PsxNewLib/PsxSingleController.h>
#include <PsxNewLib/PsxMultiController.h>

// Enable to combine both rumble motors into a single signal
// Useful when game/system only sends to one motor channel
//#define PSX_COMBINE_RUMBLE

#define NEGCON_FORCE_MODE 1

extern uint8_t spinner_speed;
extern const uint8_t spinner_speed_mult[5];

// PS2 DVD Remote IR Command Codes (7-bit, device 0x93)
// These are mapped to controller buttons when IR mode is enabled
namespace PS2IR {
  // D-Pad
  constexpr uint8_t PAD_UP    = 0x5C;
  constexpr uint8_t PAD_DOWN  = 0x5D;
  constexpr uint8_t PAD_LEFT  = 0x5E;
  constexpr uint8_t PAD_RIGHT = 0x5F;
  // Face buttons
  constexpr uint8_t CROSS     = 0x0B;  // Also "Enter"
  constexpr uint8_t CIRCLE    = 0x5B;
  constexpr uint8_t SQUARE    = 0x5E;  // Note: same as PAD_LEFT on some remotes
  constexpr uint8_t TRIANGLE  = 0x5A;
  // Shoulder buttons
  constexpr uint8_t L1        = 0x50;  // Slow Backward
  constexpr uint8_t L2        = 0x51;
  constexpr uint8_t L3        = 0x52;
  constexpr uint8_t R1        = 0x53;  // Slow Forward
  constexpr uint8_t R2        = 0x54;
  constexpr uint8_t R3        = 0x55;
  // Start/Select
  constexpr uint8_t SELECT    = 0x80;
  constexpr uint8_t START     = 0x0B;  // Enter acts as Start
  // Media controls
  constexpr uint8_t PLAY      = 0x30;
  constexpr uint8_t PAUSE     = 0x39;
  constexpr uint8_t STOP      = 0x38;
  constexpr uint8_t PREV      = 0x2C;
  constexpr uint8_t NEXT      = 0x2D;
  constexpr uint8_t SCAN_BACK = 0x31;
  constexpr uint8_t SCAN_FWD  = 0x32;
  constexpr uint8_t RETURN    = 0x0E;
  constexpr uint8_t IR_DISPLAY = 0x40;  // Named IR_DISPLAY to avoid conflict with Arduino DISPLAY macro
  // Number buttons 0-9
  constexpr uint8_t NUM_0     = 0x09;
  constexpr uint8_t NUM_1     = 0x00;
  constexpr uint8_t NUM_2     = 0x01;
  constexpr uint8_t NUM_3     = 0x02;
  constexpr uint8_t NUM_4     = 0x03;
  constexpr uint8_t NUM_5     = 0x04;
  constexpr uint8_t NUM_6     = 0x05;
  constexpr uint8_t NUM_7     = 0x06;
  constexpr uint8_t NUM_8     = 0x07;
  constexpr uint8_t NUM_9     = 0x08;
}

// Multitap consumes the physical PSX port where it is detected and exposes
// its four controller sockets as logical USB slots.

//#define PSX_ACK 2 // HDMI_08 RESISTOR roxo-amarelo ok
//#define PSX_CMD 3 // HDMI_11 marrom
//#define PSX_DAT 4 // HDMI_12 RESISTOR azul-laranja ok
//#define PSX_ATT 5 // HDMI_13 vermelho
//#define PSX_CLK 6 // HDMI_10 cinza
//#define PSX_RUM HDMI_14 (7v)
//#define 33 HDMI_17 (+3.3) laranja
//#define gnd HDMI_15 (gnd)

#define GUNCON_FORCE_MODE 3

typedef struct {
  uint8_t spi; // 0 or 1
  uint8_t att; // must be unique
  uint8_t ack; // can be shared
  uint8_t cmd; // can be shared
  uint8_t dat; // can be shared
  uint8_t clk; // can be shared
} input_psx_config_t;

const input_psx_config_t input_psx_config[] = {
  { .spi = 0, .att = HDMI_1_13, .ack = HDMI_1_08, .cmd = HDMI_1_11, .dat = HDMI_1_12, .clk = HDMI_1_10 },
  { .spi = 1, .att = HDMI_2_13, .ack = HDMI_2_08, .cmd = HDMI_2_11, .dat = HDMI_2_12, .clk = HDMI_2_10 },
};

static constexpr uint16_t PSX_MEMCARD_FRAME_SIZE = 128;
static constexpr uint16_t PSX_MEMCARD_FRAME_COUNT = 1024;
static constexpr uint8_t PSX_MEMCARD_SLOTS_PER_PORT = 4;

struct PSXMemoryCardInfo {
  bool present = false;
  uint16_t totalBlocks = 0;
  uint16_t blockSize = PSX_MEMCARD_FRAME_SIZE;
  uint8_t flag = 0;
  uint8_t endByte = 0;
  uint8_t address = 0;
  int lastResult = 0;
};

struct PSXMemoryCardRawProbe {
  bool transferOk = false;
  uint8_t address = 0;
  uint8_t command = 0;
  uint8_t padByte = 0;
  uint16_t frame = 0;
  uint8_t responseLen = 0;
  uint8_t response[24] = {};
};

enum class PSXMemoryCardBlockResult {
  SUCCESS,
  NOT_PSX,
  NOT_PRESENT,
  INVALID_BLOCK,
  ADDRESS_MISMATCH,
  CHECKSUM_ERROR,
  READ_ERROR,
  WRITE_ERROR,
};

void psxMemoryCardBridgeNoteActivity(uint16_t holdMs = 250);
bool psxMemoryCardBridgeActive();
class Print;

inline void setPsxControllerTypeName(controller_state_t& frame, const char* name) {
  setInputFrameTypeName(frame, name);
}

class RZInputPSX : public RZInputModule {
  private:
      static const uint8_t input_ports { sizeof(input_psx_config) / sizeof(input_psx_config_t) };
      static constexpr uint8_t multitap_slots = 4;
      static constexpr uint8_t logical_slots = MAX_USB_OUT;
      static const uint16_t SLOT_DISCONNECT_DEBOUNCE_MS = 120;
      PsxDriver* psxDriver[input_ports] { nullptr };//<PSX_ATT, PSX_ACK, PSX_CMD, PSX_DAT, PSX_CLK> memory-card driver;
      PsxDriver* psxControllerDriver[input_ports] { nullptr };
      PsxSingleController* psx[input_ports] { nullptr };
      PsxMultiController* psxmulti[input_ports] { nullptr };
      bool memoryCardBridgeDriversReady { false };
      bool psxControllerFramesConfigured { false };

      enum MultitapModeControllerType { // used on multitap mode
        CONT_NONE,
        CONT_SINGLE,
        CONT_MULTIPLE
      };

      MultitapModeControllerType controllerType;

      //todo change to 16 bits if any mask uses the main buttons
      const uint8_t SPECIALMASK_POPN = PSB_PAD_RIGHT | PSB_PAD_DOWN | PSB_PAD_LEFT; //0xE0;
      const uint8_t SPECIALMASK_JET  = PSB_PAD_RIGHT | PSB_PAD_LEFT; //0xC0;

      bool isMultitap { false };
      bool multitapPhysicalPresent { false };
      uint8_t multitapPhysicalPort { 0 };
      bool multitapMemoryCardPresent { false };
      uint32_t multitapMemoryCardLastProbeMs { 0 };
      mutable bool autoResolvedPhysicalProbePresent { false };
      mutable uint32_t autoResolvedPhysicalProbeLastMs { 0 };
      bool isNeGcon { false };
      bool isJogcon { false };
      bool isGuncon { false };
      bool isMouse { false };
      bool isIRRemote { false };
      // PSX peripheral output is fixed to MiSTer variants on MiSTer/Linux (DInput).
      bool isDS2[logical_slots] { false };
      bool isCrazyHit { false };
      bool isWaiwai { false };
      bool isDancePad[logical_slots] { false };
      uint8_t specialDpadMask[logical_slots] { 0 };
      bool haveController[logical_slots] { false };
      PsxControllerProtocol lastProto[logical_slots] { PSPROTO_UNKNOWN };
      PsxControllerProtocol rumbleConfiguredProto[logical_slots] { PSPROTO_UNKNOWN };
      uint32_t lastSuccessfulReadMs[logical_slots] { 0 };
      uint32_t startupGraceUntilMs[logical_slots] { 0 };
      uint32_t slotLastSeenMs[logical_slots] { 0 };
      int16_t fishingReelPosition[logical_slots] { 0 };
      uint32_t debugPollLoops { 0 };
      uint32_t debugPollSkipped[logical_slots] { 0 };
      uint32_t debugBeginAttempt[logical_slots] { 0 };
      uint32_t debugBeginSuccess[logical_slots] { 0 };
      uint32_t debugReadSuccess[logical_slots] { 0 };
      uint32_t debugReadFail[logical_slots] { 0 };
      uint32_t debugDropCount[logical_slots] { 0 };
      bool enableMouseMove { false }; //used on guncon and jogcon modes
      uint8_t pendingWebhidRaw[16] = {};
      bool pendingWebhidRawDirty = false;

      void handleDpad() {}
      void configureControllerFrame(uint8_t i);
      void stopRumble(uint8_t i);
      void tryEnableRumble(uint8_t i);
      bool tryEnableAnalogMode(uint8_t i);
      void updateControllerAttentionInterval(uint8_t i, PsxControllerProtocol proto);
      void clearPhysicalFallbackLatches();
      bool isAnyDualShock2PressureActive() const;
      void setDualShock2PressureState(uint8_t index, bool enabled);
      void applyDualShock2Pressure(uint8_t index, PsxSingleController* controller);
      void applyDualShock2Pressure(uint8_t index, PsxControllerData& controller);
      bool isGuitarFreaksSignature(PsxSingleController* controller,
                                   PsxControllerProtocol proto,
                                   uint16_t digitalData) const;
      void applyGuitarFreaksMapping(uint8_t index);
      uint16_t controllerDisconnectGraceMs() const;
      uint16_t controllerStartupGraceMs() const;
      void armControllerStartupGrace(uint8_t port);
      void markControllerAlive(uint8_t port);
      bool shouldDropController(uint8_t port) const;
      void markControllerDisconnected(uint8_t port);
      void resetFishingState(uint8_t i);
      void applyFishingExperimentalMapping(uint8_t i, PsxSingleController* controller);
      void applyFishingExperimentalMapping(uint8_t i, PsxControllerData& cont);
      void markMultitapSlotMissing(uint8_t slot, uint32_t now);
      void clearMultitapSlot(uint8_t slot);
      bool ensurePsxBusDriversReady(bool configureFrames);
      bool refreshPsxBusDrivers();
      bool detectMemoryCardPhysicalPresenceOnPort(uint8_t port);
      bool refreshMultitapMemoryCardPresence(uint32_t now);
      bool multitapHasController(uint8_t port);
      bool autoResolvedPhysicalConnectionActive() const;
      bool multitapPhysicalConnectionActive() const;
      void stageWebhidRawData(const uint8_t* raw, uint8_t len);
      void flushPendingWebhidRawData();

#include "Input_Psx_Guncon.h"
#include "Input_Psx_Negcon.h"
#include "Input_Psx_Jogcon.h"

  public:
    RZInputPSX();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;

    void setup() override;
    void setup2() override;
    bool setupMemoryCardBridgeOnly();
    bool hasPhysicalConnectionForHotSwap() const override;
    const char* physicalConnectionDisplayName() const override;

    void mapMultiController(uint8_t i, PsxControllerData& cont);

    bool is_port_connected(const uint8_t index) override;
    void handleSpecialMask(uint8_t index);
    bool pollMultitap();
    bool poll() override;
    void afterOutputFrameSent(bool polled, bool updated) override;

    bool getPSXMemoryCardInfo(uint8_t port, uint8_t slot, PSXMemoryCardInfo* info);
    bool probePSXMemoryCardRaw(uint8_t port, uint8_t slot, uint16_t frame, PSXMemoryCardRawProbe* probe,
                               uint8_t padByte = 0x00, uint8_t command = 0x52,
                               uint8_t addressOverride = 0x00);
    PSXMemoryCardBlockResult readPSXMemoryCardFrame(uint8_t port, uint8_t slot, uint16_t frame, uint8_t* buffer);
    PSXMemoryCardBlockResult writePSXMemoryCardFrame(uint8_t port, uint8_t slot, uint16_t frame, const uint8_t* buffer);
    void printDebugStatus(Print& out) const;
    void printDebugProbe(Print& out);
};
