#include "Input_GC64.h"

#ifdef ENABLE_INPUT_N64

#include <Arduino.h>
#include <string.h>

namespace {

bool validN64PakPort(uint8_t port) {
  return port < (sizeof(input_gc64_config) / sizeof(input_gc64_config_t));
}

constexpr uint16_t kN64PakBaseAddress = 0x0000;
constexpr uint8_t kN64PakIoRetries = 3;
constexpr uint16_t kTransferPakPowerAddress = 0x8000;
constexpr uint16_t kTransferPakBankAddress = 0xA000;
constexpr uint16_t kTransferPakStatusAddress = 0xB000;
constexpr uint16_t kTransferPakCartWindow = 0xC000;
constexpr uint8_t kTransferPakPowerOn = 0x84;
constexpr uint8_t kTransferPakAccessOn = 0x01;
constexpr uint8_t kTransferPakStatusNoCart = 0x40;
constexpr uint8_t kTransferPakStatusPower = 0x80;
constexpr uint16_t kGbCartBankSize = 0x4000;
constexpr uint16_t kGbExternalRamBase = 0xA000;
constexpr uint16_t kGbExternalRamBankSize = 0x2000;
constexpr uint8_t kGbHeaderSize = 0x60;
constexpr uint8_t kGbHeaderReadAttempts = 5;
constexpr uint8_t kGbHeaderRetryDelayMs = 4;
constexpr uint8_t kNintendoLogo[] = {
  0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
  0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
  0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
  0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
  0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
  0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
};

uint16_t n64PakRawAddress(uint16_t page, uint8_t rawBlock) {
  return (uint16_t)(kN64PakBaseAddress +
      (page * N64_PAK_PAGE_SIZE) + (rawBlock * N64_PAK_RAW_BLOCK_SIZE));
}

int readN64PakRawBlock(JoybusPort* joybus, uint16_t address, uint8_t* raw) {
  for (uint8_t attempt = 0; attempt < kN64PakIoRetries; ++attempt) {
    const int result = joybus->readN64Memory(address, raw);
    if (result == N64_PAK_RAW_BLOCK_SIZE) {
      return result;
    }
    delayMicroseconds(250);
  }
  return 0;
}

int writeN64PakRawBlock(JoybusPort* joybus, uint16_t address, uint8_t* raw) {
  for (uint8_t attempt = 0; attempt < kN64PakIoRetries; ++attempt) {
    const int result = joybus->writeN64Memory(address, raw);
    if (result == 1) {
      return result;
    }
    delayMicroseconds(250);
  }
  return 0;
}

void fillRawBlock(uint8_t* raw, uint8_t value) {
  memset(raw, value, N64_PAK_RAW_BLOCK_SIZE);
}

int writeTransferPakRegister(JoybusPort* joybus, uint16_t address, uint8_t value) {
  uint8_t raw[N64_PAK_RAW_BLOCK_SIZE] = {};
  fillRawBlock(raw, value);
  return writeN64PakRawBlock(joybus, address, raw);
}

int readTransferPakRegister(JoybusPort* joybus, uint16_t address, uint8_t* value) {
  uint8_t raw[N64_PAK_RAW_BLOCK_SIZE] = {};
  const int result = readN64PakRawBlock(joybus, address, raw);
  if (result == N64_PAK_RAW_BLOCK_SIZE && value != nullptr) {
    *value = raw[0];
  }
  return result;
}

bool transferPakMbcSupported(uint8_t cartType) {
  switch (cartType) {
    case 0x00:
    case 0x08:
    case 0x09:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
      return true;
    default:
      return false;
  }
}

const char* transferPakMbcName(uint8_t cartType) {
  switch (cartType) {
    case 0x00:
    case 0x08:
    case 0x09:
      return "ROM";
    case 0x01:
    case 0x02:
    case 0x03:
      return "MBC1";
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
      return "MBC3";
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
      return "MBC5";
    default:
      return "UNSUP";
  }
}

uint16_t gbRomBanksFromHeader(uint8_t code) {
  if (code <= 0x08) {
    return (uint16_t)(2u << code);
  }
  switch (code) {
    case 0x52: return 72;
    case 0x53: return 80;
    case 0x54: return 96;
    default: return 0;
  }
}

uint8_t gbRamBanksFromHeader(uint8_t code) {
  switch (code) {
    case 0x02: return 1;
    case 0x03: return 4;
    case 0x04: return 16;
    case 0x05: return 8;
    default: return 0;
  }
}

uint32_t gbSaveSizeFromHeader(uint8_t code, uint8_t cartType) {
  if (cartType == 0x06) {
    return 512;
  }
  switch (code) {
    case 0x02: return 8u * 1024u;
    case 0x03: return 32u * 1024u;
    case 0x04: return 128u * 1024u;
    case 0x05: return 64u * 1024u;
    default: return 0;
  }
}

bool gbHeaderLogoValid(const uint8_t* header) {
  return header != nullptr && memcmp(header + 0x04, kNintendoLogo, sizeof(kNintendoLogo)) == 0;
}

bool gbHeaderChecksumValid(const uint8_t* header) {
  if (header == nullptr) {
    return false;
  }
  uint8_t checksum = 0;
  for (uint8_t offset = 0x34; offset <= 0x4C; ++offset) {
    checksum = (uint8_t)(checksum - header[offset] - 1);
  }
  return checksum == header[0x4D];
}

bool transferPakInit(JoybusPort* joybus, N64TransferPakInfo* info) {
  if (joybus == nullptr) {
    return false;
  }

  uint8_t value = 0;
  if (writeTransferPakRegister(joybus, kTransferPakPowerAddress, kTransferPakPowerOn) != 1) {
    return false;
  }
  delay(150);
  int result = readTransferPakRegister(joybus, kTransferPakPowerAddress, &value);
  if (result != N64_PAK_RAW_BLOCK_SIZE || value != kTransferPakPowerOn) {
    if (info != nullptr) info->lastResult = result;
    return false;
  }

  if (writeTransferPakRegister(joybus, kTransferPakStatusAddress, kTransferPakAccessOn) != 1) {
    return false;
  }
  delayMicroseconds(500);
  result = readTransferPakRegister(joybus, kTransferPakStatusAddress, &value);
  if (info != nullptr) {
    info->status = value;
    info->lastResult = result;
    info->accessEnabled = result == N64_PAK_RAW_BLOCK_SIZE && (value & kTransferPakAccessOn);
    info->cartPresent = result == N64_PAK_RAW_BLOCK_SIZE && !(value & kTransferPakStatusNoCart);
  }
  return result == N64_PAK_RAW_BLOCK_SIZE &&
         (value & kTransferPakStatusPower) &&
         !(value & kTransferPakStatusNoCart) &&
         (value & kTransferPakAccessOn);
}

bool transferPakEnableAndReadStatus(JoybusPort* joybus, N64TransferPakInfo* info) {
  if (joybus == nullptr) {
    return false;
  }
  if (writeTransferPakRegister(joybus, kTransferPakStatusAddress, kTransferPakAccessOn) != 1) {
    return false;
  }
  delayMicroseconds(500);
  uint8_t value = 0;
  const int result = readTransferPakRegister(joybus, kTransferPakStatusAddress, &value);
  if (info != nullptr) {
    info->status = value;
    info->lastResult = result;
    info->accessEnabled = result == N64_PAK_RAW_BLOCK_SIZE && (value & kTransferPakAccessOn);
    info->cartPresent = result == N64_PAK_RAW_BLOCK_SIZE && !(value & kTransferPakStatusNoCart);
  }
  return result == N64_PAK_RAW_BLOCK_SIZE &&
         (value & kTransferPakStatusPower) &&
         !(value & kTransferPakStatusNoCart) &&
         (value & kTransferPakAccessOn);
}

bool transferPakSignaturePresent(JoybusPort* joybus) {
  uint8_t value = 0;
  if (writeTransferPakRegister(joybus, kTransferPakPowerAddress, kTransferPakPowerOn) != 1) {
    return false;
  }
  delayMicroseconds(500);
  return readTransferPakRegister(joybus, kTransferPakPowerAddress, &value) == N64_PAK_RAW_BLOCK_SIZE &&
         value == kTransferPakPowerOn;
}

uint16_t transferPakAddressForGb(uint16_t gbAddress) {
  return (uint16_t)(kTransferPakCartWindow + (gbAddress & (kGbCartBankSize - 1)));
}

bool selectTransferPakGbBank(JoybusPort* joybus, uint8_t bank) {
  if (writeTransferPakRegister(joybus, kTransferPakBankAddress, bank) != 1) {
    return false;
  }
  delayMicroseconds(120);
  return true;
}

N64PakBlockResult transferPakReadGbBlock(JoybusPort* joybus, uint16_t gbAddress, uint8_t* buffer) {
  if (buffer == nullptr || gbAddress > 0xBFE0 || (gbAddress & 0x1F) != 0) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  const uint8_t bank = (uint8_t)(gbAddress / kGbCartBankSize);
  if (!selectTransferPakGbBank(joybus, bank)) {
    return N64PakBlockResult::WRITE_ERROR;
  }
  const int result = readN64PakRawBlock(joybus, transferPakAddressForGb(gbAddress), buffer);
  return result == N64_PAK_RAW_BLOCK_SIZE ? N64PakBlockResult::SUCCESS : N64PakBlockResult::READ_ERROR;
}

N64PakBlockResult readTransferPakGbHeader(JoybusPort* joybus, uint8_t* header) {
  if (header == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  for (uint8_t offset = 0; offset < kGbHeaderSize; offset += N64_TRANSFER_PAK_BLOCK_SIZE) {
    const N64PakBlockResult result =
      transferPakReadGbBlock(joybus, (uint16_t)(0x0100 + offset), header + offset);
    if (result != N64PakBlockResult::SUCCESS) {
      return result;
    }
  }
  return N64PakBlockResult::SUCCESS;
}

N64PakBlockResult transferPakWriteGbRegister(JoybusPort* joybus, uint16_t gbAddress, uint8_t value) {
  if (gbAddress > 0xBFFF) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  const uint8_t bank = (uint8_t)(gbAddress / kGbCartBankSize);
  if (!selectTransferPakGbBank(joybus, bank)) {
    return N64PakBlockResult::WRITE_ERROR;
  }
  return writeTransferPakRegister(joybus, transferPakAddressForGb(gbAddress), value) == 1
    ? N64PakBlockResult::SUCCESS
    : N64PakBlockResult::WRITE_ERROR;
}

struct TransferPakRangeAccess {
  JoybusPort* joybus = nullptr;
  bool transferBankValid = false;
  uint8_t transferBank = 0;
  bool ramEnabled = false;
  bool mbc1RamMode = false;
  bool ramBankValid = false;
  uint8_t ramBank = 0;
  bool romBankValid = false;
  uint16_t romBank = 0xFFFF;
};

N64PakBlockResult selectTransferPakGbBankCached(TransferPakRangeAccess& access, uint8_t bank) {
  if (access.transferBankValid && access.transferBank == bank) {
    return N64PakBlockResult::SUCCESS;
  }
  if (!selectTransferPakGbBank(access.joybus, bank)) {
    return N64PakBlockResult::WRITE_ERROR;
  }
  access.transferBank = bank;
  access.transferBankValid = true;
  return N64PakBlockResult::SUCCESS;
}

N64PakBlockResult transferPakWriteGbRegisterCached(TransferPakRangeAccess& access,
                                                   uint16_t gbAddress,
                                                   uint8_t value) {
  if (gbAddress > 0xBFFF) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  const uint8_t bank = (uint8_t)(gbAddress / kGbCartBankSize);
  N64PakBlockResult result = selectTransferPakGbBankCached(access, bank);
  if (result != N64PakBlockResult::SUCCESS) {
    return result;
  }
  return writeTransferPakRegister(access.joybus, transferPakAddressForGb(gbAddress), value) == 1
    ? N64PakBlockResult::SUCCESS
    : N64PakBlockResult::WRITE_ERROR;
}

N64PakBlockResult transferPakReadGbBlockCached(TransferPakRangeAccess& access,
                                               uint16_t gbAddress,
                                               uint8_t* buffer) {
  if (buffer == nullptr || gbAddress > 0xBFE0 || (gbAddress & 0x1F) != 0) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  const uint8_t bank = (uint8_t)(gbAddress / kGbCartBankSize);
  N64PakBlockResult result = selectTransferPakGbBankCached(access, bank);
  if (result != N64PakBlockResult::SUCCESS) {
    return result;
  }
  const int rawResult = readN64PakRawBlock(access.joybus, transferPakAddressForGb(gbAddress), buffer);
  return rawResult == N64_PAK_RAW_BLOCK_SIZE ? N64PakBlockResult::SUCCESS : N64PakBlockResult::READ_ERROR;
}

N64PakBlockResult transferPakWriteGbBlockCached(TransferPakRangeAccess& access,
                                                uint16_t gbAddress,
                                                const uint8_t* buffer) {
  if (buffer == nullptr || gbAddress > 0xBFE0 || (gbAddress & 0x1F) != 0) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  const uint8_t bank = (uint8_t)(gbAddress / kGbCartBankSize);
  N64PakBlockResult result = selectTransferPakGbBankCached(access, bank);
  if (result != N64PakBlockResult::SUCCESS) {
    return result;
  }
  uint8_t raw[N64_PAK_RAW_BLOCK_SIZE] = {};
  memcpy(raw, buffer, N64_PAK_RAW_BLOCK_SIZE);
  return writeN64PakRawBlock(access.joybus, transferPakAddressForGb(gbAddress), raw) == 1
    ? N64PakBlockResult::SUCCESS
    : N64PakBlockResult::WRITE_ERROR;
}

N64PakBlockResult selectGameBoyRomBankCached(TransferPakRangeAccess& access,
                                             uint8_t cartType,
                                             uint16_t bank) {
  if (access.romBankValid && access.romBank == bank) {
    return N64PakBlockResult::SUCCESS;
  }

  N64PakBlockResult result = N64PakBlockResult::SUCCESS;
  switch (cartType) {
    case 0x00:
    case 0x08:
    case 0x09:
      result = bank <= 1 ? N64PakBlockResult::SUCCESS : N64PakBlockResult::INVALID_BLOCK;
      break;
    case 0x01:
    case 0x02:
    case 0x03:
    {
      uint8_t low = (uint8_t)(bank & 0x1F);
      if (low == 0) low = 1;
      result = transferPakWriteGbRegisterCached(access, 0x2000, low);
      if (result != N64PakBlockResult::SUCCESS) break;
      result = transferPakWriteGbRegisterCached(access, 0x4000, (uint8_t)((bank >> 5) & 0x03));
      if (result != N64PakBlockResult::SUCCESS) break;
      result = transferPakWriteGbRegisterCached(access, 0x6000, 0x00);
      break;
    }
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
      result = transferPakWriteGbRegisterCached(access, 0x2000, (uint8_t)(bank & 0x7F));
      break;
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
      result = transferPakWriteGbRegisterCached(access, 0x2000, (uint8_t)(bank & 0xFF));
      if (result != N64PakBlockResult::SUCCESS) break;
      result = transferPakWriteGbRegisterCached(access, 0x3000, (uint8_t)((bank >> 8) & 0x01));
      break;
    default:
      result = N64PakBlockResult::NOT_PRESENT;
      break;
  }

  if (result == N64PakBlockResult::SUCCESS) {
    access.romBank = bank;
    access.romBankValid = true;
  }
  return result;
}

N64PakBlockResult selectGameBoyRamBankCached(TransferPakRangeAccess& access,
                                             uint8_t cartType,
                                             uint8_t bank) {
  N64PakBlockResult result = N64PakBlockResult::SUCCESS;
  if (!access.ramEnabled) {
    result = transferPakWriteGbRegisterCached(access, 0x0000, 0x0A);
    if (result != N64PakBlockResult::SUCCESS) return result;
    access.ramEnabled = true;
  }

  if (access.ramBankValid && access.ramBank == bank) {
    return N64PakBlockResult::SUCCESS;
  }

  switch (cartType) {
    case 0x00:
    case 0x08:
    case 0x09:
      result = bank == 0 ? N64PakBlockResult::SUCCESS : N64PakBlockResult::INVALID_BLOCK;
      break;
    case 0x01:
    case 0x02:
    case 0x03:
      if (!access.mbc1RamMode) {
        result = transferPakWriteGbRegisterCached(access, 0x6000, 0x01);
        if (result != N64PakBlockResult::SUCCESS) return result;
        access.mbc1RamMode = true;
      }
      result = transferPakWriteGbRegisterCached(access, 0x4000, bank & 0x03);
      break;
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
      result = transferPakWriteGbRegisterCached(access, 0x4000, bank);
      break;
    default:
      result = N64PakBlockResult::NOT_PRESENT;
      break;
  }

  if (result == N64PakBlockResult::SUCCESS) {
    access.ramBank = bank;
    access.ramBankValid = true;
  }
  return result;
}

N64PakBlockResult selectGameBoyRomBank(JoybusPort* joybus, uint8_t cartType, uint16_t bank) {
  switch (cartType) {
    case 0x00:
    case 0x08:
    case 0x09:
      return bank <= 1 ? N64PakBlockResult::SUCCESS : N64PakBlockResult::INVALID_BLOCK;
    case 0x01:
    case 0x02:
    case 0x03:
    {
      uint8_t low = (uint8_t)(bank & 0x1F);
      if (low == 0) low = 1;
      N64PakBlockResult result = transferPakWriteGbRegister(joybus, 0x2000, low);
      if (result != N64PakBlockResult::SUCCESS) return result;
      result = transferPakWriteGbRegister(joybus, 0x4000, (uint8_t)((bank >> 5) & 0x03));
      if (result != N64PakBlockResult::SUCCESS) return result;
      return transferPakWriteGbRegister(joybus, 0x6000, 0x00);
    }
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
      return transferPakWriteGbRegister(joybus, 0x2000, (uint8_t)(bank & 0x7F));
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    {
      N64PakBlockResult result = transferPakWriteGbRegister(joybus, 0x2000, (uint8_t)(bank & 0xFF));
      if (result != N64PakBlockResult::SUCCESS) return result;
      return transferPakWriteGbRegister(joybus, 0x3000, (uint8_t)((bank >> 8) & 0x01));
    }
    default:
      return N64PakBlockResult::NOT_PRESENT;
  }
}

N64PakBlockResult selectGameBoyRamBank(JoybusPort* joybus, uint8_t cartType, uint8_t bank) {
  N64PakBlockResult result = transferPakWriteGbRegister(joybus, 0x0000, 0x0A);
  if (result != N64PakBlockResult::SUCCESS) return result;

  switch (cartType) {
    case 0x00:
    case 0x08:
    case 0x09:
      return bank == 0 ? N64PakBlockResult::SUCCESS : N64PakBlockResult::INVALID_BLOCK;
    case 0x01:
    case 0x02:
    case 0x03:
      result = transferPakWriteGbRegister(joybus, 0x6000, 0x01);
      if (result != N64PakBlockResult::SUCCESS) return result;
      return transferPakWriteGbRegister(joybus, 0x4000, bank & 0x03);
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
      return transferPakWriteGbRegister(joybus, 0x4000, bank);
    default:
      return N64PakBlockResult::NOT_PRESENT;
  }
}

void copyGbTitle(const uint8_t* header, char* title, size_t titleSize) {
  if (title == nullptr || titleSize == 0) {
    return;
  }
  const uint8_t end = (header[0x43] == 0x80 || header[0x43] == 0xC0) ? 0x3E : 0x43;
  size_t out = 0;
  for (uint8_t i = 0x34; i <= end && out + 1 < titleSize; ++i) {
    const uint8_t c = header[i];
    if (c == 0) break;
    title[out++] = (c >= 0x20 && c <= 0x7E) ? (char)c : '_';
  }
  title[out] = '\0';
}

void copyGbTitleKey(const uint8_t* header, char* titleKey, size_t titleKeySize) {
  if (titleKey == nullptr || titleKeySize == 0) {
    return;
  }
  const uint8_t end = (header[0x43] == 0x80 || header[0x43] == 0xC0) ? 0x3E : 0x43;
  size_t out = 0;
  for (uint8_t i = 0x34; i <= end && out + 1 < titleKeySize; ++i) {
    const uint8_t c = header[i];
    if (c == 0) break;
    if (c == ' ' || c == '\t') {
      continue;
    }
    if (c >= 'a' && c <= 'z') {
      titleKey[out++] = (char)(c - 32);
    } else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      titleKey[out++] = (char)c;
    } else {
      titleKey[out++] = '_';
    }
  }
  titleKey[out] = '\0';
}

int writeN64PakRawBlockDiag(JoybusPort* joybus, uint16_t address, uint8_t* raw,
                            JoybusMemoryWriteDiag* diag) {
  JoybusMemoryWriteDiag latest{};
  for (uint8_t attempt = 0; attempt < kN64PakIoRetries; ++attempt) {
    const int result = joybus->writeN64MemoryDiag(address, raw, &latest);
    if (result == 1) {
      if (diag != nullptr) {
        *diag = latest;
      }
      return result;
    }
    delayMicroseconds(250);
  }
  if (diag != nullptr) {
    *diag = latest;
  }
  return latest.result;
}

void fillN64PakTestPattern(uint16_t page, uint8_t* buffer) {
  for (uint16_t i = 0; i < N64_PAK_PAGE_SIZE; ++i) {
    buffer[i] = (uint8_t)(0x52u ^ (uint8_t)page ^ (uint8_t)i ^ (uint8_t)(i >> 3));
  }
}

bool compareN64PakPage(const uint8_t* expected, const uint8_t* actual,
                       uint16_t* mismatchOffset, uint8_t* mismatchExpected,
                       uint8_t* mismatchActual) {
  for (uint16_t i = 0; i < N64_PAK_PAGE_SIZE; ++i) {
    if (expected[i] != actual[i]) {
      if (mismatchOffset != nullptr) *mismatchOffset = i;
      if (mismatchExpected != nullptr) *mismatchExpected = expected[i];
      if (mismatchActual != nullptr) *mismatchActual = actual[i];
      return false;
    }
  }
  if (mismatchOffset != nullptr) *mismatchOffset = 0xFFFF;
  if (mismatchExpected != nullptr) *mismatchExpected = 0;
  if (mismatchActual != nullptr) *mismatchActual = 0;
  return true;
}

}  // namespace

bool RZInputGC64::getN64ControllerPakInfo(uint8_t port, N64PakInfo* info) {
  if (info != nullptr) {
    *info = N64PakInfo{};
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr) {
    return false;
  }

  N64PakInfo local{};
  local.aux = joybus[port]->accessoryAux();
  local.rumblePak = joybus[port]->isRumblePakConnected();

  if (dtype[port] != JOYBUS_DEVICE_N64PAD || local.aux == 0 || local.rumblePak) {
    if (info != nullptr) {
      *info = local;
    }
    return false;
  }

  if (transferPakSignaturePresent(joybus[port])) {
    local.lastResult = N64_PAK_RAW_BLOCK_SIZE;
    if (info != nullptr) {
      *info = local;
    }
    return false;
  }

  uint8_t probe[N64_PAK_RAW_BLOCK_SIZE] = {};
  local.lastResult = readN64PakRawBlock(joybus[port], kN64PakBaseAddress, probe);
  local.present = (local.lastResult == N64_PAK_RAW_BLOCK_SIZE);
  if (local.present) {
    local.totalBlocks = N64_PAK_PAGE_COUNT;
    local.blockSize = N64_PAK_PAGE_SIZE;
  }
  if (info != nullptr) {
    *info = local;
  }
  return local.present;
}

bool RZInputGC64::getN64TransferPakInfo(uint8_t port, N64TransferPakInfo* info) {
  if (info != nullptr) {
    *info = N64TransferPakInfo{};
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr) {
    return false;
  }

  N64TransferPakInfo local{};
  local.aux = joybus[port]->accessoryAux();
  if (dtype[port] != JOYBUS_DEVICE_N64PAD || local.aux == 0 || joybus[port]->isRumblePakConnected()) {
    if (info != nullptr) *info = local;
    return false;
  }

  local.present = transferPakInit(joybus[port], &local);
  if (!local.present && transferPakSignaturePresent(joybus[port])) {
    local.present = true;
    transferPakEnableAndReadStatus(joybus[port], &local);
  }
  if (!local.present) {
    if (info != nullptr) *info = local;
    return false;
  }
  if (!local.cartPresent || !local.accessEnabled) {
    if (info != nullptr) *info = local;
    return true;
  }

  uint8_t header[kGbHeaderSize] = {};
  for (uint8_t attempt = 0; attempt < kGbHeaderReadAttempts; ++attempt) {
    memset(header, 0, sizeof(header));
    local.headerReadAttempts = (uint8_t)(attempt + 1);
    const N64PakBlockResult result = readTransferPakGbHeader(joybus[port], header);
    local.lastResult = (int)result;
    if (result == N64PakBlockResult::SUCCESS) {
      local.logoValid = gbHeaderLogoValid(header);
      local.headerChecksumValid = gbHeaderChecksumValid(header);
      local.headerValid = local.logoValid && local.headerChecksumValid;
      if (local.headerValid) {
        break;
      }
    }

    if (attempt + 1 < kGbHeaderReadAttempts) {
      delay(kGbHeaderRetryDelayMs);
      if (!transferPakEnableAndReadStatus(joybus[port], &local) || !local.cartPresent) {
        break;
      }
    }
  }

  local.cartType = header[0x47];
  local.romSizeCode = header[0x48];
  local.ramSizeCode = header[0x49];
  local.cgbFlag = header[0x43];
  local.sgbFlag = header[0x46];
  local.headerChecksum = header[0x4D];
  local.globalChecksum = ((uint16_t)header[0x4E] << 8) | header[0x4F];
  local.romBanks = local.headerValid ? gbRomBanksFromHeader(local.romSizeCode) : 0;
  local.ramBanks = local.headerValid ? gbRamBanksFromHeader(local.ramSizeCode) : 0;
  local.romSize = (uint32_t)local.romBanks * (uint32_t)kGbCartBankSize;
  local.saveSize = local.headerValid ? gbSaveSizeFromHeader(local.ramSizeCode, local.cartType) : 0;
  local.supportedMbc = local.headerValid && transferPakMbcSupported(local.cartType);
  strncpy(local.mbcName, transferPakMbcName(local.cartType), sizeof(local.mbcName) - 1);
  copyGbTitle(header, local.title, sizeof(local.title));
  copyGbTitleKey(header, local.titleKey, sizeof(local.titleKey));
  if (info != nullptr) *info = local;
  return local.present;
}

bool RZInputGC64::probeN64TransferPak(uint8_t port, N64TransferPakProbeInfo* info) {
  if (info != nullptr) {
    *info = N64TransferPakProbeInfo{};
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr) {
    return false;
  }

  N64TransferPakProbeInfo local{};
  local.validPort = true;
  local.deviceType = (uint8_t)dtype[port];
  local.n64Device = dtype[port] == JOYBUS_DEVICE_N64PAD;
  local.aux = joybus[port]->accessoryAux();
  local.accessoryPresent = (local.aux & 0x01) != 0;
  local.rumblePak = joybus[port]->isRumblePakConnected();
  if (!local.n64Device || !local.accessoryPresent || local.rumblePak) {
    if (info != nullptr) *info = local;
    return false;
  }

  local.powerWrite = writeTransferPakRegister(joybus[port], kTransferPakPowerAddress, kTransferPakPowerOn);
  delay(150);
  local.powerRead = readTransferPakRegister(joybus[port], kTransferPakPowerAddress, &local.powerValue);
  local.statusBeforeRead = readTransferPakRegister(joybus[port], kTransferPakStatusAddress, &local.statusBefore);
  local.accessWrite = writeTransferPakRegister(joybus[port], kTransferPakStatusAddress, kTransferPakAccessOn);
  delayMicroseconds(500);
  for (uint8_t i = 0; i < 3; ++i) {
    local.statusRead[i] = readTransferPakRegister(joybus[port], kTransferPakStatusAddress, &local.status[i]);
    delayMicroseconds(500);
  }

  local.bankWrite = writeTransferPakRegister(joybus[port], kTransferPakBankAddress, 0);
  delayMicroseconds(120);
  local.bankRead = readTransferPakRegister(joybus[port], kTransferPakBankAddress, &local.bankValue);
  uint8_t header[N64_PAK_RAW_BLOCK_SIZE] = {};
  local.headerRead = readN64PakRawBlock(joybus[port], transferPakAddressForGb(0x0100), header);
  if (local.headerRead == N64_PAK_RAW_BLOCK_SIZE) {
    memcpy(local.headerPreview, header, sizeof(local.headerPreview));
  }

  if (info != nullptr) *info = local;
  return true;
}

N64PakBlockResult RZInputGC64::readN64TransferPakRomBlock(uint8_t port, uint32_t block, uint8_t* buffer) {
  if (buffer == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  N64TransferPakInfo info{};
  if (!getN64TransferPakInfo(port, &info)) {
    return dtype[port] == JOYBUS_DEVICE_N64PAD ? N64PakBlockResult::NOT_PRESENT : N64PakBlockResult::NOT_N64;
  }
  return readN64TransferPakRomBlock(port, info, block, buffer);
}

N64PakBlockResult RZInputGC64::readN64TransferPakRomBlock(uint8_t port, const N64TransferPakInfo& info,
                                                          uint32_t block, uint8_t* buffer) {
  if (buffer == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr || dtype[port] != JOYBUS_DEVICE_N64PAD) {
    return N64PakBlockResult::NOT_N64;
  }
  if (!info.headerValid || !info.supportedMbc || info.romSize == 0 ||
      block >= (info.romSize / N64_TRANSFER_PAK_BLOCK_SIZE)) {
    return N64PakBlockResult::INVALID_BLOCK;
  }

  const uint32_t byteAddress = block * N64_TRANSFER_PAK_BLOCK_SIZE;
  const uint16_t romBank = (uint16_t)(byteAddress / kGbCartBankSize);
  const uint16_t bankOffset = (uint16_t)(byteAddress & (kGbCartBankSize - 1));
  if (romBank == 0) {
    return transferPakReadGbBlock(joybus[port], bankOffset, buffer);
  }

  const N64PakBlockResult select = selectGameBoyRomBank(joybus[port], info.cartType, romBank);
  if (select != N64PakBlockResult::SUCCESS) {
    return select;
  }
  return transferPakReadGbBlock(joybus[port], (uint16_t)(0x4000 + bankOffset), buffer);
}

N64PakBlockResult RZInputGC64::readN64TransferPakSaveBlock(uint8_t port, uint32_t block, uint8_t* buffer) {
  if (buffer == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  N64TransferPakInfo info{};
  if (!getN64TransferPakInfo(port, &info)) {
    return dtype[port] == JOYBUS_DEVICE_N64PAD ? N64PakBlockResult::NOT_PRESENT : N64PakBlockResult::NOT_N64;
  }
  return readN64TransferPakSaveBlock(port, info, block, buffer);
}

N64PakBlockResult RZInputGC64::readN64TransferPakSaveBlock(uint8_t port, const N64TransferPakInfo& info,
                                                           uint32_t block, uint8_t* buffer) {
  if (buffer == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr || dtype[port] != JOYBUS_DEVICE_N64PAD) {
    return N64PakBlockResult::NOT_N64;
  }
  if (!info.headerValid || !info.supportedMbc || info.saveSize == 0 ||
      block >= (info.saveSize / N64_TRANSFER_PAK_BLOCK_SIZE)) {
    return N64PakBlockResult::INVALID_BLOCK;
  }

  const uint32_t byteAddress = block * N64_TRANSFER_PAK_BLOCK_SIZE;
  const uint8_t ramBank = (uint8_t)(byteAddress / kGbExternalRamBankSize);
  const uint16_t ramOffset = (uint16_t)(byteAddress & (kGbExternalRamBankSize - 1));
  const N64PakBlockResult select = selectGameBoyRamBank(joybus[port], info.cartType, ramBank);
  if (select != N64PakBlockResult::SUCCESS) {
    return select;
  }
  return transferPakReadGbBlock(joybus[port], (uint16_t)(kGbExternalRamBase + ramOffset), buffer);
}

N64PakBlockResult RZInputGC64::readN64TransferPakBlocks(uint8_t port, const N64TransferPakInfo& info,
                                                        uint32_t startBlock, uint32_t count, bool save,
                                                        N64TransferPakBlockCallback callback, void* context,
                                                        uint32_t* failedBlock) {
  if (failedBlock != nullptr) {
    *failedBlock = startBlock;
  }
  if (callback == nullptr || count == 0) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr || dtype[port] != JOYBUS_DEVICE_N64PAD) {
    return N64PakBlockResult::NOT_N64;
  }

  const uint32_t byteSize = save ? info.saveSize : info.romSize;
  const uint32_t maxBlocks = byteSize / N64_TRANSFER_PAK_BLOCK_SIZE;
  if (!info.headerValid || !info.supportedMbc || byteSize == 0 ||
      startBlock >= maxBlocks || count > (maxBlocks - startBlock)) {
    return N64PakBlockResult::INVALID_BLOCK;
  }

  TransferPakRangeAccess access{};
  access.joybus = joybus[port];
  uint8_t blockData[N64_TRANSFER_PAK_BLOCK_SIZE] = {};
  uint16_t selectedRomBank = 0xFFFF;
  uint8_t selectedRamBank = 0xFF;

  for (uint32_t index = 0; index < count; ++index) {
    const uint32_t block = startBlock + index;
    if (failedBlock != nullptr) {
      *failedBlock = block;
    }

    const uint32_t byteAddress = block * N64_TRANSFER_PAK_BLOCK_SIZE;
    N64PakBlockResult result = N64PakBlockResult::SUCCESS;
    if (save) {
      const uint8_t ramBank = (uint8_t)(byteAddress / kGbExternalRamBankSize);
      const uint16_t ramOffset = (uint16_t)(byteAddress & (kGbExternalRamBankSize - 1));
      if (ramBank != selectedRamBank) {
        result = selectGameBoyRamBankCached(access, info.cartType, ramBank);
        if (result != N64PakBlockResult::SUCCESS) return result;
        selectedRamBank = ramBank;
      }
      result = transferPakReadGbBlockCached(access, (uint16_t)(kGbExternalRamBase + ramOffset), blockData);
    } else {
      const uint16_t romBank = (uint16_t)(byteAddress / kGbCartBankSize);
      const uint16_t bankOffset = (uint16_t)(byteAddress & (kGbCartBankSize - 1));
      if (romBank == 0) {
        result = transferPakReadGbBlockCached(access, bankOffset, blockData);
      } else {
        if (romBank != selectedRomBank) {
          result = selectGameBoyRomBankCached(access, info.cartType, romBank);
          if (result != N64PakBlockResult::SUCCESS) return result;
          selectedRomBank = romBank;
        }
        result = transferPakReadGbBlockCached(access, (uint16_t)(0x4000 + bankOffset), blockData);
      }
    }

    if (result != N64PakBlockResult::SUCCESS) {
      return result;
    }
    if (!callback(context, block, blockData)) {
      return N64PakBlockResult::READ_ERROR;
    }
  }

  return N64PakBlockResult::SUCCESS;
}

N64PakBlockResult RZInputGC64::writeN64TransferPakSaveBlock(uint8_t port, uint32_t block,
                                                            const uint8_t* buffer) {
  if (buffer == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  N64TransferPakInfo info{};
  if (!getN64TransferPakInfo(port, &info)) {
    return dtype[port] == JOYBUS_DEVICE_N64PAD ? N64PakBlockResult::NOT_PRESENT : N64PakBlockResult::NOT_N64;
  }
  return writeN64TransferPakSaveBlock(port, info, block, buffer);
}

N64PakBlockResult RZInputGC64::writeN64TransferPakSaveBlock(uint8_t port, const N64TransferPakInfo& info,
                                                            uint32_t block, const uint8_t* buffer) {
  return writeN64TransferPakBlocks(port, info, block, 1, buffer);
}

N64PakBlockResult RZInputGC64::writeN64TransferPakBlocks(uint8_t port, const N64TransferPakInfo& info,
                                                         uint32_t startBlock, uint32_t count,
                                                         const uint8_t* data,
                                                         uint32_t* failedBlock) {
  if (failedBlock != nullptr) {
    *failedBlock = startBlock;
  }
  if (data == nullptr || count == 0) {
    return N64PakBlockResult::INVALID_BLOCK;
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr || dtype[port] != JOYBUS_DEVICE_N64PAD) {
    return N64PakBlockResult::NOT_N64;
  }

  const uint32_t maxBlocks = info.saveSize / N64_TRANSFER_PAK_BLOCK_SIZE;
  if (!info.headerValid || !info.supportedMbc || info.saveSize == 0 ||
      startBlock >= maxBlocks || count > (maxBlocks - startBlock)) {
    return N64PakBlockResult::INVALID_BLOCK;
  }

  TransferPakRangeAccess access{};
  access.joybus = joybus[port];
  uint8_t selectedRamBank = 0xFF;

  for (uint32_t index = 0; index < count; ++index) {
    const uint32_t block = startBlock + index;
    if (failedBlock != nullptr) {
      *failedBlock = block;
    }

    const uint32_t byteAddress = block * N64_TRANSFER_PAK_BLOCK_SIZE;
    const uint8_t ramBank = (uint8_t)(byteAddress / kGbExternalRamBankSize);
    const uint16_t ramOffset = (uint16_t)(byteAddress & (kGbExternalRamBankSize - 1));
    N64PakBlockResult result = N64PakBlockResult::SUCCESS;
    if (ramBank != selectedRamBank) {
      result = selectGameBoyRamBankCached(access, info.cartType, ramBank);
      if (result != N64PakBlockResult::SUCCESS) return result;
      selectedRamBank = ramBank;
    }

    result = transferPakWriteGbBlockCached(
      access,
      (uint16_t)(kGbExternalRamBase + ramOffset),
      data + (index * N64_TRANSFER_PAK_BLOCK_SIZE));
    if (result != N64PakBlockResult::SUCCESS) {
      return result;
    }
    delayMicroseconds(80);
  }

  return N64PakBlockResult::SUCCESS;
}

N64PakBlockResult RZInputGC64::readN64ControllerPakPage(uint8_t port, uint16_t page, uint8_t* buffer) {
  if (page >= N64_PAK_PAGE_COUNT || buffer == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }

  N64PakInfo info{};
  if (!getN64ControllerPakInfo(port, &info)) {
    if (dtype[port] != JOYBUS_DEVICE_N64PAD) {
      return N64PakBlockResult::NOT_N64;
    }
    return N64PakBlockResult::NOT_PRESENT;
  }

  for (uint8_t rawBlock = 0; rawBlock < N64_PAK_PAGE_SIZE / N64_PAK_RAW_BLOCK_SIZE; ++rawBlock) {
    uint8_t raw[N64_PAK_RAW_BLOCK_SIZE] = {};
    const int result = readN64PakRawBlock(joybus[port], n64PakRawAddress(page, rawBlock), raw);
    if (result != N64_PAK_RAW_BLOCK_SIZE) {
      return N64PakBlockResult::READ_ERROR;
    }
    memcpy(buffer + (rawBlock * N64_PAK_RAW_BLOCK_SIZE), raw, N64_PAK_RAW_BLOCK_SIZE);
    delayMicroseconds(80);
  }

  return N64PakBlockResult::SUCCESS;
}

N64PakBlockResult RZInputGC64::writeN64ControllerPakPage(uint8_t port, uint16_t page, const uint8_t* buffer) {
  if (page >= N64_PAK_PAGE_COUNT || buffer == nullptr) {
    return N64PakBlockResult::INVALID_BLOCK;
  }

  N64PakInfo info{};
  if (!getN64ControllerPakInfo(port, &info)) {
    if (dtype[port] != JOYBUS_DEVICE_N64PAD) {
      return N64PakBlockResult::NOT_N64;
    }
    return N64PakBlockResult::NOT_PRESENT;
  }

  for (uint8_t rawBlock = 0; rawBlock < N64_PAK_PAGE_SIZE / N64_PAK_RAW_BLOCK_SIZE; ++rawBlock) {
    uint8_t raw[N64_PAK_RAW_BLOCK_SIZE] = {};
    memcpy(raw, buffer + (rawBlock * N64_PAK_RAW_BLOCK_SIZE), N64_PAK_RAW_BLOCK_SIZE);
    const int result = writeN64PakRawBlock(joybus[port], n64PakRawAddress(page, rawBlock), raw);
    if (result != 1) {
      return N64PakBlockResult::WRITE_ERROR;
    }
    delayMicroseconds(80);
  }

  return N64PakBlockResult::SUCCESS;
}

N64PakBlockResult RZInputGC64::testN64ControllerPakPage(uint8_t port, uint16_t page,
                                                        N64PakPageTestResult* result) {
  N64PakPageTestResult local{};
  local.page = page;
  if (result != nullptr) {
    *result = local;
  }

  if (page >= N64_PAK_PAGE_COUNT) {
    local.status = N64PakBlockResult::INVALID_BLOCK;
    if (result != nullptr) *result = local;
    return local.status;
  }
  if (!validN64PakPort(port) || joybus[port] == nullptr) {
    local.status = N64PakBlockResult::NOT_N64;
    if (result != nullptr) *result = local;
    return local.status;
  }

  uint8_t original[N64_PAK_PAGE_SIZE] = {};
  uint8_t test[N64_PAK_PAGE_SIZE] = {};
  uint8_t verify[N64_PAK_PAGE_SIZE] = {};
  uint8_t restored[N64_PAK_PAGE_SIZE] = {};

  local.readBefore = readN64ControllerPakPage(port, page, original);
  if (local.readBefore != N64PakBlockResult::SUCCESS) {
    local.status = local.readBefore;
    if (result != nullptr) *result = local;
    return local.status;
  }

  fillN64PakTestPattern(page, test);
  local.write = N64PakBlockResult::SUCCESS;
  for (uint8_t rawBlock = 0; rawBlock < N64_PAK_PAGE_SIZE / N64_PAK_RAW_BLOCK_SIZE; ++rawBlock) {
    uint8_t raw[N64_PAK_RAW_BLOCK_SIZE] = {};
    memcpy(raw, test + (rawBlock * N64_PAK_RAW_BLOCK_SIZE), N64_PAK_RAW_BLOCK_SIZE);
    JoybusMemoryWriteDiag diag{};
    const int writeResult = writeN64PakRawBlockDiag(
        joybus[port], n64PakRawAddress(page, rawBlock), raw, &diag);
    if (rawBlock == 0) {
      local.firstWriteDiag = diag;
    }
    if (writeResult != 1) {
      local.write = N64PakBlockResult::WRITE_ERROR;
      local.failingRawBlock = rawBlock;
      local.failingWriteDiag = diag;
      break;
    }
    delayMicroseconds(80);
  }

  local.readAfter = readN64ControllerPakPage(port, page, verify);
  if (local.readAfter == N64PakBlockResult::SUCCESS) {
    local.verifyMatch = compareN64PakPage(
        test, verify, &local.mismatchOffset, &local.mismatchExpected, &local.mismatchActual);
  }

  local.restore = writeN64ControllerPakPage(port, page, original);
  local.readRestore = readN64ControllerPakPage(port, page, restored);
  if (local.readRestore == N64PakBlockResult::SUCCESS) {
    local.restoreMatch = compareN64PakPage(
        original, restored,
        &local.restoreMismatchOffset,
        &local.restoreMismatchExpected,
        &local.restoreMismatchActual);
  }

  if (local.write != N64PakBlockResult::SUCCESS) {
    local.status = local.write;
  } else if (local.readAfter != N64PakBlockResult::SUCCESS) {
    local.status = local.readAfter;
  } else if (!local.verifyMatch) {
    local.status = N64PakBlockResult::VERIFY_ERROR;
  } else if (local.restore != N64PakBlockResult::SUCCESS ||
             local.readRestore != N64PakBlockResult::SUCCESS ||
             !local.restoreMatch) {
    local.status = N64PakBlockResult::RESTORE_ERROR;
  } else {
    local.status = N64PakBlockResult::SUCCESS;
  }

  if (result != nullptr) {
    *result = local;
  }
  return local.status;
}

#else

bool RZInputGC64::getN64ControllerPakInfo(uint8_t, N64PakInfo* info) {
  if (info != nullptr) {
    *info = N64PakInfo{};
  }
  return false;
}

N64PakBlockResult RZInputGC64::readN64ControllerPakPage(uint8_t, uint16_t, uint8_t*) {
  return N64PakBlockResult::NOT_N64;
}

N64PakBlockResult RZInputGC64::writeN64ControllerPakPage(uint8_t, uint16_t, const uint8_t*) {
  return N64PakBlockResult::NOT_N64;
}

N64PakBlockResult RZInputGC64::writeN64TransferPakSaveBlock(uint8_t, uint32_t, const uint8_t*) {
  return N64PakBlockResult::NOT_N64;
}

N64PakBlockResult RZInputGC64::writeN64TransferPakSaveBlock(uint8_t, const N64TransferPakInfo&,
                                                            uint32_t, const uint8_t*) {
  return N64PakBlockResult::NOT_N64;
}

N64PakBlockResult RZInputGC64::writeN64TransferPakBlocks(uint8_t, const N64TransferPakInfo&,
                                                         uint32_t, uint32_t,
                                                         const uint8_t*, uint32_t*) {
  return N64PakBlockResult::NOT_N64;
}

bool RZInputGC64::getN64TransferPakInfo(uint8_t, N64TransferPakInfo* info) {
  if (info != nullptr) {
    *info = N64TransferPakInfo{};
  }
  return false;
}

bool RZInputGC64::probeN64TransferPak(uint8_t, N64TransferPakProbeInfo* info) {
  if (info != nullptr) {
    *info = N64TransferPakProbeInfo{};
  }
  return false;
}

N64PakBlockResult RZInputGC64::readN64TransferPakRomBlock(uint8_t, uint32_t, uint8_t*) {
  return N64PakBlockResult::NOT_N64;
}

N64PakBlockResult RZInputGC64::readN64TransferPakSaveBlock(uint8_t, uint32_t, uint8_t*) {
  return N64PakBlockResult::NOT_N64;
}

N64PakBlockResult RZInputGC64::testN64ControllerPakPage(uint8_t, uint16_t,
                                                        N64PakPageTestResult* result) {
  if (result != nullptr) {
    *result = N64PakPageTestResult{};
    result->status = N64PakBlockResult::NOT_N64;
  }
  return N64PakBlockResult::NOT_N64;
}

#endif
