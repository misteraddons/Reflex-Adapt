#include "../product_config.h"

#include "serial_memcard_commands.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <Arduino.h>

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include <Adafruit_TinyUSB.h>
#endif

#include "device_runtime_state.h"
#include "../input/runtime/input_module_runtime.h"
#include "../input/mixed/Input_Mixed.h"
#include "../output/runtime/output_loop_runtime.h"

#ifdef ENABLE_INPUT_DREAMCAST
#include "../input/dreamcast/Input_Dreamcast.h"
#endif

#ifdef ENABLE_INPUT_N64
#include "../input/gc64/Input_GC64.h"
#endif

#ifdef ENABLE_INPUT_PSX
#include "../input/psx/Input_Psx.h"
#endif

namespace {

char toUpperAscii(char value) {
  return (value >= 'a' && value <= 'z') ? (char)(value - ('a' - 'A')) : value;
}

char* skipSpaces(char* text) {
  while (*text == ' ' || *text == '\t') {
    ++text;
  }
  return text;
}

bool tokenEquals(const char* token, const char* expected) {
  size_t index = 0;
  while (expected[index] != '\0') {
    if (token[index] == '\0' ||
        token[index] == ' ' ||
        token[index] == '\t' ||
        toUpperAscii(token[index]) != expected[index]) {
      return false;
    }
    ++index;
  }
  return token[index] == '\0' || token[index] == ' ' || token[index] == '\t';
}

bool commandStartsWith(const char* command, const char* expected, char** remainder) {
  size_t index = 0;
  while (expected[index] != '\0') {
    if (toUpperAscii(command[index]) != expected[index]) {
      return false;
    }
    ++index;
  }

  char* tail = const_cast<char*>(&command[index]);
  if (*tail == '\0') {
    *remainder = tail;
    return true;
  }
  if (*tail != ' ' && *tail != '\t') {
    return false;
  }
  *remainder = skipSpaces(tail);
  return true;
}

bool parseLongToken(char*& text, long* value) {
  text = skipSpaces(text);
  if (*text == '\0') {
    return false;
  }

  char* end = nullptr;
  const long parsed = strtol(text, &end, 0);
  if (end == text) {
    return false;
  }
  *value = parsed;
  text = skipSpaces(end);
  return true;
}

bool parseUint32Token(char*& text, uint32_t* value) {
  text = skipSpaces(text);
  if (*text == '\0') {
    return false;
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(text, &end, 0);
  if (end == text) {
    return false;
  }
  *value = (uint32_t)parsed;
  text = skipSpaces(end);
  return true;
}

#ifdef ADAPT_FEATURE_SERIAL_MEMCARD_API
void printHexByte(Print& out, uint8_t value) {
  static const char kHex[] = "0123456789ABCDEF";
  out.write(kHex[value >> 4]);
  out.write(kHex[value & 0x0F]);
}

void printHexWord(Print& out, uint16_t value) {
  printHexByte(out, (uint8_t)(value >> 8));
  printHexByte(out, (uint8_t)(value & 0xFF));
}

int8_t parseHexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

bool parseHexBytesToken(char*& text, uint8_t* out, uint16_t* outLen, uint16_t maxLen) {
  text = skipSpaces(text);
  uint16_t len = 0;
  while (*text != '\0' && *text != ' ' && *text != '\t') {
    if (len >= maxLen) {
      return false;
    }
    const int8_t high = parseHexNibble(*text++);
    if (*text == '\0' || *text == ' ' || *text == '\t') {
      return false;
    }
    const int8_t low = parseHexNibble(*text++);
    if (high < 0 || low < 0) {
      return false;
    }
    out[len++] = (uint8_t)((high << 4) | low);
  }
  *outLen = len;
  text = skipSpaces(text);
  return len > 0;
}

void serviceUsbDuringSerialPrint() {
#ifdef ADAPT_OUTPUT_USB_DEVICE
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  TinyUSB_Device_Task();
#else
  TinyUSBDevice.task();
#endif
#endif
}
#endif

#ifdef ENABLE_INPUT_DREAMCAST
constexpr uint16_t kDreamcastVmuBlockSize = 512;

const char* vmuBlockResultName(VMUBlockResult result) {
  switch (result) {
    case VMUBlockResult::SUCCESS: return "OK";
    case VMUBlockResult::NOT_PRESENT: return "NO_CARD";
    case VMUBlockResult::INVALID_BLOCK: return "BAD_BLOCK";
    case VMUBlockResult::TIMEOUT: return "TIMEOUT";
    case VMUBlockResult::READ_ERROR: return "READ_ERROR";
    case VMUBlockResult::WRITE_ERROR: return "WRITE_ERROR";
    default: return "UNKNOWN";
  }
}
#endif

#ifdef ADAPT_FEATURE_SERIAL_MEMCARD_API
constexpr uint16_t kCardWriteStageMax = 512;
constexpr uint16_t kCardWriteChunkMax = 96;

enum class CardBackend : uint8_t {
  None,
  DreamcastVmu,
  N64ControllerPak,
  N64TransferPakGbSave,
  PSXMemoryCard,
};

struct CardBackendTarget {
  CardBackend backend = CardBackend::None;
  DeviceEnum mode = RZORD_NONE;
  bool mixed = false;
#ifdef ENABLE_INPUT_DREAMCAST
  RZInputDreamcast* dreamcast = nullptr;
#endif
#ifdef ENABLE_INPUT_N64
  RZInputGC64* gc64 = nullptr;
#endif
#ifdef ENABLE_INPUT_PSX
  RZInputPSX* psx = nullptr;
#endif
};

class ScopedCardDeviceMode {
public:
  explicit ScopedCardDeviceMode(DeviceEnum mode) : previous(deviceMode), active(mode != RZORD_NONE) {
    if (active) {
      deviceMode = mode;
    }
  }

  ~ScopedCardDeviceMode() {
    if (active) {
      deviceMode = previous;
    }
  }

private:
  DeviceEnum previous;
  bool active;
};

struct CardWriteStage {
  bool active = false;
  CardBackend backend = CardBackend::None;
  uint8_t port = 0;
  uint8_t slot = 0;
  uint16_t block = 0;
  uint16_t blockSize = 0;
  uint8_t data[kCardWriteStageMax] = {};
  bool written[kCardWriteStageMax] = {};
};

CardWriteStage cardWriteStage;

struct CardWriteRangeStage {
  bool active = false;
  CardBackend backend = CardBackend::None;
  uint8_t port = 0;
  uint8_t slot = 0;
  uint16_t startBlock = 0;
  uint16_t blockCount = 0;
  uint16_t blockSize = 0;
  uint16_t currentBlockIndex = 0;
  uint16_t currentOffset = 0;
  uint32_t absoluteOffset = 0;
  uint8_t data[kCardWriteStageMax] = {};
#ifdef ENABLE_INPUT_N64
  bool transferPakInfoValid = false;
  N64TransferPakInfo transferPakInfo = {};
#endif
};

CardWriteRangeStage cardWriteRangeStage;

const char* cardBackendName(CardBackend backend) {
  switch (backend) {
    case CardBackend::DreamcastVmu: return "VMU";
    case CardBackend::N64ControllerPak: return "N64PAK";
    case CardBackend::N64TransferPakGbSave: return "GBPAK";
    case CardBackend::PSXMemoryCard: return "PSXMEM";
    default: return "NONE";
  }
}

#ifdef ENABLE_INPUT_N64
const char* n64PakBlockResultName(N64PakBlockResult result) {
  switch (result) {
    case N64PakBlockResult::SUCCESS: return "OK";
    case N64PakBlockResult::NOT_N64: return "NOT_N64";
    case N64PakBlockResult::NOT_PRESENT: return "NO_CARD";
    case N64PakBlockResult::INVALID_BLOCK: return "BAD_BLOCK";
    case N64PakBlockResult::READ_ERROR: return "READ_ERROR";
    case N64PakBlockResult::WRITE_ERROR: return "WRITE_ERROR";
    case N64PakBlockResult::VERIFY_ERROR: return "VERIFY_ERROR";
    case N64PakBlockResult::RESTORE_ERROR: return "RESTORE_ERROR";
    default: return "UNKNOWN";
  }
}
#endif

#ifdef ENABLE_INPUT_PSX
const char* psxMemoryCardBlockResultName(PSXMemoryCardBlockResult result) {
  switch (result) {
    case PSXMemoryCardBlockResult::SUCCESS: return "OK";
    case PSXMemoryCardBlockResult::NOT_PSX: return "NOT_PSX";
    case PSXMemoryCardBlockResult::NOT_PRESENT: return "NO_CARD";
    case PSXMemoryCardBlockResult::INVALID_BLOCK: return "BAD_BLOCK";
    case PSXMemoryCardBlockResult::ADDRESS_MISMATCH: return "BAD_ADDR";
    case PSXMemoryCardBlockResult::CHECKSUM_ERROR: return "BAD_CHECKSUM";
    case PSXMemoryCardBlockResult::READ_ERROR: return "READ_ERROR";
    case PSXMemoryCardBlockResult::WRITE_ERROR: return "WRITE_ERROR";
    default: return "UNKNOWN";
  }
}
#endif

bool cardWriteStageComplete() {
  if (!cardWriteStage.active || cardWriteStage.blockSize > kCardWriteStageMax) {
    return false;
  }
  for (uint16_t i = 0; i < cardWriteStage.blockSize; ++i) {
    if (!cardWriteStage.written[i]) {
      return false;
    }
  }
  return true;
}

uint16_t cardBlockSizeForBackend(CardBackend backend) {
  switch (backend) {
#ifdef ENABLE_INPUT_DREAMCAST
    case CardBackend::DreamcastVmu:
      return kDreamcastVmuBlockSize;
#endif
#ifdef ENABLE_INPUT_N64
    case CardBackend::N64ControllerPak:
      return N64_PAK_PAGE_SIZE;
    case CardBackend::N64TransferPakGbSave:
      return N64_TRANSFER_PAK_BLOCK_SIZE;
#endif
#ifdef ENABLE_INPUT_PSX
    case CardBackend::PSXMemoryCard:
      return PSX_MEMCARD_FRAME_SIZE;
#endif
    default:
      return 0;
  }
}

long cardMaxBlockForBackend(CardBackend backend) {
  switch (backend) {
#ifdef ENABLE_INPUT_DREAMCAST
    case CardBackend::DreamcastVmu:
      return 255;
#endif
#ifdef ENABLE_INPUT_N64
    case CardBackend::N64ControllerPak:
      return N64_PAK_PAGE_COUNT - 1;
    case CardBackend::N64TransferPakGbSave:
      return ((128L * 1024L) / N64_TRANSFER_PAK_BLOCK_SIZE) - 1;
#endif
#ifdef ENABLE_INPUT_PSX
    case CardBackend::PSXMemoryCard:
      return PSX_MEMCARD_FRAME_COUNT - 1;
#endif
    default:
      return -1;
  }
}

bool isPsxMemoryCardMode(DeviceEnum mode) {
#ifdef ENABLE_INPUT_PSX
  return mode == RZORD_PSX ||
         mode == RZORD_PSX_JOG ||
         mode == RZORD_PSX_DANCE;
#else
  (void)mode;
  return false;
#endif
}

#ifdef ENABLE_INPUT_PSX
RZInputPSX* psx_memory_card_only_input = nullptr;

bool psxMemoryCardOnlyProbeAllowed() {
  if (currentPsxInputModule() != nullptr) {
    return false;
  }
  return deviceMode == RZORD_AUTODETECT || isPsxMemoryCardMode(deviceMode);
}

RZInputPSX* psxMemoryCardOnlyInputModule() {
  if (!psxMemoryCardOnlyProbeAllowed()) {
    return nullptr;
  }
  if (psx_memory_card_only_input == nullptr) {
    psx_memory_card_only_input = new RZInputPSX();
  }
  if (!psx_memory_card_only_input->setupMemoryCardBridgeOnly()) {
    return nullptr;
  }
  return psx_memory_card_only_input;
}
#endif

CardBackendTarget cardBackendTargetForModule(DeviceEnum mode, RZInputModule* module, bool mixed) {
  CardBackendTarget target{};
  target.mode = mode;
  target.mixed = mixed;
  if (module == nullptr) {
    return target;
  }

#ifdef ENABLE_INPUT_DREAMCAST
  if (mode == RZORD_DREAMCAST) {
    target.backend = CardBackend::DreamcastVmu;
    target.dreamcast = static_cast<RZInputDreamcast*>(module);
    return target;
  }
#endif
#ifdef ENABLE_INPUT_N64
  if (mode == RZORD_N64) {
    target.backend = CardBackend::N64ControllerPak;
    target.gc64 = static_cast<RZInputGC64*>(module);
    return target;
  }
#endif
#ifdef ENABLE_INPUT_PSX
  if (isPsxMemoryCardMode(mode)) {
    target.backend = CardBackend::PSXMemoryCard;
    target.psx = static_cast<RZInputPSX*>(module);
    return target;
  }
#endif
  return target;
}

CardBackendTarget cardBackendTargetForPort(uint8_t port) {
  if (port > 1) {
    return CardBackendTarget{};
  }

  RZInputMixed* mixed = currentMixedInputModule();
  if (mixed != nullptr) {
    return cardBackendTargetForModule(
      mixed->modeForPhysicalPort(port),
      mixed->moduleForPhysicalPort(port),
      true);
  }

#ifdef ENABLE_INPUT_DREAMCAST
  if (currentDreamcastInputModule() != nullptr) {
    return cardBackendTargetForModule(RZORD_DREAMCAST, currentDreamcastInputModule(), false);
  }
#endif
#ifdef ENABLE_INPUT_N64
  RZInputGC64* gc64 = currentGc64InputModule();
  if (gc64 != nullptr && deviceMode == RZORD_N64) {
    return cardBackendTargetForModule(RZORD_N64, gc64, false);
  }
#endif
#ifdef ENABLE_INPUT_PSX
  if (currentPsxInputModule() != nullptr) {
    return cardBackendTargetForModule(deviceMode, currentPsxInputModule(), false);
  }
  RZInputPSX* psxMemoryOnly = psxMemoryCardOnlyInputModule();
  if (psxMemoryOnly != nullptr) {
    return cardBackendTargetForModule(RZORD_PSX, psxMemoryOnly, false);
  }
#endif
  return CardBackendTarget{};
}

CardBackend currentCardBackend() {
  for (uint8_t port = 0; port < 2; ++port) {
    const CardBackend backend = cardBackendTargetForPort(port).backend;
    if (backend != CardBackend::None) {
      return backend;
    }
  }
  return CardBackend::None;
}

bool anyCurrentCardBackend() {
  return currentCardBackend() != CardBackend::None;
}

bool anyCurrentCardBackendIs(CardBackend backend) {
  for (uint8_t port = 0; port < 2; ++port) {
    if (cardBackendTargetForPort(port).backend == backend) {
      return true;
    }
  }
  return false;
}

long cardMaxSlotForBackend(CardBackend backend) {
#ifdef ENABLE_INPUT_PSX
  if (backend == CardBackend::PSXMemoryCard) {
    return PSX_MEMCARD_SLOTS_PER_PORT - 1;
  }
#endif
#ifdef ENABLE_INPUT_N64
  if (backend == CardBackend::N64TransferPakGbSave) {
    return 0;
  }
#endif
  (void)backend;
  return 1;
}

void printCardHelp(Print& out) {
  out.println(F("CARD CMDS:CARD STATUS,CARD SCAN,CARD STATS,CARD READ <P> <S> <B>,CARD READBIN <P> <S> <B>,CARD READRANGE <P> <S> <START> <COUNT>,CARD WRITEBEGIN <P> <S> <B>,CARD WRITECHUNK <OFF> <HEX>,CARD WRITECOMMIT,CARD WRITEABORT,CARD WRITEBLOCKSBEGIN <P> <S> <START> <COUNT>,CARD WRITEBLOCKSCHUNK <OFF> <HEX>,CARD WRITEBLOCKSCOMMIT,CARD WRITEBLOCKSABORT,CARD N64TEST <P> [B],CARD GBINFO <P>,CARD GBPROBE <P>,CARD GBREADROM <P> <START> <COUNT>,CARD GBREADROMRAW <P> <START> <COUNT>,CARD GBREADSAVE <P> <START> <COUNT>,CARD GBREADSAVERAW <P> <START> <COUNT>,CARD GBWRITESAVEBEGIN <P> <START> <COUNT>,CARD GBWRITESAVECHUNK <OFF> <HEX>,CARD GBWRITESAVECOMMIT,CARD GBWRITESAVEABORT,CARD PSXRAW <P> <S> [B] [PAD] [CMD] [ADDR]"));
}

#ifdef ENABLE_INPUT_DREAMCAST
void printCardDreamcastSlot(Print& out,
                            RZInputDreamcast& dreamcast,
                            uint8_t port,
                            uint8_t slot,
                            bool refresh) {
  VMUInfo info{};
  const bool present = refresh ?
    dreamcast.refreshVmuInfo(port, slot, &info) :
    dreamcast.getVmuInfo(port, slot, &info);

  out.print(F("CARD P="));
  out.print((int)port);
  out.print(F(" S="));
  out.print((int)slot);
  out.print(F(" TYPE="));
  out.print(present ? F("VMU") : F("NONE"));
  out.print(F(" PRESENT="));
  out.print(present ? 1 : 0);
  out.print(F(" BLOCKS="));
  out.print((int)info.totalBlocks);
  out.print(F(" BLOCK_SIZE="));
  out.print((int)info.blockSize);
  out.print(F(" FUNC=0x"));
  out.println(info.func, HEX);
}
#endif

#ifdef ENABLE_INPUT_N64
void printCardN64Slot(Print& out, RZInputGC64& gc64, uint8_t port, bool refresh) {
  (void)refresh;
  N64PakInfo info{};
  const bool present = gc64.getN64ControllerPakInfo(port, &info);
  N64TransferPakInfo gbInfo{};
  const bool transferPakPresent = !present && gc64.getN64TransferPakInfo(port, &gbInfo);
  const bool transferPakSaveReadable =
    transferPakPresent && gbInfo.cartPresent && gbInfo.headerValid &&
    gbInfo.supportedMbc && gbInfo.saveSize > 0;
  const uint32_t transferPakSaveBlocks =
    transferPakSaveReadable ? (gbInfo.saveSize / N64_TRANSFER_PAK_BLOCK_SIZE) : 0;

  out.print(F("CARD P="));
  out.print((int)port);
  out.print(F(" S=0 TYPE="));
  if (present) {
    out.print(F("N64PAK"));
  } else if (transferPakPresent) {
    out.print(F("GBPAK"));
  } else if (info.rumblePak) {
    out.print(F("RUMBLE"));
  } else {
    out.print(F("NONE"));
  }
  out.print(F(" PRESENT="));
  out.print((present || transferPakSaveReadable) ? 1 : 0);
  out.print(F(" BLOCKS="));
  out.print(present ? (uint32_t)info.totalBlocks : transferPakSaveBlocks);
  out.print(F(" BLOCK_SIZE="));
  out.print(present ? (uint32_t)info.blockSize :
                       (transferPakSaveReadable ? (uint32_t)N64_TRANSFER_PAK_BLOCK_SIZE : 0));
  if (transferPakPresent) {
    out.print(F(" RO=1"));
  }
  out.print(F(" FUNC=0x0 AUX=0x"));
  out.print((int)info.aux, HEX);
  out.print(F(" LAST="));
  out.println(transferPakPresent ? gbInfo.lastResult : info.lastResult);
}

void printN64WriteDiag(Print& out, const char* prefix, const JoybusMemoryWriteDiag& diag) {
  out.print(prefix);
  out.print(F(" ADDR=0x"));
  out.print((int)diag.address, HEX);
  out.print(F(" CSADDR=0x"));
  out.print((int)diag.checksummed_address, HEX);
  out.print(F(" CMD=0x"));
  out.print((int)diag.command, HEX);
  out.print(F(" XFER="));
  out.print(diag.transport_result);
  out.print(F(" RES="));
  out.print(diag.result);
  out.print(F(" EXP=0x"));
  printHexByte(out, diag.expected_checksum);
  out.print(F(" ACK=0x"));
  printHexByte(out, diag.response_checksum);
  out.println();
}

bool handleN64CardTestCommand(char* text, Print& out) {
  long rawPort = -1;
  long rawBlock = N64_PAK_PAGE_COUNT - 1;
  if (!parseLongToken(text, &rawPort) ||
      rawPort < 0 ||
      rawPort > 1) {
    out.println(F("ERR:BAD_CARD_N64TEST"));
    return true;
  }
  if (*skipSpaces(text) != '\0' &&
      (!parseLongToken(text, &rawBlock) ||
       rawBlock < 0 ||
       rawBlock >= N64_PAK_PAGE_COUNT)) {
    out.println(F("ERR:BAD_CARD_N64TEST"));
    return true;
  }

  const CardBackendTarget target = cardBackendTargetForPort((uint8_t)rawPort);
  if (target.backend != CardBackend::N64ControllerPak || target.gc64 == nullptr) {
    out.println(F("ERR:CARD_N64TEST_NO_BACKEND"));
    return true;
  }

  N64PakPageTestResult result{};
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  target.gc64->testN64ControllerPakPage((uint8_t)rawPort, (uint16_t)rawBlock, &result);

  out.print(F("CARD N64TEST P="));
  out.print(rawPort);
  out.print(F(" S=0 B="));
  out.print(rawBlock);
  out.print(F(" STATUS="));
  out.print(n64PakBlockResultName(result.status));
  out.print(F(" READ="));
  out.print(n64PakBlockResultName(result.readBefore));
  out.print(F(" WRITE="));
  out.print(n64PakBlockResultName(result.write));
  out.print(F(" READ2="));
  out.print(n64PakBlockResultName(result.readAfter));
  out.print(F(" RESTORE="));
  out.print(n64PakBlockResultName(result.restore));
  out.print(F(" READR="));
  out.print(n64PakBlockResultName(result.readRestore));
  out.print(F(" VERIFY="));
  out.print(result.verifyMatch ? 1 : 0);
  out.print(F(" RESTORED="));
  out.println(result.restoreMatch ? 1 : 0);

  out.print(F("CARD N64TEST MISMATCH="));
  out.print(result.mismatchOffset);
  out.print(F(" EXP=0x"));
  printHexByte(out, result.mismatchExpected);
  out.print(F(" GOT=0x"));
  printHexByte(out, result.mismatchActual);
  out.print(F(" RESTORE_MISMATCH="));
  out.print(result.restoreMismatchOffset);
  out.print(F(" REXP=0x"));
  printHexByte(out, result.restoreMismatchExpected);
  out.print(F(" RGOT=0x"));
  printHexByte(out, result.restoreMismatchActual);
  out.print(F(" FAIL_RAW="));
  out.println((int)result.failingRawBlock);

  printN64WriteDiag(out, "CARD N64TEST FIRST_WRITE", result.firstWriteDiag);
  if (result.failingRawBlock != 0xFF) {
    printN64WriteDiag(out, "CARD N64TEST FAIL_WRITE", result.failingWriteDiag);
  }
  out.println(F("OK:CARD_N64TEST"));
  return true;
}

void printN64TransferPakInfo(Print& out, uint8_t port, const N64TransferPakInfo& info) {
  out.print(F("CARD GB P="));
  out.print((int)port);
  out.print(F(" PRESENT="));
  out.print(info.present ? 1 : 0);
  out.print(F(" CART="));
  out.print(info.cartPresent ? 1 : 0);
  out.print(F(" ACCESS="));
  out.print(info.accessEnabled ? 1 : 0);
  out.print(F(" HEADER_VALID="));
  out.print(info.headerValid ? 1 : 0);
  out.print(F(" LOGO_VALID="));
  out.print(info.logoValid ? 1 : 0);
  out.print(F(" CHECKSUM_VALID="));
  out.print(info.headerChecksumValid ? 1 : 0);
  out.print(F(" HEADER_READS="));
  out.print((int)info.headerReadAttempts);
  out.print(F(" STATUS=0x"));
  printHexByte(out, info.status);
  out.print(F(" CGB=0x"));
  printHexByte(out, info.cgbFlag);
  out.print(F(" SGB=0x"));
  printHexByte(out, info.sgbFlag);
  out.print(F(" TYPE=0x"));
  printHexByte(out, info.cartType);
  out.print(F(" ROM_CODE=0x"));
  printHexByte(out, info.romSizeCode);
  out.print(F(" RAM_CODE=0x"));
  printHexByte(out, info.ramSizeCode);
  out.print(F(" HEADER_CSUM=0x"));
  printHexByte(out, info.headerChecksum);
  out.print(F(" GLOBAL_CSUM=0x"));
  printHexWord(out, info.globalChecksum);
  out.print(F(" MBC="));
  out.print(info.mbcName);
  out.print(F(" SUPPORTED="));
  out.print(info.supportedMbc ? 1 : 0);
  out.print(F(" TITLE=\""));
  out.print(info.title);
  out.print(F("\" TITLE_KEY=\""));
  out.print(info.titleKey);
  out.print(F("\" ROM_SIZE="));
  out.print(info.romSize);
  out.print(F(" ROM_BANKS="));
  out.print((int)info.romBanks);
  out.print(F(" SAVE_SIZE="));
  out.print(info.saveSize);
  out.print(F(" RAM_BANKS="));
  out.print((int)info.ramBanks);
  out.print(F(" LAST="));
  out.println(info.lastResult);
}

void printN64TransferPakBinaryBlock(Print& out,
                                    const char* kind,
                                    uint8_t port,
                                    uint32_t block,
                                    const uint8_t* blockData) {
  out.print(F("CARD GB"));
  out.print(kind);
  out.print(F(" P="));
  out.print((int)port);
  out.print(F(" B="));
  out.print(block);
  out.print(F(" LEN="));
  out.println((int)N64_TRANSFER_PAK_BLOCK_SIZE);
  out.write(blockData, N64_TRANSFER_PAK_BLOCK_SIZE);
  serviceUsbDuringSerialPrint();
  out.println();
}

struct N64TransferPakSerialPrintContext {
  Print* out = nullptr;
  const char* kind = nullptr;
  uint8_t port = 0;
  bool save = false;
  bool rawRange = false;
};

bool printN64TransferPakSerialBlock(void* rawContext, uint32_t block, const uint8_t* blockData) {
  N64TransferPakSerialPrintContext* context =
    static_cast<N64TransferPakSerialPrintContext*>(rawContext);
  if (context == nullptr || context->out == nullptr || blockData == nullptr) {
    return false;
  }
  if (context->rawRange) {
    context->out->write(blockData, N64_TRANSFER_PAK_BLOCK_SIZE);
    serviceUsbDuringSerialPrint();
    return true;
  }
  printN64TransferPakBinaryBlock(*context->out, context->kind, context->port, block, blockData);
  context->out->println(context->save ? F("OK:CARD_GBSAVE") : F("OK:CARD_GBROM"));
  return true;
}

bool handleN64GbInfoCommand(char* text, Print& out) {
  long rawPort = -1;
  if (!parseLongToken(text, &rawPort) || rawPort < 0 || rawPort > 1) {
    out.println(F("ERR:BAD_CARD_GBINFO"));
    return true;
  }

  const CardBackendTarget target = cardBackendTargetForPort((uint8_t)rawPort);
  if (target.backend != CardBackend::N64ControllerPak || target.gc64 == nullptr) {
    out.println(F("ERR:CARD_GBINFO_NO_BACKEND"));
    return true;
  }

  N64TransferPakInfo info{};
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  target.gc64->getN64TransferPakInfo((uint8_t)rawPort, &info);
  printN64TransferPakInfo(out, (uint8_t)rawPort, info);
  out.println(F("OK:CARD_GBINFO"));
  return true;
}

void printN64TransferPakProbe(Print& out, uint8_t port, const N64TransferPakProbeInfo& info) {
  out.print(F("CARD GBPROBE P="));
  out.print((int)port);
  out.print(F(" VALID="));
  out.print(info.validPort ? 1 : 0);
  out.print(F(" N64="));
  out.print(info.n64Device ? 1 : 0);
  out.print(F(" DTYPE="));
  out.print((int)info.deviceType);
  out.print(F(" AUX=0x"));
  printHexByte(out, info.aux);
  out.print(F(" ACC="));
  out.print(info.accessoryPresent ? 1 : 0);
  out.print(F(" RUMBLE="));
  out.print(info.rumblePak ? 1 : 0);
  out.print(F(" PWRW="));
  out.print(info.powerWrite);
  out.print(F(" PWRR="));
  out.print(info.powerRead);
  out.print(F(" PWR=0x"));
  printHexByte(out, info.powerValue);
  out.print(F(" STAT0R="));
  out.print(info.statusBeforeRead);
  out.print(F(" STAT0=0x"));
  printHexByte(out, info.statusBefore);
  out.print(F(" ACCW="));
  out.print(info.accessWrite);
  for (uint8_t i = 0; i < 3; ++i) {
    out.print(F(" STAT"));
    out.print((int)(i + 1));
    out.print(F("R="));
    out.print(info.statusRead[i]);
    out.print(F(" STAT"));
    out.print((int)(i + 1));
    out.print(F("=0x"));
    printHexByte(out, info.status[i]);
  }
  out.print(F(" BANKW="));
  out.print(info.bankWrite);
  out.print(F(" BANKR="));
  out.print(info.bankRead);
  out.print(F(" BANK=0x"));
  printHexByte(out, info.bankValue);
  out.print(F(" HEADR="));
  out.print(info.headerRead);
  out.print(F(" HEAD="));
  for (uint8_t i = 0; i < sizeof(info.headerPreview); ++i) {
    printHexByte(out, info.headerPreview[i]);
  }
  out.println();
}

bool handleN64GbProbeCommand(char* text, Print& out) {
  long rawPort = -1;
  if (!parseLongToken(text, &rawPort) || rawPort < 0 || rawPort > 1) {
    out.println(F("ERR:BAD_CARD_GBPROBE"));
    return true;
  }

  const CardBackendTarget target = cardBackendTargetForPort((uint8_t)rawPort);
  if (target.backend != CardBackend::N64ControllerPak || target.gc64 == nullptr) {
    out.println(F("ERR:CARD_GBPROBE_NO_BACKEND"));
    return true;
  }

  N64TransferPakProbeInfo info{};
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  target.gc64->probeN64TransferPak((uint8_t)rawPort, &info);
  printN64TransferPakProbe(out, (uint8_t)rawPort, info);
  out.println(F("OK:CARD_GBPROBE"));
  return true;
}

bool handleN64GbReadCommand(char* text, Print& out, bool save, bool rawRange = false) {
  long rawPort = -1;
  uint32_t start = 0;
  uint32_t count = 0;
  if (!parseLongToken(text, &rawPort) ||
      !parseUint32Token(text, &start) ||
      !parseUint32Token(text, &count) ||
      rawPort < 0 || rawPort > 1 ||
      count == 0) {
    out.println(save ? F("ERR:BAD_CARD_GBREADSAVE") : F("ERR:BAD_CARD_GBREADROM"));
    return true;
  }

  const CardBackendTarget target = cardBackendTargetForPort((uint8_t)rawPort);
  if (target.backend != CardBackend::N64ControllerPak || target.gc64 == nullptr) {
    out.println(save ? F("ERR:CARD_GBREADSAVE_NO_BACKEND") : F("ERR:CARD_GBREADROM_NO_BACKEND"));
    return true;
  }

  N64TransferPakInfo info{};
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  if (!target.gc64->getN64TransferPakInfo((uint8_t)rawPort, &info)) {
    out.println(save ? F("ERR:CARD_GBREADSAVE NO_CART") : F("ERR:CARD_GBREADROM NO_CART"));
    return true;
  }
  const uint32_t byteSize = save ? info.saveSize : info.romSize;
  const uint32_t maxBlocks = byteSize / N64_TRANSFER_PAK_BLOCK_SIZE;
  if (!info.headerValid || !info.supportedMbc || byteSize == 0 ||
      start >= maxBlocks || count > (maxBlocks - start)) {
    out.println(save ? F("ERR:BAD_CARD_GBREADSAVE_RANGE") : F("ERR:BAD_CARD_GBREADROM_RANGE"));
    return true;
  }

  if (rawRange) {
    out.print(save ? F("CARD GBSAVERAW P=") : F("CARD GBROMRAW P="));
  } else {
    out.print(save ? F("CARD GBSAVE_RANGE P=") : F("CARD GBROM_RANGE P="));
  }
  out.print(rawPort);
  out.print(F(" START="));
  out.print(start);
  out.print(F(" COUNT="));
  out.print(count);
  out.print(F(" BLOCK_SIZE="));
  out.print((int)N64_TRANSFER_PAK_BLOCK_SIZE);
  out.print(F(" LEN="));
  out.println(count * (uint32_t)N64_TRANSFER_PAK_BLOCK_SIZE);

  N64TransferPakSerialPrintContext context{};
  context.out = &out;
  context.kind = save ? "SAVE" : "ROM";
  context.port = (uint8_t)rawPort;
  context.save = save;
  context.rawRange = rawRange;
  uint32_t failedBlock = start;
  const N64PakBlockResult result = target.gc64->readN64TransferPakBlocks(
    (uint8_t)rawPort,
    info,
    start,
    count,
    save,
    printN64TransferPakSerialBlock,
    &context,
    &failedBlock);
  if (result != N64PakBlockResult::SUCCESS) {
    out.print(save ? F("ERR:CARD_GBREADSAVE B=") : F("ERR:CARD_GBREADROM B="));
    out.print(failedBlock);
    out.print(' ');
    out.println(n64PakBlockResultName(result));
    return true;
  }
  if (rawRange) {
    serviceUsbDuringSerialPrint();
    out.println();
    out.println(save ? F("OK:CARD_GBREADSAVERAW") : F("OK:CARD_GBREADROMRAW"));
  } else {
    out.println(save ? F("OK:CARD_GBREADSAVE") : F("OK:CARD_GBREADROM"));
  }
  return true;
}

bool handleN64GbWriteSaveBeginCommand(char* text, Print& out) {
  long rawPort = -1;
  uint32_t start = 0;
  uint32_t count = 0;
  if (!parseLongToken(text, &rawPort) ||
      !parseUint32Token(text, &start) ||
      !parseUint32Token(text, &count) ||
      rawPort < 0 || rawPort > 1 ||
      count == 0 ||
      start > 0xFFFFu ||
      count > 0xFFFFu) {
    out.println(F("ERR:BAD_CARD_GBWRITESAVEBEGIN"));
    return true;
  }

  const CardBackendTarget target = cardBackendTargetForPort((uint8_t)rawPort);
  if (target.backend != CardBackend::N64ControllerPak || target.gc64 == nullptr) {
    out.println(F("ERR:CARD_GBWRITESAVEBEGIN_NO_BACKEND"));
    return true;
  }

  N64TransferPakInfo info{};
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  if (!target.gc64->getN64TransferPakInfo((uint8_t)rawPort, &info)) {
    out.println(F("ERR:CARD_GBWRITESAVEBEGIN NO_CART"));
    return true;
  }
  const uint32_t maxBlocks = info.saveSize / N64_TRANSFER_PAK_BLOCK_SIZE;
  if (!info.headerValid || !info.supportedMbc || info.saveSize == 0 ||
      start >= maxBlocks || count > (maxBlocks - start)) {
    out.println(F("ERR:BAD_CARD_GBWRITESAVEBEGIN_RANGE"));
    return true;
  }

  memset(&cardWriteStage, 0, sizeof(cardWriteStage));
  memset(&cardWriteRangeStage, 0, sizeof(cardWriteRangeStage));
  cardWriteRangeStage.active = true;
  cardWriteRangeStage.backend = CardBackend::N64TransferPakGbSave;
  cardWriteRangeStage.port = (uint8_t)rawPort;
  cardWriteRangeStage.slot = 0;
  cardWriteRangeStage.startBlock = (uint16_t)start;
  cardWriteRangeStage.blockCount = (uint16_t)count;
  cardWriteRangeStage.blockSize = N64_TRANSFER_PAK_BLOCK_SIZE;
  cardWriteRangeStage.transferPakInfoValid = true;
  cardWriteRangeStage.transferPakInfo = info;
  out.println(F("OK:CARD_GBWRITESAVEBEGIN"));
  return true;
}
#endif

#ifdef ENABLE_INPUT_PSX
void printCardPSXSlot(Print& out, RZInputPSX& psx, uint8_t port, uint8_t slot, bool refresh) {
  (void)refresh;
  PSXMemoryCardInfo info{};
  const bool present = psx.getPSXMemoryCardInfo(port, slot, &info);

  out.print(F("CARD P="));
  out.print((int)port);
  out.print(F(" S="));
  out.print((int)slot);
  out.print(F(" TYPE="));
  out.print(present ? F("PSXMEM") : F("NONE"));
  out.print(F(" PRESENT="));
  out.print(present ? 1 : 0);
  out.print(F(" BLOCKS="));
  out.print((int)(present ? info.totalBlocks : 0));
  out.print(F(" BLOCK_SIZE="));
  out.print((int)(present ? info.blockSize : 0));
  out.print(F(" FUNC=0x0 ADDR=0x"));
  out.print((int)info.address, HEX);
  out.print(F(" LAST="));
  out.println(info.lastResult);
}

bool handlePsxRawCardProbeCommand(char* text, Print& out) {
  long rawPort = -1;
  long rawSlot = -1;
  long rawBlock = 0;
  long rawPad = 0x00;
  long rawCommand = 0x52;
  long rawAddress = 0x00;
  if (!parseLongToken(text, &rawPort) ||
      !parseLongToken(text, &rawSlot) ||
      rawPort < 0 || rawPort > 1 ||
      rawSlot < 0 || rawSlot >= PSX_MEMCARD_SLOTS_PER_PORT) {
    out.println(F("ERR:BAD_CARD_PSXRAW"));
    return true;
  }
  if (*skipSpaces(text) != '\0' &&
      (!parseLongToken(text, &rawBlock) ||
       rawBlock < 0 ||
       rawBlock >= PSX_MEMCARD_FRAME_COUNT)) {
    out.println(F("ERR:BAD_CARD_PSXRAW"));
    return true;
  }
  if (*skipSpaces(text) != '\0' &&
      (!parseLongToken(text, &rawPad) ||
       rawPad < 0 ||
       rawPad > 0xFF)) {
    out.println(F("ERR:BAD_CARD_PSXRAW"));
    return true;
  }
  if (*skipSpaces(text) != '\0' &&
      (!parseLongToken(text, &rawCommand) ||
       rawCommand < 0 ||
       rawCommand > 0xFF)) {
    out.println(F("ERR:BAD_CARD_PSXRAW"));
    return true;
  }
  if (*skipSpaces(text) != '\0' &&
      (!parseLongToken(text, &rawAddress) ||
       rawAddress < 0 ||
       rawAddress > 0xFF)) {
    out.println(F("ERR:BAD_CARD_PSXRAW"));
    return true;
  }

  const CardBackendTarget target = cardBackendTargetForPort((uint8_t)rawPort);
  if (target.backend != CardBackend::PSXMemoryCard || target.psx == nullptr) {
    out.println(F("ERR:CARD_PSXRAW_NO_BACKEND"));
    return true;
  }

  PSXMemoryCardRawProbe probe{};
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  if (!target.psx->probePSXMemoryCardRaw((uint8_t)rawPort,
                                         (uint8_t)rawSlot,
                                         (uint16_t)rawBlock,
                                         &probe,
                                         (uint8_t)rawPad,
                                         (uint8_t)rawCommand,
                                         (uint8_t)rawAddress)) {
    out.println(F("ERR:CARD_PSXRAW_PROBE"));
    return true;
  }

  out.print(F("CARD PSXRAW P="));
  out.print(rawPort);
  out.print(F(" S="));
  out.print(rawSlot);
  out.print(F(" B="));
  out.print(rawBlock);
  out.print(F(" ADDR=0x"));
  printHexByte(out, probe.address);
  out.print(F(" CMD=0x"));
  printHexByte(out, probe.command);
  out.print(F(" PAD=0x"));
  printHexByte(out, probe.padByte);
  out.print(F(" OK="));
  out.print(probe.transferOk ? 1 : 0);
  out.print(F(" HEX="));
  for (uint8_t i = 0; i < probe.responseLen; ++i) {
    printHexByte(out, probe.response[i]);
  }
  out.println();
  out.println(F("OK:CARD_PSXRAW"));
  return true;
}
#endif

bool printCurrentCardSlots(Print& out, bool refresh) {
  bool printed = false;
  for (uint8_t port = 0; port < 2; ++port) {
    const CardBackendTarget target = cardBackendTargetForPort(port);
    ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
    switch (target.backend) {
#ifdef ENABLE_INPUT_DREAMCAST
    case CardBackend::DreamcastVmu:
    {
      if (target.dreamcast == nullptr) break;
      for (uint8_t slot = 0; slot < 2; ++slot) {
        printCardDreamcastSlot(out, *target.dreamcast, port, slot, refresh);
      }
      printed = true;
      break;
    }
#endif
#ifdef ENABLE_INPUT_N64
    case CardBackend::N64ControllerPak:
    {
      if (target.gc64 == nullptr) break;
      printCardN64Slot(out, *target.gc64, port, refresh);
      printed = true;
      break;
    }
#endif
#ifdef ENABLE_INPUT_PSX
    case CardBackend::PSXMemoryCard:
    {
      if (target.psx == nullptr) break;
      for (uint8_t slot = 0; slot < PSX_MEMCARD_SLOTS_PER_PORT; ++slot) {
        printCardPSXSlot(out, *target.psx, port, slot, refresh);
      }
      printed = true;
      break;
    }
#endif
    default:
      break;
    }
  }
  return printed;
}

bool cardReadBlock(CardBackend backend, uint8_t port, uint8_t slot, uint16_t block,
                   uint8_t* buffer, uint16_t* blockSize, const char** errorName) {
  if (errorName != nullptr) {
    *errorName = "UNKNOWN";
  }
  const CardBackendTarget target = cardBackendTargetForPort(port);
  if (target.backend != backend) {
    if (errorName != nullptr) *errorName = "NO_BACKEND";
    return false;
  }
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  switch (backend) {
#ifdef ENABLE_INPUT_DREAMCAST
    case CardBackend::DreamcastVmu:
    {
      if (block > 255) {
        if (errorName != nullptr) *errorName = "BAD_BLOCK";
        return false;
      }
      if (target.dreamcast == nullptr) {
        if (errorName != nullptr) *errorName = "NO_BACKEND";
        return false;
      }
      const VMUBlockResult result = target.dreamcast->readVmuBlock(port, slot, block, buffer);
      if (result != VMUBlockResult::SUCCESS) {
        if (errorName != nullptr) *errorName = vmuBlockResultName(result);
        return false;
      }
      if (blockSize != nullptr) *blockSize = kDreamcastVmuBlockSize;
      return true;
    }
#endif
#ifdef ENABLE_INPUT_N64
    case CardBackend::N64ControllerPak:
    {
      if (slot != 0 || block >= N64_PAK_PAGE_COUNT) {
        if (errorName != nullptr) *errorName = "BAD_BLOCK";
        return false;
      }
      if (target.gc64 == nullptr) {
        if (errorName != nullptr) *errorName = "NO_BACKEND";
        return false;
      }
      const N64PakBlockResult result = target.gc64->readN64ControllerPakPage(port, block, buffer);
      if (result != N64PakBlockResult::SUCCESS) {
        if (errorName != nullptr) *errorName = n64PakBlockResultName(result);
        return false;
      }
      if (blockSize != nullptr) *blockSize = N64_PAK_PAGE_SIZE;
      return true;
    }
#endif
#ifdef ENABLE_INPUT_PSX
    case CardBackend::PSXMemoryCard:
    {
      if (slot >= PSX_MEMCARD_SLOTS_PER_PORT || block >= PSX_MEMCARD_FRAME_COUNT) {
        if (errorName != nullptr) *errorName = "BAD_BLOCK";
        return false;
      }
      if (target.psx == nullptr) {
        if (errorName != nullptr) *errorName = "NO_BACKEND";
        return false;
      }
      const PSXMemoryCardBlockResult result = target.psx->readPSXMemoryCardFrame(port, slot, block, buffer);
      if (result != PSXMemoryCardBlockResult::SUCCESS) {
        if (errorName != nullptr) *errorName = psxMemoryCardBlockResultName(result);
        return false;
      }
      if (blockSize != nullptr) *blockSize = PSX_MEMCARD_FRAME_SIZE;
      return true;
    }
#endif
    default:
      if (errorName != nullptr) *errorName = "NO_BACKEND";
      return false;
  }
}

bool cardWriteBlock(CardBackend backend, uint8_t port, uint8_t slot, uint16_t block,
                    const uint8_t* buffer, const char** errorName) {
  if (errorName != nullptr) {
    *errorName = "UNKNOWN";
  }
  const CardBackendTarget target = cardBackendTargetForPort(port);
  if (target.backend != backend) {
    if (errorName != nullptr) *errorName = "NO_BACKEND";
    return false;
  }
  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  switch (backend) {
#ifdef ENABLE_INPUT_DREAMCAST
    case CardBackend::DreamcastVmu:
    {
      if (target.dreamcast == nullptr) {
        if (errorName != nullptr) *errorName = "NO_BACKEND";
        return false;
      }
      const VMUBlockResult result = target.dreamcast->writeVmuBlock(port, slot, block, buffer);
      if (result != VMUBlockResult::SUCCESS) {
        if (errorName != nullptr) *errorName = vmuBlockResultName(result);
        return false;
      }
      return true;
    }
#endif
#ifdef ENABLE_INPUT_N64
    case CardBackend::N64ControllerPak:
    {
      if (target.gc64 == nullptr) {
        if (errorName != nullptr) *errorName = "NO_BACKEND";
        return false;
      }
      const N64PakBlockResult result = target.gc64->writeN64ControllerPakPage(port, block, buffer);
      if (result != N64PakBlockResult::SUCCESS) {
        if (errorName != nullptr) *errorName = n64PakBlockResultName(result);
        return false;
      }
      return true;
    }
#endif
#ifdef ENABLE_INPUT_PSX
    case CardBackend::PSXMemoryCard:
    {
      if (slot >= PSX_MEMCARD_SLOTS_PER_PORT) {
        if (errorName != nullptr) *errorName = "BAD_BLOCK";
        return false;
      }
      if (target.psx == nullptr) {
        if (errorName != nullptr) *errorName = "NO_BACKEND";
        return false;
      }
      const PSXMemoryCardBlockResult result = target.psx->writePSXMemoryCardFrame(port, slot, block, buffer);
      if (result != PSXMemoryCardBlockResult::SUCCESS) {
        if (errorName != nullptr) *errorName = psxMemoryCardBlockResultName(result);
        return false;
      }
      return true;
    }
#endif
    default:
      if (errorName != nullptr) *errorName = "NO_BACKEND";
      return false;
  }
}

void printCardBinaryBlock(Print& out,
                          uint8_t port,
                          uint8_t slot,
                          uint16_t block,
                          const uint8_t* blockData,
                          uint16_t blockSize) {
  out.print(F("CARD BIN P="));
  out.print((int)port);
  out.print(F(" S="));
  out.print((int)slot);
  out.print(F(" B="));
  out.print((int)block);
  out.print(F(" LEN="));
  out.println(blockSize);
  for (uint16_t i = 0; i < blockSize; i += 32) {
    const uint16_t chunk = (blockSize - i) < 32 ? (blockSize - i) : 32;
    out.write(&blockData[i], chunk);
    serviceUsbDuringSerialPrint();
  }
  serviceUsbDuringSerialPrint();
  out.println();
}

#ifdef ENABLE_INPUT_N64
bool writeN64TransferPakGbSaveStageBlock(uint16_t block, const uint8_t* blockData,
                                         const char** errorName) {
  if (errorName != nullptr) {
    *errorName = "UNKNOWN";
  }
  if (!cardWriteRangeStage.transferPakInfoValid || blockData == nullptr) {
    if (errorName != nullptr) *errorName = "BAD_STAGE";
    return false;
  }

  const CardBackendTarget target = cardBackendTargetForPort(cardWriteRangeStage.port);
  if (target.backend != CardBackend::N64ControllerPak || target.gc64 == nullptr) {
    if (errorName != nullptr) *errorName = "NO_BACKEND";
    return false;
  }

  ScopedCardDeviceMode scopedMode(target.mixed ? target.mode : RZORD_NONE);
  uint32_t failedBlock = block;
  const N64PakBlockResult result = target.gc64->writeN64TransferPakBlocks(
    cardWriteRangeStage.port,
    cardWriteRangeStage.transferPakInfo,
    block,
    1,
    blockData,
    &failedBlock);
  if (result != N64PakBlockResult::SUCCESS) {
    if (errorName != nullptr) *errorName = n64PakBlockResultName(result);
    return false;
  }
  return true;
}
#endif

bool flushCardWriteRangeBlock(Print& out) {
  if (!cardWriteRangeStage.active ||
      cardWriteRangeStage.currentBlockIndex >= cardWriteRangeStage.blockCount ||
      cardWriteRangeStage.currentOffset != cardWriteRangeStage.blockSize) {
    out.println(F("ERR:CARD_WRITEBLOCKS_INCOMPLETE_BLOCK"));
    cardWriteRangeStage.active = false;
    return false;
  }

  const uint16_t block = cardWriteRangeStage.startBlock + cardWriteRangeStage.currentBlockIndex;
  const char* errorName = nullptr;
  bool ok = false;
#ifdef ENABLE_INPUT_N64
  if (cardWriteRangeStage.backend == CardBackend::N64TransferPakGbSave) {
    ok = writeN64TransferPakGbSaveStageBlock(block, cardWriteRangeStage.data, &errorName);
  } else
#endif
  {
    ok = cardWriteBlock(cardWriteRangeStage.backend,
                        cardWriteRangeStage.port,
                        cardWriteRangeStage.slot,
                        block,
                        cardWriteRangeStage.data,
                        &errorName);
  }
  if (!ok) {
    out.print(cardWriteRangeStage.backend == CardBackend::N64TransferPakGbSave
      ? F("ERR:CARD_GBWRITESAVE B=")
      : F("ERR:CARD_WRITEBLOCKS B="));
    out.print((int)block);
    out.print(' ');
    out.println(errorName);
    cardWriteRangeStage.active = false;
    return false;
  }

  out.print(cardWriteRangeStage.backend == CardBackend::N64TransferPakGbSave
    ? F("CARD GBWRITE B=")
    : F("CARD WRITEBLOCK B="));
  out.print((int)block);
  out.println(F(" OK"));
  memset(cardWriteRangeStage.data, 0, sizeof(cardWriteRangeStage.data));
  cardWriteRangeStage.currentBlockIndex++;
  cardWriteRangeStage.currentOffset = 0;
  serviceUsbDuringSerialPrint();
  return true;
}

bool handleSerialMemcardCommandImpl(char* text, Print& out) {
  if (*text == '\0' || tokenEquals(text, "HELP")) {
    printCardHelp(out);
    return true;
  }

  if (tokenEquals(text, "STATUS") || tokenEquals(text, "SCAN")) {
    const bool refresh = tokenEquals(text, "SCAN");
    if (!printCurrentCardSlots(out, refresh)) {
      out.println(F("ERR:CARD_NO_BACKEND"));
      return true;
    }
    out.print(F("OK:CARD_"));
    out.println(refresh ? F("SCAN") : F("STATUS"));
    return true;
  }

  if (tokenEquals(text, "STATS")) {
    if (!anyCurrentCardBackend()) {
      out.println(F("ERR:CARD_NO_BACKEND"));
      return true;
    }
    const CardBackend backend = currentCardBackend();
    out.print(F("CARD STATS BACKEND="));
    out.print(cardBackendName(backend));
#ifdef ENABLE_INPUT_DREAMCAST
    if (anyCurrentCardBackendIs(CardBackend::DreamcastVmu)) {
      const DreamcastVmuSerialStats& vmuStats = dreamcastVmuSerialStats();
      const UsbDeviceRuntimeDiagnostics& usbStats = usbDeviceRuntimeDiagnostics();
      out.print(F(" READS="));
      out.print(vmuStats.read_count);
      out.print(F(" OK="));
      out.print(vmuStats.success_count);
      out.print(F(" TO="));
      out.print(vmuStats.timeout_count);
      out.print(F(" TO_ATT="));
      out.print(vmuStats.timeout_attempt_count);
      out.print(F(" RETRY="));
      out.print(vmuStats.retry_count);
      out.print(F(" REC="));
      out.print(vmuStats.recovery_count);
      out.print('/');
      out.print(vmuStats.recovery_success_count);
      out.print(F(" WR="));
      out.print(vmuStats.write_count);
      out.print('/');
      out.print(vmuStats.write_success_count);
      out.print(F(" LAST="));
      out.print((int)vmuStats.last_port);
      out.print('/');
      out.print((int)vmuStats.last_slot);
      out.print('/');
      out.print((int)vmuStats.last_block);
      out.print(F(" RES="));
      out.print(vmuBlockResultName(vmuStats.last_result));
      out.print(F(" US="));
      out.print(vmuStats.last_read_us);
      out.print('/');
      out.print(vmuStats.max_read_us);
      out.print(F(" USB_GAP_US="));
      out.print(usbStats.last_task_gap_us);
      out.print('/');
      out.print(usbStats.max_task_gap_us);
      out.print(F(" USB_MOUNT="));
      out.print(usbStats.mount_count);
      out.print('/');
      out.print(usbStats.umount_count);
      out.print(F(" USB_SUSP="));
      out.print(usbStats.suspend_count);
      out.print('/');
      out.print(usbStats.resume_count);
    }
#endif
    out.println();
    out.println(F("OK:CARD_STATS"));
    return true;
  }

  char* remainder = nullptr;
#ifdef ENABLE_INPUT_N64
  if (commandStartsWith(text, "N64TEST", &remainder)) {
    return handleN64CardTestCommand(remainder, out);
  }
  if (commandStartsWith(text, "GBINFO", &remainder)) {
    return handleN64GbInfoCommand(remainder, out);
  }
  if (commandStartsWith(text, "GBPROBE", &remainder)) {
    return handleN64GbProbeCommand(remainder, out);
  }
  if (commandStartsWith(text, "GBREADROMRAW", &remainder)) {
    return handleN64GbReadCommand(remainder, out, false, true);
  }
  if (commandStartsWith(text, "GBREADSAVERAW", &remainder)) {
    return handleN64GbReadCommand(remainder, out, true, true);
  }
  if (commandStartsWith(text, "GBREADROM", &remainder)) {
    return handleN64GbReadCommand(remainder, out, false);
  }
  if (commandStartsWith(text, "GBREADSAVE", &remainder)) {
    return handleN64GbReadCommand(remainder, out, true);
  }
  if (commandStartsWith(text, "GBWRITESAVEBEGIN", &remainder) ||
      commandStartsWith(text, "GBWRITESAVE", &remainder)) {
    return handleN64GbWriteSaveBeginCommand(remainder, out);
  }
#endif
#ifdef ENABLE_INPUT_PSX
  if (commandStartsWith(text, "PSXRAW", &remainder)) {
    return handlePsxRawCardProbeCommand(remainder, out);
  }
#endif

  if (commandStartsWith(text, "READRANGE", &remainder)) {
    long rawPort = -1;
    long rawSlot = -1;
    long rawStart = -1;
    long rawCount = -1;
    if (!parseLongToken(remainder, &rawPort) ||
        !parseLongToken(remainder, &rawSlot) ||
        !parseLongToken(remainder, &rawStart) ||
        !parseLongToken(remainder, &rawCount) ||
        rawPort < 0 || rawPort > 1 ||
        rawSlot < 0 ||
        rawStart < 0 ||
        rawCount <= 0) {
      out.println(F("ERR:BAD_CARD_READRANGE"));
      return true;
    }

    const CardBackend backend = cardBackendTargetForPort((uint8_t)rawPort).backend;
    if (backend == CardBackend::None) {
      out.println(F("ERR:CARD_READRANGE NO_BACKEND"));
      return true;
    }

    const long maxSlot = cardMaxSlotForBackend(backend);
    const long maxBlock = cardMaxBlockForBackend(backend);
    const uint16_t blockSize = cardBlockSizeForBackend(backend);
    if (rawSlot > maxSlot ||
        maxBlock < 0 ||
        rawStart > maxBlock ||
        rawCount > (maxBlock - rawStart + 1) ||
        blockSize == 0 ||
        blockSize > kCardWriteStageMax ||
        (backend == CardBackend::N64ControllerPak && rawSlot != 0)) {
      out.println(F("ERR:BAD_CARD_READRANGE"));
      return true;
    }

    out.print(F("CARD RANGE P="));
    out.print(rawPort);
    out.print(F(" S="));
    out.print(rawSlot);
    out.print(F(" START="));
    out.print(rawStart);
    out.print(F(" COUNT="));
    out.print(rawCount);
    out.print(F(" BLOCK_SIZE="));
    out.print(blockSize);
    out.print(F(" LEN="));
    out.println((uint32_t)rawCount * (uint32_t)blockSize);

    static uint8_t blockData[kCardWriteStageMax];
    for (long index = 0; index < rawCount; ++index) {
      const uint16_t block = (uint16_t)(rawStart + index);
      uint16_t readSize = 0;
      const char* errorName = nullptr;
      if (!cardReadBlock(backend,
                         (uint8_t)rawPort,
                         (uint8_t)rawSlot,
                         block,
                         blockData,
                         &readSize,
                         &errorName)) {
        out.print(F("ERR:CARD_READRANGE B="));
        out.print((int)block);
        out.print(' ');
        out.println(errorName);
        return true;
      }
      printCardBinaryBlock(out, (uint8_t)rawPort, (uint8_t)rawSlot, block, blockData, readSize);
      out.println(F("OK:CARD_BIN"));
    }
    out.println(F("OK:CARD_READRANGE"));
    return true;
  }

  if (commandStartsWith(text, "WRITEBLOCKSBEGIN", &remainder) ||
      commandStartsWith(text, "WRITEBLOCKS", &remainder)) {
    long rawPort = -1;
    long rawSlot = -1;
    long rawStart = -1;
    long rawCount = -1;
    if (!parseLongToken(remainder, &rawPort) ||
        !parseLongToken(remainder, &rawSlot) ||
        !parseLongToken(remainder, &rawStart) ||
        !parseLongToken(remainder, &rawCount) ||
        rawPort < 0 || rawPort > 1 ||
        rawSlot < 0 ||
        rawStart < 0 ||
        rawCount <= 0) {
      out.println(F("ERR:BAD_CARD_WRITEBLOCKSBEGIN"));
      return true;
    }

    const CardBackend backend = cardBackendTargetForPort((uint8_t)rawPort).backend;
    if (backend == CardBackend::None) {
      out.println(F("ERR:CARD_WRITEBLOCKSBEGIN NO_BACKEND"));
      return true;
    }

    const long maxSlot = cardMaxSlotForBackend(backend);
    const long maxBlock = cardMaxBlockForBackend(backend);
    const uint16_t blockSize = cardBlockSizeForBackend(backend);
    if (rawSlot > maxSlot ||
        maxBlock < 0 ||
        rawStart > maxBlock ||
        rawCount > (maxBlock - rawStart + 1) ||
        blockSize == 0 ||
        blockSize > kCardWriteStageMax ||
        (backend == CardBackend::N64ControllerPak && rawSlot != 0)) {
      out.println(F("ERR:BAD_CARD_WRITEBLOCKSBEGIN"));
      return true;
    }

    memset(&cardWriteStage, 0, sizeof(cardWriteStage));
    memset(&cardWriteRangeStage, 0, sizeof(cardWriteRangeStage));
    cardWriteRangeStage.active = true;
    cardWriteRangeStage.backend = backend;
    cardWriteRangeStage.port = (uint8_t)rawPort;
    cardWriteRangeStage.slot = (uint8_t)rawSlot;
    cardWriteRangeStage.startBlock = (uint16_t)rawStart;
    cardWriteRangeStage.blockCount = (uint16_t)rawCount;
    cardWriteRangeStage.blockSize = blockSize;
    out.println(F("OK:CARD_WRITEBLOCKSBEGIN"));
    return true;
  }

  const bool gbWriteSaveChunk = commandStartsWith(text, "GBWRITESAVECHUNK", &remainder);
  if (gbWriteSaveChunk || commandStartsWith(text, "WRITEBLOCKSCHUNK", &remainder)) {
    if (!cardWriteRangeStage.active) {
      out.println(gbWriteSaveChunk ? F("ERR:CARD_GBWRITESAVE_NOT_ACTIVE") : F("ERR:CARD_WRITEBLOCKS_NOT_ACTIVE"));
      return true;
    }
    if (gbWriteSaveChunk != (cardWriteRangeStage.backend == CardBackend::N64TransferPakGbSave)) {
      out.println(gbWriteSaveChunk ? F("ERR:CARD_GBWRITESAVE_NOT_ACTIVE") : F("ERR:CARD_WRITEBLOCKS_NOT_ACTIVE"));
      return true;
    }

    long rawOffset = -1;
    if (!parseLongToken(remainder, &rawOffset) || rawOffset < 0) {
      out.println(gbWriteSaveChunk ? F("ERR:BAD_CARD_GBWRITESAVECHUNK") : F("ERR:BAD_CARD_WRITEBLOCKSCHUNK"));
      return true;
    }

    uint8_t chunk[kCardWriteChunkMax];
    uint16_t chunkLen = 0;
    const uint32_t totalBytes =
      (uint32_t)cardWriteRangeStage.blockCount * (uint32_t)cardWriteRangeStage.blockSize;
    if (!parseHexBytesToken(remainder, chunk, &chunkLen, kCardWriteChunkMax) ||
        (uint32_t)rawOffset != cardWriteRangeStage.absoluteOffset ||
        (uint32_t)rawOffset + chunkLen > totalBytes) {
      out.println(gbWriteSaveChunk ? F("ERR:BAD_CARD_GBWRITESAVECHUNK") : F("ERR:BAD_CARD_WRITEBLOCKSCHUNK"));
      cardWriteRangeStage.active = false;
      return true;
    }

    for (uint16_t i = 0; i < chunkLen; ++i) {
      if (cardWriteRangeStage.currentBlockIndex >= cardWriteRangeStage.blockCount ||
          cardWriteRangeStage.currentOffset >= cardWriteRangeStage.blockSize) {
        out.println(gbWriteSaveChunk ? F("ERR:BAD_CARD_GBWRITESAVECHUNK") : F("ERR:BAD_CARD_WRITEBLOCKSCHUNK"));
        cardWriteRangeStage.active = false;
        return true;
      }
      cardWriteRangeStage.data[cardWriteRangeStage.currentOffset++] = chunk[i];
      cardWriteRangeStage.absoluteOffset++;
      if (cardWriteRangeStage.currentOffset == cardWriteRangeStage.blockSize &&
          !flushCardWriteRangeBlock(out)) {
        return true;
      }
    }

    out.println(gbWriteSaveChunk ? F("OK:CARD_GBWRITESAVECHUNK") : F("OK:CARD_WRITEBLOCKSCHUNK"));
    return true;
  }

  const bool gbWriteSaveCommit = tokenEquals(text, "GBWRITESAVECOMMIT");
  if (gbWriteSaveCommit || tokenEquals(text, "WRITEBLOCKSCOMMIT")) {
    const uint32_t totalBytes =
      (uint32_t)cardWriteRangeStage.blockCount * (uint32_t)cardWriteRangeStage.blockSize;
    if (!cardWriteRangeStage.active ||
        gbWriteSaveCommit != (cardWriteRangeStage.backend == CardBackend::N64TransferPakGbSave) ||
        cardWriteRangeStage.currentOffset != 0 ||
        cardWriteRangeStage.currentBlockIndex != cardWriteRangeStage.blockCount ||
        cardWriteRangeStage.absoluteOffset != totalBytes) {
      out.println(gbWriteSaveCommit ? F("ERR:CARD_GBWRITESAVE_INCOMPLETE") : F("ERR:CARD_WRITEBLOCKS_INCOMPLETE"));
      return true;
    }
    memset(&cardWriteRangeStage, 0, sizeof(cardWriteRangeStage));
    out.println(gbWriteSaveCommit ? F("OK:CARD_GBWRITESAVECOMMIT") : F("OK:CARD_WRITEBLOCKSCOMMIT"));
    return true;
  }

  const bool gbWriteSaveAbort = tokenEquals(text, "GBWRITESAVEABORT");
  if (gbWriteSaveAbort || tokenEquals(text, "WRITEBLOCKSABORT")) {
    memset(&cardWriteRangeStage, 0, sizeof(cardWriteRangeStage));
    out.println(gbWriteSaveAbort ? F("OK:CARD_GBWRITESAVEABORT") : F("OK:CARD_WRITEBLOCKSABORT"));
    return true;
  }

  if (commandStartsWith(text, "WRITEBEGIN", &remainder)) {
    long rawPort = -1;
    long rawSlot = -1;
    long rawBlock = -1;
    if (!parseLongToken(remainder, &rawPort) ||
        !parseLongToken(remainder, &rawSlot) ||
        !parseLongToken(remainder, &rawBlock) ||
        rawPort < 0 || rawPort > 1 ||
        rawSlot < 0 ||
        rawBlock < 0) {
      out.println(F("ERR:BAD_CARD_WRITEBEGIN"));
      return true;
    }

    const CardBackend backend = cardBackendTargetForPort((uint8_t)rawPort).backend;
    if (backend == CardBackend::None) {
      out.println(F("ERR:CARD_WRITEBEGIN NO_BACKEND"));
      return true;
    }

    const long maxSlot = cardMaxSlotForBackend(backend);
    if (rawSlot > maxSlot) {
      out.println(F("ERR:BAD_CARD_WRITEBEGIN"));
      return true;
    }

    const uint16_t blockSize = cardBlockSizeForBackend(backend);
    const long maxBlock = cardMaxBlockForBackend(backend);
    if (rawBlock > maxBlock || blockSize > kCardWriteStageMax) {
      out.println(F("ERR:BAD_CARD_WRITEBEGIN"));
      return true;
    }
    if (backend == CardBackend::N64ControllerPak && rawSlot != 0) {
      out.println(F("ERR:BAD_CARD_WRITEBEGIN"));
      return true;
    }

    memset(&cardWriteStage, 0, sizeof(cardWriteStage));
    memset(&cardWriteRangeStage, 0, sizeof(cardWriteRangeStage));
    cardWriteStage.active = true;
    cardWriteStage.backend = backend;
    cardWriteStage.port = (uint8_t)rawPort;
    cardWriteStage.slot = (uint8_t)rawSlot;
    cardWriteStage.block = (uint16_t)rawBlock;
    cardWriteStage.blockSize = blockSize;
    out.println(F("OK:CARD_WRITEBEGIN"));
    return true;
  }

  if (commandStartsWith(text, "WRITECHUNK", &remainder)) {
    if (!cardWriteStage.active) {
      out.println(F("ERR:CARD_WRITE_NOT_ACTIVE"));
      return true;
    }

    long rawOffset = -1;
    if (!parseLongToken(remainder, &rawOffset) ||
        rawOffset < 0 ||
        rawOffset >= cardWriteStage.blockSize) {
      out.println(F("ERR:BAD_CARD_WRITECHUNK"));
      return true;
    }

    uint8_t chunk[kCardWriteChunkMax];
    uint16_t chunkLen = 0;
    if (!parseHexBytesToken(remainder, chunk, &chunkLen, kCardWriteChunkMax) ||
        rawOffset + chunkLen > cardWriteStage.blockSize) {
      out.println(F("ERR:BAD_CARD_WRITECHUNK"));
      return true;
    }

    memcpy(&cardWriteStage.data[rawOffset], chunk, chunkLen);
    for (uint16_t i = 0; i < chunkLen; ++i) {
      cardWriteStage.written[rawOffset + i] = true;
    }
    out.println(F("OK:CARD_WRITECHUNK"));
    return true;
  }

  if (tokenEquals(text, "WRITECOMMIT")) {
    if (!cardWriteStageComplete()) {
      out.println(F("ERR:CARD_WRITE_INCOMPLETE"));
      return true;
    }
    const char* errorName = nullptr;
    const bool ok = cardWriteBlock(cardWriteStage.backend,
                                   cardWriteStage.port,
                                   cardWriteStage.slot,
                                   cardWriteStage.block,
                                   cardWriteStage.data,
                                   &errorName);
    cardWriteStage.active = false;
    if (!ok) {
      out.print(F("ERR:CARD_WRITE "));
      out.println(errorName);
      return true;
    }
    out.println(F("OK:CARD_WRITECOMMIT"));
    return true;
  }

  if (tokenEquals(text, "WRITEABORT")) {
    memset(&cardWriteStage, 0, sizeof(cardWriteStage));
    out.println(F("OK:CARD_WRITEABORT"));
    return true;
  }

  if (commandStartsWith(text, "READBIN", &remainder) ||
      commandStartsWith(text, "READ", &remainder)) {
    const bool binaryRead = (text[4] == 'B' || text[4] == 'b');
    long rawPort = -1;
    long rawSlot = -1;
    long rawBlock = -1;
    if (!parseLongToken(remainder, &rawPort) ||
        !parseLongToken(remainder, &rawSlot) ||
        !parseLongToken(remainder, &rawBlock) ||
        rawPort < 0 || rawPort > 1 ||
        rawSlot < 0 ||
        rawBlock < 0) {
      out.println(F("ERR:BAD_CARD_READ"));
      return true;
    }
    const CardBackend backend = cardBackendTargetForPort((uint8_t)rawPort).backend;
    if (backend == CardBackend::None) {
      out.println(F("ERR:CARD_READ NO_BACKEND"));
      return true;
    }

    const long maxSlot = cardMaxSlotForBackend(backend);
    if (rawSlot > maxSlot) {
      out.println(F("ERR:BAD_CARD_READ"));
      return true;
    }

    static uint8_t blockData[kCardWriteStageMax];
    uint16_t blockSize = 0;
    const char* errorName = nullptr;
    if (!cardReadBlock(backend,
                       (uint8_t)rawPort,
                       (uint8_t)rawSlot,
                       (uint16_t)rawBlock,
                       blockData,
                       &blockSize,
                       &errorName)) {
      out.print(F("ERR:CARD_READ "));
      out.println(errorName);
      return true;
    }

    if (binaryRead) {
      printCardBinaryBlock(out, (uint8_t)rawPort, (uint8_t)rawSlot, (uint16_t)rawBlock, blockData, blockSize);
      out.println(F("OK:CARD_READBIN"));
      return true;
    }

    out.print(F("CARD DATA P="));
    out.print(rawPort);
    out.print(F(" S="));
    out.print(rawSlot);
    out.print(F(" B="));
    out.print(rawBlock);
    out.print(F(" HEX="));
    for (uint16_t i = 0; i < blockSize; ++i) {
      printHexByte(out, blockData[i]);
      if ((i & 0x1F) == 0x1F) {
        serviceUsbDuringSerialPrint();
      }
    }
    serviceUsbDuringSerialPrint();
    out.println();
    out.println(F("OK:CARD_READ"));
    return true;
  }

  out.println(F("ERR:BAD_CARD_CMD"));
  return true;
}
#endif

}  // namespace

bool handleSerialMemcardCommand(char* text, Print& out) {
#ifdef ADAPT_FEATURE_SERIAL_MEMCARD_API
  return handleSerialMemcardCommandImpl(text, out);
#else
  (void)text;
  (void)out;
  return false;
#endif
}
