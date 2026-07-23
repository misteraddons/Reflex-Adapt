#include "Input_Snes.h"
#include "../runtime/input_frame_runtime.h"
#include "../shared/input_button_bits.h"

RZInputSnes::RZInputSnes() : RZInputModule() { }

bool RZInputSnes::confirmDisconnect(uint8_t index) {
  if (index >= MAX_USB_OUT)
    return false;
  if (disconnect_confirm_frames[index] < DISCONNECT_CONFIRM_FRAMES) {
    disconnect_confirm_frames[index]++;
    return false;
  }
  return true;
}

bool RZInputSnes::recentControllerSlotPresent() const {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    if (dtype[i] == SNES_DEVICE_NONE ||
        dtype[i] == SNES_DEVICE_NOTSUPPORTED ||
        slotLastSeenMs[i] == 0) {
      continue;
    }
    if ((uint32_t)(now - slotLastSeenMs[i]) <= HOTSWAP_RECENT_SLOT_GRACE_MS) {
      return true;
    }
  }
  return false;
}

bool RZInputSnes::is_port_connected(const uint8_t index) {
  if (index >= input_ports) {
    return false;
  }

  for (uint8_t slot = 0; slot < MAX_USB_OUT; ++slot) {
    if (slot_physical_port[slot] == index &&
        dtype[slot] != SNES_DEVICE_NONE &&
        dtype[slot] != SNES_DEVICE_NOTSUPPORTED) {
      return true;
    }
  }

  return false;
}

bool RZInputSnes::hasPhysicalConnectionForHotSwap() const {
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    if (dtype[i] != SNES_DEVICE_NONE &&
        dtype[i] != SNES_DEVICE_NOTSUPPORTED &&
        inputFrameConst(i).connected) {
      return true;
    }
  }
  return false;
}

const char* RZInputSnes::physicalConnectionDisplayName() const {
  if (!recentControllerSlotPresent()) {
    return nullptr;
  }
  switch (deviceMode) {
    #if defined(ENABLE_INPUT_NES)
    case RZORD_NES: return "NES Pad";
    #endif
    #if defined(ENABLE_INPUT_VBOY)
    case RZORD_VBOY: return "VB Pad";
    #endif
    default: return "SNES Pad";
  }
}

void RZInputSnes::applySnesFrameIdentity(uint8_t port, uint8_t index, controller_state_t& frame) {
  frame.HAS_BTN_HOME = 0;

  bool this_port_has_fourscore = (port == 0 && snes_debug_fourscore0)
                              || (port == 1 && snes_debug_fourscore1);
  bool this_port_has_multitap = (port == 0 && snes_debug_mtap_enabled0 && snes_debug_tap0 > 0)
                             || (port == 1 && snes_debug_mtap_enabled1 && snes_debug_tap1 > 0);
  if (this_port_has_fourscore) {
    setInputFrameTypeName(frame, "Four Score");
    return;
  }
  if (this_port_has_multitap) {
    setInputFrameTypeName(frame, "Multitap");
    if (index < MAX_USB_OUT && dtype[index] == SNES_DEVICE_NTT) {
      frame.HAS_BTN_HOME = 1;
    }
    return;
  }

  SnesDeviceType_Enum type = (index < MAX_USB_OUT) ? dtype[index] : SNES_DEVICE_NONE;
  switch (getInternalMode()) {
    case 1:
      if (type == SNES_DEVICE_NES_POWERPAD) {
        setInputFrameTypeName(frame, "Power Pad");
      } else if (type == SNES_DEVICE_NES_ZAPPER) {
        setInputFrameTypeName(frame, "Zapper");
      } else if (type == SNES_DEVICE_MOUSE) {
        setInputFrameTypeName(frame, "SNES Mouse");
      } else if (type == SNES_DEVICE_NES) {
        setInputFrameTypeName(frame, "NES Pad");
      } else if (type == SNES_DEVICE_NTT) {
        setInputFrameTypeName(frame, "NTT Data");
        frame.HAS_BTN_HOME = 1;
      } else if (type == SNES_DEVICE_VB) {
        setInputFrameTypeName(frame, "VB Pad");
      } else if (type == SNES_DEVICE_PAD) {
        setInputFrameTypeName(frame, "SNES Pad");
      } else {
        setInputFrameTypeName(frame, "NES Pad");
      }
      break;
    case 2:
      if (type == SNES_DEVICE_MOUSE) {
        setInputFrameTypeName(frame, "SNES Mouse");
      } else if (type == SNES_DEVICE_NTT) {
        setInputFrameTypeName(frame, "NTT Data");
        frame.HAS_BTN_HOME = 1;
      } else if (type == SNES_DEVICE_NES) {
        setInputFrameTypeName(frame, "NES Pad");
      } else if (type == SNES_DEVICE_VB) {
        setInputFrameTypeName(frame, "VB Pad");
      } else {
        setInputFrameTypeName(frame, "SNES Pad");
      }
      break;
    case 3:
      if (type == SNES_DEVICE_VB) {
        setInputFrameTypeName(frame, "VB Pad");
      } else if (type == SNES_DEVICE_MOUSE) {
        setInputFrameTypeName(frame, "SNES Mouse");
      } else if (type == SNES_DEVICE_NTT) {
        setInputFrameTypeName(frame, "NTT Data");
        frame.HAS_BTN_HOME = 1;
      } else if (type == SNES_DEVICE_NES) {
        setInputFrameTypeName(frame, "NES Pad");
      } else if (type == SNES_DEVICE_PAD) {
        setInputFrameTypeName(frame, "SNES Pad");
      } else {
        setInputFrameTypeName(frame, "VB Pad");
      }
      break;
    default:
      clearInputFrameTypeName(frame);
      break;
  }
}

void RZInputSnes::clearDisconnectConfirm(uint8_t index) {
  if (index < MAX_USB_OUT) {
    disconnect_confirm_frames[index] = 0;
  }
}

void RZInputSnes::clearTypeCandidate(uint8_t index) {
  if (index < MAX_USB_OUT) {
    candidate_type[index] = SNES_DEVICE_NONE;
    candidate_type_frames[index] = 0;
  }
}

void RZInputSnes::noteGlitchFrame(uint8_t index) {
  if (index < MAX_USB_OUT && glitch_frames[index] < 0xFFFF) {
    glitch_frames[index]++;
  }
}

bool RZInputSnes::snesRumbleTechEnabled() const {
  // RumbleTech presents as a normal SNES pad. Support is automatic in SNES
  // mode; the old persisted opt-in bit is ignored for runtime behavior.
  return deviceMode == RZORD_SNES;
}

void RZInputSnes::requestSafeSnesPoll(uint8_t port, uint32_t now_ms) {
  if (port >= input_ports) {
    return;
  }
  safe_poll_until_ms[port] = now_ms + SNES_SAFE_POLL_HOLD_MS;
}

bool RZInputSnes::snesSafePollActive(uint32_t now_ms) const {
  for (uint8_t port = 0; port < input_ports; ++port) {
    if (safe_poll_until_ms[port] != 0 &&
        (int32_t)(safe_poll_until_ms[port] - now_ms) > 0) {
      return true;
    }
  }
  return false;
}

SnesDeviceType_Enum RZInputSnes::confirmStableType(uint8_t index, SnesDeviceType_Enum observedType) {
  if (index >= MAX_USB_OUT) {
    return observedType;
  }

  if (dtype[index] == SNES_DEVICE_NONE ||
      dtype[index] == SNES_DEVICE_NOTSUPPORTED ||
      observedType == dtype[index]) {
    clearTypeCandidate(index);
    return observedType;
  }

  if (candidate_type[index] != observedType) {
    candidate_type[index] = observedType;
    candidate_type_frames[index] = 1;
  } else if (candidate_type_frames[index] < TYPE_CHANGE_CONFIRM_FRAMES) {
    candidate_type_frames[index]++;
  }

  if (candidate_type_frames[index] >= TYPE_CHANGE_CONFIRM_FRAMES) {
    clearTypeCandidate(index);
    return observedType;
  }

  return dtype[index];
}

void RZInputSnes::setDisconnected(uint8_t index, SnesDeviceType_Enum newType) {
  if (index >= MAX_USB_OUT)
    return;
  controller_state_t& frame = inputFrame(index);
  dtype[index] = newType;
  slotLastSeenMs[index] = 0;
  slot_physical_port[index] = 0xFF;
  stable_digital[index] = 0;
  setSnesPadDebugRaw(index, 0xFF, 0, 0);
  setSnesPadDebugFiltered(index, 0);
  pending_non_dpad_press[index] = 0;
  pending_non_dpad_press_frames[index] = 0;
  digital_filter_initialized[index] = false;
  clearTypeCandidate(index);
  setInputFrameConnected(frame, false);
  clearInputFrameTypeName(frame);
  clearDisconnectConfirm(index);
  setUpdated(index);
}

SnesDeviceType_Enum RZInputSnes::effectiveDeviceType(const SnesController& sc) {
  return sc.deviceType();
}

bool RZInputSnes::isStableSnesPadTransientType(uint8_t index, SnesDeviceType_Enum observedType) {
  return getInternalMode() == 2 &&
         index < MAX_USB_OUT &&
         dtype[index] == SNES_DEVICE_PAD &&
         (observedType == SNES_DEVICE_NES || observedType == SNES_DEVICE_NTT);
}

uint16_t RZInputSnes::stabilizeSnesPadDigital(uint8_t index, uint16_t rawDigital, SnesDeviceType_Enum type) {
  if (index >= MAX_USB_OUT) {
    return rawDigital;
  }

  uint16_t rawDpad = rawDigital & SNES_DPAD_DIGITAL_MASK;
  const bool impossibleDpad =
    ((rawDpad & (SNES_UP | SNES_DOWN)) == (SNES_UP | SNES_DOWN)) ||
    ((rawDpad & (SNES_LEFT | SNES_RIGHT)) == (SNES_LEFT | SNES_RIGHT));
  if (impossibleDpad) {
    rawDigital &= ~SNES_DPAD_DIGITAL_MASK;
    rawDpad = 0;
    noteSnesPadFilterDrop(index);
  }

  // SNES Pad digital reads are passed through without held-button confirmation.
  // The stable copy is only kept so obvious skipped glitch frames can reuse the
  // last valid value for debug/output fallback.
  (void)type;
  stable_digital[index] = rawDigital;
  setSnesPadDebugFiltered(index, rawDigital);
  pending_non_dpad_press[index] = 0;
  pending_non_dpad_press_frames[index] = 0;
  digital_filter_initialized[index] = true;
  return rawDigital;
}

SnesDeviceType_Enum RZInputSnes::normalizeVirtualBoyObservedType(uint8_t index, const SnesController& sc, SnesDeviceType_Enum observedType) {
  const bool stableVirtualBoy = index < MAX_USB_OUT && dtype[index] == SNES_DEVICE_VB;
  const bool virtualBoyMode = getInternalMode() == 3;
  const bool snesFamilyAlias =
      observedType == SNES_DEVICE_PAD ||
      observedType == SNES_DEVICE_NTT ||
      observedType == SNES_DEVICE_MOUSE ||
      observedType == SNES_DEVICE_NOTSUPPORTED;

  // Virtual Boy A/B share the ID nibble. In VB mode, do not let active-button
  // frames reclassify the controller as another SNES-family device.
  if ((stableVirtualBoy || virtualBoyMode) && snesFamilyAlias) {
    return SNES_DEVICE_VB;
  }

  return observedType;
}

uint8_t RZInputSnes::virtualBoyABBits(const SnesController& sc) {
  uint8_t source = (sc.currentState.id == 0x4)
    ? static_cast<uint8_t>(sc.currentState.extended)
    : sc.currentState.id;
  return source & 0x03;
}

bool RZInputSnes::isRumbleTechDetected(uint8_t port) {
  if (port >= input_ports || getInternalMode() != 2)
    return false;
  if (snes[port]->getMultitapPorts() || snes[port]->isPowerPadMode() ||
      snes[port]->isZapperMode() || snes[port]->isFourScoreDetected()) {
    return false;
  }
  if (snes[port]->getControllerCount() == 0)
    return false;

  const SnesController& sc = snes[port]->getSnesController(0);
  // RumbleTech presents as a normal SNES pad on input reads; there is no unique ID.
  return (sc.deviceType() == SNES_DEVICE_PAD);
}

bool RZInputSnes::isRumbleTechLatched(uint8_t port) const {
  return (port < input_ports && rumble_detect_confidence[port] >= RUMBLE_DETECT_THRESHOLD);
}

void RZInputSnes::updateRumbleTechDetection(uint8_t port, bool detected_now) {
  if (port >= input_ports)
    return;

  uint8_t& confidence = rumble_detect_confidence[port];
  if (detected_now) {
    if (confidence < RUMBLE_DETECT_MAX)
      confidence++;
  } else if (confidence > 0) {
    confidence--;
  }

  bool detected = (confidence >= RUMBLE_DETECT_THRESHOLD);
  if (port == 0) {
    snes_rumbletech_detected0 = detected;
  } else if (port == 1) {
    snes_rumbletech_detected1 = detected;
  }
}

void RZInputSnes::clearRumbleTechDetection() {
  for (uint8_t port = 0; port < input_ports; ++port) {
    rumble_detect_confidence[port] = 0;
  }
  snes_rumbletech_detected0 = false;
  snes_rumbletech_detected1 = false;
}

bool __not_in_flash_func(RZInputSnes::standardPadFastPathEligible)(uint8_t port, uint8_t index, const SnesController& sc,
                                                                    uint8_t portControllerCount, bool safePollActive) {
  if (port >= input_ports || index >= MAX_USB_OUT || safePollActive) {
    return false;
  }
  if (portControllerCount != 1 || snes[port]->getMultitapPorts() ||
      snes[port]->isPowerPadMode() || snes[port]->isZapperMode()) {
    return false;
  }

  const SnesDeviceType_Enum observedType = sc.deviceType();
  switch (getInternalMode()) {
    case 1:
      return dtype[index] == SNES_DEVICE_NES && observedType == SNES_DEVICE_NES;
    case 2:
      return dtype[index] == SNES_DEVICE_PAD && observedType == SNES_DEVICE_PAD;
    default:
      return false;
  }
}

void __not_in_flash_func(RZInputSnes::applyStandardPadFastPathIdentity)(controller_state_t& frame, bool nesMode) {
  frame.HAS_BTN_HOME = 0;

  const char* name = nesMode ? "NES Pad" : "SNES Pad";
  uint8_t i = 0;
  bool matches = true;
  for (; i < sizeof(frame.controller_type_name) - 1 && name[i] != '\0'; ++i) {
    if (frame.controller_type_name[i] != name[i]) {
      matches = false;
      break;
    }
  }
  if (matches && name[i] == '\0' && frame.controller_type_name[i] == '\0') {
    return;
  }

  for (i = 0; i < sizeof(frame.controller_type_name) - 1 && name[i] != '\0'; ++i) {
    frame.controller_type_name[i] = name[i];
  }
  frame.controller_type_name[i] = '\0';
}

static uint32_t __not_in_flash_func(mapStandardSnesDigitalToButtons)(uint16_t digital, bool nesMode) {
  uint32_t buttons = 0;
  buttons |= (digital & SNES_UP)     ? INPUT_PAD_U  : 0;
  buttons |= (digital & SNES_DOWN)   ? INPUT_PAD_D  : 0;
  buttons |= (digital & SNES_LEFT)   ? INPUT_PAD_L  : 0;
  buttons |= (digital & SNES_RIGHT)  ? INPUT_PAD_R  : 0;
  buttons |= (digital & SNES_SELECT) ? INPUT_SELECT : 0;
  buttons |= (digital & SNES_START)  ? INPUT_START  : 0;

  if (nesMode) {
    buttons |= (digital & SNES_B) ? INPUT_A : 0;
    buttons |= (digital & SNES_Y) ? INPUT_B : 0;
  } else {
    buttons |= (digital & SNES_A) ? INPUT_A  : 0;
    buttons |= (digital & SNES_B) ? INPUT_B  : 0;
    buttons |= (digital & SNES_X) ? INPUT_X  : 0;
    buttons |= (digital & SNES_Y) ? INPUT_Y  : 0;
    buttons |= (digital & SNES_L) ? INPUT_L1 : 0;
    buttons |= (digital & SNES_R) ? INPUT_R1 : 0;
  }

  return buttons;
}

void __not_in_flash_func(RZInputSnes::mapStandardPadFastPath)(uint8_t port, uint8_t index, const SnesController& sc, uint32_t nowMs) {
  controller_state_t& frame = inputFrame(index);
  const uint16_t digital = sc.currentState.digital;
  stable_digital[index] = digital;
  digital_filter_initialized[index] = true;
  setSnesPadDebugFiltered(index, digital);
  slotLastSeenMs[index] = nowMs;
  clearDisconnectConfirm(index);
  clearTypeCandidate(index);

  if (!sc.stateChanged() && frame.connected && frame.controller_type_name[0] != '\0') {
    return;
  }

  const bool nesMode = dtype[index] == SNES_DEVICE_NES;
  frame.digital_buttons = mapStandardSnesDigitalToButtons(digital, nesMode);
  frame.connected = true;
  applyStandardPadFastPathIdentity(frame, nesMode);
  setUpdated(index);
}

bool __not_in_flash_func(RZInputSnes::tryFastPollStandardNes)(uint8_t port, uint8_t index) {
  if (getInternalMode() != 1 || port >= input_ports || index >= MAX_USB_OUT) {
    return false;
  }
  if (snesSafePollActive(millis())) {
    return false;
  }
  if (dtype[index] != SNES_DEVICE_NES || snes[port]->getMultitapPorts() ||
      snes[port]->isFourScoreDetected() || snes[port]->isPowerPadMode() ||
      snes[port]->isZapperMode()) {
    nes_fast_validate_count[port] = 0;
    return false;
  }

  uint8_t& validateCount = nes_fast_validate_count[port];
  validateCount++;
  if (validateCount >= NES_FAST_FULL_VALIDATE_POLLS) {
    validateCount = 0;
    return false;
  }

  if (!snes[port]->fastPollStandardNes()) {
    validateCount = 0;
    return false;
  }
  return true;
}

uint8_t RZInputSnes::toSnesRumbleNibble(uint8_t value) const {
  if (value == 0)
    return 0;

  uint16_t scaled = (uint16_t(value) * 15u + 127u) / 255u;
  if (scaled == 0)
    scaled = 1;
  if (scaled > 15)
    scaled = 15;
  return (uint8_t)scaled;
}

void RZInputSnes::detectFourScoreDual() {
  // Four Score support disabled
}

uint8_t RZInputSnes::getInternalMode() {
  switch (deviceMode) {
    #if defined(ENABLE_INPUT_NES)
      case RZORD_NES:
        return 1;
    #endif
    #if defined(ENABLE_INPUT_SNES)
      case RZORD_SNES:
        return 2;
    #endif
    #if defined(ENABLE_INPUT_VBOY)
      case RZORD_VBOY:
        return 3;
    #endif
    default:
      return 0;
  }
}

const char* RZInputSnes::getUsbId() {
  switch (getInternalMode()) {
    case 1:
      return "RZRNes";
    case 2:
      return "RZRSnes";
    case 3:
      return "RZRVB";
  }
  return "";
}

void RZInputSnes::configureBcdDeviceVersion() {
  //todo also use max_devices
  switch (getInternalMode()) {
    case 1:
      bcd_device_version.platform = BCD_PLAT_NES;
      bcd_device_version.platform_sub = 0;
      break;
    case 2:
      bcd_device_version.platform = BCD_PLAT_SNES;
      bcd_device_version.platform_sub = 0;
      break;
    case 3:
      bcd_device_version.platform = BCD_PLAT_VBOY;
      bcd_device_version.platform_sub = 0;
      break;
  }
}

const char* RZInputSnes::getDescription() {
  switch (getInternalMode()) {
    case 1:
      return "NES";
    case 2:
      return "SNES";
    case 3:
      return "VBOY";
  }
  return "NES/SNES/VB";
}

void RZInputSnes::setup() {
  const uint8_t internalMode = getInternalMode();

  // NES, SNES, and Virtual Boy are all shift-register inputs. Keep the normal
  // read cadence fast; SNES RumbleTech temporarily moves to the slower safe
  // cadence only while a motor command is active.
  pollInterval = (internalMode == 1) ? NES_IDLE_POLL_INTERVAL_US : SNES_IDLE_POLL_INTERVAL_US;
  empty_port_behaviour = EMPTY_PORT_USE_INTERVAL;
  polling_empty_interval_ms = SNES_EMPTY_PORT_SCAN_INTERVAL_MS;

  setInputPortCount(input_ports);

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = inputFrame(i);
    dtype[i] = SNES_DEVICE_NONE;
    disconnect_confirm_frames[i] = 0;
    clearTypeCandidate(i);
    glitch_frames[i] = 0;
    slotLastSeenMs[i] = 0;
    stable_digital[i] = 0;
    slot_physical_port[i] = 0xFF;
    pending_non_dpad_press[i] = 0;
    pending_non_dpad_press_frames[i] = 0;
    digital_filter_initialized[i] = false;
    setInputFrameConnected(frame, false);
    clearInputFrameTypeName(frame);
  }

  for (uint8_t i = 0; i < input_ports; ++i) {
    input_snes_config_t c = input_snes_config[i];
    // Keep dat1 available in SNES mode so multitap detection/polling still works.
    // Rumble transmission is gated off when multitap is actually active.
    snes[i] = new SnesPort(c.clk, c.lat, c.dat0, c.dat1, c.sel);
    snes[i]->begin();
    last_rumble_data[i] = 0;
    rumble_detect_confidence[i] = 0;
    safe_poll_until_ms[i] = (internalMode == 2) ? millis() + SNES_SAFE_POLL_HOLD_MS : 0;
    nes_fast_validate_count[i] = 0;
    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      frame.HAS_BTN_SELECT = 1;
      frame.HAS_BTN_START = 1;
    }
  }
  clearRumbleTechDetection();

  bool found_mtap = false;
  bool found_fourscore = false;
  delay(100); // Allow multitap/fourscore to stabilize before detection

  // NES mode: detect Four Score adapter and handle Power Pad mode
  if (internalMode == 1) {
    // Check if Power Pad mode is enabled
    if (menu_powerpad_mode) {
      for (uint8_t i = 0; i < input_ports; ++i) {
        snes[i]->setPowerPadMode(true);
      }
    } else {
      // Detect Four Score (only when not in Power Pad mode)
      // Four Score detection needs both ports - reads D0 from each
      detectFourScoreDual();
      found_fourscore = fourScoreDetected;
      // Save Four Score debug info
      snes_debug_fourscore0 = fourScoreDetected;
      snes_debug_fourscore1 = fourScoreDetected;
    }
  }

  // SNES mode: detect multitap (skip if Four Score found in NES mode)
  if (!found_fourscore) {
    // Re-detect multitap after delay (initial detection in begin() may be too early)
    for (uint8_t i = 0; i < input_ports; ++i) {
      snes[i]->detectMultiTap();
    }
    // Save debug info
    if (input_ports > 0) {
      snes_debug_mtap_enabled0 = snes[0]->isMultitapEnabled();
      snes_debug_tap0 = snes[0]->getMultitapPorts();
    }
    if (input_ports > 1) {
      snes_debug_mtap_enabled1 = snes[1]->isMultitapEnabled();
      snes_debug_tap1 = snes[1]->getMultitapPorts();
    }
  }

  // Calculate total controller ports
  uint8_t total_controller_ports = 0;
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (snes[i]->isFourScoreDetected()) {
      // Four Score provides 4 controllers on one port
      total_controller_ports += 4;
      found_mtap = true;  // Reuse flag for poll interval
    } else {
      uint8_t tap = snes[i]->getMultitapPorts();
      if (tap) {
        total_controller_ports += tap;
        found_mtap = true;
      } else {
        total_controller_ports += 1;
      }
    }
  }
  setInputPortCount(min(total_controller_ports, MAX_USB_OUT));
  if (found_mtap || found_fourscore) {
    delay(50);
    pollInterval = (internalMode == 1) ? NES_IDLE_POLL_INTERVAL_US : SNES_IDLE_POLL_INTERVAL_US;
  }
}

void RZInputSnes::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);

  output_apply_switch_profile_for_snes_input_mode(getInternalMode(), nso_special);
}
