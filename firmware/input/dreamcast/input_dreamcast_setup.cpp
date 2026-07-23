#include "Input_Dreamcast.h"

#if defined(ENABLE_INPUT_AUTODETECT)
#include "../../platform/buzzer.h"
#include "../autodetect/Input_AutoDetect.h"
#endif

#if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
#include <JoybusLib/joybus_pio.hpp>
#endif

namespace {

void resetDreamcastPins(uint8_t pinA, uint8_t pinB) {
  gpio_set_oeover(pinA, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(pinA, GPIO_OVERRIDE_NORMAL);
  gpio_set_function(pinA, GPIO_FUNC_SIO);
  gpio_init(pinA);
  gpio_set_dir(pinA, GPIO_IN);
  gpio_pull_up(pinA);

  gpio_set_oeover(pinB, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(pinB, GPIO_OVERRIDE_NORMAL);
  gpio_set_function(pinB, GPIO_FUNC_SIO);
  gpio_init(pinB);
  gpio_set_dir(pinB, GPIO_IN);
  gpio_pull_up(pinB);
}

void releaseJoybusPioForDreamcast() {
#if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
  joybus_pio_unload(pio0);
  joybus_pio_unload(pio1);
#endif
}

void prepareDreamcastManualBus() {
#if defined(ENABLE_INPUT_AUTODETECT)
  for (uint8_t port = 0; port < kAutoDetectPortCount; ++port) {
    AutoDetector::resetSharedPins(autodetect_pins[port], false);
  }
#else
  constexpr uint8_t kDreamcastConfigPorts =
      sizeof(input_dreamcast_config) / sizeof(input_dreamcast_config[0]);
  for (uint8_t port = 0; port < kDreamcastConfigPorts; ++port) {
    resetDreamcastPins(input_dreamcast_config[port].pinA, input_dreamcast_config[port].pinB);
  }
  delay(50);
#endif

  releaseJoybusPioForDreamcast();
  delay(2);
}

bool beginDreamcastPort(MaplePort& port, uint8_t pinA, uint8_t pinB) {
  if (pinB != pinA + 1) {
    return false;
  }

  resetDreamcastPins(pinA, pinB);
  if (port.begin(pinA)) {
    return true;
  }

  // Match AUTO's recovery path: if Joybus left PIO state behind, unload and retry.
  releaseJoybusPioForDreamcast();
  resetDreamcastPins(pinA, pinB);
  delay(2);
  return port.begin(pinA);
}

void warmupDreamcastPort(uint8_t pinA, uint8_t pinB) {
  MaplePort probe;
  if (!beginDreamcastPort(probe, pinA, pinB)) {
    return;
  }

  const uint16_t readOkStart = maple_read_ok;
  const uint32_t seenFuncStart = probe.getLastSeenFunction();
  const uint32_t start = millis();
  while ((millis() - start) < 250) {
    probe.update();
    if (probe.isConnected() ||
        probe.getLastSeenFunction() != seenFuncStart ||
        maple_read_ok != readOkStart) {
      break;
    }
    delay(4);
  }

  probe.end();
  resetDreamcastPins(pinA, pinB);
  delay(2);
}

void warmupActiveDreamcastPort(MaplePort& port) {
  const uint32_t start = millis();
  uint32_t connectedSince = 0;

  while ((millis() - start) < 700) {
    port.update();
    if (port.isConnected()) {
      if (connectedSince == 0) {
        connectedSince = millis();
      }
      // Keep the real port alive long enough for its accessory scan to run.
      if ((millis() - connectedSince) >= 80) {
        break;
      }
    }
    delay(4);
  }
}

}  // namespace

RZInputDreamcast::RZInputDreamcast() : RZInputModule() {}

RZInputDreamcast::~RZInputDreamcast() {
  for (uint8_t i = 0; i < input_ports; ++i) {
    delete maple[i];
    maple[i] = nullptr;
  }
}

const char* RZInputDreamcast::getUsbId() {
  return "RZRDC";
}

void RZInputDreamcast::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_DREAMCAST;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputDreamcast::getDescription() {
  return "Dreamcast";
}

void RZInputDreamcast::setup() {
  setInputPortCount(input_ports);
  // MaplePort applies its own connected/empty cadence. Keep the outer module
  // scheduler deterministic so Dreamcast never inherits another input mode's
  // static poll interval during a live Auto-mode transition.
  pollInterval = 500;
  prepareDreamcastManualBus();

#if DREAMCAST_SERIAL_DEBUG
  #if !defined(ADAPT_OUTPUT_USB_DEVICE)
  Serial.begin(115200);
  #endif
#endif

  for (uint8_t i = 0; i < input_ports; ++i) {
    maple[i] = new MaplePort();

    uint8_t pinA = input_dreamcast_config[i].pinA;
    uint8_t pinB = input_dreamcast_config[i].pinB;

    warmupDreamcastPort(pinA, pinB);
    initSuccess[i] = beginDreamcastPort(*maple[i], pinA, pinB);
    if (initSuccess[i]) {
      warmupActiveDreamcastPort(*maple[i]);
    }

    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      wasConnected[i] = false;
      lastConnectedMs[i] = 0;
      vmuPollQuietUntil[i] = 0;
      autoVmuScanDone[i] = false;
      autoVmuScanDueAt[i] = 0;
      frame.HAS_BTN_SELECT = 0;
      frame.HAS_BTN_START = 1;
      frame.HAS_ANALOG_TRIGGERS = 1;
      frame.HAS_ANALOG_STICK_MAIN = 1;
      frame.HAS_ANALOG_STICK_AUX = 0;
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
      setInputFrameConnected(frame, false);
    }
  }
}

void RZInputDreamcast::setup2() {
  for (uint8_t i = 0; i < input_ports && i < MAX_USB_OUT; ++i) {
    setPortLed(i, initSuccess[i] ? HIGH : LOW);
    controller_state_t& frame = inputFrame(i);
    setInputFrameTypeName(frame, getDebugStatus(i));
    setInputFrameConnected(frame, false);
  }
}
