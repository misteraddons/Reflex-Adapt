#include "Input_Snes.h"

#include "../../core/rumble_test_runtime.h"
#include "../../platform/latency_trace_gpio.h"

bool RZInputSnes::pollFourScore() {
  if (input_ports < 2 || !fourScoreDetected)
    return false;

  // Get pin numbers
  uint8_t clk0 = input_snes_config[0].clk;
  uint8_t clk1 = input_snes_config[1].clk;
  uint8_t lat0 = input_snes_config[0].lat;
  uint8_t lat1 = input_snes_config[1].lat;
  uint8_t dat0_p0 = input_snes_config[0].dat0;
  uint8_t dat0_p1 = input_snes_config[1].dat0;

  // Strobe both ports
  digitalWrite(lat0, HIGH);
  digitalWrite(lat1, HIGH);
  delayMicroseconds(12);
  digitalWrite(lat0, LOW);
  digitalWrite(lat1, LOW);
  delayMicroseconds(6);

  uint32_t data0 = 0;  // Port 0 D0: P1 + P3 + signature
  uint32_t data1 = 0;  // Port 1 D0: P2 + P4 + signature

  // Read 24 bits from both ports' D0 lines
  for (uint8_t i = 0; i < 24; i++) {
    data0 |= (uint32_t)digitalRead(dat0_p0) << i;
    data1 |= (uint32_t)digitalRead(dat0_p1) << i;

    // Clock both ports
    digitalWrite(clk0, LOW);
    digitalWrite(clk1, LOW);
    delayMicroseconds(6);
    digitalWrite(clk0, HIGH);
    digitalWrite(clk1, HIGH);
    delayMicroseconds(6);
  }

  // Invert (active low)
  data0 = ~data0;
  data1 = ~data1;

  // Extract controller data (8 bits each)
  // If ports are swapped, data0 has P2+P4 and data1 has P1+P3
  uint8_t p1_data;
  uint8_t p2_data;
  uint8_t p3_data;
  uint8_t p4_data;
  if (fourScorePortsSwapped) {
    p1_data = data1 & 0xFF;         // P1: port1 bits 0-7
    p2_data = data0 & 0xFF;         // P2: port0 bits 0-7
    p3_data = (data1 >> 8) & 0xFF;  // P3: port1 bits 8-15
    p4_data = (data0 >> 8) & 0xFF;  // P4: port0 bits 8-15
  } else {
    p1_data = data0 & 0xFF;         // P1: port0 bits 0-7
    p2_data = data1 & 0xFF;         // P2: port1 bits 0-7
    p3_data = (data0 >> 8) & 0xFF;  // P3: port0 bits 8-15
    p4_data = (data1 >> 8) & 0xFF;  // P4: port1 bits 8-15
  }

  // Store raw data for WebHID debugging
  uint8_t raw[16] = {0};
  raw[0] = 4;  // Device type: Four Score
  raw[1] = p1_data;
  raw[2] = p2_data;
  raw[3] = p3_data;
  raw[4] = p4_data;
  raw[5] = (data0 >> 16) & 0xFF;  // Signature byte from port 0
  raw[6] = (data1 >> 16) & 0xFF;  // Signature byte from port 1
  webhid_store_raw_data(raw, 16);

  // Map NES buttons to our internal format
  // NES: A, B, Select, Start, Up, Down, Left, Right (bits 0-7)
  auto mapNES = [](uint8_t nes, controller_state_t& r) {
    r.A      = (nes >> 0) & 1;  // A
    r.B      = (nes >> 1) & 1;  // B
    r.SELECT = (nes >> 2) & 1;  // Select
    r.START  = (nes >> 3) & 1;  // Start
    r.PAD_U  = (nes >> 4) & 1;  // Up
    r.PAD_D  = (nes >> 5) & 1;  // Down
    r.PAD_L  = (nes >> 6) & 1;  // Left
    r.PAD_R  = (nes >> 7) & 1;  // Right
  };

  bool changed = false;

  // Update all 4 players
  uint8_t playerData[4] = { p1_data, p2_data, p3_data, p4_data };
  for (uint8_t i = 0; i < 4 && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = inputFrame(i);
    // Check if changed
    uint8_t oldState = (frame.A) | (frame.B << 1) |
                      (frame.SELECT << 2) | (frame.START << 3) |
                      (frame.PAD_U << 4) | (frame.PAD_D << 5) |
                      (frame.PAD_L << 6) | (frame.PAD_R << 7);

    if (playerData[i] != oldState || !frame.connected) {
      resetState(i);
      mapNES(playerData[i], frame);
      setInputFrameConnected(frame, true);
      setInputFrameTypeName(frame, "Four Score");
      frame.HAS_BTN_SELECT = 1;
      frame.HAS_BTN_START = 1;
      dtype[i] = SNES_DEVICE_NES_FOURSCORE;
      setUpdated(i);
      changed = true;
    }
  }

  return changed;
}

bool __not_in_flash_func(RZInputSnes::poll)() {
  beginPollCycle();
  const uint32_t now_ms = millis();
  if (snes_debug_reset_pending) {
    for (uint8_t slot = 0; slot < MAX_USB_OUT; ++slot) {
      glitch_frames[slot] = 0;
    }
    snes_debug_reset_pending = false;
  }

  // Four Score mode: use dedicated polling that reads from both ports
  if (fourScoreDetected) {
    clearRumbleTechDetection();
    if (pollFourScore()) {
      markPollUpdated();
    }
    return endPollCycle();
  }

  //output index
  uint8_t i = 0;

  //update each port and get number of connected controllers
  uint8_t port_controller_count[input_ports] = {0};
  const bool snes_rumbletech_mode = snesRumbleTechEnabled();
  const bool snes_rumble_writes_allowed =
    snes_rumbletech_mode && rumble_level != 0;
  bool safe_poll_active = snesSafePollActive(now_ms);
  bool rumble_poll_active = false;

  {
    LatencyPhaseTraceScope snesUpdateTrace(LATENCY_TRACE_PHASE_SNES_UPDATE);
    for (uint8_t port = 0; port < input_ports; ++port) {
      if (!should_poll_port(input_ports, port)) {
        continue;
      }
      if (port < MAX_USB_OUT) {
        bool used_fast_nes_poll = tryFastPollStandardNes(port, port);
        // SNES Rumble (Doom FX3 / RumbleTech): there is no unique input ID, so a
        // normal SNES pad signature is only a candidate. The confidence latch
        // prevents brief bad reads from hiding the Rumble menu or cutting off an
        // active motor command.
        bool snes_rumble_possible = (snes_rumbletech_mode &&
                                     getInternalMode() == 2 &&
                                     !snes[port]->getMultitapPorts() &&
                                     !snes[port]->isPowerPadMode() &&
                                     !snes[port]->isZapperMode());
        bool rumbletech_detected_now = snes_rumble_possible && isRumbleTechDetected(port);
        bool rumbletech_detected = snes_rumble_possible &&
                                   (rumbletech_detected_now || isRumbleTechLatched(port));

        if (!used_fast_nes_poll && snes_rumble_possible && (rumbletech_detected || last_rumble_data[port] != 0)) {
          uint8_t rumbleData = 0;
          // RumbleTech packet lower byte is [RRRR][LLLL].
          // SNES pad wiring observed on hardware:
          //   left physical motor  = heavy/low-frequency host motor
          //   right physical motor = light/high-frequency host motor
          //
          // Keep computing from the current host/test rumble values even if the
          // controller read briefly fails while an effect is already active.
          // Otherwise a single bad signature queues a zero byte and held XInput
          // effects become choppy, even though the decoded host value is stable.
          uint8_t src_left = 0;
          uint8_t src_right = 0;
          rumbleRuntimeGetEffectiveFeedback(port, &src_left, &src_right);

          uint8_t right = toSnesRumbleNibble(src_right);
          uint8_t left  = toSnesRumbleNibble(src_left);
          rumbleData = (right << 4) | left;

          const bool active = (rumbleData != 0);
          // Queue rumble and let SnesPort send it immediately after the next
          // single-pad controller read. This differs from the original MPG
          // interleaved write, but keeps tested RP2040 reads stable.
          // Do not rate-limit held RumbleTech commands. A nonzero motor command
          // needs a faster keepalive than the 3 ms glitch-recovery cadence; the
          // only zero we send is the release byte after the host/test effect ends.
          const bool should_send = snes_rumble_writes_allowed &&
                                   (active || last_rumble_data[port] != 0);
          if (should_send) {
            requestSafeSnesPoll(port, now_ms);
            safe_poll_active = true;
            if (active) {
              rumble_poll_active = true;
            }
            snes[port]->queueRumble(rumbleData);
            if (port == 0) {
              snes_rumble_debug_data0 = rumbleData;
              snes_rumble_debug_tx0++;
            } else if (port == 1) {
              snes_rumble_debug_data1 = rumbleData;
              snes_rumble_debug_tx1++;
            }
            last_rumble_data[port] = rumbleData;
          } else if (!snes_rumble_writes_allowed) {
            last_rumble_data[port] = 0;
          }
        }

        if (!used_fast_nes_poll) {
          snes[port]->update();
          bool rumbletech_after_update = snes_rumble_possible && isRumbleTechDetected(port);
          updateRumbleTechDetection(port, rumbletech_after_update);
        }

        port_controller_count[port] = snes[port]->getControllerCount();
      }
    }
  }

  // Update global controller count per physical port for display
  snes_port0_controller_count = (input_ports > 0) ? port_controller_count[0] : 0;
  snes_port1_controller_count = (input_ports > 1) ? port_controller_count[1] : 0;
  snes_debug_safe_poll0 = safe_poll_active;
  snes_debug_safe_poll1 = safe_poll_active;
  if (getInternalMode() == 2) {
    pollInterval = rumble_poll_active
      ? SNES_RUMBLETECH_ACTIVE_POLL_INTERVAL_US
      : (safe_poll_active ? SNES_RUMBLETECH_POLL_INTERVAL_US : SNES_IDLE_POLL_INTERVAL_US);
  }

  // Debug: capture raw ID and device type from first controller on each port
  if (input_ports > 0 && port_controller_count[0] > 0) {
    snes_debug_dtype0 = snes[0]->getSnesController(0).deviceType();
  } else if (input_ports > 0 && physical_port_enabled(0)) {
    snes_debug_id0 = 0xFF;
    snes_debug_dtype0 = 0xFF; // No controller
  }
  if (input_ports > 1 && port_controller_count[1] > 0) {
    snes_debug_dtype1 = snes[1]->getSnesController(0).deviceType();
  } else if (input_ports > 1 && physical_port_enabled(1)) {
    snes_debug_id1 = 0xFF;
    snes_debug_dtype1 = 0xFF; // No controller
  }

  {
    LatencyPhaseTraceScope snesMapTrace(LATENCY_TRACE_PHASE_SNES_MAP);
    //each port
    for (uint8_t port = 0; port < input_ports; ++port) {
      //nothing connected. skip but increment
      if (port_controller_count[port] == 0) {
        if (dtype[i] != SNES_DEVICE_NONE) { // device just disconnected
          if (slotLastSeenMs[i] != 0 && (now_ms - slotLastSeenMs[i]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
            clearDisconnectConfirm(i);
          } else if (confirmDisconnect(i)) {
            setDisconnected(i);
          }
        } else {
          clearDisconnectConfirm(i);
        }
        ++i;
        continue;
      }

      //each controller on that port
      for (uint8_t c = 0; c < port_controller_count[port]; ++c) {
        if (i >= MAX_USB_OUT)
          break;

      controller_state_t& frame = inputFrame(i);
      const SnesController& sc = snes[port]->getSnesController(c);
      slot_physical_port[i] = port;
      setSnesPadDebugRaw(i, sc.currentState.id, sc.currentState.digital, sc.currentState.extended);

      // Ignore obvious bus-glitch frames that can transiently set many buttons.
      const bool invalid_frame = (sc.currentState.id == 0xFF || sc.currentState.digital == 0xFFFF);
      bool invalid_id_frame = false;
      bool all_buttons_frame = false;
      bool obvious_glitch = invalid_frame;
      if (!obvious_glitch && getInternalMode() == 2) {
        uint8_t id = sc.currentState.id;
        bool valid_id = (id == 0x0 || id == 0x2 || id == 0x4 ||
                         id == 0xF || id == 0x10 || id == 0x11);
        #ifdef ENABLE_EXPERIMENTAL_CONSOLE_MOUSE
        valid_id = valid_id || id == 0x8 || id == 0xE;
        #endif
        if (!valid_id) {
          invalid_id_frame = true;
          obvious_glitch = true;
        }
        // SNES rumble-pad edge case: occasional "all buttons pressed" ghost frame.
        // A real SNES pad should not report all 12 buttons at once.
        if (!obvious_glitch && (sc.currentState.digital & 0x0FFF) == 0x0FFF) {
          all_buttons_frame = true;
          obvious_glitch = true;
        }
      }
      if (obvious_glitch) {
        noteGlitchFrame(i);
        requestSafeSnesPoll(port, now_ms);
        safe_poll_active = true;
        if (port == 0) {
          snes_debug_safe_poll0 = true;
        } else if (port == 1) {
          snes_debug_safe_poll1 = true;
        }
        if (invalid_frame) {
          noteSnesPadInvalidFrame(i);
        }
        if (invalid_id_frame) {
          noteSnesPadInvalidId(i);
        }
        if (all_buttons_frame) {
          noteSnesPadAllPressed(i);
        }
        setSnesPadDebugFiltered(i, stable_digital[i]);
        ++i;
        continue;
      }

      if (standardPadFastPathEligible(port, i, sc, port_controller_count[port], safe_poll_active)) {
        mapStandardPadFastPath(port, i, sc, now_ms);
        ++i;
        continue;
      }

      SnesDeviceType_Enum observedType = effectiveDeviceType(sc);
      observedType = normalizeVirtualBoyObservedType(i, sc, observedType);
      if (getInternalMode() == 2 &&
          dtype[i] == SNES_DEVICE_PAD &&
          observedType == SNES_DEVICE_VB) {
        observedType = SNES_DEVICE_PAD;
      }
      bool observedConnectedType = (observedType != SNES_DEVICE_NONE &&
                                    observedType != SNES_DEVICE_NOTSUPPORTED);
      bool snesTransientType = isStableSnesPadTransientType(i, observedType);
      if (observedConnectedType || snesTransientType) {
        slotLastSeenMs[i] = now_ms;
      }

      SnesDeviceType_Enum detectedType = observedType;
      // In SNES mode, transient NES IDs from noisy reads should not reclassify
      // the controller away from SNES Pad. Keep real NTT controllers visible so
      // their keypad/special buttons work.
      if (isStableSnesPadTransientType(i, detectedType) &&
          detectedType == SNES_DEVICE_NES) {
        detectedType = SNES_DEVICE_PAD;
      }

      bool typeChanged = false;
      if (observedConnectedType || snesTransientType) {
        detectedType = confirmStableType(i, detectedType);
        if (detectedType != dtype[i]) {
          dtype[i] = detectedType;
          typeChanged = true;
        }
      }

      bool disconnect_candidate = false;
      SnesDeviceType_Enum disconnect_type = SNES_DEVICE_NONE;
      const uint16_t previous_stable_digital = stable_digital[i];
      const uint16_t stabilized_digital = stabilizeSnesPadDigital(i, sc.currentState.digital, dtype[i]);
      const bool filteredStateChanged = (stabilized_digital != previous_stable_digital);

      if (sc.stateChanged() || typeChanged || filteredStateChanged) {
        resetState(i);
        setInputFrameConnected(
          frame,
          (dtype[i] != SNES_DEVICE_NONE &&
           dtype[i] != SNES_DEVICE_NOTSUPPORTED));

        applySnesFrameIdentity(port, i, frame);

        uint16_t digital = stabilized_digital;
        auto digitalPressed = [digital](SnesDigital_Enum bit) {
          return (digital & bit) != 0;
        };

        switch (dtype[i]) {
          case SNES_DEVICE_NONE:
          case SNES_DEVICE_NOTSUPPORTED:
          case SNES_DEVICE_NES_FOURSCORE:
            break;
          case SNES_DEVICE_MOUSE:
          {
            frame.mouse_x = sc.currentState.mouseX;
            frame.mouse_y = sc.currentState.mouseY;
            frame.A = (sc.currentState.mouseButtons & 0x01) ? 1 : 0;
            frame.B = (sc.currentState.mouseButtons & 0x02) ? 1 : 0;
            break;
          }
          case SNES_DEVICE_NES:
          case SNES_DEVICE_PAD:
          case SNES_DEVICE_NTT:
          case SNES_DEVICE_VB:
          {
            frame.PAD_U   = digitalPressed(SNES_UP);
            frame.PAD_D   = digitalPressed(SNES_DOWN);
            frame.PAD_L   = digitalPressed(SNES_LEFT);
            frame.PAD_R   = digitalPressed(SNES_RIGHT);
            frame.SELECT  = digitalPressed(SNES_SELECT);
            frame.START   = digitalPressed(SNES_START);

            if (dtype[i] == SNES_DEVICE_NES) {
              // NES A button (right) is at SNES_B position in protocol
              // NES B button (left) is at SNES_Y position in protocol
              // Always use physical button mapping (button_map_mode applied at USB output stage)
              frame.A = digitalPressed(SNES_B);  // NES A -> HID A
              frame.B = digitalPressed(SNES_Y);  // NES B -> HID B
            } else if (dtype[i] == SNES_DEVICE_VB) {
              // Virtual Boy reports physical A/B in the controller ID low
              // bits; the SNES A/B bit positions are right-dpad Right/Down.
              uint8_t vb_ab = virtualBoyABBits(sc);
              const bool vb_a = (vb_ab & 0x02) != 0;
              const bool vb_b = (vb_ab & 0x01) != 0;
              const bool rdpad_right = digitalPressed(SNES_A);
              const bool rdpad_down  = digitalPressed(SNES_B);
              const bool rdpad_up    = digitalPressed(SNES_X);
              const bool rdpad_left  = digitalPressed(SNES_Y);

              frame.A = vb_a;
              frame.B = vb_b;
              frame.X = 0;
              frame.Y = 0;
              frame.L1 = sc.digitalPressed(SNES_L);
              frame.R1 = sc.digitalPressed(SNES_R);
              frame.L2 = 0;
              frame.R2 = 0;
              frame.HAS_ANALOG_STICK_AUX = false;
              frame.RX = 0;
              frame.RY = 0;
              frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;

              if (output_maps_vboy_right_dpad_to_face_buttons()) {
                frame.X  = rdpad_up;
                frame.Y  = rdpad_left;
                frame.L2 = rdpad_down;
                frame.R2 = rdpad_right;
              } else {
                frame.HAS_ANALOG_STICK_AUX = true;
                if (rdpad_left) {
                  frame.RX = -127;
                } else if (rdpad_right) {
                  frame.RX = 127;
                }
                if (rdpad_up) {
                  frame.RY = -127;
                } else if (rdpad_down) {
                  frame.RY = 127;
                }
              }
            } else {
              // SNES: Always use physical button mapping (button_map_mode applied at USB output stage)
              frame.A = digitalPressed(SNES_A);  // SNES A -> HID A
              frame.B = digitalPressed(SNES_B);  // SNES B -> HID B
              frame.X = digitalPressed(SNES_X);  // SNES X -> HID X
              frame.Y = digitalPressed(SNES_Y);  // SNES Y -> HID Y
              frame.L1 = digitalPressed(SNES_L);
              frame.R1 = digitalPressed(SNES_R);
            }
            break;
          }
          case SNES_DEVICE_NES_POWERPAD:
          {
            // Power Pad: 12 buttons mapped to gamepad buttons
            // Layout (Side A):
            //  +----+----+----+----+
            //  |  2 |  1 |  5 |  9 |
            //  +----+----+----+----+
            //  |  6 | 10 | 11 |  7 |
            //  +----+----+----+----+
            //            |  4 |  8 |
            //            +----+----+
            //
            // Mapping: buttons 1-4 -> A,B,X,Y; 5-8 -> L1,R1,L2,R2; 9-12 -> Select,Start,L3,R3
            uint16_t pp = sc.currentState.powerPadButtons;
            frame.A      = (pp >> 0) & 1;  // Button 1
            frame.B      = (pp >> 1) & 1;  // Button 2
            frame.X      = (pp >> 2) & 1;  // Button 3
            frame.Y      = (pp >> 3) & 1;  // Button 4
            frame.L1     = (pp >> 4) & 1;  // Button 5
            frame.R1     = (pp >> 5) & 1;  // Button 6
            frame.L2     = (pp >> 6) & 1;  // Button 7
            frame.R2     = (pp >> 7) & 1;  // Button 8
            frame.SELECT = (pp >> 8) & 1;  // Button 9
            frame.START  = (pp >> 9) & 1;  // Button 10
            frame.L3     = (pp >> 10) & 1; // Button 11
            frame.R3     = (pp >> 11) & 1; // Button 12
            // Also store raw 12-bit value in EXTRA for software that understands Power Pad
            frame.EXTRA  = pp;
            break;
          }
          case SNES_DEVICE_NES_ZAPPER:
          {
            // Zapper: trigger on A, light sensor on B
            // Light sensor is active when pointing at bright area (e.g., CRT white target)
            frame.A = sc.currentState.zapperTrigger ? 1 : 0;
            frame.B = sc.currentState.zapperLightSense ? 1 : 0;
            // Store combined state in EXTRA for software that understands Zapper
            // Bit 0 = trigger, Bit 1 = light sense
            frame.EXTRA = (sc.currentState.zapperTrigger ? 1 : 0) |
                          (sc.currentState.zapperLightSense ? 2 : 0);
            break;
          }
        }

        switch (dtype[i]) {
          case SNES_DEVICE_NONE:
          case SNES_DEVICE_NOTSUPPORTED:
          case SNES_DEVICE_NES:
          case SNES_DEVICE_NES_FOURSCORE:
          case SNES_DEVICE_NES_POWERPAD:
          case SNES_DEVICE_NES_ZAPPER:
          case SNES_DEVICE_PAD:
          case SNES_DEVICE_MOUSE:
            break;
          case SNES_DEVICE_NTT:
          {
            frame.L2 = sc.nttPressed(SNES_NTT_STAR);
            frame.R2 = sc.nttPressed(SNES_NTT_C);
            frame.L3 = sc.nttPressed(SNES_NTT_HASH);
            frame.R3 = sc.nttPressed(SNES_NTT_DOT);
            frame.HOME = sc.nttPressed(SNES_NTT_EQUAL);
            // NTT keypad number buttons (0-9) mapped to EXTRA field
            frame.EXTRA = 0
              | (sc.nttPressed(SNES_NTT_0) ? (1 << 0) : 0)
              | (sc.nttPressed(SNES_NTT_1) ? (1 << 1) : 0)
              | (sc.nttPressed(SNES_NTT_2) ? (1 << 2) : 0)
              | (sc.nttPressed(SNES_NTT_3) ? (1 << 3) : 0)
              | (sc.nttPressed(SNES_NTT_4) ? (1 << 4) : 0)
              | (sc.nttPressed(SNES_NTT_5) ? (1 << 5) : 0)
              | (sc.nttPressed(SNES_NTT_6) ? (1 << 6) : 0)
              | (sc.nttPressed(SNES_NTT_7) ? (1 << 7) : 0)
              | (sc.nttPressed(SNES_NTT_8) ? (1 << 8) : 0)
              | (sc.nttPressed(SNES_NTT_9) ? (1 << 9) : 0);
            break;
          }
          case SNES_DEVICE_VB:
          {
            // VB-specific A/B and right-dpad mapping is handled in the primary
            // digital block so native A/B remain neutral A/B for menu control.
            break;
          }
        }
        setUpdated(i);

        // Store raw data for WebHID debugging (first controller only)
        if (i == 0) {
          uint8_t raw[16] = {0};
          raw[0] = dtype[0];  // Device type
          raw[1] = sc.currentState.digital >> 8;  // Buttons high byte
          raw[2] = sc.currentState.digital & 0xFF;  // Buttons low byte
          raw[3] = sc.currentState.id;  // Device ID
          raw[4] = (sc.currentState.extended >> 8) & 0xFF;  // Extended high byte
          raw[5] = sc.currentState.extended & 0xFF;  // Extended low byte
          raw[6] = sc.currentState.mouseX & 0xFF;  // Mouse X
          raw[7] = sc.currentState.mouseY & 0xFF;  // Mouse Y
          raw[8] = sc.currentState.mouseButtons;  // Mouse buttons
          raw[9] = sc.currentState.powerPadButtons >> 8;  // Power Pad high byte
          raw[10] = sc.currentState.powerPadButtons & 0xFF;  // Power Pad low byte
          raw[11] = port;  // Port number
          raw[12] = c;  // Controller index
          raw[13] = getInternalMode();  // Mode (NES/SNES/VBOY)
          raw[14] = 0;  // Reserved
          raw[15] = 0;  // Reserved
          webhid_store_raw_data(raw, 16);
        }
      } // end if statechanged

      if (frame.connected && frame.controller_type_name[0] == '\0') {
        applySnesFrameIdentity(port, i, frame);
        setUpdated(i);
      }

      // Verify connection state - catches missed disconnections
      // SnesLib's deviceJustChanged() doesn't always detect transitions FROM valid devices
      // (e.g., when id stays 0x0 but extended changes from 0xFFFF to garbage)
      if (frame.connected) {
        SnesDeviceType_Enum currentDtype = observedType;
        bool snes_transient_noise =
          (getInternalMode() == 2 && dtype[i] == SNES_DEVICE_PAD &&
           (currentDtype == SNES_DEVICE_NONE || currentDtype == SNES_DEVICE_NOTSUPPORTED)) ||
          isStableSnesPadTransientType(i, currentDtype);
        if (!snes_transient_noise &&
            (currentDtype == SNES_DEVICE_NONE ||
             currentDtype == SNES_DEVICE_NOTSUPPORTED)) {
          disconnect_candidate = true;
          disconnect_type = currentDtype;
        }
      }

      if (disconnect_candidate) {
        if (slotLastSeenMs[i] != 0 && (now_ms - slotLastSeenMs[i]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
          clearDisconnectConfirm(i);
        } else if (confirmDisconnect(i)) {
          // Device appears disconnected but connection flag is still set.
          setDisconnected(i, disconnect_type);
        }
      } else {
        clearDisconnectConfirm(i);
      }

        ++i;
      }// end if controllercount
    } //end for ports

    //final check for disconnected controllers
    while (i < MAX_USB_OUT) {
      if (dtype[i] != SNES_DEVICE_NONE) { // device just disconnected
        if (slotLastSeenMs[i] != 0 && (now_ms - slotLastSeenMs[i]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
          clearDisconnectConfirm(i);
        } else if (confirmDisconnect(i)) {
          setDisconnected(i);
        }
      } else {
        clearDisconnectConfirm(i);
      }
      ++i;
    }
  }

  snes_debug_stable_dtype0 = dtype[0];
  snes_debug_candidate_dtype0 = candidate_type[0];
  snes_debug_candidate_count0 = candidate_type_frames[0];
  snes_debug_glitch_frames0 = glitch_frames[0];
  if (MAX_USB_OUT > 1) {
    snes_debug_stable_dtype1 = dtype[1];
    snes_debug_candidate_dtype1 = candidate_type[1];
    snes_debug_candidate_count1 = candidate_type_frames[1];
    snes_debug_glitch_frames1 = glitch_frames[1];
  } else {
    snes_debug_stable_dtype1 = 0xFF;
    snes_debug_candidate_dtype1 = 0xFF;
    snes_debug_candidate_count1 = 0;
    snes_debug_glitch_frames1 = 0;
  }

  if (getInternalMode() == 2) {
    pollInterval = rumble_poll_active
      ? SNES_RUMBLETECH_ACTIVE_POLL_INTERVAL_US
      : ((safe_poll_active || snesSafePollActive(now_ms))
          ? SNES_RUMBLETECH_POLL_INTERVAL_US
          : SNES_IDLE_POLL_INTERVAL_US);
  }

  return endPollCycle();
}
