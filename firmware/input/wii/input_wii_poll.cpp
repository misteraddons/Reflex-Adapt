#include "Input_Wii.h"
#include "input_wii_trace.h"

#include "../../core/classic_analog_range.h"

namespace {

bool __not_in_flash_func(wii_classic_frame_is_corrupt)(ClassicController::Shared* controller) {
  if (controller == nullptr) {
    return true;
  }

  const size_t requestSize = controller->getRequestSize();
  if (requestSize < NintendoExtensionCtrl::ExtensionController::MinRequestSize) {
    return true;
  }

  bool allZero = true;
  bool allOnes = true;
  for (size_t index = 0; index < requestSize; ++index) {
    const uint8_t value = controller->getControlData(index);
    allZero &= value == 0x00;
    allOnes &= value == 0xFF;
  }
  if (allZero || allOnes) {
    return true;
  }

  // Classic Controller reports include a constant 1 bit in the button byte.
  // If it reads as 0, the frame is almost certainly bus noise/open-drain trouble.
  const bool highRes = controller->getHighRes();
  const uint8_t constantByteIndex = highRes ? 6 : 4;
  if (requestSize <= constantByteIndex) {
    return true;
  }
  return (controller->getControlData(constantByteIndex) & 0x01) == 0;
}

}  // namespace

bool RZInputWii::wii_connect(uint8_t i) {
  if (i >= input_ports || !portEnabled(i) || wii[i] == nullptr) {
    return false;
  }

  auto tryPair = [&](uint8_t pair) -> bool {
    if (!beginWiiBus(i, pair)) {
      return false;
    }
    const bool result = wii[i]->connect();
    endWiiBus(i);
    if (result) {
      activePinPair[i] = pair;
    }
    return result;
  };

  if (activePinPair[i] < INPUT_WII_PIN_PAIR_COUNT && tryPair(activePinPair[i])) {
    return true;
  }

  for (uint8_t pair = 0; pair < INPUT_WII_PIN_PAIR_COUNT; ++pair) {
    if (pair == activePinPair[i]) {
      continue;
    }
    if (tryPair(pair)) {
      return true;
    }
  }

  activePinPair[i] = INPUT_WII_PIN_PAIR_INVALID;
  return false;
}

bool __not_in_flash_func(RZInputWii::wii_update)(uint8_t i) {
  if (i >= input_ports || !portEnabled(i) || wii[i] == nullptr ||
      activePinPair[i] >= INPUT_WII_PIN_PAIR_COUNT) {
    return false;
  }
  if (!beginWiiBus(i, activePinPair[i])) {
    return false;
  }
  bool result = wii[i]->update();
  endWiiBus(i);
  return result;
}

void __not_in_flash_func(RZInputWii::resetControllerCache)(uint8_t port) {
  if (port >= input_ports) {
    return;
  }
  cachedClassicPro[port] = false;
  cachedClassicProValid[port] = false;
  cachedControllerType[port] = ExtensionType::NoController;
}

bool __not_in_flash_func(RZInputWii::isCachedClassicPro)(uint8_t port) {
  if (port >= input_ports || wii_classic[port] == nullptr) {
    return false;
  }
  if (!cachedClassicProValid[port]) {
    cachedClassicPro[port] = wii_classic[port]->isClassicPro();
    cachedClassicProValid[port] = true;
  }
  return cachedClassicPro[port];
}

void __not_in_flash_func(RZInputWii::queueWebhidRawData)(uint8_t port,
                                                        ExtensionType conType) {
  if (port != 0) {
    return;
  }
  queuedWebhidRawPort = port;
  queuedWebhidRawType = conType;
  queuedWebhidRawBuild = true;
}

void __not_in_flash_func(RZInputWii::buildQueuedWebhidRawData)() {
  if (!queuedWebhidRawBuild) {
    return;
  }
  queuedWebhidRawBuild = false;

  const uint8_t port = queuedWebhidRawPort;
  const ExtensionType conType = queuedWebhidRawType;
  const bool traceEnabled = wiiTraceIsEnabled();
  const uint32_t traceStartUs = traceEnabled ? micros() : 0;

  for (uint8_t i = 0; i < sizeof(pendingWebhidRaw); ++i) {
    pendingWebhidRaw[i] = 0;
  }
  pendingWebhidRaw[0] = (uint8_t)conType;
  if (conType == ExtensionType::ClassicController) {
    pendingWebhidRaw[1] = wii_classic[port]->dpadUp() | (wii_classic[port]->dpadDown() << 1) |
             (wii_classic[port]->dpadLeft() << 2) | (wii_classic[port]->dpadRight() << 3);
    pendingWebhidRaw[2] = wii_classic[port]->buttonA() | (wii_classic[port]->buttonB() << 1) |
             (wii_classic[port]->buttonX() << 2) | (wii_classic[port]->buttonY() << 3) |
             (wii_classic[port]->buttonL() << 4) | (wii_classic[port]->buttonR() << 5) |
             (wii_classic[port]->buttonZL() << 6) | (wii_classic[port]->buttonZR() << 7);
    pendingWebhidRaw[3] = wii_classic[port]->buttonPlus() | (wii_classic[port]->buttonMinus() << 1) |
             (wii_classic[port]->buttonHome() << 2);
    pendingWebhidRaw[4] = wii_classic[port]->leftJoyX();
    pendingWebhidRaw[5] = wii_classic[port]->leftJoyY();
    pendingWebhidRaw[6] = wii_classic[port]->rightJoyX();
    pendingWebhidRaw[7] = wii_classic[port]->rightJoyY();
    pendingWebhidRaw[8] = wii_classic[port]->triggerL();
    pendingWebhidRaw[9] = wii_classic[port]->triggerR();
    pendingWebhidRaw[10] = isCachedClassicPro(port) ? 1 : 0;
  } else if (conType == ExtensionType::Nunchuk) {
    pendingWebhidRaw[1] = wii_nchuk[port]->buttonC() | (wii_nchuk[port]->buttonZ() << 1);
    pendingWebhidRaw[4] = wii_nchuk[port]->joyX();
    pendingWebhidRaw[5] = wii_nchuk[port]->joyY();
  }
  #ifdef ENABLE_WII_GUITAR
  else if (conType == ExtensionType::GuitarController) {
    pendingWebhidRaw[1] = wii_guitar[port]->fretGreen() | (wii_guitar[port]->fretRed() << 1) |
             (wii_guitar[port]->fretYellow() << 2) | (wii_guitar[port]->fretBlue() << 3) |
             (wii_guitar[port]->fretOrange() << 4);
    pendingWebhidRaw[2] = wii_guitar[port]->strumUp() | (wii_guitar[port]->strumDown() << 1);
    pendingWebhidRaw[3] = wii_guitar[port]->buttonPlus() | (wii_guitar[port]->buttonMinus() << 1);
    pendingWebhidRaw[4] = wii_guitar[port]->joyX();
    pendingWebhidRaw[5] = wii_guitar[port]->joyY();
    pendingWebhidRaw[6] = wii_guitar[port]->whammyBar();
  }
  #endif
  pendingWebhidRaw[11] = port;
  pendingWebhidRawDirty = true;
  if (traceEnabled) {
    wiiTraceRecord(WII_TRACE_EVENT_STAGE_RAW, port, traceStartUs, micros(), 1, 0,
                   (uint8_t)conType);
  }
}

void __not_in_flash_func(RZInputWii::flushPendingWebhidRawData)() {
  const bool traceEnabled = wiiTraceIsEnabled();
  const uint32_t traceStartUs = traceEnabled ? micros() : 0;
  if (!pendingWebhidRawDirty) {
    if (traceEnabled) {
      wiiTraceRecord(WII_TRACE_EVENT_FLUSH_RAW, 0, traceStartUs, micros(), 0, 0, 0);
    }
    return;
  }
  webhid_store_raw_data(pendingWebhidRaw, sizeof(pendingWebhidRaw));
  pendingWebhidRawDirty = false;
  if (traceEnabled) {
    wiiTraceRecord(WII_TRACE_EVENT_FLUSH_RAW, 0, traceStartUs, micros(), 1, 0, 0);
  }
}

bool __not_in_flash_func(RZInputWii::poll)() {
  const bool traceEnabled = wiiTraceIsEnabled();
  const uint32_t pollTraceStartUs = traceEnabled ? micros() : 0;
  const uint32_t analogRangeMode = wii_analog_range;
  beginPollCycle();

  for (uint32_t i = 0; i < input_ports; ++i) {
    const uint32_t portTraceStartUs = traceEnabled ? micros() : 0;
    ExtensionType conType = ExtensionType::NoController;
    if (!portEnabled(i) || wii[i] == nullptr) {
      continue;
    }
    controller_state_t& frame = inputFrame(i);

    if (!haveController[i]) {
      const uint32_t nowUs = time_us_32();
      // Zero means "connect immediately." Without the explicit sentinel check,
      // the wrap-safe signed comparison blocks all initial attempts whenever
      // time_us_32() is in the upper half of its 71-minute rollover period.
      if (nextConnectAttemptUs[i] != 0 &&
          (int32_t)(nowUs - nextConnectAttemptUs[i]) < 0) {
        continue;
      }
      nextConnectAttemptUs[i] = nowUs + WII_EMPTY_PORT_CONNECT_INTERVAL_US;
      const uint32_t connectTraceStartUs = traceEnabled ? micros() : 0;
      const bool connected = wii_connect(i);
      if (traceEnabled) {
        wiiTraceRecord(WII_TRACE_EVENT_CONNECT, i, connectTraceStartUs, micros(),
                       connected ? 1 : 0, updateFailCount[i], 0);
      }
      if (connected) {
        haveController[i] = true;
        nextConnectAttemptUs[i] = 0;
        updateFailCount[i] = 0;
        settlingAfterBadFrame[i] = false;
        resetControllerCache(i);
        resetClassicAnalogLearnState(deviceMode, i);
        setInputFrameConnected(frame, true);
        setInputFrameTypeName(frame, "Wii");
      }
    } else {
      const uint32_t updateTraceStartUs = traceEnabled ? micros() : 0;
      bool updateOk = wii_update(i);
      if (updateOk) {
        conType = wii[i]->getControllerType();
      }
      bool corruptClassicFrame =
          updateOk && conType == ExtensionType::ClassicController &&
          wii_classic_frame_is_corrupt(wii_classic[i]);
      if (!updateOk || corruptClassicFrame) {
        const bool retryOk = wii_update(i);
        if (retryOk) {
          const ExtensionType retryType = wii[i]->getControllerType();
          const bool retryCorruptClassicFrame =
              retryType == ExtensionType::ClassicController &&
              wii_classic_frame_is_corrupt(wii_classic[i]);
          if (!retryCorruptClassicFrame) {
            updateOk = true;
            conType = retryType;
            corruptClassicFrame = false;
          }
        }
      }
      if (traceEnabled) {
        wiiTraceRecord(WII_TRACE_EVENT_UPDATE, i, updateTraceStartUs, micros(),
                       (updateOk && !corruptClassicFrame) ? 1 : 0,
                       updateFailCount[i], (uint8_t)conType);
      }
      if (!updateOk || corruptClassicFrame) {
        settlingAfterBadFrame[i] = true;
        if (updateFailCount[i] < 255) {
          updateFailCount[i]++;
        }
        if (updateFailCount[i] >= WII_UPDATE_FAIL_DISCONNECT_THRESHOLD) {
          haveController[i] = false;
          nextConnectAttemptUs[i] = 0;
          setInputFrameConnected(frame, false);
          clearInputFrameTypeName(frame);
          wii_last_digital_state[i] = 0;
          wii_last_analog_sticks_state[i] = 0;
          wii_last_analog_buttons_state[i] = 0;
          settlingAfterBadFrame[i] = false;
          resetControllerCache(i);
          activePinPair[i] = INPUT_WII_PIN_PAIR_INVALID;
          if (traceEnabled) {
            wiiTraceRecordInstant(WII_TRACE_EVENT_DISCONNECT, i, 0, updateFailCount[i], 0);
          }
        }
      } else {
        updateFailCount[i] = 0;
        if (settlingAfterBadFrame[i]) {
          settlingAfterBadFrame[i] = false;
          continue;
        }
        resetState(i);

        switch (conType) {
          case ExtensionType::ClassicController:
          {
            frame.HAS_ANALOG_STICK_AUX = true;
            const bool typeChanged = cachedControllerType[i] != conType;

            frame.PAD_U = wii_classic[i]->dpadUp();
            frame.PAD_D = wii_classic[i]->dpadDown();
            frame.PAD_L = wii_classic[i]->dpadLeft();
            frame.PAD_R = wii_classic[i]->dpadRight();

            frame.A = wii_classic[i]->buttonA();
            frame.B = wii_classic[i]->buttonB();
            frame.X = wii_classic[i]->buttonX();
            frame.Y = wii_classic[i]->buttonY();
            frame.SELECT = wii_classic[i]->buttonMinus();
            frame.START = wii_classic[i]->buttonPlus();
            frame.HOME = wii_classic[i]->buttonHome();

            if (isCachedClassicPro(i)) {
              frame.HAS_BTN_HOME = 1;
              if (typeChanged) {
                setInputFrameTypeName(frame, "ClassicPro");
              }
              frame.HAS_ANALOG_TRIGGERS = false;
              frame.L1 = wii_classic[i]->buttonL();
              frame.R1 = wii_classic[i]->buttonR();
              frame.L2 = wii_classic[i]->buttonZL();
              frame.R2 = wii_classic[i]->buttonZR();
            } else {
              frame.HAS_BTN_HOME = 1;
              if (typeChanged) {
                setInputFrameTypeName(frame, "Classic");
              }
              frame.HAS_ANALOG_TRIGGERS = true;
              frame.L1 = wii_classic[i]->buttonL();
              frame.R1 = wii_classic[i]->buttonR();
              frame.L2 = wii_classic[i]->buttonZL();
              frame.R2 = wii_classic[i]->buttonZR();
            }

            {
              int8_t lx = wii_classic[i]->leftJoyX() - 0x80;
              int8_t ly = ~(wii_classic[i]->leftJoyY() - 0x80);
              int8_t rx = wii_classic[i]->rightJoyX() - 0x80;
              int8_t ry = ~(wii_classic[i]->rightJoyY() - 0x80);

              if (analogRangeMode == CLASSIC_ANALOG_RANGE_NORMALIZED) {
                const int8_t WII_NATIVE_MAX = 100;
                int16_t scaled_lx = (int16_t)lx * 127 / WII_NATIVE_MAX;
                int16_t scaled_ly = (int16_t)ly * 127 / WII_NATIVE_MAX;
                int16_t scaled_rx = (int16_t)rx * 127 / WII_NATIVE_MAX;
                int16_t scaled_ry = (int16_t)ry * 127 / WII_NATIVE_MAX;

                if (scaled_lx > 127) scaled_lx = 127; if (scaled_lx < -128) scaled_lx = -128;
                if (scaled_ly > 127) scaled_ly = 127; if (scaled_ly < -128) scaled_ly = -128;
                if (scaled_rx > 127) scaled_rx = 127; if (scaled_rx < -128) scaled_rx = -128;
                if (scaled_ry > 127) scaled_ry = 127; if (scaled_ry < -128) scaled_ry = -128;

                lx = (int8_t)scaled_lx;
                ly = (int8_t)scaled_ly;
                rx = (int8_t)scaled_rx;
                ry = (int8_t)scaled_ry;
              } else if (analogRangeMode == CLASSIC_ANALOG_RANGE_LEARN) {
                lx = applyClassicAnalogLearnAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LX, lx);
                ly = applyClassicAnalogLearnAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LY, ly);
                rx = applyClassicAnalogLearnAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_RX, rx);
                ry = applyClassicAnalogLearnAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_RY, ry);
              }

              if (analogRangeMode != CLASSIC_ANALOG_RANGE_LEARN) {
                recordClassicAnalogRangeAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LX,
                                             wii_classic[i]->leftJoyX() - 0x80, lx);
                recordClassicAnalogRangeAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LY,
                                             ~(wii_classic[i]->leftJoyY() - 0x80), ly);
                recordClassicAnalogRangeAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_RX,
                                             wii_classic[i]->rightJoyX() - 0x80, rx);
                recordClassicAnalogRangeAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_RY,
                                             ~(wii_classic[i]->rightJoyY() - 0x80), ry);
              }

              frame.LX = lx;
              frame.LY = ly;
              frame.RX = rx;
              frame.RY = ry;
            }

            {
              const uint8_t WII_TRIGGER_MIN = 34;
              const uint8_t WII_TRIGGER_MAX = 248;

              uint8_t rawL = wii_classic[i]->triggerL();
              uint8_t rawR = wii_classic[i]->triggerR();

              if (rawL <= WII_TRIGGER_MIN) rawL = 0;
              else if (rawL >= WII_TRIGGER_MAX) rawL = 255;
              else rawL = ((uint16_t)(rawL - WII_TRIGGER_MIN) * 255) / (WII_TRIGGER_MAX - WII_TRIGGER_MIN);

              if (rawR <= WII_TRIGGER_MIN) rawR = 0;
              else if (rawR >= WII_TRIGGER_MAX) rawR = 255;
              else rawR = ((uint16_t)(rawR - WII_TRIGGER_MIN) * 255) / (WII_TRIGGER_MAX - WII_TRIGGER_MIN);

              const uint8_t learnedRawL = rawL;
              const uint8_t learnedRawR = rawR;
              if (analogRangeMode == CLASSIC_ANALOG_RANGE_LEARN) {
                rawL = applyClassicAnalogLearnTrigger(deviceMode, i, CLASSIC_ANALOG_TRIGGER_L2, rawL);
                rawR = applyClassicAnalogLearnTrigger(deviceMode, i, CLASSIC_ANALOG_TRIGGER_R2, rawR);
              }
              if (analogRangeMode != CLASSIC_ANALOG_RANGE_LEARN) {
                recordClassicAnalogRangeTrigger(
                    deviceMode, i, CLASSIC_ANALOG_TRIGGER_L2, learnedRawL, rawL);
                recordClassicAnalogRangeTrigger(
                    deviceMode, i, CLASSIC_ANALOG_TRIGGER_R2, learnedRawR, rawR);
              }

              frame.ANALOG_L2 = rawL;
              frame.ANALOG_R2 = rawR;
            }
            cachedControllerType[i] = conType;
            break;
          }
          case ExtensionType::Nunchuk:
          {
            const bool typeChanged = cachedControllerType[i] != conType;
            if (typeChanged) {
              cachedClassicProValid[i] = false;
            }
            frame.HAS_ANALOG_TRIGGERS = false;
            frame.HAS_ANALOG_STICK_AUX = false;
            frame.A = wii_nchuk[i]->buttonC();
            frame.B = wii_nchuk[i]->buttonZ();
            int8_t lx = wii_nchuk[i]->joyX() - 0x80;
            int8_t ly = ~(wii_nchuk[i]->joyY() - 0x80);
            const int8_t raw_lx = lx;
            const int8_t raw_ly = ly;

            if (analogRangeMode == CLASSIC_ANALOG_RANGE_NORMALIZED) {
              const int8_t WII_NATIVE_MAX = 100;
              int16_t scaled_lx = (int16_t)lx * 127 / WII_NATIVE_MAX;
              int16_t scaled_ly = (int16_t)ly * 127 / WII_NATIVE_MAX;

              if (scaled_lx > 127) scaled_lx = 127; if (scaled_lx < -128) scaled_lx = -128;
              if (scaled_ly > 127) scaled_ly = 127; if (scaled_ly < -128) scaled_ly = -128;

              lx = (int8_t)scaled_lx;
              ly = (int8_t)scaled_ly;
            } else if (analogRangeMode == CLASSIC_ANALOG_RANGE_LEARN) {
              lx = applyClassicAnalogLearnAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LX, lx);
              ly = applyClassicAnalogLearnAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LY, ly);
            }

            if (analogRangeMode != CLASSIC_ANALOG_RANGE_LEARN) {
              recordClassicAnalogRangeAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx, lx);
              recordClassicAnalogRangeAxis(deviceMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly, ly);
            }

            frame.LX = lx;
            frame.LY = ly;
            frame.RX = 0;
            frame.RY = 0;
            if (typeChanged) {
              setInputFrameTypeName(frame, "Nunchuk");
            }
            cachedControllerType[i] = conType;
            break;
          }
          #ifdef ENABLE_WII_GUITAR
          case ExtensionType::GuitarController:
          {
            const bool typeChanged = cachedControllerType[i] != conType;
            if (typeChanged) {
              cachedClassicProValid[i] = false;
            }
            frame.HAS_ANALOG_TRIGGERS = false;
            frame.HAS_ANALOG_STICK_AUX = true;
            frame.HAS_BTN_HOME = false;
            frame.HAS_BTN_SELECT = 1;
            frame.HAS_BTN_START = 1;

            frame.A = wii_guitar[i]->fretBlue();
            frame.B = wii_guitar[i]->fretRed();
            frame.X = wii_guitar[i]->fretYellow();
            frame.Y = wii_guitar[i]->fretGreen();
            frame.L1 = wii_guitar[i]->fretOrange();

            frame.PAD_U = wii_guitar[i]->strumUp();
            frame.PAD_D = wii_guitar[i]->strumDown();

            frame.START = wii_guitar[i]->buttonPlus();
            frame.SELECT = wii_guitar[i]->buttonMinus();

            frame.LX = wii_guitar[i]->joyX() - 0x20;
            frame.LY = ~(wii_guitar[i]->joyY() - 0x20);

            uint8_t whammy = wii_guitar[i]->whammyBar();
            frame.RX = map(whammy, 15, 31, -128, 127);
            frame.RY = 0;

            if (typeChanged) {
              setInputFrameTypeName(frame, "Guitar");
            }
            cachedControllerType[i] = conType;
            break;
          }
          #endif
          default:
            if (cachedControllerType[i] != conType) {
              cachedClassicProValid[i] = false;
              setInputFrameTypeName(frame, "Wii");
            }
            cachedControllerType[i] = conType;
            break;
        }

        if (frame.digital_buttons != wii_last_digital_state[i]
         || frame.analog_sticks != wii_last_analog_sticks_state[i]
         || frame.analog_buttons != wii_last_analog_buttons_state[i]) {
          setUpdated(i);
        }

        wii_last_digital_state[i] = frame.digital_buttons;
        wii_last_analog_sticks_state[i] = frame.analog_sticks;
        wii_last_analog_buttons_state[i] = frame.analog_buttons;

        queueWebhidRawData(i, conType);
      }
    }
    if (traceEnabled) {
      wiiTraceRecord(WII_TRACE_EVENT_PORT, i, portTraceStartUs, micros(),
                     haveController[i] ? 1 : 0, updateFailCount[i],
                     (uint8_t)conType);
    }
  }

  const bool updated = endPollCycle();
  if (traceEnabled) {
    wiiTraceRecord(WII_TRACE_EVENT_POLL, 0xFF, pollTraceStartUs, micros(),
                   updated ? 1 : 0, 0, 0);
  }
  return updated;
}

void RZInputWii::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  buildQueuedWebhidRawData();
  flushPendingWebhidRawData();
}
