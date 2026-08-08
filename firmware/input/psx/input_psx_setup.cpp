#include "Input_Psx.h"

#include "../../product_config.h"
#include "../../core/controller_settings_state.h"
#include "../../core/turbo.h"
#include "../../menu/menu_runtime_state.h"
#include <PsxNewLib/PsxDriverHwSpi.h>
#include <PsxNewLib/PsxDriverHwSpiWithAck.h>
#ifdef ENABLE_INPUT_AUTODETECT
#include "../autodetect/input_autodetect_support.h"
#include "../autodetect/input_autodetect_runtime_state.h"
#endif

namespace {
constexpr uint8_t kPsxSetupDetectAttempts = 2;
constexpr uint8_t kPsxSetupRetryDelayMs = 8;
constexpr uint8_t kPsxMultitapSetupDetectAttempts = 5;
constexpr uint8_t kPsxMultitapSetupRetryDelayMs = 16;
constexpr uint8_t kPsxPostControllerBeginDelayMs = 5;
constexpr uint16_t kPsxDefaultAttentionIntervalUs = 250;
constexpr uint16_t kPsxJetAttentionIntervalUs = 1000;
constexpr uint16_t kPsxDualShock2AttentionIntervalUs = 3000;
constexpr uint16_t kPsxAutoResolvedPhysicalProbeIntervalMs = 250;

uint8_t autoResolvedPsxPortHint(uint8_t input_ports) {
  #ifdef ENABLE_INPUT_AUTODETECT
  if (savedDeviceMode == RZORD_AUTODETECT &&
      deviceMode == RZORD_PSX &&
      input_auto_resolve_port < input_ports) {
    return input_auto_resolve_port;
  }
  #else
  (void)input_ports;
  #endif
  return 0xFF;
}

uint8_t psxDebugTransferByte(const input_psx_config_t& c, uint8_t txByte) {
  uint8_t rxByte = 0;

  for (uint8_t i = 0; i < 8; ++i) {
    gpio_put(c.cmd, (txByte >> i) & 0x01);
    delayMicroseconds(2);

    gpio_put(c.clk, LOW);
    delayMicroseconds(2);

    gpio_put(c.clk, HIGH);
    delayMicroseconds(2);

    if (gpio_get(c.dat)) {
      rxByte |= (1u << i);
    }
  }

  return rxByte;
}

void psxDebugRawPoll(const input_psx_config_t& c, byte* in, size_t len) {
  static const byte poll[] = {0x01, 0x42, 0x00, 0xFF, 0xFF};
  if (in == nullptr || len < sizeof(poll)) {
    return;
  }

  gpio_init(c.att);
  gpio_init(c.cmd);
  gpio_init(c.dat);
  gpio_init(c.clk);
  gpio_init(c.ack);

  gpio_set_dir(c.att, GPIO_OUT);
  gpio_set_dir(c.cmd, GPIO_OUT);
  gpio_set_dir(c.dat, GPIO_IN);
  gpio_set_dir(c.clk, GPIO_OUT);
  gpio_set_dir(c.ack, GPIO_IN);
  gpio_pull_up(c.dat);
  gpio_pull_up(c.ack);

  gpio_put(c.att, HIGH);
  gpio_put(c.clk, HIGH);
  gpio_put(c.cmd, HIGH);
  delayMicroseconds(50);

  gpio_put(c.att, LOW);
  delayMicroseconds(20);

  for (size_t i = 0; i < sizeof(poll); ++i) {
    in[i] = psxDebugTransferByte(c, poll[i]);
    delayMicroseconds(20);
  }

  gpio_put(c.att, HIGH);
  gpio_set_dir(c.att, GPIO_IN);
  gpio_set_dir(c.cmd, GPIO_IN);
  gpio_set_dir(c.clk, GPIO_IN);
}
}  // namespace

RZInputPSX::RZInputPSX() : RZInputModule() { }

const char* RZInputPSX::getUsbId() {
  return output_effective_psx_usb_id(isIRRemote);
}

void RZInputPSX::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_PSX;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputPSX::getDescription() {
  return "PSX";
}

void RZInputPSX::configureControllerFrame(uint8_t i) {
  if (i >= MAX_USB_OUT) {
    return;
  }

  controller_state_t& frame = inputFrame(i);
  frame.HAS_BTN_SELECT = 1;
  frame.HAS_BTN_START = 1;
  frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
}

void RZInputPSX::stopRumble(uint8_t i) {
  if (i >= input_ports || psx[i] == nullptr) {
    return;
  }

  psx[i]->setRumble(false, 0);
  psx[i]->read();
}

void RZInputPSX::tryEnableRumble(uint8_t i) {
  if (i >= input_ports || psx[i] == nullptr) {
    return;
  }

  const PsxControllerProtocol proto = psx[i]->getProtocol();
  if (proto != PSPROTO_DUALSHOCK &&
      proto != PSPROTO_DUALSHOCK2 &&
      proto != PSPROTO_JOGCON) {
    rumbleConfiguredProto[i] = PSPROTO_UNKNOWN;
    return;
  }

  if (rumbleConfiguredProto[i] == proto) {
    return;
  }

  //if ((options.inputMode == INPUT_MODE_XINPUT || detectDS2) && !isJogcon){ //try to enable rumble
  if (!isJogcon) { //try to enable rumble
    psx[i]->setRumble(false, 0);
    if (psx[i]->enterConfigMode()) {
//      if (isWaiwai) // not required but handy if needing to check the return data (on rx/ry)
//        psx[i]->enableAnalogSticks();
      const bool rumbleEnabled = psx[i]->enableRumble();
      setDualShock2PressureState(i, false);
      psx[i]->exitConfigMode();
      stopRumble(i);
      if (rumbleEnabled) {
        rumbleConfiguredProto[i] = proto;
      }
    }
  }
}

bool RZInputPSX::tryEnableAnalogMode(uint8_t i) {
  if (i >= input_ports || psx[i] == nullptr)
    return false;

  if (psx[i]->getProtocol() != PSPROTO_DIGITAL)
    return false;

  if (!psx[i]->enterConfigMode())
    return false;

  const bool enabled = psx[i]->enableAnalogSticks();
  psx[i]->exitConfigMode();

  if (enabled) {
    delay(1);
    psx[i]->read();
  }

  return enabled;
}

void RZInputPSX::updateControllerAttentionInterval(uint8_t i, PsxControllerProtocol proto) {
  if (i >= input_ports || psxControllerDriver[i] == nullptr) {
    return;
  }

  uint16_t interval = kPsxDefaultAttentionIntervalUs;
  if (i < logical_slots && (proto == PSPROTO_DUALSHOCK2 || isDS2[i])) {
    interval = kPsxDualShock2AttentionIntervalUs;
  } else if (i < logical_slots && specialDpadMask[i] == SPECIALMASK_JET) {
    interval = kPsxJetAttentionIntervalUs;
  }

  psxControllerDriver[i]->setAttentionInterval(interval);
}

void RZInputPSX::clearPhysicalFallbackLatches() {
  isMultitap = false;
  multitapPhysicalPresent = false;
  multitapPhysicalPort = 0;
  multitapMemoryCardPresent = false;
  multitapMemoryCardLastProbeMs = 0;
  autoResolvedPhysicalProbePresent = false;
  autoResolvedPhysicalProbeLastMs = 0;
}

bool RZInputPSX::isGuitarFreaksSignature(PsxSingleController* controller,
                                         PsxControllerProtocol proto,
                                         uint16_t digitalData) const {
  if ((digitalData & SPECIALMASK_JET) != SPECIALMASK_JET)
    return false;

  if (proto == PSPROTO_DIGITAL)
    return true;

  if (proto == PSPROTO_DUALSHOCK) {
    byte analogX;
    byte analogY;
    return controller->getRightAnalog(analogX, analogY) && analogX == 0xFF;
  }

  return false;
}

void RZInputPSX::applyGuitarFreaksMapping(uint8_t index) {
  controller_state_t& frame = inputFrame(index);
  const bool redFretPressed = frame.R2 || frame.X || frame.A;
  uint8_t redFretAnalog = frame.ANALOG_R2;
  if (frame.ANALOG_X > redFretAnalog)
    redFretAnalog = frame.ANALOG_X;
  if (frame.ANALOG_A > redFretAnalog)
    redFretAnalog = frame.ANALOG_A;

  // Normalize the guitar so hosts see purple/green/red on Y/B/R2, with wailing on L2.
  frame.R2 = redFretPressed;
  frame.A = 0;
  frame.X = 0;
  frame.ANALOG_A = 0;
  frame.ANALOG_X = 0;
  frame.ANALOG_R2 = redFretAnalog;
}

uint16_t RZInputPSX::controllerDisconnectGraceMs() const {
  if (isJogcon)
    return 1200;
  if (isNeGcon || isGuncon || isMouse || isIRRemote)
    return 600;
  return 250;
}

uint16_t RZInputPSX::controllerStartupGraceMs() const {
  if (isJogcon)
    return 2200;
  if (isNeGcon || isGuncon || isMouse || isIRRemote)
    return 1800;
  return 1800;
}

void RZInputPSX::armControllerStartupGrace(uint8_t port) {
  if (port >= logical_slots) {
    return;
  }
  startupGraceUntilMs[port] = millis() + controllerStartupGraceMs();
}

void RZInputPSX::markControllerAlive(uint8_t port) {
  if (port >= logical_slots) {
    return;
  }
  lastSuccessfulReadMs[port] = millis();
}

bool RZInputPSX::shouldDropController(uint8_t port) const {
  if (port >= logical_slots) {
    return true;
  }

  if (lastSuccessfulReadMs[port] == 0)
    return true;

  if (startupGraceUntilMs[port] != 0 &&
      (int32_t)(millis() - startupGraceUntilMs[port]) < 0)
    return false;

  return (millis() - lastSuccessfulReadMs[port]) >= controllerDisconnectGraceMs();
}

void RZInputPSX::markControllerDisconnected(uint8_t port) {
  if (port >= logical_slots) {
    return;
  }

  debugDropCount[port]++;
  stopRumble(port);
  haveController[port] = false;
  lastProto[port] = PSPROTO_UNKNOWN;
  rumbleConfiguredProto[port] = PSPROTO_UNKNOWN;
  lastSuccessfulReadMs[port] = 0;
  startupGraceUntilMs[port] = 0;
  slotLastSeenMs[port] = 0;
  setDualShock2PressureState(port, false);
  isDancePad[port] = false;
  printf("lost\n");
}

void RZInputPSX::resetFishingState(uint8_t i) {
  if (i >= logical_slots) {
    return;
  }
  fishingReelPosition[i] = 0;
}

void RZInputPSX::applyFishingExperimentalMapping(uint8_t i, PsxSingleController* controller) {
  controller_state_t& frame = inputFrame(i);
  byte fishingRaw[8];
  byte fishingRawLen = 0;
  if (!controller->getFishingRawData(fishingRaw, fishingRawLen) || fishingRawLen < 8) {
    frame.spinner = 0;
    frame.paddle = 0x80;
    return;
  }

  int8_t reelDelta = (int8_t)fishingRaw[7];  // Reply byte 12
  if (reelDelta >= -1 && reelDelta <= 1)
    reelDelta = 0;

  int16_t scaledDelta = ((int16_t)reelDelta * spinner_speed_mult[spinner_speed]) / 4;
  if (scaledDelta == 0 && reelDelta != 0)
    scaledDelta = (reelDelta > 0) ? 1 : -1;

  fishingReelPosition[i] = constrain(fishingReelPosition[i] + scaledDelta, -128, 127);
  frame.RX = (int8_t)fishingReelPosition[i];
  frame.spinner = constrain((int16_t)reelDelta * 4, -127, 127);
  frame.paddle = (uint8_t)(fishingReelPosition[i] + 128);
}

void RZInputPSX::applyFishingExperimentalMapping(uint8_t i, PsxControllerData& cont) {
  controller_state_t& frame = inputFrame(i);
  byte fishingRaw[8];
  byte fishingRawLen = 0;
  if (!cont.getFishingRawData(fishingRaw, fishingRawLen) || fishingRawLen < 8) {
    frame.spinner = 0;
    frame.paddle = 0x80;
    return;
  }

  int8_t reelDelta = (int8_t)fishingRaw[7];  // Reply byte 12
  if (reelDelta >= -1 && reelDelta <= 1)
    reelDelta = 0;

  int16_t scaledDelta = ((int16_t)reelDelta * spinner_speed_mult[spinner_speed]) / 4;
  if (scaledDelta == 0 && reelDelta != 0)
    scaledDelta = (reelDelta > 0) ? 1 : -1;

  fishingReelPosition[i] = constrain(fishingReelPosition[i] + scaledDelta, -128, 127);
  frame.RX = (int8_t)fishingReelPosition[i];
  frame.spinner = constrain((int16_t)reelDelta * 4, -127, 127);
  frame.paddle = (uint8_t)(fishingReelPosition[i] + 128);
}

bool RZInputPSX::ensurePsxBusDriversReady(bool configureFrames) {
  if (configureFrames && !psxControllerFramesConfigured) {
    for (uint8_t i = 0; i < logical_slots; ++i) {
      configureControllerFrame(i);
    }
    psxControllerFramesConfigured = true;
  }

  if (memoryCardBridgeDriversReady) {
    return true;
  }

  for (uint8_t i = 0; i < input_ports; ++i) {
    if (psxDriver[i] == nullptr) {
      input_psx_config_t c = input_psx_config[i];
      psxDriver[i] = new PsxDriverHwSpiWithAck(c.spi, c.att, c.ack, c.cmd, c.dat, c.clk);
    }
    if (psxControllerDriver[i] == nullptr) {
      input_psx_config_t c = input_psx_config[i];
      psxControllerDriver[i] = new PsxDriverHwSpi(c.spi, c.att, c.ack, c.cmd, c.dat, c.clk); // without ACK
    }
    if (psx[i] == nullptr) {
      psx[i] = new PsxSingleController();
    }
    if (psxmulti[i] == nullptr) {
      psxmulti[i] = new PsxMultiController();
    }
  }

  return refreshPsxBusDrivers();
}

bool RZInputPSX::refreshPsxBusDrivers() {
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (psxDriver[i] == nullptr || !psxDriver[i]->begin() ||
        psxControllerDriver[i] == nullptr || !psxControllerDriver[i]->begin()) {
      return false;
    }
    psxControllerDriver[i]->setAttentionInterval(kPsxDefaultAttentionIntervalUs);
  }

  memoryCardBridgeDriversReady = true;
  return true;
}

bool RZInputPSX::setupMemoryCardBridgeOnly() {
  return ensurePsxBusDriversReady(false) && refreshPsxBusDrivers();
}

bool RZInputPSX::detectMemoryCardPhysicalPresenceOnPort(uint8_t port) {
  if (port >= input_ports || !physical_port_enabled(port)) {
    return false;
  }

  PSXMemoryCardRawProbe probe;
  for (uint8_t slot = 0; slot < PSX_MEMCARD_SLOTS_PER_PORT; ++slot) {
    if (probePSXMemoryCardRaw(port, slot, 0, &probe) &&
        probe.response[2] == 0x5A &&
        probe.response[3] == 0x5D) {
      return true;
    }
  }
  return false;
}

bool RZInputPSX::multitapHasController(uint8_t port) {
  if (port >= input_ports || psxmulti[port] == nullptr) {
    return false;
  }

  PsxControllerData controller;
  for (uint8_t slot = 0; slot < multitap_slots; ++slot) {
    controller.clear();
    if (psxmulti[port]->read(slot, controller) &&
        controller.protocol != PSPROTO_UNKNOWN) {
      return true;
    }
  }
  return false;
}

bool RZInputPSX::autoResolvedPhysicalConnectionActive() const {
#ifdef ENABLE_INPUT_AUTODETECT
  if (savedDeviceMode != RZORD_AUTODETECT ||
      deviceMode != RZORD_PSX ||
      input_auto_resolve_port >= input_ports ||
      !physical_port_enabled(input_auto_resolve_port)) {
    return false;
  }

  const uint32_t now = millis();
  if ((uint32_t)(now - autoResolvedPhysicalProbeLastMs) <
      kPsxAutoResolvedPhysicalProbeIntervalMs) {
    return autoResolvedPhysicalProbePresent;
  }

  autoResolvedPhysicalProbeLastMs = now;
  autoResolvedPhysicalProbePresent =
      detectAutoInputPortPsxOnly(input_auto_resolve_port, true) == AUTODETECT_PSX;
  const_cast<RZInputPSX*>(this)->refreshPsxBusDrivers();
  return autoResolvedPhysicalProbePresent;
#else
  return false;
#endif
}

bool RZInputPSX::multitapPhysicalConnectionActive() const {
  return isMultitap &&
         (multitapPhysicalPresent ||
          multitapMemoryCardPresent);
}

bool RZInputPSX::hasPhysicalConnectionForHotSwap() const {
  return multitapPhysicalConnectionActive() ||
         autoResolvedPhysicalConnectionActive();
}

const char* RZInputPSX::physicalConnectionDisplayName() const {
  if (multitapPhysicalConnectionActive()) {
    return multitapMemoryCardPresent ? "Multitap+Mem" : "Multitap";
  }
  if (autoResolvedPhysicalConnectionActive()) {
    return "PSX Device";
  }
  return nullptr;
}

void RZInputPSX::setup() {
  setInputPortCount(input_ports);

  empty_port_behaviour = EMPTY_PORT_USE_INTERVAL;
  polling_empty_interval_ms = 500;

  controllerType = CONT_NONE;
  for (uint8_t i = 0; i < logical_slots; ++i) {
    setDualShock2PressureState(i, false);
  }

  // Force dance pad mode if selected as input mode
  #ifdef ENABLE_INPUT_PSX_DANCE
  if (deviceMode == RZORD_PSX_DANCE) {
    for (uint8_t i = 0; i < input_ports; ++i) {
      isDancePad[i] = true;
    }
    dpad_mode = DPAD_MODE_BUTTONS;
    socdMode = SOCD_OFF;
  }
  #endif

  if (!ensurePsxBusDriversReady(true)) {
    #ifdef USE_I2C_DISPLAY
      Wire.begin();
      display.clear();
      display.print(F("Cannot initialize psx driver"));
      Wire.end();
    #endif
    panic("Cannot initialize psx driver");
  }

  isMultitap = false;
  multitapPhysicalPresent = false;
  multitapPhysicalPort = 0;
  multitapMemoryCardPresent = false;
  multitapMemoryCardLastProbeMs = 0;
  autoResolvedPhysicalProbePresent = false;
  autoResolvedPhysicalProbeLastMs = 0;

  const uint8_t autoPsxPortHint = autoResolvedPsxPortHint(input_ports);
  PsxControllerProtocol proto = PSPROTO_UNKNOWN;
  uint8_t controllerSetupPort = 0xFF;
  const uint8_t preferredControllerPort =
      (autoPsxPortHint != 0xFF && physical_port_enabled(autoPsxPortHint))
          ? autoPsxPortHint
          : 0;

  auto trySetupSingleController = [&]() {
    for (uint8_t attempt = 0; attempt < kPsxSetupDetectAttempts && controllerSetupPort == 0xFF; ++attempt) {
      for (uint8_t scan = 0; scan < input_ports; ++scan) {
        const uint8_t port = (uint8_t)((preferredControllerPort + scan) % input_ports);
        if (!physical_port_enabled(port) || psx[port] == nullptr ||
            psxControllerDriver[port] == nullptr) {
          continue;
        }
        if (psx[port]->begin(*psxControllerDriver[port])) {
          haveController[port] = true;
          armControllerStartupGrace(port);
          markControllerAlive(port);
          controllerSetupPort = port;
          break;
        }
      }
      if (controllerSetupPort == 0xFF && attempt + 1 < kPsxSetupDetectAttempts) {
        delay(kPsxSetupRetryDelayMs);
      }
    }
  };

  // Single-controller replies are stronger evidence than a multitap fallback.
  // Always exhaust this path first so normal DualShock/analog pads are not
  // disturbed by the heavier multitap probe sequence.
  refreshPsxBusDrivers();
  trySetupSingleController();

  // Try each enabled physical PSX connector. A multitap occupies that
  // connector and its sockets become logical USB player slots.
  if (controllerSetupPort == 0xFF) {
    for (uint8_t attempt = 0; attempt < kPsxMultitapSetupDetectAttempts && !isMultitap; ++attempt) {
      for (uint8_t port = 0; port < input_ports; ++port) {
        if (!physical_port_enabled(port) || psxmulti[port] == nullptr ||
            psxControllerDriver[port] == nullptr) {
          continue;
        }

        psxmulti[port]->begin(*psxControllerDriver[port]);
        if (psxmulti[port]->enableMultiTap() && multitapHasController(port)) {
          multitapPhysicalPort = port;
          isMultitap = true;
          break;
        }
      }

      if (!isMultitap && attempt + 1 < kPsxMultitapSetupDetectAttempts) {
        delay(kPsxMultitapSetupRetryDelayMs);
      }
    }
  }

  if (isMultitap) {
    setInputPortCount(min(multitap_slots, (uint8_t)MAX_USB_OUT));
    multitapPhysicalPresent = true;
    multitapMemoryCardPresent = detectMemoryCardPhysicalPresenceOnPort(multitapPhysicalPort);
    multitapMemoryCardLastProbeMs = millis();
    return;
  }

  if (controllerSetupPort == 0xFF) {
    if (autoPsxPortHint != 0xFF && physical_port_enabled(autoPsxPortHint)) {
      // AUTO already observed a strict PSX reply on this connector. Remember
      // that for hotswap status, but do not present a memory-card-only or
      // no-controller multitap route as an active PSX input mode.
      autoResolvedPhysicalProbePresent = true;
      autoResolvedPhysicalProbeLastMs = millis();
    }
  }

  //detect controller
  if (controllerSetupPort == 0xFF) {
    trySetupSingleController();
  }

  if (controllerSetupPort != 0xFF) {
    const uint8_t i = controllerSetupPort;
    clearPhysicalFallbackLatches();
    rumbleConfiguredProto[i] = PSPROTO_UNKNOWN;
    delay(kPsxPostControllerBeginDelayMs);

    //if not forced a mode, then read from currenct connected controller
    if (proto == PSPROTO_UNKNOWN)
      proto = psx[i]->getProtocol();

    if (proto == PSPROTO_GUNCON) {
      isGuncon = true;
    } else if (proto == PSPROTO_MOUSE) {
      // PSX mouse input is not supported by this release.
      haveController[i] = false;
    } else if (proto == PSPROTO_NEGCON) {
      isNeGcon = true;
    } else if (proto == PSPROTO_JOGCON) {
      isJogcon = true;
      if (psx[i]->buttonPressed(PSB_L2))
        enableMouseMove = true;
      // neGcon MiSTer mode is fixed on the MiSTer/Linux (DInput) path
    } else { //jogcon can't be detected during boot as it needs to be in analog mode. also waiwai
      //Try to detect by it's id
      if (proto == PSPROTO_DIGITAL) {
        // Try to detect IR receiver using 0x04 command (returns 0x12 identifier)
        // IR receiver responds to both 0x01 (as digital controller) and 0x61 (as IR receiver)
        if (psx[i]->detectIR()) {
          isIRRemote = true;
        } else {
          tryEnableAnalogMode(i);
          proto = psx[i]->getProtocol();
          if (proto == PSPROTO_NEGCON) {
            isNeGcon = true;
          } else if (proto == PSPROTO_JOGCON) {
            isJogcon = true;
            if (psx[i]->buttonPressed(PSB_L2))
              enableMouseMove = true;
          } else if (psx[i]->enterConfigMode()) {
            auto ct = psx[i]->getControllerType();
            if (ct == PSCTRL_JOGCON) {
              isJogcon = true;
              if (psx[i]->buttonPressed(PSB_L2))
                enableMouseMove = true;
            } else if (ct == PSCTRL_WAIWAI) {
              isWaiwai = true;
            }
            psx[i]->exitConfigMode();
          }
        }
      }

      if (!isIRRemote && !isNeGcon && !isJogcon && psx[i]->buttonPressed(PSB_SELECT)) { //dualshock used in guncon mode to help map axis on emulators.
        isGuncon = true;
      }
    }

    lastProto[i] = proto;

    if (!isIRRemote) {
      tryEnableRumble(i);
    }
  } else { //no controller connected
    if (proto == PSPROTO_JOGCON)
      isJogcon = true;
  }

  if (isNeGcon) {
    turbo.setInputMode(TURBO_MODE_PSX_NEGCON);
    #ifdef PRODUCT_CLASSIC2USB
    menu_classic_dual_merge = 0;
    classic_dual_merge_enabled = 0;
    #endif
    negconSetup();
  } else if (isJogcon) {
    turbo.setInputMode(TURBO_MODE_PSX_JOG);
    jogconSetup();
  } else if (isGuncon) {
    turbo.setInputMode(TURBO_MODE_PSX);
    gunconSetup();
  } else { //dualshock [default]
    turbo.setInputMode(TURBO_MODE_PSX);
    const uint8_t i = controllerSetupPort == 0xFF ? 0 : controllerSetupPort;
    uint16_t digitalData = psx[i]->getButtonWord();

    if (proto == PSPROTO_DIGITAL && (digitalData & SPECIALMASK_POPN) == SPECIALMASK_POPN)
      specialDpadMask[i] = SPECIALMASK_POPN;
    else if (isGuitarFreaksSignature(psx[i], proto, digitalData))
      specialDpadMask[i] = SPECIALMASK_JET;

    setInputPortCount(input_ports);//MAX_USB_STICKS;
  }

  // Keep PSX peripherals on their MiSTer-style descriptors whenever the
  // base USB mode is the generic MiSTer/Linux (DInput) HID path.
  output_promote_psx_peripheral_mode(isJogcon, isNeGcon, isGuncon);
}

void RZInputPSX::setup2() {
  if (isNeGcon) {
    negconSetup2();
  } else if (isJogcon) {
    jogconSetup2();
  } else if (isGuncon) {
    gunconSetup2();
  }

  // Multitap mode exposes tap sockets as logical slots from the occupied
  // physical connector, so the normal physical-port LED pass does not apply.
  if (!isMultitap)
    for (uint8_t i = 0; i < input_ports; ++i)
      setPortLed(i, HIGH);
}

void RZInputPSX::printDebugStatus(Print& out) const {
  out.print(F("STATUS PSX MT="));
  out.print(isMultitap ? 1 : 0);
  out.print(F(" MTPHYS="));
  out.print(multitapPhysicalPresent ? 1 : 0);
  out.print(F(" MTPORT="));
  out.print((int)multitapPhysicalPort);
  out.print(F(" MTMEM="));
  out.print(multitapMemoryCardPresent ? 1 : 0);
  out.print(F(" ARPHYS="));
  out.print(autoResolvedPhysicalProbePresent ? 1 : 0);
  out.print(F(" CTRL="));
  for (uint8_t i = 0; i < logical_slots; ++i) {
    out.print(haveController[i] ? '1' : '0');
  }
  out.print(F(" RESPORT="));
  #ifdef ENABLE_INPUT_AUTODETECT
  if (input_auto_resolve_port == 0xFF) {
    out.print(F("-"));
  } else {
    out.print((int)input_auto_resolve_port);
  }
  #else
  out.print(F("-"));
#endif
  out.print(F(" POLL="));
  out.print(debugPollLoops);
  out.print(F(" SK="));
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (i) out.print(',');
    out.print(debugPollSkipped[i]);
  }
  out.print(F(" BA="));
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (i) out.print(',');
    out.print(debugBeginAttempt[i]);
  }
  out.print(F(" BO="));
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (i) out.print(',');
    out.print(debugBeginSuccess[i]);
  }
  out.print(F(" RS="));
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (i) out.print(',');
    out.print(debugReadSuccess[i]);
  }
  out.print(F(" RF="));
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (i) out.print(',');
    out.print(debugReadFail[i]);
  }
  out.print(F(" DR="));
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (i) out.print(',');
    out.print(debugDropCount[i]);
  }
  out.println();
}

void RZInputPSX::printDebugProbe(Print& out) {
  ensurePsxBusDriversReady(false);
  static const byte poll[] = {0x01, 0x42, 0x00, 0xFF, 0xFF};

  for (uint8_t port = 0; port < input_ports; ++port) {
    out.print(F("PSXPROBE P"));
    out.print((int)(port + 1));
    out.print(F(" EN="));
    out.print(physical_port_enabled(port) ? 1 : 0);

    byte raw[sizeof(poll)] = {};
    if (physical_port_enabled(port)) {
      psxDebugRawPoll(input_psx_config[port], raw, sizeof(raw));
      refreshPsxBusDrivers();
    }
    out.print(F(" RAW_RX="));
    for (byte i = 0; i < sizeof(raw); ++i) {
      if (i) {
        out.print(',');
      }
      if (raw[i] < 0x10) {
        out.print('0');
      }
      out.print(raw[i], HEX);
    }

    byte in[sizeof(poll)] = {};
    bool ok = false;
    if (physical_port_enabled(port) && psxControllerDriver[port] != nullptr) {
      psxControllerDriver[port]->selectController();
      ok = psxControllerDriver[port]->shiftInOut(poll, sizeof(poll), in, sizeof(in), false);
      psxControllerDriver[port]->deselectController();
    }
    out.print(F(" CTRL_OK="));
    out.print(ok ? 1 : 0);
    out.print(F(" CTRL_RX="));
    for (byte i = 0; i < sizeof(in); ++i) {
      if (i) {
        out.print(',');
      }
      if (in[i] < 0x10) {
        out.print('0');
      }
      out.print(in[i], HEX);
    }

    memset(in, 0, sizeof(in));
    ok = false;
    if (physical_port_enabled(port) && psxDriver[port] != nullptr) {
      psxDriver[port]->selectController();
      ok = psxDriver[port]->shiftInOut(poll, sizeof(poll), in, sizeof(in), false);
      psxDriver[port]->deselectController();
    }
    out.print(F(" ACK_OK="));
    out.print(ok ? 1 : 0);
    out.print(F(" ACK_RX="));
    for (byte i = 0; i < sizeof(in); ++i) {
      if (i) {
        out.print(',');
      }
      if (in[i] < 0x10) {
        out.print('0');
      }
      out.print(in[i], HEX);
    }

    bool beginOk = false;
    bool readOk = false;
    PsxControllerProtocol probeProto = PSPROTO_UNKNOWN;
    PsxButtons probeButtons = PSB_NONE;
    byte lx = 0x80;
    byte ly = 0x80;
    byte rx = 0x80;
    byte ry = 0x80;
    if (physical_port_enabled(port) && psxControllerDriver[port] != nullptr) {
      refreshPsxBusDrivers();
      PsxSingleController probeController;
      beginOk = probeController.begin(*psxControllerDriver[port]);
      if (beginOk) {
        readOk = probeController.read();
        probeProto = probeController.getProtocol();
        probeButtons = probeController.getButtonWord();
        probeController.getLeftAnalog(lx, ly);
        probeController.getRightAnalog(rx, ry);
      }
      refreshPsxBusDrivers();
    }
    out.print(F(" BEGIN_OK="));
    out.print(beginOk ? 1 : 0);
    out.print(F(" READ_OK="));
    out.print(readOk ? 1 : 0);
    out.print(F(" PROTO="));
    out.print((int)probeProto);
    out.print(F(" BTN=0x"));
    if ((uint16_t)probeButtons < 0x1000) out.print('0');
    if ((uint16_t)probeButtons < 0x0100) out.print('0');
    if ((uint16_t)probeButtons < 0x0010) out.print('0');
    out.print((uint16_t)probeButtons, HEX);
    out.print(F(" AX="));
    out.print((int)lx);
    out.print(',');
    out.print((int)ly);
    out.print(',');
    out.print((int)rx);
    out.print(',');
    out.print((int)ry);
    out.println();
  }
}
