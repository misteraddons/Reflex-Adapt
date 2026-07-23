#pragma once

#include "../base/RZInputModule.h"
#include "input_gc64_runtime_state.h"
#include <JoybusLib/JoybusLib.h>

typedef struct {
  PIO pio;
  uint8_t sm;
  uint8_t dat;
} input_gc64_config_t;

const input_gc64_config_t input_gc64_config[] = {
  { .pio =  pio0, .sm =  0, .dat =  HDMI_1_13 },
  { .pio =  pio0, .sm =  1, .dat =  HDMI_2_13 },
};

static constexpr int8_t N64_ANALOG_RAW_MAX = 80;
static constexpr int8_t N64_ANALOG_RAW_MIN = -N64_ANALOG_RAW_MAX;
static constexpr int8_t GC_ANALOG_RAW_MAX = 100;
static constexpr int8_t GC_ANALOG_RAW_MIN = -(GC_ANALOG_RAW_MAX + 1);
static constexpr uint8_t GC_TRIGGER_RAW_MIN = 10;
static constexpr uint8_t GC_TRIGGER_RAW_MAX = 160;
static constexpr uint16_t N64_PAK_PAGE_SIZE = 256;
static constexpr uint16_t N64_PAK_PAGE_COUNT = 128;
static constexpr uint16_t N64_PAK_RAW_BLOCK_SIZE = 32;
static constexpr uint16_t N64_TRANSFER_PAK_BLOCK_SIZE = 32;

struct N64PakInfo {
  bool present = false;
  bool rumblePak = false;
  uint8_t aux = 0;
  uint16_t totalBlocks = 0;
  uint16_t blockSize = N64_PAK_PAGE_SIZE;
  int lastResult = 0;
};

struct N64TransferPakInfo {
  bool present = false;
  bool cartPresent = false;
  bool accessEnabled = false;
  bool headerValid = false;
  bool logoValid = false;
  bool headerChecksumValid = false;
  bool supportedMbc = false;
  uint8_t aux = 0;
  uint8_t status = 0;
  uint8_t cgbFlag = 0;
  uint8_t sgbFlag = 0;
  uint8_t cartType = 0;
  uint8_t romSizeCode = 0;
  uint8_t ramSizeCode = 0;
  uint8_t headerChecksum = 0;
  uint16_t globalChecksum = 0;
  uint16_t romBanks = 0;
  uint8_t ramBanks = 0;
  uint32_t romSize = 0;
  uint32_t saveSize = 0;
  uint8_t headerReadAttempts = 0;
  int lastResult = 0;
  char title[17] = {};
  char titleKey[17] = {};
  char mbcName[8] = {};
};

struct N64TransferPakProbeInfo {
  bool validPort = false;
  bool n64Device = false;
  bool accessoryPresent = false;
  bool rumblePak = false;
  uint8_t deviceType = 0;
  uint8_t aux = 0;
  int powerWrite = 0;
  int powerRead = 0;
  uint8_t powerValue = 0;
  int statusBeforeRead = 0;
  uint8_t statusBefore = 0;
  int accessWrite = 0;
  int statusRead[3] = {};
  uint8_t status[3] = {};
  int bankWrite = 0;
  int bankRead = 0;
  uint8_t bankValue = 0;
  int headerRead = 0;
  uint8_t headerPreview[8] = {};
};

enum class N64PakBlockResult {
  SUCCESS,
  NOT_N64,
  NOT_PRESENT,
  INVALID_BLOCK,
  READ_ERROR,
  WRITE_ERROR,
  VERIFY_ERROR,
  RESTORE_ERROR,
};

typedef bool (*N64TransferPakBlockCallback)(void* context, uint32_t block, const uint8_t* data);

struct N64PakPageTestResult {
  N64PakBlockResult status = N64PakBlockResult::NOT_PRESENT;
  N64PakBlockResult readBefore = N64PakBlockResult::NOT_PRESENT;
  N64PakBlockResult write = N64PakBlockResult::NOT_PRESENT;
  N64PakBlockResult readAfter = N64PakBlockResult::NOT_PRESENT;
  N64PakBlockResult restore = N64PakBlockResult::NOT_PRESENT;
  N64PakBlockResult readRestore = N64PakBlockResult::NOT_PRESENT;
  JoybusMemoryWriteDiag firstWriteDiag = {};
  JoybusMemoryWriteDiag failingWriteDiag = {};
  uint16_t page = 0;
  uint8_t failingRawBlock = 0xFF;
  uint16_t mismatchOffset = 0xFFFF;
  uint8_t mismatchExpected = 0;
  uint8_t mismatchActual = 0;
  uint16_t restoreMismatchOffset = 0xFFFF;
  uint8_t restoreMismatchExpected = 0;
  uint8_t restoreMismatchActual = 0;
  bool verifyMatch = false;
  bool restoreMatch = false;
};

static inline int16_t invertSignedInt8Axis(int16_t raw) {
  raw = constrain(raw, (int16_t)INT8_MIN, (int16_t)INT8_MAX);
  return (raw == INT8_MIN) ? INT8_MAX : -raw;
}

static inline int8_t normalizeSignedAxisToFullInt8(int16_t raw, int8_t raw_min, int8_t raw_max) {
  if (raw == 0) {
    return 0;
  }

  int32_t normalized = 0;
  if (raw < 0) {
    if (raw < raw_min) raw = raw_min;
    normalized = ((int32_t)raw * 127) / (-raw_min);
  } else {
    if (raw > raw_max) raw = raw_max;
    normalized = ((int32_t)raw * 127) / raw_max;
  }

  return (int8_t)constrain(normalized, -127, 127);
}

static inline int8_t normalizeN64StickAxis(int16_t raw) {
  return normalizeSignedAxisToFullInt8(raw, N64_ANALOG_RAW_MIN, N64_ANALOG_RAW_MAX);
}

static inline int8_t normalizeGCStickAxis(int16_t raw) {
  return normalizeSignedAxisToFullInt8(raw, GC_ANALOG_RAW_MIN, GC_ANALOG_RAW_MAX);
}

static inline uint8_t normalizeGCTriggerAxis(uint8_t raw) {
  if (raw <= GC_TRIGGER_RAW_MIN) return 0;
  if (raw >= GC_TRIGGER_RAW_MAX) return 255;
  return (uint8_t)(((uint16_t)(raw - GC_TRIGGER_RAW_MIN) * 255) /
                   (GC_TRIGGER_RAW_MAX - GC_TRIGGER_RAW_MIN));
}

class RZInputGC64 : public RZInputModule {
  private:
    static const uint8_t input_ports { sizeof(input_gc64_config) / sizeof(input_gc64_config_t) };
    JoybusPort* joybus[input_ports] = {};
    JoybusDeviceType_Enum dtype[input_ports] = {};
    bool wavebirdReceiverSeen[input_ports] = {};
    uint8_t pendingWebhidRawPort = 0;
    JoybusDeviceType_Enum pendingWebhidRawType = JOYBUS_DEVICE_NONE;
    bool pendingWebhidRawDirty = false;

    void stageWebhidRawData(uint8_t port, JoybusDeviceType_Enum type);
    void flushPendingWebhidRawData();

  public:
    RZInputGC64();

    uint8_t getInternalMode();
    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    bool is_port_connected(const uint8_t index) override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    void afterOutputFrameSent(bool polled, bool updated) override;

    bool getN64ControllerPakInfo(uint8_t port, N64PakInfo* info);
    N64PakBlockResult readN64ControllerPakPage(uint8_t port, uint16_t page, uint8_t* buffer);
    N64PakBlockResult writeN64ControllerPakPage(uint8_t port, uint16_t page, const uint8_t* buffer);
    N64PakBlockResult testN64ControllerPakPage(uint8_t port, uint16_t page,
                                               N64PakPageTestResult* result);
    bool getN64TransferPakInfo(uint8_t port, N64TransferPakInfo* info);
    bool probeN64TransferPak(uint8_t port, N64TransferPakProbeInfo* info);
    N64PakBlockResult readN64TransferPakRomBlock(uint8_t port, uint32_t block, uint8_t* buffer);
    N64PakBlockResult readN64TransferPakSaveBlock(uint8_t port, uint32_t block, uint8_t* buffer);
    N64PakBlockResult readN64TransferPakRomBlock(uint8_t port, const N64TransferPakInfo& info,
                                                 uint32_t block, uint8_t* buffer);
    N64PakBlockResult readN64TransferPakSaveBlock(uint8_t port, const N64TransferPakInfo& info,
                                                  uint32_t block, uint8_t* buffer);
    N64PakBlockResult readN64TransferPakBlocks(uint8_t port, const N64TransferPakInfo& info,
                                               uint32_t startBlock, uint32_t count, bool save,
                                               N64TransferPakBlockCallback callback, void* context,
                                               uint32_t* failedBlock = nullptr);
    N64PakBlockResult writeN64TransferPakSaveBlock(uint8_t port, uint32_t block, const uint8_t* buffer);
    N64PakBlockResult writeN64TransferPakSaveBlock(uint8_t port, const N64TransferPakInfo& info,
                                                   uint32_t block, const uint8_t* buffer);
    N64PakBlockResult writeN64TransferPakBlocks(uint8_t port, const N64TransferPakInfo& info,
                                                uint32_t startBlock, uint32_t count,
                                                const uint8_t* data,
                                                uint32_t* failedBlock = nullptr);
};
