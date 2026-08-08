#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../core/firmware_support.h"
#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
#ifdef ENABLE_INPUT_PSX
bool AutoDetector::isKnownPsxReplyType(uint8_t controllerType) {
  switch (controllerType) {
    case 0x12:
    case 0x23:
    case 0x41:
    case 0x53:
    case 0x55:
    case 0x63:
    case 0x73:
    case 0x79:
    case 0xE3:
    case 0xE5:
      return true;
    default:
      return false;
  }
}

bool AutoDetector::isLegacyPsxReplyFamily(uint8_t controllerType) {
  if (controllerType == 0x00 || controllerType == 0xFF) {
    return false;
  }

  uint8_t typeNibble = controllerType & 0x0F;
  return typeNibble == 0x01 || typeNibble == 0x02 || typeNibble == 0x03;
}

AutoDetectResult AutoDetector::probePSX(const autodetect_pins_t& pins, uint8_t port) {
  gpio_init(pins.psx_att);
  gpio_init(pins.psx_cmd);
  gpio_init(pins.psx_dat);
  gpio_init(pins.psx_clk);
  gpio_init(pins.psx_ack);

  gpio_set_dir(pins.psx_att, GPIO_OUT);
  gpio_set_dir(pins.psx_cmd, GPIO_OUT);
  gpio_set_dir(pins.psx_dat, GPIO_IN);
  gpio_set_dir(pins.psx_clk, GPIO_OUT);
  gpio_set_dir(pins.psx_ack, GPIO_IN);
  gpio_pull_up(pins.psx_dat);
  gpio_pull_up(pins.psx_ack);

  gpio_put(pins.psx_att, HIGH);
  gpio_put(pins.psx_clk, HIGH);
  gpio_put(pins.psx_cmd, HIGH);
  delayMicroseconds(50);

  gpio_put(pins.psx_att, LOW);
  delayMicroseconds(20);

  // Use the normal controller poll frame instead of stopping after the type
  // byte. neGcon and other PSX specialty controllers can be unreliable during
  // AUTO if ATT is released before the 0x5A sync phase has been clocked.
  uint8_t recv1 = psxTransferByte(pins, 0x01);
  delayMicroseconds(20);

  uint8_t controllerType = psxTransferByte(pins, 0x42);
  delayMicroseconds(20);

  uint8_t sync = psxTransferByte(pins, 0x00);
  delayMicroseconds(20);

  (void)psxTransferByte(pins, 0xFF);
  delayMicroseconds(20);

  (void)psxTransferByte(pins, 0xFF);
  delayMicroseconds(20);

  gpio_put(pins.psx_att, HIGH);

  gpio_set_dir(pins.psx_att, GPIO_IN);
  gpio_set_dir(pins.psx_cmd, GPIO_IN);
  gpio_set_dir(pins.psx_clk, GPIO_IN);

  #ifdef AUTODETECT_DEBUG
  lastDebug[port].psx_recv1 = recv1;
  lastDebug[port].psx_type = controllerType;
  lastDebug[port].psx_sync = sync;
  #endif

  const bool validHeader = (recv1 == 0xFF);
  const bool validSync = (sync == 0x5A);
  const bool knownType = isKnownPsxReplyType(controllerType);

  if (validSync && knownType) {
    return AUTODETECT_PSX;
  }

  if (validHeader && knownType) {
    return AUTODETECT_PSX;
  }

  if (validHeader && validSync && isLegacyPsxReplyFamily(controllerType)) {
    return AUTODETECT_PSX;
  }

  return AUTODETECT_NONE;
}

bool AutoDetector::psxWaitForAck(const autodetect_pins_t& pins, uint16_t timeoutUs) {
  const uint32_t started = micros();
  bool sawLow = false;

  while ((uint32_t)(micros() - started) < timeoutUs) {
    if (!gpio_get(pins.psx_ack)) {
      sawLow = true;
      break;
    }
  }

  if (!sawLow) {
    return false;
  }

  while ((uint32_t)(micros() - started) < timeoutUs) {
    if (gpio_get(pins.psx_ack)) {
      return true;
    }
  }

  return true;
}

uint8_t AutoDetector::psxTransferByte(const autodetect_pins_t& pins, uint8_t txByte) {
  uint8_t rxByte = 0;

  for (uint8_t i = 0; i < 8; i++) {
    gpio_put(pins.psx_cmd, (txByte >> i) & 0x01);
    delayMicroseconds(2);

    gpio_put(pins.psx_clk, LOW);
    delayMicroseconds(2);

    gpio_put(pins.psx_clk, HIGH);
    delayMicroseconds(2);

    if (gpio_get(pins.psx_dat)) {
      rxByte |= (1 << i);
    }
  }

  return rxByte;
}
#endif
#endif
