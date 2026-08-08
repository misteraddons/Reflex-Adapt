#include "latency_test.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../core/controller_delivery_state.h"
#include "../core/controller_frame_state.h"
#include "../core/device_runtime_state.h"
#include "../output/output_runtime_state.h"

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include <tusb.h>
#endif

namespace {

constexpr uint32_t kSyntheticButtonMasks[] = {
  1u << 0,  // A
  1u << 1,  // B
  1u << 2,  // X
  1u << 3,  // Y
  1u << 4,  // L1
  1u << 5,  // R1
  1u << 6,  // L2
  1u << 8,  // L3
  1u << 9,  // R3
  1u << 10, // Start
  1u << 11, // Select
};
LatencyTest* g_latency_test_instance = nullptr;
#ifdef ADAPT_OUTPUT_USB_DEVICE
volatile uint32_t g_lastUsbSofFrame = 0;
volatile uint32_t g_lastUsbSofUs = 0;
volatile bool g_usbSofSeen = false;
#endif

uint32_t syntheticButtonMaskForSequence(uint32_t seq) {
  constexpr uint8_t kMaskCount = sizeof(kSyntheticButtonMasks) / sizeof(kSyntheticButtonMasks[0]);
  return kSyntheticButtonMasks[(seq == 0 ? 0 : (seq - 1u)) % kMaskCount];
}

bool timeReached(uint32_t nowUs, uint32_t targetUs) {
  return (int32_t)(nowUs - targetUs) >= 0;
}

void latencyReturnInterruptThunk() {
  if (g_latency_test_instance != nullptr) {
    g_latency_test_instance->handleReturnInterrupt();
  }
}

#ifdef ADAPT_OUTPUT_USB_DEVICE
void setLatencySofEnabled(bool enabled) {
  tud_sof_cb_enable(enabled);
  if (!enabled) {
    g_usbSofSeen = false;
    g_lastUsbSofFrame = 0;
    g_lastUsbSofUs = 0;
  }
}

uint16_t latestUsbSofFrame() {
  return (uint16_t)(g_lastUsbSofFrame & 0x07FFu);
}

uint16_t usbSofAgeUs(uint32_t nowUs) {
  if (!g_usbSofSeen) {
    return 0xFFFFu;
  }
  const uint32_t age = nowUs - g_lastUsbSofUs;
  return (age > 0xFFFEu) ? 0xFFFEu : (uint16_t)age;
}
#else
void setLatencySofEnabled(bool) {}
uint16_t latestUsbSofFrame() { return 0; }
uint16_t usbSofAgeUs(uint32_t) { return 0xFFFFu; }
#endif

enum LatencyDeltaField : uint8_t {
  LATENCY_DELTA_RAW = 0,
  LATENCY_DELTA_DETECT,
  LATENCY_DELTA_PREPARE,
  LATENCY_DELTA_USB_SUBMIT,
  LATENCY_DELTA_HOST_RETURN,
};

uint32_t sampleDeltaFromStimulus(const LatencySample& sample, LatencyDeltaField field) {
  uint32_t eventUs = 0;
  switch (field) {
    case LATENCY_DELTA_RAW:
      eventUs = sample.raw_detect_us;
      break;
    case LATENCY_DELTA_DETECT:
      eventUs = sample.detect_us;
      break;
    case LATENCY_DELTA_PREPARE:
      eventUs = sample.output_prepare_us;
      break;
    case LATENCY_DELTA_USB_SUBMIT:
      eventUs = sample.usb_submit_us;
      break;
    case LATENCY_DELTA_HOST_RETURN:
      eventUs = sample.host_return_us;
      break;
  }

  return (eventUs != 0 && sample.stimulus_us != 0)
    ? (eventUs - sample.stimulus_us)
    : 0u;
}

size_t appendToBuffer(char* buffer, size_t capacity, size_t offset, const char* format, ...) {
  if (capacity == 0) {
    va_list args;
    va_start(args, format);
    const int written = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    return offset + ((written > 0) ? (size_t)written : 0u);
  }

  va_list args;
  va_start(args, format);
  const int written = vsnprintf(buffer + (offset < capacity ? offset : capacity - 1u),
                                (offset < capacity) ? (capacity - offset) : 0u,
                                format,
                                args);
  va_end(args);

  if (written <= 0) {
    return offset;
  }
  return offset + (size_t)written;
}

}  // namespace

#ifdef ADAPT_OUTPUT_USB_DEVICE
extern "C" void tud_sof_cb(uint32_t frame_count) {
  g_lastUsbSofFrame = frame_count;
  g_lastUsbSofUs = micros();
  g_usbSofSeen = true;
}
#endif

const char* latencyTestModeName(uint8_t mode) {
  switch ((LatencyTestMode)mode) {
    case LATENCY_MODE_OFF:
      return "OFF";
    case LATENCY_MODE_PULSE:
      return "PULSE";
    case LATENCY_MODE_SYNTH_INTERNAL:
      return "SYNTH_INTERNAL";
    case LATENCY_MODE_SYNTH_PC:
      return "SYNTH_PC";
    case LATENCY_MODE_SYNTH_MISTER:
      return "SYNTH_MISTER";
    case LATENCY_MODE_TRIGGER_INTERNAL:
      return "TRIG_INTERNAL";
    case LATENCY_MODE_TRIGGER_PC:
      return "TRIG_PC";
    case LATENCY_MODE_TRIGGER_MISTER:
      return "TRIG_MISTER";
    default:
      return "UNKNOWN";
  }
}

const char* latencyHostTypeName(uint8_t hostType) {
  switch ((LatencyHostType)hostType) {
    case LATENCY_HOST_INTERNAL:
      return "INTERNAL";
    case LATENCY_HOST_PC:
      return "PC";
    case LATENCY_HOST_MISTER:
      return "MISTER";
    default:
      return "UNKNOWN";
  }
}

LatencyTest::LatencyTest()
  : pin(LATENCY_PULSE_PIN),
    triggerPin(LATENCY_TRIGGER_PIN),
    returnPin(LATENCY_RETURN_PIN),
    enabled(false),
    controllerInLoop(false),
    hostType(LATENCY_HOST_INTERNAL),
    currentMode(LATENCY_MODE_OFF),
    triggerActiveHigh(false),
    lastRawButtonState(0),
    lastButtonState(0),
    pulseEndTime(0),
    pulsing(false),
    pulsePinArmed(false),
    benchRunning(false),
    triggerHeld(false),
    syntheticActive(false),
    outputFrameBackedUp(false),
    pendingBenchStopLog(false),
    serialLogEnabled(false),
    pendingReturnEdge(false),
    pendingReturnEdgeUs(0),
    nextStimulusAtUs(0),
    holdUntilUs(0),
    sampleDeadlineUs(0),
    activeTargetSamples(0),
    completedRunSamples(0),
    nextSequence(1),
    runStartUs(0),
    runId(0),
    runInputMode(RZORD_NONE),
    runConfiguredOutputMode(OUTPUT_AUTO),
    runRuntimeOutputMode(OUTPUT_AUTO),
    outputFrameBackup{},
    activeSample{},
    activeSampleInUse(false),
#if defined(ADAPT_ENABLE_LATENCY_TEST)
    sampleRing{},
#endif
    sampleHead(0),
    sampleCount(0),
    pendingLogCount(0) {}

bool LatencyTest::isBenchMode(uint8_t mode) const {
  switch ((LatencyTestMode)mode) {
    case LATENCY_MODE_SYNTH_INTERNAL:
    case LATENCY_MODE_SYNTH_PC:
    case LATENCY_MODE_SYNTH_MISTER:
    case LATENCY_MODE_TRIGGER_INTERNAL:
    case LATENCY_MODE_TRIGGER_PC:
    case LATENCY_MODE_TRIGGER_MISTER:
      return true;
    default:
      return false;
  }
}

bool LatencyTest::usesSyntheticStimulus() const {
  return currentMode == LATENCY_MODE_SYNTH_INTERNAL ||
         currentMode == LATENCY_MODE_SYNTH_PC ||
         currentMode == LATENCY_MODE_SYNTH_MISTER;
}

bool LatencyTest::usesTriggerOutput() const {
  return currentMode == LATENCY_MODE_TRIGGER_INTERNAL ||
         currentMode == LATENCY_MODE_TRIGGER_PC ||
         currentMode == LATENCY_MODE_TRIGGER_MISTER;
}

bool LatencyTest::usesHostGpioReturn() const {
  return currentMode == LATENCY_MODE_SYNTH_MISTER ||
         currentMode == LATENCY_MODE_TRIGGER_MISTER;
}

bool LatencyTest::usesPcAckReturn() const {
  return currentMode == LATENCY_MODE_SYNTH_PC ||
         currentMode == LATENCY_MODE_TRIGGER_PC;
}

bool LatencyTest::usesInternalLoopReturn() const {
  return currentMode == LATENCY_MODE_SYNTH_INTERNAL ||
         currentMode == LATENCY_MODE_TRIGGER_INTERNAL;
}

uint32_t LatencyTest::randomSampleDelayUs() const {
  if (LATENCY_SAMPLE_DELAY_MAX_US <= LATENCY_SAMPLE_DELAY_MIN_US) {
    return LATENCY_SAMPLE_DELAY_MIN_US;
  }
  return (uint32_t)random((long)LATENCY_SAMPLE_DELAY_MIN_US,
                          (long)LATENCY_SAMPLE_DELAY_MAX_US + 1L);
}

void LatencyTest::syncModeFromConfiguration() {
  if (!enabled) {
    currentMode = LATENCY_MODE_OFF;
    return;
  }

  switch ((LatencyHostType)hostType) {
    case LATENCY_HOST_PC:
      currentMode = controllerInLoop ? LATENCY_MODE_TRIGGER_PC : LATENCY_MODE_SYNTH_PC;
      break;
    case LATENCY_HOST_MISTER:
      currentMode = controllerInLoop ? LATENCY_MODE_TRIGGER_MISTER : LATENCY_MODE_SYNTH_MISTER;
      break;
    case LATENCY_HOST_INTERNAL:
    default:
      currentMode = controllerInLoop ? LATENCY_MODE_TRIGGER_INTERNAL : LATENCY_MODE_SYNTH_INTERNAL;
      break;
  }
}

void LatencyTest::armPulsePin() {
  if (pin == LATENCY_PIN_UNUSED || pulsePinArmed) {
    return;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  pulsePinArmed = true;
}

void LatencyTest::releasePulsePin() {
  if (pin == LATENCY_PIN_UNUSED || !pulsePinArmed) {
    return;
  }
  digitalWrite(pin, LOW);
  pinMode(pin, INPUT);
  pulsePinArmed = false;
}

void LatencyTest::begin() {
#if !defined(ADAPT_ENABLE_LATENCY_TEST)
  enabled = false;
  currentMode = LATENCY_MODE_OFF;
  return;
#else
  g_latency_test_instance = this;
  randomSeed(micros() ^ ((uint32_t)pin << 8) ^ ((uint32_t)triggerPin << 16) ^
             ((uint32_t)returnPin << 24));

  setLatencySofEnabled(enabled);
  if (enabled) {
    armPulsePin();
  }

  releaseTrigger();

  if (returnPin != LATENCY_PIN_UNUSED) {
    pinMode(returnPin, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(returnPin), latencyReturnInterruptThunk, RISING);
  }
#endif
}

void LatencyTest::setEnabled(bool en) {
#if !defined(ADAPT_ENABLE_LATENCY_TEST)
  (void)en;
  enabled = false;
  currentMode = LATENCY_MODE_OFF;
  return;
#else
  if (enabled == en) {
    syncModeFromConfiguration();
    return;
  }

  stopBench();
  enabled = en;
  if (enabled) {
    setLatencySofEnabled(true);
    armPulsePin();
  } else {
    releasePulsePin();
    setLatencySofEnabled(false);
  }
  syncModeFromConfiguration();
  reset();
#endif
}

void LatencyTest::setMode(uint8_t mode) {
#if !defined(ADAPT_ENABLE_LATENCY_TEST)
  (void)mode;
  enabled = false;
  currentMode = LATENCY_MODE_OFF;
  return;
#else
  if (mode >= LATENCY_MODE_LAST) {
    mode = LATENCY_MODE_OFF;
  }

  stopBench();
  clearActiveStimulusState();

  switch ((LatencyTestMode)mode) {
    case LATENCY_MODE_OFF:
      enabled = false;
      controllerInLoop = false;
      hostType = LATENCY_HOST_INTERNAL;
      break;
    case LATENCY_MODE_PULSE:
    case LATENCY_MODE_SYNTH_INTERNAL:
      enabled = true;
      controllerInLoop = false;
      hostType = LATENCY_HOST_INTERNAL;
      break;
    case LATENCY_MODE_SYNTH_PC:
      enabled = true;
      controllerInLoop = false;
      hostType = LATENCY_HOST_PC;
      break;
    case LATENCY_MODE_SYNTH_MISTER:
      enabled = true;
      controllerInLoop = false;
      hostType = LATENCY_HOST_MISTER;
      break;
    case LATENCY_MODE_TRIGGER_INTERNAL:
      enabled = true;
      controllerInLoop = true;
      hostType = LATENCY_HOST_INTERNAL;
      break;
    case LATENCY_MODE_TRIGGER_PC:
      enabled = true;
      controllerInLoop = true;
      hostType = LATENCY_HOST_PC;
      break;
    case LATENCY_MODE_TRIGGER_MISTER:
      enabled = true;
      controllerInLoop = true;
      hostType = LATENCY_HOST_MISTER;
      break;
    default:
      enabled = false;
      controllerInLoop = false;
      hostType = LATENCY_HOST_INTERNAL;
      break;
  }

  if (enabled) {
    setLatencySofEnabled(true);
    armPulsePin();
  } else {
    releasePulsePin();
    setLatencySofEnabled(false);
  }

  syncModeFromConfiguration();
  reset();
#endif
}

void LatencyTest::setControllerInLoop(bool en) {
  if (controllerInLoop == en) {
    return;
  }
  stopBench();
  controllerInLoop = en;
  syncModeFromConfiguration();
  reset();
}

void LatencyTest::setHostType(uint8_t host) {
  if (host >= LATENCY_HOST_LAST) {
    host = LATENCY_HOST_INTERNAL;
  }
  if (hostType == host) {
    return;
  }
  stopBench();
  hostType = host;
  syncModeFromConfiguration();
  reset();
}

void LatencyTest::setTriggerActiveHigh(bool activeHigh) {
  if (triggerActiveHigh == activeHigh) {
    return;
  }
  stopBench();
  triggerActiveHigh = activeHigh;
  releaseTrigger();
}

bool LatencyTest::isCurrentModeReady() const {
  if (!enabled) {
    return false;
  }
  if (!isBenchMode(currentMode)) {
    return pin != LATENCY_PIN_UNUSED;
  }
  if (usesTriggerOutput() && triggerPin == LATENCY_PIN_UNUSED) {
    return false;
  }
  if (usesHostGpioReturn() && returnPin == LATENCY_PIN_UNUSED) {
    return false;
  }
  return true;
}

void LatencyTest::checkButtons(uint32_t buttonState) {
  observeButtons(0, buttonState, true);
}

void LatencyTest::observeRawButtons(uint8_t port, uint32_t rawState, bool connected) {
  if (!enabled || port != 0) {
    return;
  }

  const uint32_t currentButtons = connected ? rawState : 0;
  const uint32_t newPresses = currentButtons & ~lastRawButtonState;
  lastRawButtonState = currentButtons;

  if (!activeSampleInUse ||
      (activeSample.flags & LATENCY_FLAG_RAW_DETECTED) ||
      newPresses == 0) {
    return;
  }

  activeSample.raw_detect_us = micros();
  activeSample.flags |= LATENCY_FLAG_RAW_DETECTED;
}

void LatencyTest::observeButtons(uint8_t port, uint32_t buttonState, bool connected) {
  if (!enabled || port != 0) {
    return;
  }

  const uint32_t nowUs = micros();
  const uint32_t currentButtons = connected ? buttonState : 0;
  const uint32_t newPresses = currentButtons & ~lastButtonState;
  lastButtonState = currentButtons;

  if (newPresses != 0) {
    pulse();
  }

  if (!benchRunning && serialLogEnabled && !activeSampleInUse && newPresses != 0) {
    armPassiveInputSample(nowUs, newPresses);
    return;
  }

  if (!activeSampleInUse || (activeSample.flags & LATENCY_FLAG_DETECTED) || newPresses == 0) {
    return;
  }

  activeSample.button_mask = newPresses;
  activeSample.detect_us = nowUs;
  activeSample.flags |= LATENCY_FLAG_DETECTED;
}

void LatencyTest::pulse() {
  if (!enabled || pin == LATENCY_PIN_UNUSED) {
    return;
  }
  armPulsePin();
  digitalWrite(pin, HIGH);
  pulsing = true;
  pulseEndTime = micros() + LATENCY_PULSE_US;
}

void LatencyTest::pulseBlocking() {
  if (!enabled || pin == LATENCY_PIN_UNUSED) {
    return;
  }
  armPulsePin();
  digitalWrite(pin, HIGH);
  delayMicroseconds(LATENCY_PULSE_US);
  digitalWrite(pin, LOW);
}

void LatencyTest::pressTrigger() {
  if (triggerPin == LATENCY_PIN_UNUSED) {
    return;
  }

  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, triggerActiveHigh ? HIGH : LOW);
  triggerHeld = true;
}

void LatencyTest::releaseTrigger() {
  if (triggerPin == LATENCY_PIN_UNUSED) {
    triggerHeld = false;
    return;
  }

  digitalWrite(triggerPin, LOW);
  if (triggerActiveHigh) {
    pinMode(triggerPin, OUTPUT);
  } else {
    pinMode(triggerPin, INPUT_PULLUP);
  }
  triggerHeld = false;
}

void LatencyTest::clearActiveStimulusState() {
  releaseTrigger();
  syntheticActive = false;
  outputFrameBackedUp = false;
  activeSampleInUse = false;
}

void LatencyTest::pushCompletedSample(const LatencySample& sample) {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  sampleRing[sampleHead] = sample;
  sampleHead = (uint16_t)((sampleHead + 1u) % kSampleRingSize);
  if (sampleCount < kSampleRingSize) {
    ++sampleCount;
  }
  if (pendingLogCount < kSampleRingSize) {
    ++pendingLogCount;
  }
#else
  (void)sample;
#endif
}

void LatencyTest::completeActiveSample(uint32_t nowUs, uint16_t extraFlags) {
  if (!activeSampleInUse) {
    return;
  }

  LatencySample completed = activeSample;
  completed.complete_us = nowUs;
  completed.flags = (uint16_t)(completed.flags | extraFlags);
  pushCompletedSample(completed);

  activeSampleInUse = false;
  syntheticActive = false;
  outputFrameBackedUp = false;
  releaseTrigger();

  ++completedRunSamples;
  if (activeTargetSamples > 0) {
    --activeTargetSamples;
    if (activeTargetSamples == 0) {
      benchRunning = false;
      pendingBenchStopLog = true;
      return;
    }
  }

  nextStimulusAtUs = nowUs + randomSampleDelayUs();
}

void LatencyTest::armNextBenchSample(uint32_t nowUs) {
  activeSample = {};
  activeSample.seq = nextSequence++;
  activeSample.mode = currentMode;
  activeSample.stimulus_us = nowUs;
  activeSample.input_mode = (uint8_t)deviceMode;
  activeSample.configured_output_mode = (uint8_t)configuredOutputMode;
  activeSample.runtime_output_mode = (uint8_t)outputMode;
  activeSampleInUse = true;
  sampleDeadlineUs = nowUs + LATENCY_SAMPLE_TIMEOUT_US;

  if (usesSyntheticStimulus()) {
    activeSample.button_mask = syntheticButtonMaskForSequence(activeSample.seq);
    activeSample.detect_us = nowUs;
    activeSample.flags = (uint16_t)(LATENCY_FLAG_SYNTHETIC | LATENCY_FLAG_DETECTED);
    syntheticActive = true;
    holdUntilUs = nowUs + LATENCY_TRIGGER_PRESS_US;
  } else if (usesTriggerOutput()) {
    activeSample.flags = LATENCY_FLAG_TRIGGER;
    holdUntilUs = nowUs + LATENCY_TRIGGER_PRESS_US;
    pressTrigger();
  }
}

void LatencyTest::handlePendingReturnEdge(uint32_t nowUs) {
  if (!pendingReturnEdge) {
    return;
  }

  noInterrupts();
  const uint32_t edgeUs = pendingReturnEdgeUs;
  pendingReturnEdge = false;
  interrupts();

  if (activeSampleInUse && usesHostGpioReturn() && !(activeSample.flags & LATENCY_FLAG_HOST_GPIO)) {
    activeSample.host_return_us = edgeUs;
    completeActiveSample(nowUs, LATENCY_FLAG_HOST_GPIO);
  }
}

void LatencyTest::armPassiveInputSample(uint32_t nowUs, uint32_t buttonMask) {
  activeSample = {};
  activeSample.seq = nextSequence++;
  activeSample.mode = currentMode;
  activeSample.button_mask = buttonMask;
  activeSample.stimulus_us = nowUs;
  activeSample.detect_us = nowUs;
  activeSample.input_mode = (uint8_t)deviceMode;
  activeSample.configured_output_mode = (uint8_t)configuredOutputMode;
  activeSample.runtime_output_mode = (uint8_t)outputMode;
  activeSample.flags = LATENCY_FLAG_DETECTED;
  activeSampleInUse = true;
  sampleDeadlineUs = nowUs + LATENCY_SAMPLE_TIMEOUT_US;
}

void LatencyTest::update() {
  const uint32_t nowUs = micros();

  if (pulsing && timeReached(nowUs, pulseEndTime)) {
    if (pulsePinArmed) {
      digitalWrite(pin, LOW);
    }
    pulsing = false;
  }

  handlePendingReturnEdge(nowUs);

  if (!benchRunning && activeSampleInUse && timeReached(nowUs, sampleDeadlineUs)) {
    completeActiveSample(nowUs, LATENCY_FLAG_TIMEOUT);
    return;
  }

  if (!benchRunning) {
    return;
  }

  if (triggerHeld && timeReached(nowUs, holdUntilUs)) {
    releaseTrigger();
  }

  if (syntheticActive && !usesPcAckReturn() && timeReached(nowUs, holdUntilUs)) {
    syntheticActive = false;
  }

  if (activeSampleInUse && timeReached(nowUs, sampleDeadlineUs)) {
    completeActiveSample(nowUs, LATENCY_FLAG_TIMEOUT);
    return;
  }

  if (!activeSampleInUse && timeReached(nowUs, nextStimulusAtUs)) {
    armNextBenchSample(nowUs);
  }
}

void LatencyTest::reset() {
  lastRawButtonState = 0;
  lastButtonState = 0;
  pulseEndTime = 0;
  pulsing = false;
  pendingReturnEdge = false;
  pendingReturnEdgeUs = 0;
  clearActiveStimulusState();
  if (pulsePinArmed) {
    digitalWrite(pin, LOW);
  }
  if (!enabled) {
    releasePulsePin();
  }
}

void LatencyTest::startBench(uint32_t sampleCount) {
#if !defined(ADAPT_ENABLE_LATENCY_TEST)
  (void)sampleCount;
  return;
#else
  if (!isCurrentModeReady() || !isBenchMode(currentMode)) {
    return;
  }
  armPulsePin();

  if (sampleCount == 0) {
    sampleCount = LATENCY_DEFAULT_SAMPLE_COUNT;
  }
  if (sampleCount > kSampleRingSize) {
    sampleCount = kSampleRingSize;
  }

  clearSamples();
  ++runId;
  runStartUs = micros();
  runInputMode = (uint8_t)deviceMode;
  runConfiguredOutputMode = (uint8_t)configuredOutputMode;
  runRuntimeOutputMode = (uint8_t)outputMode;
  activeTargetSamples = sampleCount;
  lastRawButtonState = 0;
  lastButtonState = 0;
  benchRunning = true;
  pendingBenchStopLog = false;
  clearActiveStimulusState();
  nextStimulusAtUs = runStartUs + 10000u;
#endif
}

void LatencyTest::stopBench() {
  benchRunning = false;
  activeTargetSamples = 0;
  pendingBenchStopLog = false;
  clearActiveStimulusState();
}

void LatencyTest::clearSamples() {
  stopBench();
  sampleHead = 0;
  sampleCount = 0;
  pendingLogCount = 0;
  completedRunSamples = 0;
  nextSequence = 1;
  lastRawButtonState = 0;
  lastButtonState = 0;
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  memset(sampleRing, 0, sizeof(sampleRing));
#endif
  activeSample = {};
}

void LatencyTest::prepareOutputFrame() {
  if (!enabled || !usesSyntheticStimulus() || !activeSampleInUse || !syntheticActive || outputFrameBackedUp) {
    return;
  }

  const bool pcAckSynthetic = usesPcAckReturn();
  const uint32_t nowUs = micros();
  if (pcAckSynthetic && (activeSample.flags & LATENCY_FLAG_USB_SUBMIT) && !timeReached(nowUs, holdUntilUs)) {
    return;
  }
  if (pcAckSynthetic) {
    holdUntilUs = nowUs + LATENCY_TRIGGER_PRESS_US;
  }

  controller_state_t& frame = controllerFrame(0);
  outputFrameBackup = frame;
  outputFrameBackedUp = true;
  frame.connected = true;
  frame.HAS_BTN_HOME = 1;
  frame.HAS_BTN_SELECT = 1;
  frame.HAS_BTN_START = 1;
  frame.A = 1;
  frame.digital_buttons |= activeSample.button_mask ? activeSample.button_mask : syntheticButtonMaskForSequence(activeSample.seq);
  if (frame.controller_type_name[0] == '\0') {
    strncpy(frame.controller_type_name, "LATENCY", sizeof(frame.controller_type_name) - 1u);
    frame.controller_type_name[sizeof(frame.controller_type_name) - 1u] = '\0';
  }
  requestControllerFrameDelivery(0);
}

void LatencyTest::restoreOutputFrame() {
  if (!outputFrameBackedUp) {
    return;
  }
  controllerFrame(0) = outputFrameBackup;
  outputFrameBackedUp = false;
}

void LatencyTest::noteOutputFramePrepared() {
  if (!activeSampleInUse || (activeSample.flags & LATENCY_FLAG_OUTPUT_PREPARE)) {
    return;
  }
  if (!(activeSample.flags & LATENCY_FLAG_DETECTED)) {
    return;
  }

  activeSample.output_prepare_us = micros();
  activeSample.flags |= LATENCY_FLAG_OUTPUT_PREPARE;
}

void LatencyTest::noteUsbSubmit(uint8_t port, uint32_t) {
  if (port != 0 || !activeSampleInUse || (activeSample.flags & LATENCY_FLAG_USB_SUBMIT)) {
    return;
  }
  if (usesTriggerOutput() && !(activeSample.flags & LATENCY_FLAG_DETECTED)) {
    return;
  }

  activeSample.usb_submit_us = micros();
  activeSample.usb_submit_sof = latestUsbSofFrame();
  activeSample.usb_submit_sof_age_us = usbSofAgeUs(activeSample.usb_submit_us);
  if (!(activeSample.flags & LATENCY_FLAG_OUTPUT_PREPARE)) {
    activeSample.output_prepare_us = activeSample.usb_submit_us;
    activeSample.flags |= LATENCY_FLAG_OUTPUT_PREPARE;
  }
  activeSample.flags |= LATENCY_FLAG_USB_SUBMIT;

  if (usesInternalLoopReturn()) {
    activeSample.host_return_us = activeSample.usb_submit_us;
    completeActiveSample(activeSample.usb_submit_us, LATENCY_FLAG_HOST_INTERNAL);
  }
}

void LatencyTest::acknowledgeHost(uint32_t seq) {
  if (!activeSampleInUse || !usesPcAckReturn() || (activeSample.flags & LATENCY_FLAG_HOST_ACK)) {
    return;
  }
  if (seq != 0 && seq != activeSample.seq) {
    return;
  }

  activeSample.host_return_us = micros();
  completeActiveSample(activeSample.host_return_us, LATENCY_FLAG_HOST_ACK);
}

void LatencyTest::printSampleLine(Print& out, const LatencySample& sample) const {
  out.print(F("LATDONE SEQ="));
  out.print(sample.seq);
  out.print(F(" MODE="));
  out.print(latencyTestModeName(sample.mode));
  out.print(F(" BTN=0x"));
  out.print(sample.button_mask, HEX);
  out.print(F(" STIM="));
  out.print(sample.stimulus_us);
  out.print(F(" DET="));
  out.print(sample.detect_us);
  out.print(F(" PREP="));
  out.print(sample.output_prepare_us);
  out.print(F(" USB="));
  out.print(sample.usb_submit_us);
  out.print(F(" SOF="));
  out.print(sample.usb_submit_sof);
  out.print(F(" SOFAGE="));
  if (sample.usb_submit_sof_age_us == 0xFFFFu) {
    out.print(F("NA"));
  } else {
    out.print(sample.usb_submit_sof_age_us);
  }
  out.print(F(" RET="));
  out.print(sample.host_return_us);
  out.print(F(" D_DET="));
  out.print((sample.detect_us != 0 && sample.stimulus_us != 0) ? (sample.detect_us - sample.stimulus_us) : 0);
  out.print(F(" D_RAW="));
  out.print(sampleDeltaFromStimulus(sample, LATENCY_DELTA_RAW));
  out.print(F(" D_PREP="));
  out.print(sampleDeltaFromStimulus(sample, LATENCY_DELTA_PREPARE));
  out.print(F(" D_USB="));
  out.print((sample.usb_submit_us != 0 && sample.stimulus_us != 0) ? (sample.usb_submit_us - sample.stimulus_us) : 0);
  out.print(F(" D_RET="));
  out.print((sample.host_return_us != 0 && sample.stimulus_us != 0) ? (sample.host_return_us - sample.stimulus_us) : 0);
  out.print(F(" IM="));
  out.print(sample.input_mode);
  out.print(F(" CO="));
  out.print(sample.configured_output_mode);
  out.print(F(" RO="));
  out.print(sample.runtime_output_mode);
  out.print(F(" FLAGS=0x"));
  out.println(sample.flags, HEX);
}

void LatencyTest::flushLog(Print& out) {
  if (!serialLogEnabled) {
    return;
  }

  if (pendingBenchStopLog) {
    out.print(F("LATSTOP DONE="));
    out.println(completedRunSamples);
    pendingBenchStopLog = false;
  }

  if (pendingLogCount == 0) {
    return;
  }

#if defined(ADAPT_ENABLE_LATENCY_TEST)
  const uint16_t start = (uint16_t)((sampleHead + kSampleRingSize - pendingLogCount) % kSampleRingSize);
  for (uint16_t i = 0; i < pendingLogCount; ++i) {
    const uint16_t index = (uint16_t)((start + i) % kSampleRingSize);
    printSampleLine(out, sampleRing[index]);
  }
#endif
  pendingLogCount = 0;
}

void LatencyTest::writeStatus(Print& out) const {
  out.print(F("LAT MODE="));
  out.print(latencyTestModeName(currentMode));
  out.print(F(" ENABLED="));
  out.print(enabled ? 1 : 0);
  out.print(F(" CTRL="));
  out.print(controllerInLoop ? 1 : 0);
  out.print(F(" HOST="));
  out.print(latencyHostTypeName(hostType));
  out.print(F(" LOG="));
  out.print(serialLogEnabled ? 1 : 0);
  out.print(F(" READY="));
  out.print(isCurrentModeReady() ? 1 : 0);
  out.print(F(" RUN="));
  out.print(benchRunning ? 1 : 0);
  out.print(F(" TARGET="));
  out.print(activeTargetSamples);
  out.print(F(" DONE="));
  out.print(completedRunSamples);
  out.print(F(" STORED="));
  out.print(sampleCount);
  out.print(F("/"));
  out.print((int)kSampleRingSize);
  out.print(F(" ACTIVE_SEQ="));
  out.print(activeSampleInUse ? activeSample.seq : 0);
  out.print(F(" RUN_ID="));
  out.print(runId);
  out.print(F(" PULSE_PIN="));
  out.print((int)pin);
  out.print(F(" TRIG_PIN="));
  out.print((int)triggerPin);
  out.print(F(" TRIG_POL="));
  out.print(triggerActiveHigh ? F("HIGH") : F("LOW"));
  out.print(F(" RET_PIN="));
  out.println((int)returnPin);
}

void LatencyTest::dumpSamples(Print& out) const {
  out.print(F("LATDUMP COUNT="));
  out.println(sampleCount);
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  const uint16_t start = (uint16_t)((sampleHead + kSampleRingSize - sampleCount) % kSampleRingSize);
  for (uint16_t i = 0; i < sampleCount; ++i) {
    const uint16_t index = (uint16_t)((start + i) % kSampleRingSize);
    printSampleLine(out, sampleRing[index]);
  }
#endif
}

size_t LatencyTest::writeExportStatus(char* buffer, size_t capacity) const {
  size_t offset = 0;
  if (capacity > 0) {
    buffer[0] = '\0';
  }

#if !defined(ADAPT_ENABLE_LATENCY_TEST)
  offset = appendToBuffer(buffer, capacity, offset,
                          "Reflex Latency Export\r\n"
                          "====================\r\n"
                          "\r\n"
                          "Latency export is unavailable in retail firmware.\r\n");
  if (capacity > 0) {
    buffer[min(offset, capacity - 1u)] = '\0';
  }
  return (offset < capacity) ? offset : (capacity - 1u);
#else
  offset = appendToBuffer(buffer, capacity, offset,
                          "Reflex Latency Export\r\n"
                          "====================\r\n"
                          "\r\n"
                          "Enabled: %u\r\n"
                          "Mode: %s\r\n"
                          "Controller in loop: %s\r\n"
                          "Host type: %s\r\n"
                          "Ready: %u\r\n"
                          "Running: %u\r\n"
                          "Run ID: %lu\r\n"
                          "Run start us: %lu\r\n"
                          "Input mode: %u\r\n"
                          "Configured output: %u (%s)\r\n"
                          "Runtime output: %u (%s)\r\n"
                          "Samples stored: %u / %u\r\n"
                          "Samples completed: %lu\r\n"
                          "Pulse pin: %d\r\n"
                          "Trigger pin: %d\r\n"
                          "Trigger polarity: %s\r\n"
                          "Return pin: %d\r\n",
                          enabled ? 1u : 0u,
                          latencyTestModeName(currentMode),
                          controllerInLoop ? "yes" : "no",
                          latencyHostTypeName(hostType),
                          isCurrentModeReady() ? 1u : 0u,
                          benchRunning ? 1u : 0u,
                          (unsigned long)runId,
                          (unsigned long)runStartUs,
                          (unsigned)runInputMode,
                          (unsigned)runConfiguredOutputMode,
                          get_mode_name((outputMode_t)runConfiguredOutputMode),
                          (unsigned)runRuntimeOutputMode,
                          get_mode_name((outputMode_t)runRuntimeOutputMode),
                          (unsigned)sampleCount,
                          (unsigned)kSampleRingSize,
                          (unsigned long)completedRunSamples,
                          (int)pin,
                          (int)triggerPin,
                          triggerActiveHigh ? "active-high" : "active-low",
                          (int)returnPin);

  const uint16_t start = (uint16_t)((sampleHead + kSampleRingSize - sampleCount) % kSampleRingSize);
  uint16_t timeoutCount = 0;
  for (uint16_t i = 0; i < sampleCount; ++i) {
    const LatencySample& sample = sampleRing[(uint16_t)((start + i) % kSampleRingSize)];
    if (sample.flags & LATENCY_FLAG_TIMEOUT) {
      timeoutCount++;
    }
  }

  offset = appendToBuffer(buffer, capacity, offset,
                          "\r\n"
                          "Summary:\r\n"
                          "  Timeouts: %u\r\n",
                          (unsigned)timeoutCount);

  auto appendDeltaStats = [&](const char* label, LatencyDeltaField field) {
    uint16_t validCount = 0;
    uint32_t minValue = UINT32_MAX;
    uint32_t maxValue = 0;

    for (uint16_t i = 0; i < sampleCount; ++i) {
      const LatencySample& sample = sampleRing[(uint16_t)((start + i) % kSampleRingSize)];
      const uint32_t value = sampleDeltaFromStimulus(sample, field);
      if (value == 0 || (sample.flags & LATENCY_FLAG_TIMEOUT)) {
        continue;
      }
      validCount++;
      if (value < minValue) minValue = value;
      if (value > maxValue) maxValue = value;
    }

    if (validCount == 0) {
      offset = appendToBuffer(buffer, capacity, offset,
                              "  %s: n=0\r\n",
                              label);
      return;
    }

    auto selectRank = [&](uint16_t rank) -> uint32_t {
      uint32_t lo = minValue;
      uint32_t hi = maxValue;
      while (lo < hi) {
        const uint32_t mid = lo + ((hi - lo) / 2u);
        uint16_t atOrBelow = 0;
        for (uint16_t i = 0; i < sampleCount; ++i) {
          const LatencySample& sample = sampleRing[(uint16_t)((start + i) % kSampleRingSize)];
          const uint32_t value = sampleDeltaFromStimulus(sample, field);
          if (value != 0 && !(sample.flags & LATENCY_FLAG_TIMEOUT) && value <= mid) {
            atOrBelow++;
          }
        }
        if (atOrBelow > rank) {
          hi = mid;
        } else {
          lo = mid + 1u;
        }
      }
      return lo;
    };

    const uint16_t medianRank = (uint16_t)(((uint32_t)(validCount - 1u) * 50u) / 100u);
    const uint16_t p95Rank = (uint16_t)(((uint32_t)(validCount - 1u) * 95u) / 100u);
    const uint16_t p99Rank = (uint16_t)(((uint32_t)(validCount - 1u) * 99u) / 100u);
    offset = appendToBuffer(buffer, capacity, offset,
                            "  %s: n=%u min=%lu med=%lu p95=%lu p99=%lu max=%lu us\r\n",
                            label,
                            (unsigned)validCount,
                            (unsigned long)minValue,
                            (unsigned long)selectRank(medianRank),
                            (unsigned long)selectRank(p95Rank),
                            (unsigned long)selectRank(p99Rank),
                            (unsigned long)maxValue);
  };

  appendDeltaStats("raw", LATENCY_DELTA_RAW);
  appendDeltaStats("detect", LATENCY_DELTA_DETECT);
  appendDeltaStats("output_prepare", LATENCY_DELTA_PREPARE);
  appendDeltaStats("usb_submit", LATENCY_DELTA_USB_SUBMIT);
  appendDeltaStats("host_return", LATENCY_DELTA_HOST_RETURN);

  if (capacity > 0) {
    buffer[min(offset, capacity - 1u)] = '\0';
  }
  return (offset < capacity) ? offset : (capacity - 1u);
#endif
}

size_t LatencyTest::writeExportCsv(char* buffer, size_t capacity) const {
  size_t offset = 0;
  if (capacity > 0) {
    buffer[0] = '\0';
  }

#if !defined(ADAPT_ENABLE_LATENCY_TEST)
  return 0;
#else
  offset = appendToBuffer(buffer, capacity, offset,
                          "# format=compact-v4\r\n"
                          "# run_id=%lu\r\n"
                          "# run_start_us=%lu\r\n"
                          "# input_mode=%u\r\n"
                          "# configured_output_mode=%u\r\n"
                          "# runtime_output_mode=%u\r\n"
                          "# controller_in_loop=%u\r\n"
                          "# host_type=%s\r\n"
                          "# samples_stored=%u\r\n"
                          "# sample_capacity=%u\r\n"
                          "q,b,f,s,w,d,p,u,r,c,sof,sofage,i,o,e\r\n",
                          (unsigned long)runId,
                          (unsigned long)runStartUs,
                          (unsigned)runInputMode,
                          (unsigned)runConfiguredOutputMode,
                          (unsigned)runRuntimeOutputMode,
                          controllerInLoop ? 1u : 0u,
                          latencyHostTypeName(hostType),
                          (unsigned)sampleCount,
                          (unsigned)kSampleRingSize);

  const uint16_t start = (uint16_t)((sampleHead + kSampleRingSize - sampleCount) % kSampleRingSize);
  for (uint16_t i = 0; i < sampleCount; ++i) {
    const LatencySample& sample = sampleRing[(uint16_t)((start + i) % kSampleRingSize)];
    const uint32_t stimulusOffset = (sample.stimulus_us >= runStartUs)
      ? (sample.stimulus_us - runStartUs)
      : 0u;
    const uint32_t deltaDetect = (sample.detect_us != 0 && sample.stimulus_us != 0)
      ? (sample.detect_us - sample.stimulus_us)
      : 0u;
    const uint32_t deltaRaw = sampleDeltaFromStimulus(sample, LATENCY_DELTA_RAW);
    const uint32_t deltaPrepare = sampleDeltaFromStimulus(sample, LATENCY_DELTA_PREPARE);
    const uint32_t deltaUsb = (sample.usb_submit_us != 0 && sample.stimulus_us != 0)
      ? (sample.usb_submit_us - sample.stimulus_us)
      : 0u;
    const uint32_t deltaReturn = (sample.host_return_us != 0 && sample.stimulus_us != 0)
      ? (sample.host_return_us - sample.stimulus_us)
      : 0u;
    const uint32_t deltaComplete = (sample.complete_us != 0 && sample.stimulus_us != 0)
      ? (sample.complete_us - sample.stimulus_us)
      : 0u;

    offset = appendToBuffer(buffer, capacity, offset,
                            "%lu,%lX,%X,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%u,%u,%u,%u,%u\r\n",
                            (unsigned long)sample.seq,
                            (unsigned long)sample.button_mask,
                            (unsigned)sample.flags,
                            (unsigned long)stimulusOffset,
                            (unsigned long)deltaRaw,
                            (unsigned long)deltaDetect,
                            (unsigned long)deltaPrepare,
                            (unsigned long)deltaUsb,
                            (unsigned long)deltaReturn,
                            (unsigned long)deltaComplete,
                            (unsigned)sample.usb_submit_sof,
                            (unsigned)sample.usb_submit_sof_age_us,
                            (unsigned)sample.input_mode,
                            (unsigned)sample.configured_output_mode,
                            (unsigned)sample.runtime_output_mode);
  }

  if (capacity > 0) {
    buffer[min(offset, capacity - 1u)] = '\0';
  }
  return (offset < capacity) ? offset : (capacity - 1u);
#endif
}

uint32_t LatencyTest::exportFileStamp() const {
  return runStartUs / 1000u;
}

void LatencyTest::handleReturnInterrupt() {
  pendingReturnEdgeUs = micros();
  pendingReturnEdge = true;
}

LatencyTest latencyTest;
