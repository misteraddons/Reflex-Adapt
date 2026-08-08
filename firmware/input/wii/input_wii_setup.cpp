#include "Input_Wii.h"

RZInputWii::RZInputWii() : RZInputModule() {}

bool RZInputWii::portEnabled(uint8_t port) const {
  return port < input_ports && (activePortMask & (uint8_t)(1u << port));
}

void RZInputWii::setPhysicalPortMask(uint8_t mask) {
  const uint8_t validMask = (uint8_t)((1u << input_ports) - 1u);
  const uint8_t sanitized = mask & validMask;
  activePortMask = sanitized == 0 ? validMask : sanitized;
}

bool RZInputWii::beginWiiBus(uint8_t port, uint8_t pair) {
  if (port >= input_ports || pair >= INPUT_WII_PIN_PAIR_COUNT) {
    return false;
  }
  if (initializedBusPort == port && initializedBusPair == pair) {
    return true;
  }
  if (initializedBusPort < input_ports && initializedBusPair < INPUT_WII_PIN_PAIR_COUNT) {
    input_wii_config[initializedBusPort].wire->end();
  }
  const input_wii_config_t& c = input_wii_config[port];
  const input_wii_pin_pair_t& pins = c.pinPairs[pair];
  c.wire->setSDA(pins.sda);
  c.wire->setSCL(pins.scl);
  c.wire->setClock(INPUT_WII_WIRE_SPEED);
  c.wire->begin();
  initializedBusPort = port;
  initializedBusPair = pair;
  return true;
}

void RZInputWii::endWiiBus(uint8_t port) {
  (void)port;
}

const char* RZInputWii::getUsbId() {
  return "RZRWii";
}

void RZInputWii::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_WII;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputWii::getDescription() {
  return "WII";
}

void RZInputWii::setup() {
  setInputPortCount(input_ports);

  pollInterval = WII_CONNECTED_POLL_INTERVAL_US;
  initializedBusPort = INPUT_WII_PIN_PAIR_INVALID;
  initializedBusPair = INPUT_WII_PIN_PAIR_INVALID;

  for (uint8_t i = 0; i < input_ports; ++i) {
    input_wii_config_t c = input_wii_config[i];
    haveController[i] = false;
    updateFailCount[i] = 0;
    settlingAfterBadFrame[i] = false;
    resetControllerCache(i);
    activePinPair[i] = INPUT_WII_PIN_PAIR_INVALID;
    nextConnectAttemptUs[i] = 0;
    wii_last_digital_state[i] = 0;
    wii_last_analog_sticks_state[i] = 0;
    wii_last_analog_buttons_state[i] = 0;

    if (!portEnabled(i)) {
      if (i < MAX_USB_OUT) {
        setInputFrameConnected(inputFrame(i), false);
        clearInputFrameTypeName(inputFrame(i));
      }
      continue;
    }

    wii[i] = new ExtensionPort(*c.wire);
    wii_classic[i] = new ClassicController::Shared(*wii[i]);
    wii_nchuk[i] = new Nunchuk::Shared(*wii[i]);
    #ifdef ENABLE_WII_GUITAR
    wii_guitar[i] = new GuitarController::Shared(*wii[i]);
    #endif

    c.wire->setClock(INPUT_WII_WIRE_SPEED);

    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      frame.HAS_BTN_SELECT = 1;
      frame.HAS_BTN_START = 1;
      frame.HAS_ANALOG_STICK_MAIN = 1;
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
    }
  }
}

void RZInputWii::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (!portEnabled(i)) {
      continue;
    }
    setPortLed(i, HIGH);
  }
}
