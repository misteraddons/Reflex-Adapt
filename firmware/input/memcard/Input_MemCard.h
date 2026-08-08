// Memory Card Manager Input Module
// Presents VMU or N64 Controller Pak as USB Mass Storage device
//
// This mode allows:
// - Backup memory card to PC via drag-and-drop
// - Restore memory card from backup file
// - Works with any OS (Windows, Mac, Linux, MiSTer)
//
// The memory card appears as a USB flash drive with:
// - RAW_BACKUP.BIN - Complete memory card dump (for emulators)
// - Individual save files (future enhancement)
//
// MiSTer Bridge Protocol (USB CDC Serial):
// - INFO - Returns card type and size
// - READ <block> - Returns 512 bytes as hex
// - WRITE <block> <hex_data> - Writes 512 bytes
// - FLUSH - Commits to physical card
// - STATUS - Returns current state

#pragma once

#include "../base/RZInputModule.h"

#include <Adafruit_TinyUSB.h>

#include <MapleLib/MapleLib.h>

// Memory card types
enum MemCardType : uint8_t {
  MEMCARD_NONE = 0,
  MEMCARD_VMU,        // Dreamcast VMU - 128KB
  MEMCARD_N64_PAK,    // N64 Controller Pak - 32KB
};

// Memory card sizes
#define VMU_SIZE        (128 * 1024)  // 128KB
#define N64_PAK_SIZE    (32 * 1024)   // 32KB
#define VMU_BLOCK_SIZE  512
#define N64_PAGE_SIZE   256

// Virtual FAT16 disk parameters
// We need enough space for the memory card data + FAT overhead
// Using 512-byte sectors, FAT16
#define DISK_BLOCK_SIZE   512
#define DISK_BLOCK_NUM    544   // 272KB total: boot + FAT + root + 256 data blocks (128KB)

class RZInputMemCard : public RZInputModule {
private:
  Adafruit_USBD_MSC usb_msc;
  Adafruit_USBD_CDC usb_cdc;  // Serial interface for MiSTer bridge
  MemCardType cardType = MEMCARD_NONE;
  bool cardPresent = false;
  bool diskDirty = false;

  // Maple Bus for VMU access
  MaplePort maplePort;
  bool mapleInitialized = false;
  uint8_t currentVMUSlot = 0;
  uint32_t lastPollTime = 0;
  static const uint32_t POLL_INTERVAL_MS = 500;  // Check for card every 500ms

  // Reading state machine (non-blocking read)
  enum class ReadState { IDLE, READING, COMPLETE, ERROR };
  ReadState readState = ReadState::IDLE;
  uint16_t currentReadBlock = 0;

  // Serial command buffer for MiSTer bridge
  char cmdBuffer[1200];  // Enough for WRITE command with 512 bytes hex (1024 chars)
  uint16_t cmdBufferPos = 0;

  // Virtual disk image (generated on-the-fly for most blocks)
  // We only cache the memory card data, FAT/directory are generated
  static uint8_t memCardCache[VMU_SIZE];  // Large enough for VMU (largest)
  static uint32_t memCardSize;
  static bool cacheValid;
  static RZInputMemCard* instance;

  // FAT16 layout:
  // Block 0: Boot sector
  // Block 1-15: FAT (16 blocks for FAT16)
  // Block 16-31: Root directory (16 blocks = 256 entries)
  // Block 32+: Data area
  static const uint32_t FAT_START = 1;
  static const uint32_t FAT_SECTORS = 16;
  static const uint32_t ROOT_START = FAT_START + FAT_SECTORS;  // 17
  static const uint32_t ROOT_SECTORS = 16;
  static const uint32_t DATA_START = ROOT_START + ROOT_SECTORS;  // 33

  void generateBootSector(uint8_t* buffer);
  void generateFATSector(uint32_t fatSector, uint8_t* buffer);
  void generateRootSector(uint32_t rootSector, uint8_t* buffer);
  void readDataBlock(uint32_t dataBlock, uint8_t* buffer);
  void writeDataBlock(uint32_t dataBlock, uint8_t* buffer);

  static uint8_t hexToNibble(char c);
  static uint8_t hexToByte(const char* hex);
  void sendHexByte(uint8_t b);
  void processSerialCommand(const char* cmd);
  void pollSerial();

  static int32_t mscReadCallback(uint32_t lba, void* buffer, uint32_t bufsize);
  static int32_t mscWriteCallback(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
  static void mscFlushCallback(void);
  static bool mscReadyCallback(void);

public:
  RZInputMemCard();

  const char* getUsbId() override;
  void configureBcdDeviceVersion() override;
  const char* getDescription() override;

  void setup() override;
  void setup2() override;
  void updateDisplay();
  bool poll() override;

  static int32_t readBlock(uint32_t lba, void* buffer, uint32_t bufsize);
  static int32_t writeBlock(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
  static void flushDisk();
  static bool isReady();

  // Public methods for setting card type/data (called by detection code)
  void setCardPresent(MemCardType type, uint32_t size);
  void setCardRemoved();
  uint8_t* getCache();
  uint32_t getCacheSize();
};
