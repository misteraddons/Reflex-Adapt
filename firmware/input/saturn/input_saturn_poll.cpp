#include "Input_Saturn.h"

namespace {

constexpr uint8_t kSaturnAxisCenter = 0x80;
int8_t saturnSignedAxis(uint8_t raw) {
  return (int16_t)raw - (int16_t)kSaturnAxisCenter;
}

uint8_t saturnWheelRawAxis(const SaturnController& sc) {
  return sc.analog(SAT_ANALOG_X);
}

bool saturnTypeHasContinuousAnalog(SatDeviceType_Enum type) {
  switch (type) {
    case SAT_DEVICE_3DPAD:
    case SAT_DEVICE_WHEEL:
    case SAT_DEVICE_MISSION3:
#ifdef SATLIB_ENABLE_MISSION6
    case SAT_DEVICE_MISSION6:
#endif
      return true;
    default:
      return false;
  }
}

}  // namespace

bool RZInputSaturn::isMegadriveType(SatDeviceType_Enum type) const {
  return type == SAT_DEVICE_MEGA3 || type == SAT_DEVICE_MEGA6;
}

void RZInputSaturn::resetMegadriveTypeStability(uint8_t slot) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  stableMegadriveType[slot] = SAT_DEVICE_NONE;
  megadriveTypeDowngradeStartedMs[slot] = 0;
}

SatDeviceType_Enum RZInputSaturn::stabilizeMegadriveType(
    uint8_t slot,
    SatDeviceType_Enum detectedType,
    uint32_t now
) {
  if (slot >= MAX_USB_OUT) {
    return detectedType;
  }

  if (detectedType == SAT_DEVICE_MEGA6) {
    stableMegadriveType[slot] = SAT_DEVICE_MEGA6;
    megadriveTypeDowngradeStartedMs[slot] = 0;
    return SAT_DEVICE_MEGA6;
  }

  if (detectedType == SAT_DEVICE_MEGA3) {
    if (!isMegadriveType(stableMegadriveType[slot])) {
      stableMegadriveType[slot] = SAT_DEVICE_MEGA3;
      megadriveTypeDowngradeStartedMs[slot] = 0;
      return SAT_DEVICE_MEGA3;
    }

    if (stableMegadriveType[slot] == SAT_DEVICE_MEGA6) {
      if (megadriveTypeDowngradeStartedMs[slot] == 0) {
        megadriveTypeDowngradeStartedMs[slot] = now;
      }
      if ((uint32_t)(now - megadriveTypeDowngradeStartedMs[slot]) >=
          MEGA6_DOWNGRADE_DEBOUNCE_MS) {
        stableMegadriveType[slot] = SAT_DEVICE_MEGA3;
        megadriveTypeDowngradeStartedMs[slot] = 0;
        return SAT_DEVICE_MEGA3;
      }
      return SAT_DEVICE_MEGA6;
    }

    stableMegadriveType[slot] = SAT_DEVICE_MEGA3;
    megadriveTypeDowngradeStartedMs[slot] = 0;
    return SAT_DEVICE_MEGA3;
  }

  if (isMegadriveType(stableMegadriveType[slot]) &&
      (detectedType == SAT_DEVICE_NONE || detectedType == SAT_DEVICE_NOTSUPPORTED)) {
    return stableMegadriveType[slot];
  }

  if (!isMegadriveType(detectedType)) {
    resetMegadriveTypeStability(slot);
    return detectedType;
  }

  return detectedType;
}

bool __not_in_flash_func(RZInputSaturn::poll)() {
  beginPollCycle();

  uint8_t port_controller_count[input_ports] = {0};
  uint8_t controller_count = 0;
  uint8_t total_reserved_slots = 0;

  for (uint8_t port = 0; port < input_ports; ++port) {
    if (!should_poll_port(input_ports, port) || saturn[port] == nullptr) {
      total_reserved_slots += portSlotCapacity[port];
      input_saturn_debug_update_port(port, 0, lastTapPorts[port], 0, portSlotCapacity[port]);
      continue;
    }

    saturn[port]->update();
    port_controller_count[port] = saturn[port]->getControllerCount();
    uint32_t now_ms = millis();
    uint8_t currentTap = refreshTapPresence(port, saturn[port]->getMultitapPorts(), now_ms);
    uint8_t observedCapacity = currentTap ? currentTap : 1;
    if (observedCapacity > portSlotCapacity[port]) {
      portSlotCapacity[port] = observedCapacity;
    }
    input_saturn_debug_update_port(
        port, currentTap, lastTapPorts[port], port_controller_count[port], portSlotCapacity[port]);
    total_reserved_slots += portSlotCapacity[port];
    controller_count += port_controller_count[port];
  }
  setInputPortCount(min(total_reserved_slots, (uint8_t)MAX_USB_OUT));
  input_saturn_debug_set_total_reserved_slots(inputPortCount());

  #if defined(AUTODETECT_DEBUG_CDC)
  static uint32_t last_debug = 0;
  if (millis() - last_debug > 2000) {
    last_debug = millis();
    printf("Saturn poll: port0=%d port1=%d total=%d port_count=%d\n",
           port_controller_count[0], port_controller_count[1], controller_count, inputPortCount());
  }
  #endif

  uint8_t slotBase = 0;

  for (uint8_t port = 0; port < input_ports; ++port) {
    if (slotBase >= MAX_USB_OUT) {
      break;
    }

    uint8_t slotCapacity = portSlotCapacity[port];
    uint8_t reservedSlots = min(slotCapacity, (uint8_t)(MAX_USB_OUT - slotBase));
    uint8_t activeSlots = min(port_controller_count[port], reservedSlots);

    if (!physical_port_enabled(port) || saturn[port] == nullptr) {
      slotBase += reservedSlots;
      continue;
    }

    for (uint8_t c = 0; c < activeSlots; ++c) {
      uint8_t i = slotBase + c;
      controller_state_t& frame = inputFrame(i);
      const SaturnController& sc = saturn[port]->getSaturnController(c);
      SatDeviceType_Enum rawDetectedType = sc.deviceType();
      slotLastSeenMs[i] = millis();
      uint32_t now_ms = slotLastSeenMs[i];
      SatDeviceType_Enum detectedType = stabilizeMegadriveType(i, rawDetectedType, now_ms);
      input_saturn_debug_update_slot(i, (uint8_t)dtype[i], (uint8_t)rawDetectedType,
                                     (uint8_t)stableMegadriveType[i],
                                     megadriveTypeDowngradeStartedMs[i],
                                     port, c, saturn[port]->getMultitapPorts());

      if (detectedType == SAT_DEVICE_NOTSUPPORTED) {
        if (dtype[i] != SAT_DEVICE_NONE || frame.connected) {
          resetState(i);
          dtype[i] = SAT_DEVICE_NONE;
          resetMegadriveTypeStability(i);
          input_saturn_debug_clear_slot(i);
          slotLastSeenMs[i] = 0;
          setInputFrameConnected(frame, false);
          clearInputFrameTypeName(frame);
          setUpdated(i);
        }
        continue;
      }

      #ifdef AUTODETECT_DEBUG
      #if defined(AUTODETECT_DEBUG_CDC)
      static uint32_t last_satdbg = 0;
      if (i == 0 && (millis() - last_satdbg > 250)) {
        last_satdbg = millis();
        uint16_t d = sc.currentState.digital;
        Serial.printf("[SATDBG] p%d c%d usb%d dt=%u dig=%04X "
                      "UDLR=%u%u%u%u SACB=%u%u%u%u RXYZ=%u%u%u%u LR=%u%u st=%u\n",
                      port, c, i, (unsigned)sc.deviceType(), (unsigned)d,
                      (unsigned)sc.digitalPressed(SAT_PAD_UP),
                      (unsigned)sc.digitalPressed(SAT_PAD_DOWN),
                      (unsigned)sc.digitalPressed(SAT_PAD_LEFT),
                      (unsigned)sc.digitalPressed(SAT_PAD_RIGHT),
                      (unsigned)sc.digitalPressed(SAT_START),
                      (unsigned)sc.digitalPressed(SAT_A),
                      (unsigned)sc.digitalPressed(SAT_C),
                      (unsigned)sc.digitalPressed(SAT_B),
                      (unsigned)sc.digitalPressed(SAT_R),
                      (unsigned)sc.digitalPressed(SAT_Y),
                      (unsigned)sc.digitalPressed(SAT_X),
                      (unsigned)sc.digitalPressed(SAT_Z),
                      (unsigned)sc.digitalPressed(SAT_L),
                      (unsigned)sc.digitalPressed(SAT_R),
                      (unsigned)sc.digitalPressed(SAT_START));
      }
      #endif
      #endif

      bool justConnected = !frame.connected;
      if (justConnected) {
        setInputFrameConnected(frame, true);
        setUpdated(i);
      }

      bool typeChanged = (dtype[i] != detectedType);
      updateSaturnRawDebugRange(i, sc.currentState);

      const bool stateChanged = sc.stateChanged();
      const bool analogNeedsRefresh = saturnTypeHasContinuousAnalog(detectedType);

      if (stateChanged || justConnected || typeChanged || analogNeedsRefresh) {
        resetState(i);
        frame.HAS_ANALOG_TRIGGERS = false;
        frame.HAS_ANALOG_STICK_MAIN = false;
        frame.HAS_ANALOG_STICK_AUX = false;
        frame.LX = 0;
        frame.LY = 0;
        frame.RX = 0;
        frame.RY = 0;
        frame.ANALOG_L2 = 0;
        frame.ANALOG_R2 = 0;

        #if defined(AUTODETECT_DEBUG_CDC)
        static uint32_t last_map_debug = 0;
        if (now_ms - last_map_debug > 2000) {
          last_map_debug = now_ms;
          printf("  port%d ctrl%d -> USB[%d] type=%d\n", port, c, i, rawDetectedType);
        }
        #endif

        if (sc.deviceJustChanged() || justConnected || typeChanged) {
          dtype[i] = detectedType;
          input_saturn_debug_update_slot(i, (uint8_t)dtype[i], (uint8_t)rawDetectedType,
                                         (uint8_t)stableMegadriveType[i],
                                         megadriveTypeDowngradeStartedMs[i],
                                         port, c, saturn[port]->getMultitapPorts());
          setInputFrameConnected(frame, dtype[i] != SAT_DEVICE_NONE);

          switch (dtype[i]) {
            case SAT_DEVICE_3DPAD:
              setInputFrameTypeName(frame, "3D Pad");
              break;
            case SAT_DEVICE_WHEEL:
              setInputFrameTypeName(frame, "Wheel");
              break;
            case SAT_DEVICE_MISSION3:
              setInputFrameTypeName(frame, "Mission");
              break;
#ifdef SATLIB_ENABLE_MISSION6
            case SAT_DEVICE_MISSION6:
              setInputFrameTypeName(frame, "Mission6");
              break;
#endif
            case SAT_DEVICE_MEGA3:
              setInputFrameTypeName(frame, "Mega3");
              break;
            case SAT_DEVICE_MEGA6:
              setInputFrameTypeName(frame, "Mega6");
              break;
            case SAT_DEVICE_PAD:
              setInputFrameTypeName(frame, "Saturn");
              break;
            case SAT_DEVICE_MOUSE:
              setInputFrameTypeName(frame, "Saturn Mouse");
              break;
            case SAT_DEVICE_NONE:
            default:
              clearInputFrameTypeName(frame);
              break;
          }
        }

        switch (dtype[i]) {
          case SAT_DEVICE_NONE:
          case SAT_DEVICE_NOTSUPPORTED:
            break;
          case SAT_DEVICE_MOUSE:
            frame.mouse_x = sc.currentState.mouseX;
            frame.mouse_y = sc.currentState.mouseY;
            frame.A = (sc.currentState.mouseButtons & 0x01) ? 1 : 0;
            frame.B = (sc.currentState.mouseButtons & 0x02) ? 1 : 0;
            frame.X = (sc.currentState.mouseButtons & 0x04) ? 1 : 0;
            frame.START = (sc.currentState.mouseButtons & 0x08) ? 1 : 0;
            updateSaturnMouseDebugRange(i, sc.currentState);
            break;
          case SAT_DEVICE_MEGA3:
          case SAT_DEVICE_MEGA6:
          case SAT_DEVICE_PAD:
            frame.HAS_ANALOG_TRIGGERS = false;
            [[fallthrough]];
          case SAT_DEVICE_3DPAD:
            frame.PAD_U = sc.digitalPressed(SAT_PAD_UP);
            frame.PAD_D = sc.digitalPressed(SAT_PAD_DOWN);
            frame.PAD_L = sc.digitalPressed(SAT_PAD_LEFT);
            frame.PAD_R = sc.digitalPressed(SAT_PAD_RIGHT);
            [[fallthrough]];
          case SAT_DEVICE_WHEEL:
          case SAT_DEVICE_MISSION3:
#ifdef SATLIB_ENABLE_MISSION6
          case SAT_DEVICE_MISSION6:
#endif
          {
            switch (dtype[i]) {
              case SAT_DEVICE_NONE:
              case SAT_DEVICE_NOTSUPPORTED:
              case SAT_DEVICE_MEGA3:
              case SAT_DEVICE_MEGA6:
              case SAT_DEVICE_MOUSE:
              case SAT_DEVICE_PAD:
                break;
              case SAT_DEVICE_3DPAD:
                frame.HAS_ANALOG_TRIGGERS = true;
                [[fallthrough]];
              case SAT_DEVICE_WHEEL:
              case SAT_DEVICE_MISSION3:
#ifdef SATLIB_ENABLE_MISSION6
              case SAT_DEVICE_MISSION6:
#endif
                frame.HAS_ANALOG_STICK_MAIN = true;
                break;
            }
#ifdef SATLIB_ENABLE_MISSION6
            if (dtype[i] == SAT_DEVICE_MISSION6) {
              frame.HAS_ANALOG_STICK_AUX = true;
              frame.RX = sc.analog(SAT_ANALOG_X2) - 0x80;
              frame.RY = sc.analog(SAT_ANALOG_Y2) - 0x80;
            }
#endif
            if (dtype[i] == SAT_DEVICE_WHEEL) {
              frame.PAD_U = sc.digitalPressed(SAT_PAD_UP);
              frame.PAD_D = sc.digitalPressed(SAT_PAD_DOWN);
            }
            const uint8_t rawX = sc.analog(SAT_ANALOG_X);
            const uint8_t rawY = sc.analog(SAT_ANALOG_Y);
            const uint8_t rawL = sc.analog(SAT_ANALOG_L);
            const uint8_t rawR = sc.analog(SAT_ANALOG_R);
            frame.A = sc.digitalPressed(SAT_A);
            frame.B = sc.digitalPressed(SAT_B);
            frame.X = sc.digitalPressed(SAT_X);
            frame.Y = sc.digitalPressed(SAT_Y);
            frame.L1 = sc.digitalPressed(SAT_Z);
            frame.R1 = sc.digitalPressed(SAT_C);
            frame.L2 = sc.digitalPressed(SAT_L);
            frame.R2 = sc.digitalPressed(SAT_R);
            frame.START = sc.digitalPressed(SAT_START);
            if (dtype[i] == SAT_DEVICE_MEGA3) {
              // Three-button pads have no XYZ/Mode page. Ignore transient
              // probe data so connector noise cannot create held buttons.
              frame.X = 0;
              frame.Y = 0;
              frame.L1 = 0;
              frame.L2 = 0;
              frame.R2 = 0;
            }
            if (dtype[i] == SAT_DEVICE_WHEEL) {
              const uint8_t wheelRaw = saturnWheelRawAxis(sc);
              frame.LX = saturnSignedAxis(wheelRaw);
              frame.LY = 0;
              frame.paddle = wheelRaw;
              frame.ANALOG_L2 = 0;
              frame.ANALOG_R2 = 0;
              updateSaturnMapDebugRange(i, frame.LX, frame.paddle, analogNeedsRefresh, stateChanged);
            } else {
              frame.LX = saturnSignedAxis(rawX);
              frame.LY = saturnSignedAxis(rawY);
              if (dtype[i] == SAT_DEVICE_MISSION3) {
                frame.HAS_ANALOG_STICK_AUX = false;
                frame.HAS_ANALOG_TRIGGERS = false;
                frame.paddle = rawR;
                frame.ANALOG_L2 = 0;
                frame.ANALOG_R2 = 0;
              } else {
                frame.ANALOG_L2 = rawL;
                frame.ANALOG_R2 = rawR;
              }
            }
            break;
          }
        }

        if (output_uses_switch_n64_profile()) {
          frame.HAS_ANALOG_TRIGGERS = false;

          frame.A = sc.digitalPressed(SAT_X);
          frame.B = sc.digitalPressed(SAT_A);

          frame.R2 = sc.digitalPressed(SAT_B);
          frame.SELECT = sc.digitalPressed(SAT_C);
          frame.X = sc.digitalPressed(SAT_Z);
          frame.Y = sc.digitalPressed(SAT_Y);

          if (dtype[i] == SAT_DEVICE_3DPAD) {
            frame.L1 = 0;
            frame.R1 = sc.digitalPressed(SAT_R);
            frame.L2 = sc.digitalPressed(SAT_L);
          } else if (dtype[i] == SAT_DEVICE_PAD) {
            frame.L1 = sc.digitalPressed(SAT_L);
            frame.R1 = sc.digitalPressed(SAT_R);
            frame.L2 = 0;
            frame.LX = 0;
            frame.LY = 0;
          }
        }
        setUpdated(i);

        if (i == 0) {
          uint8_t raw[16];
          raw[0] = dtype[0];
          raw[1] = sc.currentState.digital >> 8;
          raw[2] = sc.currentState.digital & 0xFF;
          raw[3] = sc.currentState.analogX;
          raw[4] = sc.currentState.analogY;
          raw[5] = sc.currentState.analogL;
          raw[6] = sc.currentState.analogR;
          raw[7] = sc.currentState.analogX2;
          raw[8] = sc.currentState.analogY2;
          raw[9] = sc.currentState.mouseX & 0xFF;
          raw[10] = sc.currentState.mouseY & 0xFF;
          raw[11] = sc.currentState.mouseButtons;
          raw[12] = sc.currentState.mouseFlags;
          raw[13] = sc.currentState.mouseRawX;
          raw[14] = sc.currentState.mouseRawY;
          raw[15] = sc.currentState.mouseOverflow ? 1 : 0;
          webhid_store_raw_data(raw, 16);
        }
      }
    }

    for (uint8_t stale = activeSlots; stale < reservedSlots; ++stale) {
      uint8_t i = slotBase + stale;
      if (dtype[i] != SAT_DEVICE_NONE) {
        uint32_t now_ms = millis();
        if (slotLastSeenMs[i] != 0 && (now_ms - slotLastSeenMs[i]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
          continue;
        }
        controller_state_t& frame = inputFrame(i);
        dtype[i] = SAT_DEVICE_NONE;
        resetMegadriveTypeStability(i);
        input_saturn_debug_clear_slot(i);
        slotLastSeenMs[i] = 0;
        setInputFrameConnected(frame, false);
        clearInputFrameTypeName(frame);
        setUpdated(i);
      }
    }
    slotBase += reservedSlots;
  }

  while (slotBase < MAX_USB_OUT) {
    if (dtype[slotBase] != SAT_DEVICE_NONE) {
      uint32_t now_ms = millis();
      if (slotLastSeenMs[slotBase] != 0 &&
          (now_ms - slotLastSeenMs[slotBase]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
        ++slotBase;
        continue;
      }
      controller_state_t& frame = inputFrame(slotBase);
      dtype[slotBase] = SAT_DEVICE_NONE;
      resetMegadriveTypeStability(slotBase);
      input_saturn_debug_clear_slot(slotBase);
      slotLastSeenMs[slotBase] = 0;
      setInputFrameConnected(frame, false);
      clearInputFrameTypeName(frame);
      setUpdated(slotBase);
    }
    ++slotBase;
  }

  for (uint8_t i = 0; i < input_ports && i < MAX_USB_OUT; ++i) {
    if (!physical_port_enabled(i)) {
      setInputFrameConnected(inputFrame(i), false);
      clearInputFrameTypeName(inputFrame(i));
    }
  }

  #if defined(AUTODETECT_DEBUG_CDC)
  static uint32_t last_status_debug = 0;
  if (millis() - last_status_debug > 2000) {
    last_status_debug = millis();
    printf("USB status: ");
    for (uint8_t x = 0; x < inputPortCount() && x < MAX_USB_OUT; ++x) {
      printf("[%d]=%s ", x, inputFrameConst(x).connected ? "ON" : "off");
    }
    printf("\n");
  }
  #endif

  pollInterval = 16000;
  const bool megadriveMode = getInternalMode() == 1;
  bool found_16ms = false;
  bool found_1ms = false;
  bool found_fast_megadrive = false;
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    switch (dtype[i]) {
      case SAT_DEVICE_NONE:
      case SAT_DEVICE_NOTSUPPORTED:
        break;
      case SAT_DEVICE_MOUSE:
        found_1ms = true;
        break;
      case SAT_DEVICE_MISSION3:
      case SAT_DEVICE_WHEEL:
#ifdef SATLIB_ENABLE_MISSION6
      case SAT_DEVICE_MISSION6:
#endif
        found_16ms = true;
        break;
      case SAT_DEVICE_MEGA3:
      case SAT_DEVICE_MEGA6:
        if (megadriveMode) {
          found_fast_megadrive = true;
        } else {
          found_1ms = true;
        }
        break;
      case SAT_DEVICE_PAD:
      case SAT_DEVICE_3DPAD:
        found_1ms = true;
        break;
    }
  }

  if (found_16ms) {
    pollInterval = 16000;
  } else if (found_1ms) {
    pollInterval = SATURN_CONNECTED_POLL_INTERVAL_US;
  } else if (found_fast_megadrive) {
    pollInterval = MEGADRIVE_CONNECTED_POLL_INTERVAL_US;
  }

  bool found_megadrive_tap = false;
  bool found_saturn_tap = false;
  uint8_t max_saturn_connected = 0;
  for (uint8_t port = 0; port < input_ports; ++port) {
    if (!physical_port_enabled(port) || saturn[port] == nullptr) {
      continue;
    }
    uint8_t tap_ports = lastTapPorts[port];
    if (tap_ports == TAP_MEGA_PORTS) {
      found_megadrive_tap = true;
    } else if (tap_ports == TAP_SAT_PORTS) {
      found_saturn_tap = true;
      max_saturn_connected = max(max_saturn_connected, port_controller_count[port]);
    }
  }

  if (found_megadrive_tap) {
    if (pollInterval < 16000) {
      pollInterval = 16000;
    }
  } else if (found_saturn_tap) {
    if (max_saturn_connected >= 5) {
      if (pollInterval < 16000) {
        pollInterval = 16000;
      }
    } else {
      if (pollInterval < 8000) {
        pollInterval = 8000;
      }
    }
  }

  return endPollCycle();
}
