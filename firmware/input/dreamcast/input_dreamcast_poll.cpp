#include "Input_Dreamcast.h"

#include "../../core/classic_analog_range.h"
#include "../../core/controller_settings_state.h"
#include "../../platform/latency_trace_gpio.h"

namespace {
constexpr uint16_t kAutoVmuScanDelayMs = 250;

constexpr uint8_t kDreamcastAxisCenter = 128;
constexpr uint32_t kDreamcastCapAnalog2X = 1UL << 12;
constexpr uint32_t kDreamcastCapAnalog2Y = 1UL << 13;
constexpr uint32_t kDreamcastCapAnalogX = 1UL << 18;
constexpr uint32_t kDreamcastCapAnalogY = 1UL << 19;
constexpr uint16_t kDreamcastBtnDpad2Up = 1u << 12;
constexpr uint16_t kDreamcastBtnDpad2Down = 1u << 13;
constexpr uint16_t kDreamcastBtnDpad2Left = 1u << 14;
constexpr uint16_t kDreamcastBtnDpad2Right = 1u << 15;

int16_t dreamcastAxisFromRaw(uint8_t raw) {
  return (int16_t)raw - (int16_t)kDreamcastAxisCenter;
}

bool dreamcastButtonPressed(const DreamcastControllerCondition& dc, uint16_t button) {
  return (dc.buttons & button) == 0;
}

uint16_t dreamcastSecondaryDpadButtons(const DreamcastControllerCondition& dc) {
  uint16_t extra = 0;
  if (dreamcastButtonPressed(dc, kDreamcastBtnDpad2Up)) extra |= (1u << 0);
  if (dreamcastButtonPressed(dc, kDreamcastBtnDpad2Down)) extra |= (1u << 1);
  if (dreamcastButtonPressed(dc, kDreamcastBtnDpad2Left)) extra |= (1u << 2);
  if (dreamcastButtonPressed(dc, kDreamcastBtnDpad2Right)) extra |= (1u << 3);
  return extra;
}

bool dreamcastHasControllerCapability(const MapleDeviceInfo& info, uint32_t capability) {
  return (info.func_data[0] & capability) != 0;
}

bool dreamcastIsWheelController(const MapleDeviceInfo& info) {
  return dreamcastHasControllerCapability(info, kDreamcastCapAnalogX) &&
         !dreamcastHasControllerCapability(info, kDreamcastCapAnalogY) &&
         dreamcastHasControllerCapability(info, DC_BTN_LEFT) &&
         dreamcastHasControllerCapability(info, DC_BTN_RIGHT);
}
}  // namespace

void RZInputDreamcast::quietVmuPolling(uint8_t port, uint16_t ms) {
  if (port >= MAX_USB_OUT) {
    return;
  }
  vmuPollQuietUntil[port] = millis() + ms;
}

bool RZInputDreamcast::isVmuPollingQuiet(uint8_t port, uint32_t now) const {
  if (port >= MAX_USB_OUT) {
    return false;
  }
  return (int32_t)(vmuPollQuietUntil[port] - now) > 0;
}

void RZInputDreamcast::scheduleAutoVmuScan(uint8_t port, uint32_t now) {
  if (port >= MAX_USB_OUT) {
    return;
  }
  autoVmuScanDone[port] = false;
  autoVmuScanDueAt[port] = now + kAutoVmuScanDelayMs;
}

void RZInputDreamcast::serviceAutoVmuScan(uint8_t port, uint32_t now) {
  if (port >= input_ports || port >= MAX_USB_OUT || autoVmuScanDone[port] || !maple[port]) {
    return;
  }
  if ((int32_t)(now - autoVmuScanDueAt[port]) < 0) {
    return;
  }

  VMUInfo info{};
  const bool slotA = refreshVmuInfo(port, 0, &info);
  if (!slotA) {
    refreshVmuInfo(port, 1, nullptr);
  }
  autoVmuScanDone[port] = true;
}

bool RZInputDreamcast::poll() {
  beginPollCycle();
  uint32_t now = millis();

  for (uint8_t i = 0; i < input_ports && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = inputFrame(i);
    if (!isVmuPollingQuiet(i, now)) {
      LatencyPhaseTraceScope dcUpdateTrace(LATENCY_TRACE_PHASE_DREAMCAST_UPDATE);
      maple[i]->update();
    }

    bool isConnected = maple[i]->isConnected();
    setInputFrameConnected(frame, isConnected);
    if (isConnected) {
      lastConnectedMs[i] = now;
    }

    if (isConnected && !wasConnected[i]) {
      scheduleAutoVmuScan(i, now);
      resetClassicAnalogLearnState(RZORD_DREAMCAST, i);
    }
    if (isConnected) {
      serviceAutoVmuScan(i, now);
    }

    {
      LatencyPhaseTraceScope dcLabelTrace(LATENCY_TRACE_PHASE_DREAMCAST_LABEL);
      if (isConnected) {
        setInputFrameTypeName(frame, getControllerDeviceLabel(maple[i]->getDeviceInfo(), maple[i]->getAccessoryFunctionMask()));
      } else {
        uint32_t seen_func = maple[i]->getLastSeenFunction();
        seen_func |= maple[i]->getAccessoryFunctionMask();
        if (seen_func != 0) {
          setInputFrameTypeName(frame, getDeviceLabel(seen_func));
        } else {
          setInputFrameTypeName(frame, getDebugStatus(i));
        }
      }
    }

    if (isConnected != wasConnected[i]) {
      wasConnected[i] = isConnected;
      if (!isConnected) {
        autoVmuScanDone[i] = false;
        autoVmuScanDueAt[i] = 0;
        resetState(i);
      }

      setPortLed(i, isConnected ? HIGH : (initSuccess[i] ? LOW : HIGH));
      setUpdated(i);
    }

    if (!isConnected) {
      continue;
    }

    {
      LatencyPhaseTraceScope dcMapTrace(LATENCY_TRACE_PHASE_DREAMCAST_MAP);
      const DreamcastControllerCondition& dc = maple[i]->getController();

      resetState(i);

      frame.PAD_U = maple[i]->buttonPressed(DC_BTN_UP);
      frame.PAD_D = maple[i]->buttonPressed(DC_BTN_DOWN);
      const bool isWheelController = dreamcastIsWheelController(maple[i]->getDeviceInfo());
      frame.PAD_L = !isWheelController && maple[i]->buttonPressed(DC_BTN_LEFT);
      frame.PAD_R = !isWheelController && maple[i]->buttonPressed(DC_BTN_RIGHT);

      frame.A = maple[i]->buttonPressed(DC_BTN_A);
      frame.B = maple[i]->buttonPressed(DC_BTN_B);
      frame.X = maple[i]->buttonPressed(DC_BTN_X);
      frame.Y = maple[i]->buttonPressed(DC_BTN_Y);

      frame.L1 = maple[i]->buttonPressed(DC_BTN_Z);
      frame.R1 = maple[i]->buttonPressed(DC_BTN_C);
      if (dreamcastButtonPressed(dc, DC_BTN_D)) {
        frame.EXTRA |= (1u << 4);
      }
      frame.EXTRA |= dreamcastSecondaryDpadButtons(dc);

      frame.START = maple[i]->buttonPressed(DC_BTN_START);

      frame.ANALOG_L2 = dc.ltrigger;
      frame.ANALOG_R2 = dc.rtrigger;
      frame.HAS_ANALOG_TRIGGERS = true;

      frame.L2 = (dc.ltrigger > 128);
      frame.R2 = (dc.rtrigger > 128);

      constexpr DeviceEnum analogMode = RZORD_DREAMCAST;
      int16_t raw_lx = dreamcastAxisFromRaw(dc.joyx);
      int16_t raw_ly = dreamcastAxisFromRaw(dc.joyy);
      if (wii_analog_range == CLASSIC_ANALOG_RANGE_LEARN) {
        frame.LX = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx);
        frame.LY = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly);
      } else {
        frame.LX = raw_lx;
        frame.LY = raw_ly;
      }
      recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx, frame.LX);
      recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly, frame.LY);
      if (isWheelController) {
        frame.paddle = dc.joyx;
      }

      const bool hasAuxX = dreamcastHasControllerCapability(maple[i]->getDeviceInfo(), kDreamcastCapAnalog2X);
      const bool hasAuxY = dreamcastHasControllerCapability(maple[i]->getDeviceInfo(), kDreamcastCapAnalog2Y);
      frame.RX = 0;
      frame.RY = 0;
      if (hasAuxX) {
        int16_t raw_rx = dreamcastAxisFromRaw(dc.joyx2);
        frame.RX = (wii_analog_range == CLASSIC_ANALOG_RANGE_LEARN)
                     ? applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RX, raw_rx)
                     : raw_rx;
        recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RX, raw_rx, frame.RX);
      }
      if (hasAuxY) {
        int16_t raw_ry = dreamcastAxisFromRaw(dc.joyy2);
        frame.RY = (wii_analog_range == CLASSIC_ANALOG_RANGE_LEARN)
                     ? applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RY, raw_ry)
                     : raw_ry;
        recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_RY, raw_ry, frame.RY);
      }
      frame.HAS_ANALOG_STICK_AUX = hasAuxX || hasAuxY;

      setUpdated(i);
    }
  }

  pendingWebHidDebugFrame = true;
  pendingSerialDebugNow = now;

  return endPollCycle();
}

void RZInputDreamcast::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  if (pendingWebHidDebugFrame) {
    LatencyPhaseTraceScope dcWebhidTrace(LATENCY_TRACE_PHASE_DREAMCAST_WEBHID);
    storeWebHidDebugFrame();
    printSerialDebug(pendingSerialDebugNow);
    pendingWebHidDebugFrame = false;
  }
}

bool RZInputDreamcast::hasPhysicalConnectionForHotSwap() const {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < input_ports && i < MAX_USB_OUT; ++i) {
    if (wasConnected[i]) {
      return true;
    }
    if (lastConnectedMs[i] != 0 &&
        (uint32_t)(now - lastConnectedMs[i]) <= HOTSWAP_RECENT_CONNECTION_GRACE_MS) {
      return true;
    }
  }
  return false;
}
