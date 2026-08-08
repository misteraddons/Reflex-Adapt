#include "Input_Psx.h"

#include <Arduino.h>
#include <string.h>

#ifdef ENABLE_INPUT_PSX

namespace {

uint32_t psx_memcard_bridge_hold_until_ms = 0;

}  // namespace

void psxMemoryCardBridgeNoteActivity(uint16_t holdMs) {
  psx_memcard_bridge_hold_until_ms = millis() + holdMs;
}

bool psxMemoryCardBridgeActive() {
  return (int32_t)(psx_memcard_bridge_hold_until_ms - millis()) > 0;
}

namespace {

static constexpr uint8_t kPsxMemCardAddress = 0x81;
static constexpr uint8_t kPsxMemCardReadCommand = 0x52;   // ASCII 'R'
static constexpr uint8_t kPsxMemCardWriteCommand = 0x57;  // ASCII 'W'
static constexpr uint8_t kPsxMemCardId1 = 0x5A;
static constexpr uint8_t kPsxMemCardId2 = 0x5D;
static constexpr uint8_t kPsxMemCardAck1 = 0x5C;
static constexpr uint8_t kPsxMemCardAck2 = 0x5D;
static constexpr uint8_t kPsxMemCardGoodEnd = 0x47;       // ASCII 'G'
static constexpr uint16_t kPsxMemCardAckTimeoutUs = INTER_CMD_BYTE_TIMEOUT;
static constexpr uint16_t kPsxMemCardLateReadAckTimeoutUs = 2000;
static constexpr uint8_t kPsxMemCardLateReadAckByte = 6;
static constexpr uint16_t kPsxMemCardAttentionIntervalUs = 1000;
static constexpr uint8_t kPsxMemCardReadLen = 140;
static constexpr uint8_t kPsxMemCardWriteLen = 138;

bool validPsxMemCardPort(uint8_t port) {
  return port < (sizeof(input_psx_config) / sizeof(input_psx_config_t));
}

bool validPsxMemCardSlot(uint8_t slot) {
  return slot < PSX_MEMCARD_SLOTS_PER_PORT;
}

uint8_t psxMemCardAddressForSlot(uint8_t slot) {
  return kPsxMemCardAddress + slot;
}

uint8_t psxMemCardChecksum(uint8_t msb, uint8_t lsb, const uint8_t* data) {
  uint8_t checksum = msb ^ lsb;
  for (uint16_t i = 0; i < PSX_MEMCARD_FRAME_SIZE; ++i) {
    checksum ^= data[i];
  }
  return checksum;
}

class ScopedPsxAttentionInterval {
public:
  explicit ScopedPsxAttentionInterval(PsxDriver& driver)
    : driver_(driver), previous_(driver.getAttentionInterval()) {
    driver_.setAttentionInterval(kPsxMemCardAttentionIntervalUs);
  }

  ~ScopedPsxAttentionInterval() {
    driver_.setAttentionInterval(previous_);
  }

private:
  PsxDriver& driver_;
  uint16_t previous_;
};

PSXMemoryCardBlockResult psxMemCardReadFrame(PsxDriver& driver, uint8_t slot, uint16_t frame, uint8_t* buffer) {
  if (buffer == nullptr || !validPsxMemCardSlot(slot) || frame >= PSX_MEMCARD_FRAME_COUNT) {
    return PSXMemoryCardBlockResult::INVALID_BLOCK;
  }
  psxMemoryCardBridgeNoteActivity();

  const uint8_t msb = (uint8_t)(frame >> 8);
  const uint8_t lsb = (uint8_t)(frame & 0xFF);
  uint8_t out[kPsxMemCardReadLen] = {};
  uint8_t in[kPsxMemCardReadLen] = {};
  out[0] = psxMemCardAddressForSlot(slot);
  out[1] = kPsxMemCardReadCommand;
  out[4] = msb;
  out[5] = lsb;

  ScopedPsxAttentionInterval interval(driver);
  driver.selectController();
  const bool transferOk = driver.shiftInOutTimedWithLateAck(
    out, sizeof(out), in, sizeof(in), false,
    kPsxMemCardAckTimeoutUs,
    kPsxMemCardLateReadAckByte,
    kPsxMemCardLateReadAckTimeoutUs);
  driver.deselectController();
  psxMemoryCardBridgeNoteActivity();

  if (in[2] != kPsxMemCardId1 || in[3] != kPsxMemCardId2) {
    return PSXMemoryCardBlockResult::NOT_PRESENT;
  }
  if (!transferOk ||
      in[6] != kPsxMemCardAck1 ||
      in[7] != kPsxMemCardAck2 ||
      in[139] != kPsxMemCardGoodEnd) {
    return PSXMemoryCardBlockResult::READ_ERROR;
  }
  if (in[8] != msb || in[9] != lsb) {
    return PSXMemoryCardBlockResult::ADDRESS_MISMATCH;
  }

  const uint8_t checksum = psxMemCardChecksum(msb, lsb, &in[10]);
  if (checksum != in[138]) {
    return PSXMemoryCardBlockResult::CHECKSUM_ERROR;
  }

  memcpy(buffer, &in[10], PSX_MEMCARD_FRAME_SIZE);
  return PSXMemoryCardBlockResult::SUCCESS;
}

bool psxMemCardProbeRaw(PsxDriver& driver,
                        uint8_t slot,
                        uint16_t frame,
                        PSXMemoryCardRawProbe* probe,
                        uint8_t padByte,
                        uint8_t command,
                        uint8_t addressOverride) {
  if (probe == nullptr || !validPsxMemCardSlot(slot) || frame >= PSX_MEMCARD_FRAME_COUNT) {
    return false;
  }
  psxMemoryCardBridgeNoteActivity();

  *probe = PSXMemoryCardRawProbe{};
  probe->address = (addressOverride != 0x00) ? addressOverride : psxMemCardAddressForSlot(slot);
  probe->command = command;
  probe->padByte = padByte;
  probe->frame = frame;
  probe->responseLen = sizeof(probe->response);

  const uint8_t msb = (uint8_t)(frame >> 8);
  const uint8_t lsb = (uint8_t)(frame & 0xFF);
  uint8_t out[sizeof(probe->response)];
  memset(out, padByte, sizeof(out));
  out[0] = probe->address;
  out[1] = command;
  out[4] = msb;
  out[5] = lsb;

  ScopedPsxAttentionInterval interval(driver);
  driver.selectController();
  probe->transferOk = driver.shiftInOutTimedWithLateAck(
    out, sizeof(out), probe->response, sizeof(probe->response), false,
    kPsxMemCardAckTimeoutUs,
    kPsxMemCardLateReadAckByte,
    kPsxMemCardLateReadAckTimeoutUs);
  driver.deselectController();
  psxMemoryCardBridgeNoteActivity();
  return true;
}

PSXMemoryCardBlockResult psxMemCardWriteFrame(PsxDriver& driver, uint8_t slot, uint16_t frame, const uint8_t* buffer) {
  if (buffer == nullptr || !validPsxMemCardSlot(slot) || frame >= PSX_MEMCARD_FRAME_COUNT) {
    return PSXMemoryCardBlockResult::INVALID_BLOCK;
  }
  psxMemoryCardBridgeNoteActivity();

  const uint8_t msb = (uint8_t)(frame >> 8);
  const uint8_t lsb = (uint8_t)(frame & 0xFF);
  uint8_t out[kPsxMemCardWriteLen] = {};
  uint8_t in[kPsxMemCardWriteLen] = {};
  out[0] = psxMemCardAddressForSlot(slot);
  out[1] = kPsxMemCardWriteCommand;
  out[4] = msb;
  out[5] = lsb;
  memcpy(&out[6], buffer, PSX_MEMCARD_FRAME_SIZE);
  out[134] = psxMemCardChecksum(msb, lsb, buffer);

  ScopedPsxAttentionInterval interval(driver);
  driver.selectController();
  const bool transferOk = driver.shiftInOutTimed(
    out, sizeof(out), in, sizeof(in), false, kPsxMemCardAckTimeoutUs);
  driver.deselectController();
  psxMemoryCardBridgeNoteActivity();

  if (in[2] != kPsxMemCardId1 || in[3] != kPsxMemCardId2) {
    return PSXMemoryCardBlockResult::NOT_PRESENT;
  }
  if (!transferOk || in[135] != kPsxMemCardAck1 || in[136] != kPsxMemCardAck2) {
    return PSXMemoryCardBlockResult::WRITE_ERROR;
  }
  if (in[137] != kPsxMemCardGoodEnd) {
    return in[137] == 0xFF ? PSXMemoryCardBlockResult::INVALID_BLOCK
                           : PSXMemoryCardBlockResult::WRITE_ERROR;
  }

  return PSXMemoryCardBlockResult::SUCCESS;
}

}  // namespace

bool RZInputPSX::getPSXMemoryCardInfo(uint8_t port, uint8_t slot, PSXMemoryCardInfo* info) {
  if (info != nullptr) {
    *info = PSXMemoryCardInfo{};
    info->address = validPsxMemCardSlot(slot) ? psxMemCardAddressForSlot(slot) : 0;
  }
  if (!validPsxMemCardPort(port) || !validPsxMemCardSlot(slot) || psxDriver[port] == nullptr) {
    if (info != nullptr) {
      info->lastResult = (int)PSXMemoryCardBlockResult::NOT_PSX;
    }
    return false;
  }

  uint8_t header[PSX_MEMCARD_FRAME_SIZE] = {};
  const PSXMemoryCardBlockResult result = readPSXMemoryCardFrame(port, slot, 0, header);
  if (info != nullptr) {
    info->lastResult = (int)result;
  }
  if (result != PSXMemoryCardBlockResult::SUCCESS || header[0] != 'M' || header[1] != 'C') {
    return false;
  }

  if (info != nullptr) {
    info->present = true;
    info->totalBlocks = PSX_MEMCARD_FRAME_COUNT;
    info->blockSize = PSX_MEMCARD_FRAME_SIZE;
    info->endByte = kPsxMemCardGoodEnd;
    info->address = psxMemCardAddressForSlot(slot);
  }
  return true;
}

PSXMemoryCardBlockResult RZInputPSX::readPSXMemoryCardFrame(uint8_t port,
                                                           uint8_t slot,
                                                           uint16_t frame,
                                                           uint8_t* buffer) {
  if (!validPsxMemCardPort(port) || !validPsxMemCardSlot(slot) || psxDriver[port] == nullptr) {
    return PSXMemoryCardBlockResult::NOT_PSX;
  }
  psxMemoryCardBridgeNoteActivity();
  return psxMemCardReadFrame(*psxDriver[port], slot, frame, buffer);
}

bool RZInputPSX::probePSXMemoryCardRaw(uint8_t port,
                                       uint8_t slot,
                                       uint16_t frame,
                                       PSXMemoryCardRawProbe* probe,
                                       uint8_t padByte,
                                       uint8_t command,
                                       uint8_t addressOverride) {
  if (!validPsxMemCardPort(port) || !validPsxMemCardSlot(slot) || psxDriver[port] == nullptr) {
    return false;
  }
  psxMemoryCardBridgeNoteActivity();
  return psxMemCardProbeRaw(*psxDriver[port], slot, frame, probe, padByte, command, addressOverride);
}

PSXMemoryCardBlockResult RZInputPSX::writePSXMemoryCardFrame(uint8_t port,
                                                            uint8_t slot,
                                                            uint16_t frame,
                                                            const uint8_t* buffer) {
  if (!validPsxMemCardPort(port) || !validPsxMemCardSlot(slot) || psxDriver[port] == nullptr) {
    return PSXMemoryCardBlockResult::NOT_PSX;
  }
  psxMemoryCardBridgeNoteActivity();
  return psxMemCardWriteFrame(*psxDriver[port], slot, frame, buffer);
}

#endif
