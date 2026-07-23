#include "Input_Pce.h"

#include <Arduino.h>

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include "hardware/gpio.h"
#endif

const input_pce_config_t input_pce_config[] = {
  { .sel = HDMI_1_05, .clr = HDMI_1_06, .d0 = HDMI_1_01, .d1 = HDMI_1_02, .d2 = HDMI_1_03, .d3 = HDMI_1_04 },
  { .sel = HDMI_2_05, .clr = HDMI_2_06, .d0 = HDMI_2_01, .d1 = HDMI_2_02, .d2 = HDMI_2_03, .d3 = HDMI_2_04 },
};

namespace {

__force_inline void __not_in_flash_func(writePcePin)(uint8_t pin, bool value) {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  gpio_put(pin, value ? 1 : 0);
#else
  digitalWrite(pin, value ? HIGH : LOW);
#endif
}

__force_inline uint8_t __not_in_flash_func(readPcePin)(uint8_t pin) {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  return gpio_get(pin) ? 1 : 0;
#else
  return digitalRead(pin) ? 1 : 0;
#endif
}

}  // namespace

RZInputPce::RZInputPce() : RZInputModule() {}

const char* RZInputPce::getUsbId() {
  return "RZRPce";
}

void RZInputPce::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_PCE;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputPce::getDescription() {
  return "PC-ENGINE";
}

bool __not_in_flash_func(RZInputPce::rawControllerPresent)(uint8_t port) const {
  if (port >= input_ports) {
    return false;
  }

  const input_pce_config_t& c = input_pce_config[port];

  writePcePin(c.sel, true);
  writePcePin(c.clr, true);
  delayMicroseconds(2);

  uint8_t nibble =
    (readPcePin(c.d3) << 3) |
    (readPcePin(c.d2) << 2) |
    (readPcePin(c.d1) << 1) |
    readPcePin(c.d0);

  writePcePin(c.clr, false);
  writePcePin(c.sel, false);
  delayMicroseconds(2);

  return nibble == 0x0;
}

void RZInputPce::setup() {
  setInputPortCount(input_ports);

  pollInterval = 125;

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    dtype[i] = PCE_DEVICE_NONE;
    clearPendingDeviceType(i);
    slotLastSeenMs[i] = 0;
    controller_state_t& frame = inputFrame(i);
    setInputFrameConnected(i, false);
    clearInputFrameTypeName(i);
    frame.HAS_BTN_SELECT = 1;
    frame.HAS_BTN_START = 1;
  }

  for (uint8_t i = 0; i < input_ports; ++i) {
    input_pce_config_t c = input_pce_config[i];
    pce[i] = new PcePort(c.sel, c.clr, c.d0, c.d1, c.d2, c.d3);
    pce[i]->begin();
    portSlotCapacity[i] = 1;
  }

  #ifdef PCE_ENABLE_MULTITAP
    bool found_mtap = false;
    delay(16);
    uint8_t total_controller_ports = 0;
    for (uint8_t i = 0; i < input_ports; ++i) {
      pce[i]->detectMultitap();
      uint8_t tap = pce[i]->getMultitapPorts();
      pce[i]->setMultitapProbeIntervalMs(tap ? 0 : 250);
      portSlotCapacity[i] = tap ? tap : 1;
      total_controller_ports += portSlotCapacity[i];
      if (tap)
        found_mtap = true;
    }
    setInputPortCount(min(total_controller_ports, MAX_USB_OUT));
    if (found_mtap)
      delay(16);
  #endif
}

void RZInputPce::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}
