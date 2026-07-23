#include "Input_Psx.h"
#include "input_psx_runtime_state.h"

#include "../../core/rumble_test_runtime.h"
#include "../../platform/latency_trace_gpio.h"
#ifdef ENABLE_INPUT_AUTODETECT
#include "../autodetect/input_autodetect_runtime_state.h"
#endif

namespace {

uint8_t psxDebugFlags(bool isMultitap, bool isNeGcon, bool isJogcon, bool isGuncon,
                      bool isMouse, bool isIRRemote, bool isDancePad) {
  uint8_t flags = 0;
  if (isNeGcon) flags |= PSX_DEBUG_FLAG_NEGCON;
  if (isJogcon) flags |= PSX_DEBUG_FLAG_JOGCON;
  if (isGuncon) flags |= PSX_DEBUG_FLAG_GUNCON;
  if (isMouse) flags |= PSX_DEBUG_FLAG_MOUSE;
  if (isIRRemote) flags |= PSX_DEBUG_FLAG_IR;
  if (isDancePad) flags |= PSX_DEBUG_FLAG_DANCE;
  if (isMultitap) flags |= PSX_DEBUG_FLAG_MULTITAP;
  return flags;
}

const char* psxProtocolDisplayName(PsxControllerProtocol protocol) {
  switch (protocol) {
    case PSPROTO_DUALSHOCK2:
      return "DualShock2";
    case PSPROTO_DUALSHOCK:
      return "DualShock";
    case PSPROTO_FLIGHTSTICK:
      return "FlightStick";
    case PSPROTO_JOGCON:
      return "JogCon";
    case PSPROTO_NEGCON:
      return "NeGcon";
    case PSPROTO_GUNCON:
      return "GunCon";
    case PSPROTO_MOUSE:
      return "Mouse";
    case PSPROTO_FISHING:
      return "Fishing";
    case PSPROTO_SPACEBALL:
      return "Spaceball";
    case PSPROTO_DIGITAL:
      return "Digital";
    default:
      return "PSX";
  }
}

bool isAutoResolvedPsxPort(uint8_t port) {
#ifdef ENABLE_INPUT_AUTODETECT
  return savedDeviceMode == RZORD_AUTODETECT &&
         deviceMode == RZORD_PSX &&
         input_auto_resolve_port == port;
#else
  (void)port;
  return false;
#endif
}

}  // namespace

void RZInputPSX::stageWebhidRawData(const uint8_t* raw, uint8_t len) {
  if (raw == nullptr) {
    return;
  }

  uint8_t count = len;
  if (count > sizeof(pendingWebhidRaw)) {
    count = sizeof(pendingWebhidRaw);
  }

  for (uint8_t i = 0; i < count; ++i) {
    pendingWebhidRaw[i] = raw[i];
  }
  for (uint8_t i = count; i < sizeof(pendingWebhidRaw); ++i) {
    pendingWebhidRaw[i] = 0;
  }
  pendingWebhidRawDirty = true;
}

void RZInputPSX::flushPendingWebhidRawData() {
  if (!pendingWebhidRawDirty) {
    return;
  }
  uint8_t raw[16] = {0};
  for (uint8_t i = 0; i < sizeof(pendingWebhidRaw); ++i) {
    raw[i] = pendingWebhidRaw[i];
  }
  webhid_store_raw_data(raw, 16);
  pendingWebhidRawDirty = false;
}

bool RZInputPSX::isAnyDualShock2PressureActive() const {
  for (uint8_t i = 0; i < logical_slots; ++i) {
    if (isDS2[i]) {
      return true;
    }
  }
  return false;
}

void RZInputPSX::setDualShock2PressureState(uint8_t index, bool enabled) {
  if (index >= logical_slots) {
    return;
  }

  isDS2[index] = enabled;
  controller_state_t& frame = inputFrame(index);
  frame.HAS_ANALOG_TRIGGERS = isDS2[index];
  frame.HAS_ANALOG_MAIN_BUTTONS = isDS2[index];
  frame.HAS_ANALOG_DPAD = isDS2[index];

  if (!enabled) {
    frame.ANALOG_PAD_U = 0;
    frame.ANALOG_PAD_D = 0;
    frame.ANALOG_PAD_L = 0;
    frame.ANALOG_PAD_R = 0;
    frame.ANALOG_A = 0;
    frame.ANALOG_B = 0;
    frame.ANALOG_X = 0;
    frame.ANALOG_Y = 0;
    frame.ANALOG_L1 = 0;
    frame.ANALOG_R1 = 0;
    frame.ANALOG_L2 = 0;
    frame.ANALOG_R2 = 0;
  }
}

void RZInputPSX::applyDualShock2Pressure(uint8_t index, PsxSingleController* controller) {
  if (index >= logical_slots || controller == nullptr) {
    return;
  }

  controller_state_t& frame = inputFrame(index);
  frame.HAS_ANALOG_TRIGGERS = isDS2[index];
  frame.HAS_ANALOG_MAIN_BUTTONS = isDS2[index];
  frame.HAS_ANALOG_DPAD = isDS2[index];
  if (!isDS2[index]) {
    return;
  }

  frame.ANALOG_PAD_U = controller->getAnalogButton(PSAB_PAD_UP);
  frame.ANALOG_PAD_D = controller->getAnalogButton(PSAB_PAD_DOWN);
  frame.ANALOG_PAD_L = controller->getAnalogButton(PSAB_PAD_LEFT);
  frame.ANALOG_PAD_R = controller->getAnalogButton(PSAB_PAD_RIGHT);
  frame.ANALOG_A = controller->getAnalogButton(PSAB_CROSS);
  frame.ANALOG_B = controller->getAnalogButton(PSAB_CIRCLE);
  frame.ANALOG_X = controller->getAnalogButton(PSAB_SQUARE);
  frame.ANALOG_Y = controller->getAnalogButton(PSAB_TRIANGLE);
  frame.ANALOG_L1 = controller->getAnalogButton(PSAB_L1);
  frame.ANALOG_R1 = controller->getAnalogButton(PSAB_R1);
  frame.ANALOG_L2 = controller->getAnalogButton(PSAB_L2);
  frame.ANALOG_R2 = controller->getAnalogButton(PSAB_R2);
}

void RZInputPSX::applyDualShock2Pressure(uint8_t index, PsxControllerData& controller) {
  if (index >= logical_slots) {
    return;
  }

  controller_state_t& frame = inputFrame(index);
  frame.HAS_ANALOG_TRIGGERS = isDS2[index];
  frame.HAS_ANALOG_MAIN_BUTTONS = isDS2[index];
  frame.HAS_ANALOG_DPAD = isDS2[index];
  if (!isDS2[index]) {
    return;
  }

  frame.ANALOG_PAD_U = controller.getAnalogButton(PSAB_PAD_UP);
  frame.ANALOG_PAD_D = controller.getAnalogButton(PSAB_PAD_DOWN);
  frame.ANALOG_PAD_L = controller.getAnalogButton(PSAB_PAD_LEFT);
  frame.ANALOG_PAD_R = controller.getAnalogButton(PSAB_PAD_RIGHT);
  frame.ANALOG_A = controller.getAnalogButton(PSAB_CROSS);
  frame.ANALOG_B = controller.getAnalogButton(PSAB_CIRCLE);
  frame.ANALOG_X = controller.getAnalogButton(PSAB_SQUARE);
  frame.ANALOG_Y = controller.getAnalogButton(PSAB_TRIANGLE);
  frame.ANALOG_L1 = controller.getAnalogButton(PSAB_L1);
  frame.ANALOG_R1 = controller.getAnalogButton(PSAB_R1);
  frame.ANALOG_L2 = controller.getAnalogButton(PSAB_L2);
  frame.ANALOG_R2 = controller.getAnalogButton(PSAB_R2);
}

void RZInputPSX::clearMultitapSlot(uint8_t slot) {
  if (slot >= MAX_USB_OUT) {
    return;
  }

  controller_state_t& frame = inputFrame(slot);
  haveController[slot] = false;
  lastProto[slot] = PSPROTO_UNKNOWN;
  rumbleConfiguredProto[slot] = PSPROTO_UNKNOWN;
  lastSuccessfulReadMs[slot] = 0;
  startupGraceUntilMs[slot] = 0;
  slotLastSeenMs[slot] = 0;
  setDualShock2PressureState(slot, false);
  isDancePad[slot] = false;
  specialDpadMask[slot] = 0;
  resetFishingState(slot);
  resetState(slot);
  setInputFrameConnected(frame, false);
  clearInputFrameTypeName(frame);
  input_psx_debug_clear_slot(slot);
  setUpdated(slot);
}

bool RZInputPSX::refreshMultitapMemoryCardPresence(uint32_t now) {
  if (!isMultitap ||
      multitapPhysicalPort >= input_ports ||
      !physical_port_enabled(multitapPhysicalPort)) {
    multitapMemoryCardPresent = false;
    multitapMemoryCardLastProbeMs = now;
    return false;
  }

  multitapMemoryCardPresent = detectMemoryCardPhysicalPresenceOnPort(multitapPhysicalPort);
  multitapMemoryCardLastProbeMs = now;
  if (multitapMemoryCardPresent) {
    multitapPhysicalPresent = true;
  }
  return multitapMemoryCardPresent;
}

void RZInputPSX::markMultitapSlotMissing(uint8_t slot, uint32_t now) {
  if (slot >= MAX_USB_OUT) {
    return;
  }

  if (!haveController[slot] && !inputFrameConst(slot).connected) {
    input_psx_debug_clear_slot(slot);
    return;
  }

  if (slotLastSeenMs[slot] != 0 &&
      (now - slotLastSeenMs[slot]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
    return;
  }

  clearMultitapSlot(slot);
}

void RZInputPSX::mapMultiController(uint8_t i, PsxControllerData& cont) {
  if (i >= MAX_USB_OUT) {
    return;
  }

  controller_state_t& frame = inputFrame(i);
  if (cont.protocol == PSPROTO_UNKNOWN) {
    markMultitapSlotMissing(i, millis());
    return;
  }

  haveController[i] = true;
  lastProto[i] = cont.protocol;
  lastSuccessfulReadMs[i] = millis();
  slotLastSeenMs[i] = lastSuccessfulReadMs[i];
  setInputFrameConnected(frame, true);
  setDualShock2PressureState(i, cont.protocol == PSPROTO_DUALSHOCK2);
  setPsxControllerTypeName(frame, psxProtocolDisplayName(cont.protocol));
  input_psx_debug_update_slot(
    i,
    true,
    (uint8_t)cont.protocol,
    (uint16_t)cont.buttonWord,
    0,
    psxDebugFlags(isMultitap, isNeGcon, isJogcon, isGuncon, isMouse,
                  isIRRemote, (i < logical_slots) ? isDancePad[i] : false)
  );

  byte lx = 0x80;
  byte ly = 0x80;
  byte rx = 0x80;
  byte ry = 0x80;
  if (cont.protocol != PSPROTO_DIGITAL) {
    cont.getLeftAnalog(lx, ly);
    cont.getRightAnalog(rx, ry);
  }

  frame.PAD_U = cont.buttonPressed(PSB_PAD_UP);
  frame.PAD_D = cont.buttonPressed(PSB_PAD_DOWN);
  frame.PAD_L = cont.buttonPressed(PSB_PAD_LEFT);
  frame.PAD_R = cont.buttonPressed(PSB_PAD_RIGHT);

  frame.LX = lx - 0x80;
  frame.LY = ly - 0x80;
  frame.RX = rx - 0x80;
  frame.RY = ry - 0x80;

  if (cont.protocol == PSPROTO_FISHING)
    applyFishingExperimentalMapping(i, cont);

  frame.A = cont.buttonPressed(PSB_CROSS);
  frame.B = cont.buttonPressed(PSB_CIRCLE);
  frame.X = cont.buttonPressed(PSB_SQUARE);
  frame.Y = cont.buttonPressed(PSB_TRIANGLE);
  frame.L1 = cont.buttonPressed(PSB_L1);
  frame.R1 = cont.buttonPressed(PSB_R1);
  frame.L2 = cont.buttonPressed(PSB_L2);
  frame.R2 = cont.buttonPressed(PSB_R2);
  frame.L3 = cont.buttonPressed(PSB_L3);
  frame.R3 = cont.buttonPressed(PSB_R3);
  frame.SELECT = cont.buttonPressed(PSB_SELECT);
  frame.START = cont.buttonPressed(PSB_START);
  applyDualShock2Pressure(i, cont);
}

bool RZInputPSX::is_port_connected(const uint8_t index) {
  if (index >= logical_slots) {
    return false;
  }
  return haveController[index];
}

void RZInputPSX::handleSpecialMask(uint8_t index) {
  if (index >= logical_slots) {
    return;
  }

  controller_state_t& frame = inputFrame(index);
  // For Pop'n Music and Guitar Freaks controllers, clear the d-pad directions
  // used for detection so they don't activate virtual buttons
  // Pop'n uses L+R+D for detection, Guitar Freaks uses L+R
  if (specialDpadMask[index] == 0)
    return;

  if (specialDpadMask[index] & PSB_PAD_RIGHT)
    frame.PAD_R = 0;
  if (specialDpadMask[index] & PSB_PAD_DOWN)
    frame.PAD_D = 0;
  if (specialDpadMask[index] & PSB_PAD_LEFT)
    frame.PAD_L = 0;
  // Note: PAD_UP is NOT cleared - it's the only usable d-pad direction for Pop'n
}

bool RZInputPSX::pollMultitap() {
  const uint32_t now = millis();
  const uint8_t tapPort = multitapPhysicalPort;
  if (tapPort >= input_ports || !physical_port_enabled(tapPort)) {
    multitapPhysicalPresent = false;
    multitapMemoryCardPresent = false;
    multitapMemoryCardLastProbeMs = now;
    for (byte ctrlId = 0; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
      markMultitapSlotMissing(ctrlId, now);
    }
    controllerType = CONT_NONE;
    return true;
  }

  PsxMultiController* tap = psxmulti[tapPort];
  PsxDriver* driver = psxControllerDriver[tapPort];
  if (tap == nullptr || driver == nullptr) {
    multitapPhysicalPresent = false;
    multitapMemoryCardPresent = false;
    multitapMemoryCardLastProbeMs = now;
    for (byte ctrlId = 0; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
      markMultitapSlotMissing(ctrlId, now);
    }
    controllerType = CONT_NONE;
    return true;
  }

  if (controllerType == CONT_NONE) {
    tap->begin(*driver);
    if (tap->enableMultiTap()) { // Found MultiTap
      multitapPhysicalPresent = true;
      bool anyControllerPresent = false;
      // Go through each port and test if there's a controller there
      PsxControllerData cont;
      for (byte i = 0; i < multitap_slots && i < MAX_USB_OUT; ++i) {
        cont.clear();
        if (tap->read(i, cont)) {
          if (cont.protocol != PSPROTO_UNKNOWN) {
            anyControllerPresent = true;
          }
          /* Single-controller read was fine, so controller must be
           * there, try to enable the analog sticks
           */
          if (tap->enterConfigMode(i)) {
            tap->enableAnalogSticks(i);
            tap->exitConfigMode(i);
          }
        }
      }

      if (!anyControllerPresent) {
        multitapPhysicalPresent = false;
        multitapMemoryCardPresent = false;
        multitapMemoryCardLastProbeMs = now;
        for (byte ctrlId = 0; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
          markMultitapSlotMissing(ctrlId, now);
        }
        controllerType = CONT_NONE;
        return true;
      }

      // Entering config mode disables MultiTap functions, enable them again
      if (!tap->enableMultiTap()) { // Cannot re-enable MultiTap
        multitapPhysicalPresent = false;
        multitapMemoryCardPresent = false;
        multitapMemoryCardLastProbeMs = now;
        for (byte ctrlId = 0; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
          markMultitapSlotMissing(ctrlId, now);
        }
        controllerType = CONT_NONE;
      } else {
        multitapPhysicalPresent = true;
        controllerType = CONT_MULTIPLE;
      }
    } else { // Naaah, looks like it is a single controller
      multitapPhysicalPresent = false;
      multitapMemoryCardPresent = false;
      multitapMemoryCardLastProbeMs = now;
      bool singleControllerPresent = false;
      if (tap->begin(*driver)) {
        PsxControllerData controller;
        if (tap->read(0, controller)) { // Found Single Controller
          singleControllerPresent = true;
          if (tap->enterConfigMode(0)) {
            tap->enableAnalogSticks(0);
            tap->exitConfigMode(0);
          }

          controllerType = CONT_SINGLE;
        }
      }
      const byte firstMissingSlot = singleControllerPresent ? 1 : 0;
      for (byte ctrlId = firstMissingSlot; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
        markMultitapSlotMissing(ctrlId, now);
      }
    }
  } else if (controllerType == CONT_MULTIPLE) {
    PsxControllerData* controllers;

    if (!tap->readAll(&controllers)) {
      multitapPhysicalPresent = false;
      multitapMemoryCardPresent = false;
      multitapMemoryCardLastProbeMs = now;
      for (byte ctrlId = 0; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
        markMultitapSlotMissing(ctrlId, now);
      }
      controllerType = CONT_NONE;
    } else { // multiple
      multitapPhysicalPresent = true;
      for (byte ctrlId = 0; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
        PsxControllerData& cont = controllers[ctrlId];
        if (cont.protocol == PSPROTO_UNKNOWN) {
          markMultitapSlotMissing(ctrlId, now);
        } else {
          mapMultiController(ctrlId, cont);
        }
      }
    }
  } else if (controllerType == CONT_SINGLE) {
    PsxControllerData controller;

    if (!tap->read(0, controller)) {
      multitapPhysicalPresent = false;
      multitapMemoryCardPresent = false;
      multitapMemoryCardLastProbeMs = now;
      for (byte ctrlId = 0; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
        markMultitapSlotMissing(ctrlId, now);
      }
      controllerType = CONT_NONE;
    } else {
      multitapPhysicalPresent = false;
      multitapMemoryCardPresent = false;
      multitapMemoryCardLastProbeMs = now;
      mapMultiController(0, controller);
      for (byte ctrlId = 1; ctrlId < multitap_slots && ctrlId < MAX_USB_OUT; ++ctrlId) {
        markMultitapSlotMissing(ctrlId, now);
      }
    }
  }
  return true;
}

bool RZInputPSX::poll() {
  debugPollLoops++;
  beginPollCycle();

  if (psxMemoryCardBridgeActive()) {
    return endPollCycle();
  }

  if (isMultitap) {
    pollInterval = 1000; // 1 ms
    setUpdated(0);
    setUpdated(1);
    setUpdated(2);
    setUpdated(3);
    pollMultitap();
    return endPollCycle();
  }

  if (isJogcon) {
    pollInterval = 0; // 0.5
    controller_state_t& frame = inputFrame(0);
    if (!haveController[0]) {
      init_jogcon();
      if (haveController[0]) {
        markControllerAlive(0);
      }
    } else if (!psx[0]->read()) {
      if (shouldDropController(0)) {
        markControllerDisconnected(0);
      }
    } else {
      markControllerAlive(0);
      handleJogconData();
    }

    setInputFrameConnected(frame, haveController[0]);
    if (haveController[0])
      update_jogcon_display_name();
    else
      clearInputFrameTypeName(frame);
    if (haveController[0]) {
      input_psx_debug_update_slot(
        0,
        true,
        (uint8_t)psx[0]->getProtocol(),
        jogcon_newbtn,
        0,
        psxDebugFlags(isMultitap, isNeGcon, isJogcon, isGuncon, isMouse,
                      isIRRemote, isDancePad[0])
      );
    } else {
      input_psx_debug_clear_slot(0);
    }
    setUpdated(0);
    return endPollCycle();
  }

  for (uint8_t port = 0; port < input_ports; ++port) {
    uint8_t i = port;
    controller_state_t& frame = inputFrame(i);

    if (!should_poll_port(input_ports, port)) {
      debugPollSkipped[i]++;
      continue;
    }

    bool isReadSuccess = false;

    if (!haveController[i]) {
      debugBeginAttempt[i]++;
      refreshPsxBusDrivers();
      if (psx[i]->begin(*psxControllerDriver[i])) { // controller found
        debugBeginSuccess[i]++;
        delay(1); // needed?
        haveController[i] = true;
        armControllerStartupGrace(i);
        markControllerAlive(i);
        resetFishingState(i);
        clearPhysicalFallbackLatches();
        tryEnableAnalogMode(i);
        tryEnableRumble(i);
        lastProto[i] = psx[i]->getProtocol();
        updateControllerAttentionInterval(i, lastProto[i]);
      }
    } else { // have controller
      //Right (small) motor: 0 or 1 (off/on)
      //left (large) motor: 0 to 255
      if (!isWaiwai) {
        uint8_t effective_left = 0;
        uint8_t effective_right = 0;
        rumbleRuntimeGetEffectiveFeedback(i, &effective_left, &effective_right);
        RumbleRuntimePortDiag rumble_diag{};
        const bool rumble_test_active =
          rumbleRuntimeGetPortDiag(i, &rumble_diag) && rumble_diag.test_active;
        if (!rumble_test_active && (effective_left == 0) != (effective_right == 0)) {
          const uint8_t single_channel = effective_left | effective_right;
          effective_left = single_channel;
          effective_right = single_channel;
        }
        #ifdef PSX_COMBINE_RUMBLE
          // Combine both motors into single signal
          psx[i]->setRumble((effective_left | effective_right) != 0, (effective_left | effective_right));
        #else
          psx[i]->setRumble(effective_right ? 1 : 0, effective_left);
        #endif
      }

      // waiwai
      // send command on rumble data
      // returns some data on rx and ry.
      PsxControllerProtocol proto = psx[i]->getProtocol();
      if (proto == PSPROTO_MOUSE) {
        // Mouse input is intentionally deferred; do not let a PSX mouse look
        // like a connected generic controller in normal builds.
        markControllerDisconnected(i);
        resetFishingState(i);
        input_psx_debug_clear_slot(i);
        setInputFrameConnected(frame, false);
        clearInputFrameTypeName(frame);
        setUpdated(i);
        continue;
      }

      // Use readIR() for IR receiver mode, read() for regular controllers
      {
        LatencyPhaseTraceScope psxReadTrace(LATENCY_TRACE_PHASE_PSX_READ);
        if (isIRRemote) {
          isReadSuccess = psx[i]->readIR();
        } else {
          isReadSuccess = psx[i]->read();
        }
      }

      if (!isReadSuccess) {
        debugReadFail[i]++;
        if (shouldDropController(i)) {
        markControllerDisconnected(i);
        resetFishingState(i);
        input_psx_debug_clear_slot(i);
      }
      }

      if (haveController[i] && isReadSuccess) {
        LatencyPhaseTraceScope psxMapTrace(LATENCY_TRACE_PHASE_PSX_MAP);
        debugReadSuccess[i]++;
        markControllerAlive(i);
        resetState(i);
        proto = psx[i]->getProtocol();
        if (lastProto[i] != proto)
          tryEnableRumble(i);
        lastProto[i] = proto;
        updateControllerAttentionInterval(i, proto);

        if (isNeGcon) {
          loopNeGcon(i);
        } else if (isGuncon) {
          loopGuncon();
        } else if (isMouse) {
          // PSX Mouse: report relative movement and buttons
          int8_t mouseX;
          int8_t mouseY;
          uint8_t mouseButtons;
          if (psx[i]->getMouseData(mouseX, mouseY, mouseButtons)) {
            frame.mouse_x = mouseX;
            frame.mouse_y = mouseY;
            // Map mouse buttons: bit 0=Right, bit 1=Left (after un-inversion)
            // Map to A=Left click, B=Right click (matching Saturn/SNES mouse)
            frame.A = (mouseButtons >> 1) & 1; // Left click
            frame.B = (mouseButtons >> 0) & 1; // Right click
          }
        } else if (isIRRemote) {
          // PS2 IR Remote: receiver acts as digital controller in 0x42 mode
          // Button mapping is handled internally by the receiver
          frame.PAD_U = psx[i]->buttonPressed(PSB_PAD_UP);
          frame.PAD_D = psx[i]->buttonPressed(PSB_PAD_DOWN);
          frame.PAD_L = psx[i]->buttonPressed(PSB_PAD_LEFT);
          frame.PAD_R = psx[i]->buttonPressed(PSB_PAD_RIGHT);
          frame.A = psx[i]->buttonPressed(PSB_CROSS);
          frame.B = psx[i]->buttonPressed(PSB_CIRCLE);
          frame.X = psx[i]->buttonPressed(PSB_SQUARE);
          frame.Y = psx[i]->buttonPressed(PSB_TRIANGLE);
          frame.L1 = psx[i]->buttonPressed(PSB_L1);
          frame.R1 = psx[i]->buttonPressed(PSB_R1);
          frame.L2 = psx[i]->buttonPressed(PSB_L2);
          frame.R2 = psx[i]->buttonPressed(PSB_R2);
          frame.L3 = psx[i]->buttonPressed(PSB_L3);
          frame.R3 = psx[i]->buttonPressed(PSB_R3);
          frame.SELECT = psx[i]->buttonPressed(PSB_SELECT);
          frame.START = psx[i]->buttonPressed(PSB_START);
        } else if (isCrazyHit) { // map it as a pop n music controller
          frame.A = (psx[i]->buttonPressed(PSB_PAD_UP) && psx[i]->buttonPressed(PSB_PAD_LEFT));
          frame.B = (psx[i]->buttonPressed(PSB_PAD_UP) && psx[i]->buttonPressed(PSB_L1));
          frame.X = (psx[i]->buttonPressed(PSB_CROSS) && psx[i]->buttonPressed(PSB_SQUARE));
          frame.Y = (psx[i]->buttonPressed(PSB_PAD_UP) && psx[i]->buttonPressed(PSB_L2));
          frame.L1 = (psx[i]->buttonPressed(PSB_PAD_DOWN) && psx[i]->buttonPressed(PSB_SQUARE));
          frame.R1 = (psx[i]->buttonPressed(PSB_PAD_RIGHT) && psx[i]->buttonPressed(PSB_SQUARE));
          frame.L2 = (psx[i]->buttonPressed(PSB_TRIANGLE) && psx[i]->buttonPressed(PSB_R2));
          frame.R2 = (psx[i]->buttonPressed(PSB_CIRCLE) && psx[i]->buttonPressed(PSB_TRIANGLE));
          frame.L3 = 0;
          frame.R3 = 0;
          frame.SELECT = psx[i]->buttonPressed(PSB_SELECT);
          frame.START = psx[i]->buttonPressed(PSB_START);
          frame.PAD_U = (psx[i]->buttonPressed(PSB_TRIANGLE) && psx[i]->buttonPressed(PSB_R1));
          frame.PAD_D = (dpad_mode == DPAD_MODE_BUTTONS);
          frame.PAD_L = (dpad_mode == DPAD_MODE_BUTTONS);
          frame.PAD_R = (dpad_mode == DPAD_MODE_BUTTONS);
          frame.LX = 0;
          frame.LY = 0;
          frame.RX = 0;
          frame.RY = 0;
        } else {
          frame.HAS_ANALOG_TRIGGERS = 0;      //contains analog L2 and R2
          frame.HAS_ANALOG_MAIN_BUTTONS = 0;  //contains analog buttons (except L2 and R2)
          frame.HAS_ANALOG_DPAD = 0;          //contains analog DPAD

          //enable crazy hit to popn map by pressing the 3 button combo:
          //   0 X 0 0
          //  0 X X 0 0
          if (!isCrazyHit && i == 0 && specialDpadMask[0] == 0 && proto == PSPROTO_DIGITAL &&
              psx[i]->buttonPressed(PSB_PAD_UP) && psx[i]->buttonPressed(PSB_PAD_LEFT) &&
              psx[i]->buttonPressed(PSB_PAD_RIGHT) && psx[i]->buttonPressed(PSB_PAD_DOWN) &&
              psx[i]->buttonPressed(PSB_SQUARE)) {
            isCrazyHit = true;
          }

          byte lx;
          byte ly;
          byte rx;
          byte ry;
          psx[i]->getLeftAnalog(lx, ly);
          psx[i]->getRightAnalog(rx, ry);

          frame.PAD_U = psx[i]->buttonPressed(PSB_PAD_UP);
          frame.PAD_D = psx[i]->buttonPressed(PSB_PAD_DOWN);
          frame.PAD_L = psx[i]->buttonPressed(PSB_PAD_LEFT);
          frame.PAD_R = psx[i]->buttonPressed(PSB_PAD_RIGHT);

          frame.LX = lx - 0x80;
          frame.LY = ly - 0x80;
          frame.RX = rx - 0x80;
          frame.RY = ry - 0x80;

          if (proto == PSPROTO_FISHING) {
            applyFishingExperimentalMapping(i, psx[i]);
          } else {
            frame.spinner = 0;
            frame.paddle = 0x80;
          }

          // Pop'n and GuitarFreaks controllers don't have analog sticks - zero the axes
          if (specialDpadMask[i] == SPECIALMASK_POPN || specialDpadMask[i] == SPECIALMASK_JET) {
            frame.LX = 0;
            frame.LY = 0;
            frame.RX = 0;
            frame.RY = 0;
          }

          frame.A = psx[i]->buttonPressed(PSB_CROSS);
          frame.B = psx[i]->buttonPressed(PSB_CIRCLE);
          frame.X = psx[i]->buttonPressed(PSB_SQUARE);
          frame.Y = psx[i]->buttonPressed(PSB_TRIANGLE);
          frame.L1 = psx[i]->buttonPressed(PSB_L1);
          frame.R1 = psx[i]->buttonPressed(PSB_R1);
          frame.L2 = psx[i]->buttonPressed(PSB_L2);
          frame.R2 = psx[i]->buttonPressed(PSB_R2);
          frame.L3 = psx[i]->buttonPressed(PSB_L3);
          frame.R3 = psx[i]->buttonPressed(PSB_R3);
          frame.SELECT = psx[i]->buttonPressed(PSB_SELECT);
          frame.START = psx[i]->buttonPressed(PSB_START);

          setDualShock2PressureState(i, proto == PSPROTO_DUALSHOCK2 || isDS2[i]);
          applyDualShock2Pressure(i, psx[i]);
        }

        // Store raw data for WebHID debugging
        {
          uint8_t raw[16] = {0};
          PsxControllerProtocol currentProto = psx[i]->getProtocol();
          uint16_t buttons = psx[i]->getButtonWord();
          input_psx_debug_update_slot(
            i,
            haveController[i],
            (uint8_t)currentProto,
            buttons,
            specialDpadMask[i],
            psxDebugFlags(isMultitap, isNeGcon, isJogcon, isGuncon, isMouse,
                          isIRRemote, isDancePad[i])
          );
          byte lx;
          byte ly;
          byte rx;
          byte ry;
          psx[i]->getLeftAnalog(lx, ly);
          psx[i]->getRightAnalog(rx, ry);
          raw[0] = (uint8_t)currentProto;    // Controller protocol
          raw[1] = buttons & 0xFF;           // Buttons low byte
          raw[2] = (buttons >> 8) & 0xFF;    // Buttons high byte
          raw[3] = lx;                       // Left stick X
          raw[4] = ly;                       // Left stick Y
          raw[5] = rx;                       // Right stick X
          raw[6] = ry;                       // Right stick Y
          raw[7] = i;                        // Port number
          if (currentProto == PSPROTO_FISHING) {
            byte fishingRaw[8];
            byte fishingRawLen = 0;
            if (psx[i]->getFishingRawData(fishingRaw, fishingRawLen)) {
              for (byte rawIndex = 0; rawIndex < fishingRawLen && rawIndex < 8; ++rawIndex) {
                raw[8 + rawIndex] = fishingRaw[rawIndex];  // Fishing reply bytes 5-12
              }
            }
          }

          stageWebhidRawData(raw, 16);
        }
      } // end read controller data
    } // end if have controller

    setInputFrameConnected(frame, haveController[i]);
    if (!haveController[i]) {
      input_psx_debug_clear_slot(i);
    }

    // Pop'n and Guitar Freaks detection (sticky once detected)
    // Pop'n: L+R+D held on Digital controller
    // Guitar Freaks: L+R held on Digital mode, or on DualShock with the
    // right analog X axis pinned to max (0xFF)
    if (haveController[i] && specialDpadMask[i] == 0 && !isCrazyHit && !isDancePad[i]) {
      const PsxControllerProtocol proto = psx[i]->getProtocol();
      uint16_t digitalData = psx[i]->getButtonWord();

      // Pop'n: Digital controller with L+R+D held
      if (proto == PSPROTO_DIGITAL && (digitalData & SPECIALMASK_POPN) == SPECIALMASK_POPN) {
        specialDpadMask[i] = SPECIALMASK_POPN;
      } else if (isGuitarFreaksSignature(psx[i], proto, digitalData)) {
        specialDpadMask[i] = SPECIALMASK_JET;
      }
    }

    if (haveController[i] && specialDpadMask[i] == SPECIALMASK_JET) {
      applyGuitarFreaksMapping(i);
    }

    // Dance pad detection: U + D pressed simultaneously (impossible on regular d-pad)
    // Pop'n holds L+R+D, Guitar Freaks holds L+R - exclude those cases
    // Note: Auto-detection may not be reliable; user may need to select dance pad mode manually
    // Once detected, stays detected until controller disconnects
    if (haveController[i] && !isDancePad[i] && !isCrazyHit && specialDpadMask[i] == 0) {
      bool upDown = frame.PAD_U && frame.PAD_D;
      bool leftRight = frame.PAD_L && frame.PAD_R;
      if (upDown && !leftRight) {
        isDancePad[i] = true;
        // Force settings for dance pad mode
        dpad_mode = DPAD_MODE_BUTTONS;
        socdMode = SOCD_OFF;
      }
    }

    // Set controller type name for display
    if (haveController[i]) {
      if (isDancePad[i]) {
        setPsxControllerTypeName(frame, "Dance Pad");
      } else if (isNeGcon) {
        setPsxControllerTypeName(frame, "NeGcon");
      } else if (isGuncon) {
        setPsxControllerTypeName(frame, "GunCon");
      } else if (isMouse) {
        setPsxControllerTypeName(frame, "Mouse");
      } else if (isIRRemote) {
        setPsxControllerTypeName(frame, "IR Remote");
      } else if (isWaiwai) {
        setPsxControllerTypeName(frame, "WaiWai");
      } else if (isCrazyHit) {
        setPsxControllerTypeName(frame, "PopN");
      } else if (specialDpadMask[i] == SPECIALMASK_POPN) {
        setPsxControllerTypeName(frame, "Pop'n");
      } else if (specialDpadMask[i] == SPECIALMASK_JET) {
        setPsxControllerTypeName(frame, "GuitarFreaks");
      } else if (isDS2[i]) {
        setPsxControllerTypeName(frame, "DualShock2");
      } else {
        const PsxControllerProtocol proto = psx[i]->getProtocol();
        switch (proto) {
          case PSPROTO_DUALSHOCK2:
            setPsxControllerTypeName(frame, "DualShock2");
            break;
          case PSPROTO_DUALSHOCK:
            setPsxControllerTypeName(frame, "DualShock");
            break;
          case PSPROTO_FLIGHTSTICK:
            setPsxControllerTypeName(frame, "FlightStick");
            break;
          case PSPROTO_JOGCON:
            setPsxControllerTypeName(frame, "JogCon");
            break;
          case PSPROTO_DIGITAL:
            setPsxControllerTypeName(frame, "Digital");
            break;
          case PSPROTO_FISHING:
            setPsxControllerTypeName(frame, "Fishing");
            break;
          case PSPROTO_SPACEBALL:
            setPsxControllerTypeName(frame, "Spaceball");
            break;
          default:
            setPsxControllerTypeName(frame, "PSX");
            break;
        }
      }
    } else {
      // Preserve controller type name if specialDpadMask is set (sticky detection)
      // This prevents flickering when controller read temporarily fails
      if (specialDpadMask[i] == SPECIALMASK_POPN) {
        setPsxControllerTypeName(frame, "Pop'n");
      } else if (specialDpadMask[i] == SPECIALMASK_JET) {
        setPsxControllerTypeName(frame, "GuitarFreaks");
      } else {
        clearInputFrameTypeName(frame);
      }
    }

    handleSpecialMask(i);
    updateControllerAttentionInterval(i, lastProto[i]);

    setUpdated(i); //todo only updade when state changes
  } // end for ports

  //pollInterval = 16000; // 16 ms
  //pollInterval = 1000; // 1 ms
  pollInterval = 500; // 0.5 ms

  //static const uint8_t PS_INTERVAL_DEFAULT  = 250;
  //static const uint16_t PS_INTERVAL_JET  = 1000;
  //static const uint16_t PS_INTERVAL_DS2  = 3000;
  if (isAnyDualShock2PressureActive())
    pollInterval = 1000;

  //todo check all ports for special mask
  if (specialDpadMask[0] == SPECIALMASK_JET)
    pollInterval = 1000; // 1ms

  //pollInterval = 2000; // Microbike. 1.8ms seems ok. 2ms to be sure
  //pollInterval = 16666; // Taito TCPP-20001 ddgo one handle. requires 16ms and usage of ACK pin

  return endPollCycle();
}

void RZInputPSX::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  flushPendingWebhidRawData();
}
