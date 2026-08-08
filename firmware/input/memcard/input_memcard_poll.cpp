#include "Input_MemCard.h"

#include "../../platform/display_runtime_state.h"

void RZInputMemCard::updateDisplay() {
#ifdef USE_I2C_DISPLAY
  display.clear();
  display.setFont(System5x7);
  display.setCursor(0, 0);

  switch (readState) {
    case ReadState::READING:
      display.println(F("Reading VMU..."));
      display.println();
      display.print(F("Block "));
      display.print(currentReadBlock);
      display.println(F("/256"));
      // Progress bar
      {
        uint8_t progress = (currentReadBlock * 100) / 256;
        display.print(F("["));
        for (uint8_t i = 0; i < 10; i++) {
          display.print(i < progress / 10 ? '#' : '-');
        }
        display.print(F("] "));
        display.print(progress);
        display.println(F("%"));
      }
      break;

    case ReadState::COMPLETE:
      display.println(F("VMU Ready!"));
      display.println();
      display.println(F("USB Mass Storage"));
      display.println(F("active."));
      display.println();
      display.println(F("Drag RAW_BACKUP.BIN"));
      display.println(F("to backup saves."));
      break;

    case ReadState::ERROR:
      display.println(F("VMU Error!"));
      display.println();
      display.println(F("Check connection"));
      display.println(F("and try again."));
      break;

    case ReadState::IDLE:
    default:
      display.println(F("MemCard Manager"));
      display.println();
      if (mapleInitialized) {
        display.println(F("Waiting for"));
        display.println(F("VMU or N64 Pak..."));
      } else {
        display.println(F("Maple Bus"));
        display.println(F("init failed!"));
      }
      break;
  }
#endif
}

bool RZInputMemCard::poll() {
  // Process MiSTer bridge serial commands
  pollSerial();

  if (!mapleInitialized) {
    return false;
  }

  uint32_t now = millis();
  ReadState prevState = readState;

  // State machine for reading VMU data
  switch (readState) {
    case ReadState::READING:
      // Continue reading blocks
      if (currentReadBlock < 256) {  // VMU has 256 blocks
        VMUBlockResult result = maplePort.readVMUBlock(
          currentVMUSlot,
          currentReadBlock,
          &memCardCache[currentReadBlock * VMU_BLOCK_SIZE]
        );

        if (result == VMUBlockResult::SUCCESS) {
          currentReadBlock++;
          // Update display every 16 blocks (approx 6% progress)
          if ((currentReadBlock % 16) == 0) {
            updateDisplay();
          }
        } else if (result == VMUBlockResult::TIMEOUT) {
          // Retry on timeout
        } else {
          // Read error - abort
          readState = ReadState::ERROR;
          cardPresent = false;
          usb_msc.setUnitReady(false);
        }
      } else {
        // All blocks read successfully
        readState = ReadState::COMPLETE;
        memCardSize = VMU_SIZE;
        cacheValid = true;
        cardPresent = true;
        usb_msc.setUnitReady(true);
      }
      break;

    case ReadState::COMPLETE:
      // Card is ready, just keep polling to detect removal
      if (now - lastPollTime >= POLL_INTERVAL_MS) {
        lastPollTime = now;
        maplePort.update();

        // Check if VMU is still present
        if (!maplePort.isVMUPresent(currentVMUSlot)) {
          cardPresent = false;
          cacheValid = false;
          usb_msc.setUnitReady(false);
          readState = ReadState::IDLE;
        }
      }
      break;

    case ReadState::ERROR:
    case ReadState::IDLE:
    default:
      // Periodically check for VMU insertion
      if (now - lastPollTime >= POLL_INTERVAL_MS) {
        lastPollTime = now;

        // Update Maple Bus state
        maplePort.update();

        // Check if a controller is connected
        if (maplePort.isConnected()) {
          // Query for VMU in slot A first, then slot B
          for (uint8_t slot = 0; slot < 2; slot++) {
            if (maplePort.queryVMU(slot)) {
              if (maplePort.getVMUMemoryInfo(slot)) {
                const VMUInfo& info = maplePort.getVMUInfo(slot);
                if (info.present && (info.func & MAPLE_FUNC_MEMORY)) {
                  // VMU found! Start reading
                  currentVMUSlot = slot;
                  currentReadBlock = 0;
                  cardType = MEMCARD_VMU;
                  readState = ReadState::READING;
                  updateDisplay();  // Show initial reading status
                  break;
                }
              }
            }
          }
        }
      }
      break;
  }

  // Update display on state change
  if (readState != prevState) {
    updateDisplay();
  }

  return false;
}
