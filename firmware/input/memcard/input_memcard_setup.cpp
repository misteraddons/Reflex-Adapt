#include "Input_MemCard.h"

#include "../../platform/display_runtime_state.h"

RZInputMemCard::RZInputMemCard() : RZInputModule() {
  instance = this;
}

const char* RZInputMemCard::getUsbId() {
  return "RZRMC";
}

void RZInputMemCard::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_MEMCARD;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputMemCard::getDescription() {
  return "MemCard Mgr";
}

void RZInputMemCard::setup() {
  setInputPortCount(0);  // No controller output in this mode

  // Initialize MSC (USB Mass Storage)
  usb_msc.setID("Reflex", "MemCard", "1.0");
  usb_msc.setCapacity(DISK_BLOCK_NUM, DISK_BLOCK_SIZE);
  usb_msc.setReadWriteCallback(
    RZInputMemCard::mscReadCallback,
    RZInputMemCard::mscWriteCallback,
    RZInputMemCard::mscFlushCallback
  );
  usb_msc.setReadyCallback(RZInputMemCard::mscReadyCallback);
  usb_msc.setUnitReady(false);  // Not ready until card detected
  usb_msc.begin();

  // Initialize CDC (Serial for MiSTer bridge)
  usb_cdc.begin(115200);

  // Re-enumerate USB to add MSC + CDC
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  memCardSize = 0;
  cacheValid = false;
  cmdBufferPos = 0;

  // Initialize Maple Bus on HDMI pins 8,9 (GPIO 5,6)
  // Pin 8 = SDCKA (GPIO 5), Pin 9 = SDCKB (GPIO 6)
  // MapleLib expects pinB = pinA + 1
  if (maplePort.begin(5)) {
    mapleInitialized = true;
  }
}

void RZInputMemCard::setup2() {
  // Display mode info
  setPortLed(0, HIGH);

#ifdef USE_I2C_DISPLAY
  // Show initial status on OLED
  display.clear();
  display.setFont(System5x7);
  display.setCursor(0, 0);
  display.println(F("MemCard Manager"));
  display.println();
  display.println(F("Waiting for"));
  display.println(F("VMU or N64 Pak..."));
#endif
}

void RZInputMemCard::setCardPresent(MemCardType type, uint32_t size) {
  cardType = type;
  memCardSize = size;
  cardPresent = true;
  cacheValid = false;
  usb_msc.setUnitReady(true);
}

void RZInputMemCard::setCardRemoved() {
  cardPresent = false;
  usb_msc.setUnitReady(false);
}

uint8_t* RZInputMemCard::getCache() {
  return memCardCache;
}

uint32_t RZInputMemCard::getCacheSize() {
  return memCardSize;
}
