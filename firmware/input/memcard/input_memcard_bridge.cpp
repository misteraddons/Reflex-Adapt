#include "Input_MemCard.h"

uint8_t RZInputMemCard::hexToNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

uint8_t RZInputMemCard::hexToByte(const char* hex) {
  return (hexToNibble(hex[0]) << 4) | hexToNibble(hex[1]);
}

void RZInputMemCard::sendHexByte(uint8_t b) {
  static const char hexChars[] = "0123456789ABCDEF";
  char hex[2] = { hexChars[b >> 4], hexChars[b & 0xF] };
  usb_cdc.write(hex, 2);
}

void RZInputMemCard::processSerialCommand(const char* cmd) {
  // INFO - Return card info
  if (strncmp(cmd, "INFO", 4) == 0) {
    usb_cdc.print("TYPE:");
    switch (cardType) {
      case MEMCARD_VMU:     usb_cdc.print("VMU"); break;
      case MEMCARD_N64_PAK: usb_cdc.print("N64PAK"); break;
      default:              usb_cdc.print("NONE"); break;
    }
    usb_cdc.print(" SIZE:");
    usb_cdc.print(memCardSize);
    usb_cdc.print(" BLOCKS:");
    usb_cdc.println(memCardSize / 512);
    return;
  }

  // STATUS - Return current state
  if (strncmp(cmd, "STATUS", 6) == 0) {
    usb_cdc.print("READY:");
    usb_cdc.print(cardPresent ? "1" : "0");
    usb_cdc.print(" DIRTY:");
    usb_cdc.print(diskDirty ? "1" : "0");
    usb_cdc.print(" STATE:");
    switch (readState) {
      case ReadState::IDLE:     usb_cdc.println("IDLE"); break;
      case ReadState::READING:  usb_cdc.println("READING"); break;
      case ReadState::COMPLETE: usb_cdc.println("READY"); break;
      case ReadState::ERROR:    usb_cdc.println("ERROR"); break;
    }
    return;
  }

  // READ <block> - Return 512 bytes as hex
  if (strncmp(cmd, "READ ", 5) == 0) {
    if (!cardPresent) {
      usb_cdc.println("ERROR:NO_CARD");
      return;
    }
    uint32_t block = atoi(&cmd[5]);
    if (block >= memCardSize / 512) {
      usb_cdc.println("ERROR:INVALID_BLOCK");
      return;
    }
    usb_cdc.print("DATA:");
    uint32_t offset = block * 512;
    for (uint32_t i = 0; i < 512; i++) {
      sendHexByte(memCardCache[offset + i]);
    }
    usb_cdc.println();
    return;
  }

  // WRITE <block> <hex_data> - Write 512 bytes
  if (strncmp(cmd, "WRITE ", 6) == 0) {
    if (!cardPresent) {
      usb_cdc.println("ERROR:NO_CARD");
      return;
    }
    // Parse block number
    const char* p = &cmd[6];
    uint32_t block = 0;
    while (*p >= '0' && *p <= '9') {
      block = block * 10 + (*p - '0');
      p++;
    }
    if (*p != ' ') {
      usb_cdc.println("ERROR:SYNTAX");
      return;
    }
    p++;  // Skip space

    if (block >= memCardSize / 512) {
      usb_cdc.println("ERROR:INVALID_BLOCK");
      return;
    }

    // Parse hex data (1024 chars = 512 bytes)
    if (strlen(p) < 1024) {
      usb_cdc.println("ERROR:DATA_TOO_SHORT");
      return;
    }

    uint32_t offset = block * 512;
    for (uint32_t i = 0; i < 512; i++) {
      memCardCache[offset + i] = hexToByte(&p[i * 2]);
    }
    diskDirty = true;
    usb_cdc.println("OK");
    return;
  }

  // FLUSH - Commit to physical card
  if (strncmp(cmd, "FLUSH", 5) == 0) {
    if (!cardPresent) {
      usb_cdc.println("ERROR:NO_CARD");
      return;
    }
    flushDisk();
    usb_cdc.println("OK");
    return;
  }

  // HELP - List commands
  if (strncmp(cmd, "HELP", 4) == 0) {
    usb_cdc.println("REFLEX MEMCARD BRIDGE v1.0");
    usb_cdc.println("Commands:");
    usb_cdc.println("  INFO   - Card type and size");
    usb_cdc.println("  STATUS - Current state");
    usb_cdc.println("  READ n - Read block n (hex)");
    usb_cdc.println("  WRITE n <hex> - Write block");
    usb_cdc.println("  FLUSH  - Commit to card");
    return;
  }

  usb_cdc.println("ERROR:UNKNOWN_CMD");
}

void RZInputMemCard::pollSerial() {
  while (usb_cdc.available()) {
    char c = usb_cdc.read();
    if (c == '\r' || c == '\n') {
      if (cmdBufferPos > 0) {
        cmdBuffer[cmdBufferPos] = '\0';
        processSerialCommand(cmdBuffer);
        cmdBufferPos = 0;
      }
    } else if (cmdBufferPos < sizeof(cmdBuffer) - 1) {
      cmdBuffer[cmdBufferPos++] = c;
    }
  }
}
