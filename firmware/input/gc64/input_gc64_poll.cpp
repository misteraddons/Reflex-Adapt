#include "Input_GC64.h"

#include "../../core/classic_analog_range.h"
#include "../../core/controller_settings_state.h"
#include "../../core/rumble_test_runtime.h"
#include "../../platform/latency_trace_gpio.h"
#include "../../platform/latency_test.h"

namespace {

DeviceEnum joybusAnalogModeForType(JoybusDeviceType_Enum type) {
  switch (type) {
    #if defined(ENABLE_INPUT_N64)
    case JOYBUS_DEVICE_N64PAD:
    case JOYBUS_DEVICE_N64MOUSE:
      return RZORD_N64;
    #endif
    #if defined(ENABLE_INPUT_GAMECUBE)
    case JOYBUS_DEVICE_GCPAD:
    case JOYBUS_DEVICE_GCWIRELESS:
    case JOYBUS_DEVICE_GCWHEEL:
      return RZORD_GAMECUBE;
    #endif
    #if defined(ENABLE_INPUT_GBA)
    case JOYBUS_DEVICE_GBA:
      return RZORD_GBA;
    #endif
    default:
      return deviceMode;
  }
}

}  // namespace

void __not_in_flash_func(RZInputGC64::stageWebhidRawData)(uint8_t port, JoybusDeviceType_Enum type) {
  if (port != 0 || joybus[port] == nullptr) {
    return;
  }

  pendingWebhidRawPort = port;
  pendingWebhidRawType = type;
  pendingWebhidRawDirty = true;
}

void RZInputGC64::flushPendingWebhidRawData() {
  if (!pendingWebhidRawDirty) {
    return;
  }

  const uint8_t port = pendingWebhidRawPort;
  const JoybusDeviceType_Enum type = pendingWebhidRawType;
  pendingWebhidRawDirty = false;

  if (port != 0 || port >= input_ports || joybus[port] == nullptr) {
    return;
  }

  uint8_t raw[16] = {0};
  raw[0] = type;
  if (type == JOYBUS_DEVICE_N64PAD || type == JOYBUS_DEVICE_N64MOUSE) {
    const N64ControllerState state = joybus[port]->getN64PadState();
    raw[1] = state.joystick_x;
    raw[2] = state.joystick_y;
    raw[3] = (state.buttons >> 8) & 0xFF;
    raw[4] = state.buttons & 0xFF;
  } else if (type == JOYBUS_DEVICE_GCPAD || type == JOYBUS_DEVICE_GCWIRELESS) {
    const NormalizedGCControllerState state = joybus[port]->getGCPadState();
    raw[1] = state.joystick_x;
    raw[2] = state.joystick_y;
    raw[3] = state.cstick_x;
    raw[4] = state.cstick_y;
    raw[5] = state.analog_l;
    raw[6] = state.analog_r;
    raw[7] = (state.buttons >> 8) & 0xFF;
    raw[8] = state.buttons & 0xFF;
  } else if (type == JOYBUS_DEVICE_GBA) {
    const GBAState state = joybus[port]->getGBAState();
    raw[3] = (state >> 8) & 0xFF;
    raw[4] = state & 0xFF;
  }
  raw[9] = port;
  raw[10] = getInternalMode();

  webhid_store_raw_data(raw, 16);
}

bool __not_in_flash_func(RZInputGC64::poll)() {
  beginPollCycle();

  for (uint8_t port = 0; port < input_ports; ++port) {
    uint8_t i = port;
    controller_state_t& frame = inputFrame(i);

    if (!should_poll_port(input_ports, port) || joybus[port] == nullptr) {
      continue;
    }

    uint8_t effective_left = 0;
    uint8_t effective_right = 0;
    rumbleRuntimeGetEffectiveFeedback(port, &effective_left, &effective_right);
    joybus[port]->setRumble((effective_left | effective_right) ? 1 : 0);
    {
      LatencyPhaseTraceScope gc64UpdateTrace(LATENCY_TRACE_PHASE_GC64_UPDATE);
      joybus[port]->update();
    }
    if (i == 0 && latencyTest.isEnabled() &&
        joybus[port]->deviceType() == JOYBUS_DEVICE_N64PAD) {
      latencyTest.observeRawButtons(0, joybus[port]->getN64PadState().buttons, true);
    }
    if (port < GC64_DEBUG_PORTS) {
      gc64_debug_device_type[port] = (uint8_t)joybus[port]->deviceType();
      gc64_n64_accessory_aux[port] = joybus[port]->accessoryAux();
      gc64_n64_rumble_pak_detected[port] = joybus[port]->isRumblePakConnected();
      gc64_n64_rumble_command_pending[port] = joybus[port]->isRumbleCommandPending();
      gc64_n64_rumble_probe_attempts[port] = joybus[port]->rumbleProbeAttempts();
      gc64_n64_rumble_probe_result[port] = joybus[port]->rumbleLastProbeResult();
      gc64_n64_rumble_probe_byte[port] = joybus[port]->rumbleLastProbeByte();
      gc64_n64_rumble_motor_result[port] = joybus[port]->rumbleLastMotorResult();
      const JoybusMemoryWriteDiag& motorDiag = joybus[port]->rumbleLastMotorDiag();
      gc64_n64_rumble_motor_transport[port] = motorDiag.transport_result;
      gc64_n64_rumble_motor_expected[port] = motorDiag.expected_checksum;
      gc64_n64_rumble_motor_response[port] = motorDiag.response_checksum;
    }

    if (joybus[port]->stateChanged()) {
      resetState(i);
      if (joybus[port]->deviceJustChanged()) {
        JoybusDeviceType_Enum observedType = joybus[port]->deviceType();
        if (observedType == JOYBUS_DEVICE_GCWIRELESS) {
          wavebirdReceiverSeen[port] = true;
        } else if (observedType != JOYBUS_DEVICE_GCPAD) {
          wavebirdReceiverSeen[port] = false;
        }
        dtype[port] = (wavebirdReceiverSeen[port] && observedType == JOYBUS_DEVICE_GCPAD)
                        ? JOYBUS_DEVICE_GCWIRELESS
                        : observedType;
        if (dtype[port] == JOYBUS_DEVICE_N64MOUSE) {
          // N64 mouse parsing exists as prototype code, but mouse input/output
          // is deferred until the whole path is validated.
          dtype[port] = JOYBUS_DEVICE_NOTSUPPORTED;
        }
        resetClassicAnalogLearnState(joybusAnalogModeForType(dtype[port]), i);
        setInputFrameConnected(i, dtype[port] != JOYBUS_DEVICE_NONE);

        switch (dtype[port]) {
          case JOYBUS_DEVICE_N64PAD:
            setInputFrameTypeName(i, "N64 Pad");
            break;
          case JOYBUS_DEVICE_N64MOUSE:
            setInputFrameTypeName(i, "N64 Mouse");
            break;
          case JOYBUS_DEVICE_GCPAD:
            setInputFrameTypeName(i, "GC Pad");
            break;
          case JOYBUS_DEVICE_GCWIRELESS:
            setInputFrameTypeName(i, "WaveBird");
            break;
          case JOYBUS_DEVICE_GBA:
            setInputFrameTypeName(i, "GBA");
            break;
          case JOYBUS_DEVICE_NONE:
          default:
            clearInputFrameTypeName(i);
            break;
        }
      }

      frame.HAS_ANALOG_TRIGGERS = false;
      frame.HAS_ANALOG_STICK_AUX = false;
      switch (dtype[port]) {
        case JOYBUS_DEVICE_NONE:
        case JOYBUS_DEVICE_NOTSUPPORTED:
          break;
        case JOYBUS_DEVICE_GBA:
        {
          const GBAState buttons = joybus[port]->getGBAState();
          frame.PAD_U = (buttons & GBAB_DPAD_UP) != 0;
          frame.PAD_D = (buttons & GBAB_DPAD_DOWN) != 0;
          frame.PAD_L = (buttons & GBAB_DPAD_LEFT) != 0;
          frame.PAD_R = (buttons & GBAB_DPAD_RIGHT) != 0;
          frame.A = (buttons & GBAB_B_BUTTON) != 0;
          frame.B = (buttons & GBAB_A_BUTTON) != 0;
          frame.L1 = (buttons & GBAB_L_TRIGGER) != 0;
          frame.R1 = (buttons & GBAB_R_TRIGGER) != 0;
          frame.SELECT = (buttons & GBAB_SELECT) != 0;
          frame.START = (buttons & GBAB_START) != 0;
          break;
        }
        case JOYBUS_DEVICE_N64MOUSE:
        {
          N64MouseState mouse = joybus[port]->getN64MouseState();
          if (mouse.valid) {
            frame.mouse_x = mouse.movement_x;
            frame.mouse_y = -mouse.movement_y;
          }
          frame.A = (mouse.buttons & N64M_LEFT_BUTTON) ? 1 : 0;
          frame.B = (mouse.buttons & N64M_RIGHT_BUTTON) ? 1 : 0;
          break;
        }
        case JOYBUS_DEVICE_N64PAD:
        {
          LatencyPhaseTraceScope n64MapTrace(LATENCY_TRACE_PHASE_N64_MAP);
          constexpr DeviceEnum analogMode = RZORD_N64;
          const N64ControllerState state = joybus[port]->getN64PadState();
          const uint16_t buttons = state.buttons;
          setInputFrameTypeName(i, joybus[port]->isRumblePakConnected() ? "N64 Rumble" : "N64 Pad");
          frame.PAD_U = (buttons & N64B_DPAD_UP) != 0;
          frame.PAD_D = (buttons & N64B_DPAD_DOWN) != 0;
          frame.PAD_L = (buttons & N64B_DPAD_LEFT) != 0;
          frame.PAD_R = (buttons & N64B_DPAD_RIGHT) != 0;
          int16_t raw_lx = state.joystick_x;
          int16_t raw_ly = invertSignedInt8Axis(state.joystick_y);
          frame.LX = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx);
          frame.LY = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly);
          recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx, frame.LX);
          recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly, frame.LY);
          frame.RX = 0;
          frame.RY = 0;
          frame.A = (buttons & N64B_A_BUTTON) != 0;
          frame.B = (buttons & N64B_B_BUTTON) != 0;
          frame.X = 0;
          frame.Y = 0;
          frame.L1 = (buttons & N64B_L_TRIGGER) != 0;
          frame.L2 = (buttons & N64B_Z_TRIGGER) != 0;
          frame.R1 = (buttons & N64B_R_TRIGGER) != 0;
          frame.R2 = 0;
          frame.L3 = 0;
          frame.R3 = 0;
          frame.SELECT = 0;
          frame.START = (buttons & N64B_START) != 0;

          if (output_maps_n64_c_buttons_to_face_buttons()) {
            frame.L3 = (buttons & N64B_C_DOWN) != 0;
            frame.R3 = (buttons & N64B_C_RIGHT) != 0;
            frame.X = (buttons & N64B_C_UP) != 0;
            frame.Y = (buttons & N64B_C_LEFT) != 0;
          } else {
            if (buttons & N64B_C_LEFT) {
              frame.RX = -127;
            } else if (buttons & N64B_C_RIGHT) {
              frame.RX = 127;
            }
            if (buttons & N64B_C_UP) {
              frame.RY = -127;
            } else if (buttons & N64B_C_DOWN) {
              frame.RY = 127;
            }
          }
          break;
        }
        case JOYBUS_DEVICE_GCPAD:
        case JOYBUS_DEVICE_GCWIRELESS:
        {
          constexpr DeviceEnum analogMode = RZORD_GAMECUBE;
          const NormalizedGCControllerState state = joybus[port]->getGCPadState();
          const uint16_t buttons = state.buttons;
          frame.HAS_ANALOG_TRIGGERS = true;
          frame.HAS_ANALOG_STICK_AUX = true;
          frame.PAD_U = (buttons & GCB_DPAD_UP) != 0;
          frame.PAD_D = (buttons & GCB_DPAD_DOWN) != 0;
          frame.PAD_L = (buttons & GCB_DPAD_LEFT) != 0;
          frame.PAD_R = (buttons & GCB_DPAD_RIGHT) != 0;
          int16_t raw_lx = state.joystick_x;
          int16_t raw_ly = ~state.joystick_y;
          int16_t raw_rx = state.cstick_x;
          int16_t raw_ry = ~state.cstick_y;
          if (wii_analog_range == CLASSIC_ANALOG_RANGE_NORMALIZED) {
            frame.LX = normalizeGCStickAxis(raw_lx);
            frame.LY = normalizeGCStickAxis(raw_ly);
            frame.RX = normalizeGCStickAxis(raw_rx);
            frame.RY = normalizeGCStickAxis(raw_ry);
          } else if (wii_analog_range == CLASSIC_ANALOG_RANGE_LEARN) {
            frame.LX = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx);
            frame.LY = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly);
            frame.RX = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RX, raw_rx);
            frame.RY = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RY, raw_ry);
          } else {
            frame.LX = raw_lx;
            frame.LY = raw_ly;
            frame.RX = raw_rx;
            frame.RY = raw_ry;
          }
          recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx, frame.LX);
          recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly, frame.LY);
          recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RX, raw_rx, frame.RX);
          recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RY, raw_ry, frame.RY);
          frame.A = (buttons & GCB_A_BUTTON) != 0;
          frame.B = (buttons & GCB_B_BUTTON) != 0;
          frame.X = (buttons & GCB_X_BUTTON) != 0;
          frame.Y = (buttons & GCB_Y_BUTTON) != 0;
          frame.L1 = (buttons & GCB_L_TRIGGER) != 0;
          frame.R1 = (buttons & GCB_Z_TRIGGER) != 0;
          frame.SELECT = 0;
          frame.L2 = (buttons & GCB_L_TRIGGER) != 0;
          frame.R2 = (buttons & GCB_R_TRIGGER) != 0;
          frame.START = (buttons & GCB_START) != 0;
          uint8_t raw_l2 = state.analog_l;
          uint8_t raw_r2 = state.analog_r;
          frame.ANALOG_L2 = raw_l2;
          frame.ANALOG_R2 = raw_r2;
          if (wii_analog_range == CLASSIC_ANALOG_RANGE_NORMALIZED) {
            frame.ANALOG_L2 = normalizeGCTriggerAxis(frame.ANALOG_L2);
            frame.ANALOG_R2 = normalizeGCTriggerAxis(frame.ANALOG_R2);
          } else if (wii_analog_range == CLASSIC_ANALOG_RANGE_LEARN) {
            frame.ANALOG_L2 = applyClassicAnalogLearnTrigger(
                analogMode, i, CLASSIC_ANALOG_TRIGGER_L2, frame.ANALOG_L2);
            frame.ANALOG_R2 = applyClassicAnalogLearnTrigger(
                analogMode, i, CLASSIC_ANALOG_TRIGGER_R2, frame.ANALOG_R2);
          }
          recordClassicAnalogRangeTrigger(
              analogMode, i, CLASSIC_ANALOG_TRIGGER_L2, raw_l2, frame.ANALOG_L2);
          recordClassicAnalogRangeTrigger(
              analogMode, i, CLASSIC_ANALOG_TRIGGER_R2, raw_r2, frame.ANALOG_R2);
          break;
        }
        case JOYBUS_DEVICE_GCWHEEL:
        {
          break;
        }
      }
      setUpdated(i);

      stageWebhidRawData(port, dtype[port]);
    }
  }

  constexpr uint16_t N64_COMPAT_POLL_INTERVAL_US = 1000;
  pollInterval = 16000;
  bool found_16ms = false;
  bool found_8ms = false;
  bool found_n64 = false;
  bool found_1ms = false;
  for (uint8_t port = 0; port < input_ports; ++port) {
    switch (dtype[port]) {
      case JOYBUS_DEVICE_NONE:
      case JOYBUS_DEVICE_GCWHEEL:
      case JOYBUS_DEVICE_NOTSUPPORTED:
        break;
      case JOYBUS_DEVICE_GBA:
        found_16ms = true;
        break;
      case JOYBUS_DEVICE_GCWIRELESS:
        found_8ms = true;
        break;
      case JOYBUS_DEVICE_N64PAD:
      case JOYBUS_DEVICE_N64MOUSE:
        found_n64 = true;
        break;
      case JOYBUS_DEVICE_GCPAD:
        found_1ms = true;
        break;
    }
  }

  if (found_16ms) {
    pollInterval = 16000;
  } else if (found_8ms) {
    pollInterval = 8000;
  } else if (found_n64) {
    pollInterval = N64_COMPAT_POLL_INTERVAL_US;
  } else if (found_1ms) {
    pollInterval = 1000;
  }

  return endPollCycle();
}

void RZInputGC64::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  flushPendingWebhidRawData();
}
