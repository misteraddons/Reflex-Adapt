#include "Input_MemCard.h"

uint8_t RZInputMemCard::memCardCache[VMU_SIZE];
uint32_t RZInputMemCard::memCardSize = 0;
bool RZInputMemCard::cacheValid = false;
RZInputMemCard* RZInputMemCard::instance = nullptr;

void RZInputMemCard::generateBootSector(uint8_t* buffer) {
  memset(buffer, 0, 512);

  // Jump instruction
  buffer[0] = 0xEB; buffer[1] = 0x3C; buffer[2] = 0x90;

  // OEM name
  memcpy(&buffer[3], "REFLEXMC", 8);

  // BPB (BIOS Parameter Block)
  buffer[11] = (DISK_BLOCK_SIZE & 0xFF);       // Bytes per sector (low)
  buffer[12] = (DISK_BLOCK_SIZE >> 8);         // Bytes per sector (high)
  buffer[13] = 1;                               // Sectors per cluster
  buffer[14] = 1; buffer[15] = 0;              // Reserved sectors
  buffer[16] = 1;                               // Number of FATs
  buffer[17] = 0; buffer[18] = 1;              // Root entries (256)
  buffer[19] = (DISK_BLOCK_NUM & 0xFF);        // Total sectors (low)
  buffer[20] = (DISK_BLOCK_NUM >> 8);          // Total sectors (high)
  buffer[21] = 0xF8;                            // Media descriptor (fixed disk)
  buffer[22] = FAT_SECTORS; buffer[23] = 0;    // Sectors per FAT
  buffer[24] = 1; buffer[25] = 0;              // Sectors per track
  buffer[26] = 1; buffer[27] = 0;              // Number of heads
  // Hidden sectors, total sectors 32-bit: 0

  // Extended boot record
  buffer[36] = 0x80;                            // Drive number
  buffer[38] = 0x29;                            // Extended boot signature
  buffer[39] = 0x34; buffer[40] = 0x12;        // Volume serial (low)
  buffer[41] = 0x00; buffer[42] = 0x00;        // Volume serial (high)

  // Volume label
  const char* label = (cardType == MEMCARD_VMU) ? "REFLEX_VMU " :
                      (cardType == MEMCARD_N64_PAK) ? "REFLEX_N64 " : "REFLEX_MC  ";
  memcpy(&buffer[43], label, 11);

  // Filesystem type
  memcpy(&buffer[54], "FAT16   ", 8);

  // Boot signature
  buffer[510] = 0x55;
  buffer[511] = 0xAA;
}

void RZInputMemCard::generateFATSector(uint32_t fatSector, uint8_t* buffer) {
  memset(buffer, 0, 512);

  if (fatSector == 0) {
    // First FAT sector - media descriptor and reserved entries
    buffer[0] = 0xF8; buffer[1] = 0xFF;  // Media descriptor
    buffer[2] = 0xFF; buffer[3] = 0xFF;  // End of chain marker

    // File allocation for RAW_BACKUP.BIN
    // Calculate how many clusters the file needs
    uint32_t fileClusters = (memCardSize + DISK_BLOCK_SIZE - 1) / DISK_BLOCK_SIZE;

    // Create chain: 2 -> 3 -> 4 -> ... -> EOF
    for (uint32_t i = 0; i < fileClusters && (4 + i * 2) < 512; i++) {
      uint32_t offset = 4 + i * 2;  // Start at cluster 2
      if (i == fileClusters - 1) {
        // Last cluster - EOF marker
        buffer[offset] = 0xFF;
        buffer[offset + 1] = 0xFF;
      } else {
        // Next cluster
        uint16_t next = 3 + i;
        buffer[offset] = next & 0xFF;
        buffer[offset + 1] = (next >> 8) & 0xFF;
      }
    }
  }
  // Other FAT sectors are 0 (unused clusters)
}

void RZInputMemCard::generateRootSector(uint32_t rootSector, uint8_t* buffer) {
  memset(buffer, 0, 512);

  if (rootSector == 0) {
    // First entry: Volume label
    const char* label = (cardType == MEMCARD_VMU) ? "REFLEX_VMU " :
                        (cardType == MEMCARD_N64_PAK) ? "REFLEX_N64 " : "REFLEX_MC  ";
    memcpy(&buffer[0], label, 11);
    buffer[11] = 0x08;  // Volume label attribute

    // Second entry: RAW_BACKUP.BIN
    memcpy(&buffer[32], "RAW_BACKUPBIN", 11);  // 8.3 filename
    buffer[32 + 11] = 0x20;  // Archive attribute

    // File size (4 bytes at offset 28)
    buffer[32 + 28] = (memCardSize >> 0) & 0xFF;
    buffer[32 + 29] = (memCardSize >> 8) & 0xFF;
    buffer[32 + 30] = (memCardSize >> 16) & 0xFF;
    buffer[32 + 31] = (memCardSize >> 24) & 0xFF;

    // First cluster (2 bytes at offset 26)
    buffer[32 + 26] = 2;  // Cluster 2
    buffer[32 + 27] = 0;

    // Timestamps (simplified - all zeros = 1980-01-01)
  }
}

void RZInputMemCard::readDataBlock(uint32_t dataBlock, uint8_t* buffer) {
  uint32_t offset = dataBlock * DISK_BLOCK_SIZE;

  if (offset < memCardSize) {
    uint32_t remaining = memCardSize - offset;
    uint32_t toRead = (remaining < DISK_BLOCK_SIZE) ? remaining : DISK_BLOCK_SIZE;
    memcpy(buffer, &memCardCache[offset], toRead);
    if (toRead < DISK_BLOCK_SIZE) {
      memset(&buffer[toRead], 0, DISK_BLOCK_SIZE - toRead);
    }
  } else {
    memset(buffer, 0, DISK_BLOCK_SIZE);
  }
}

void RZInputMemCard::writeDataBlock(uint32_t dataBlock, uint8_t* buffer) {
  uint32_t offset = dataBlock * DISK_BLOCK_SIZE;

  if (offset < memCardSize) {
    uint32_t remaining = memCardSize - offset;
    uint32_t toWrite = (remaining < DISK_BLOCK_SIZE) ? remaining : DISK_BLOCK_SIZE;
    memcpy(&memCardCache[offset], buffer, toWrite);
    diskDirty = true;
  }
}

int32_t RZInputMemCard::mscReadCallback(uint32_t lba, void* buffer, uint32_t bufsize) {
  return RZInputMemCard::readBlock(lba, buffer, bufsize);
}

int32_t RZInputMemCard::mscWriteCallback(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  return RZInputMemCard::writeBlock(lba, buffer, bufsize);
}

void RZInputMemCard::mscFlushCallback(void) {
  RZInputMemCard::flushDisk();
}

bool RZInputMemCard::mscReadyCallback(void) {
  return RZInputMemCard::isReady();
}

int32_t RZInputMemCard::readBlock(uint32_t lba, void* buffer, uint32_t bufsize) {
  if (!instance || !instance->cardPresent) return -1;

  uint8_t* buf = (uint8_t*)buffer;
  uint32_t blocksRead = 0;

  while (bufsize >= DISK_BLOCK_SIZE) {
    if (lba == 0) {
      // Boot sector
      instance->generateBootSector(buf);
    } else if (lba >= FAT_START && lba < ROOT_START) {
      // FAT sector
      instance->generateFATSector(lba - FAT_START, buf);
    } else if (lba >= ROOT_START && lba < DATA_START) {
      // Root directory sector
      instance->generateRootSector(lba - ROOT_START, buf);
    } else if (lba >= DATA_START) {
      // Data sector
      instance->readDataBlock(lba - DATA_START, buf);
    } else {
      memset(buf, 0, DISK_BLOCK_SIZE);
    }

    buf += DISK_BLOCK_SIZE;
    bufsize -= DISK_BLOCK_SIZE;
    lba++;
    blocksRead++;
  }

  return blocksRead * DISK_BLOCK_SIZE;
}

int32_t RZInputMemCard::writeBlock(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  if (!instance || !instance->cardPresent) return -1;

  uint32_t blocksWritten = 0;

  while (bufsize >= DISK_BLOCK_SIZE) {
    if (lba >= DATA_START) {
      // Data sector - write to cache
      instance->writeDataBlock(lba - DATA_START, buffer);
    }
    // Ignore writes to boot/FAT/root sectors (read-only structure)

    buffer += DISK_BLOCK_SIZE;
    bufsize -= DISK_BLOCK_SIZE;
    lba++;
    blocksWritten++;
  }

  return blocksWritten * DISK_BLOCK_SIZE;
}

void RZInputMemCard::flushDisk() {
  if (!instance || !instance->diskDirty) return;
  if (!instance->cardPresent || !instance->mapleInitialized) return;

  // Write cache back to VMU
  if (instance->cardType == MEMCARD_VMU) {
    // Write all 256 blocks back to VMU
    for (uint16_t block = 0; block < 256; block++) {
      VMUBlockResult result = instance->maplePort.writeVMUBlock(
        instance->currentVMUSlot,
        block,
        &memCardCache[block * VMU_BLOCK_SIZE]
      );

      if (result != VMUBlockResult::SUCCESS) {
        // Write failed - card may have been removed
        instance->cardPresent = false;
        instance->usb_msc.setUnitReady(false);
        instance->readState = ReadState::ERROR;
        break;
      }
    }
  }
  // TODO: Add N64 Controller Pak support via Joybus

  instance->diskDirty = false;
}

bool RZInputMemCard::isReady() {
  return instance && instance->cardPresent;
}
