#pragma once

#include <Arduino.h>

#include "../product_config.h"
#include "../core/controller_state.h"

#ifndef LATENCY_PIN_UNUSED
#define LATENCY_PIN_UNUSED 0xFF
#endif

// Board targets can set PIN_LATENCY_* in product_config.h or via build flags.
// Anything left undefined simply disables that role for the current target.
// Do not fall back to a real GPIO here: on Classic2USB, GPIO2 is a controller
// connector pin and is unsafe to drive as a generic diagnostic pulse.
#ifndef LATENCY_PIN
#define LATENCY_PIN LATENCY_PIN_UNUSED
#endif

#if defined(PIN_LATENCY_PULSE)
  #undef LATENCY_PIN
  #define LATENCY_PIN PIN_LATENCY_PULSE
#endif

#ifndef LATENCY_PULSE_PIN
#define LATENCY_PULSE_PIN LATENCY_PIN
#endif

#if defined(PIN_LATENCY_TRIGGER)
  #ifndef LATENCY_TRIGGER_PIN
  #define LATENCY_TRIGGER_PIN PIN_LATENCY_TRIGGER
  #endif
#endif

#ifndef LATENCY_TRIGGER_PIN
#define LATENCY_TRIGGER_PIN LATENCY_PIN_UNUSED
#endif

#if defined(PIN_LATENCY_RETURN)
  #ifndef LATENCY_RETURN_PIN
  #define LATENCY_RETURN_PIN PIN_LATENCY_RETURN
  #endif
#endif

#ifndef LATENCY_RETURN_PIN
#define LATENCY_RETURN_PIN LATENCY_PIN_UNUSED
#endif

#ifndef LATENCY_PULSE_US
#define LATENCY_PULSE_US 100
#endif

#ifndef LATENCY_TRIGGER_PRESS_US
#define LATENCY_TRIGGER_PRESS_US 20000
#endif

#ifndef LATENCY_SAMPLE_DELAY_MIN_US
#define LATENCY_SAMPLE_DELAY_MIN_US 20000
#endif

#ifndef LATENCY_SAMPLE_DELAY_MAX_US
#define LATENCY_SAMPLE_DELAY_MAX_US 70000
#endif

#ifndef LATENCY_SAMPLE_TIMEOUT_US
#define LATENCY_SAMPLE_TIMEOUT_US 250000
#endif

#ifndef LATENCY_DEFAULT_SAMPLE_COUNT
#define LATENCY_DEFAULT_SAMPLE_COUNT 512
#endif

#ifndef LATENCY_SAMPLE_RING_SIZE
#define LATENCY_SAMPLE_RING_SIZE 512
#endif

enum LatencyHostType : uint8_t {
  LATENCY_HOST_INTERNAL = 0,
  LATENCY_HOST_PC,
  LATENCY_HOST_MISTER,
  LATENCY_HOST_LAST,
};

enum LatencyTestMode : uint8_t {
  LATENCY_MODE_OFF = 0,
  LATENCY_MODE_PULSE,
  LATENCY_MODE_SYNTH_INTERNAL,
  LATENCY_MODE_SYNTH_PC,
  LATENCY_MODE_SYNTH_MISTER,
  LATENCY_MODE_TRIGGER_INTERNAL,
  LATENCY_MODE_TRIGGER_PC,
  LATENCY_MODE_TRIGGER_MISTER,
  LATENCY_MODE_LAST,
};

enum LatencySampleFlags : uint16_t {
  LATENCY_FLAG_DETECTED = 1u << 0,
  LATENCY_FLAG_USB_SUBMIT = 1u << 1,
  LATENCY_FLAG_HOST_GPIO = 1u << 2,
  LATENCY_FLAG_HOST_ACK = 1u << 3,
  LATENCY_FLAG_SYNTHETIC = 1u << 4,
  LATENCY_FLAG_TRIGGER = 1u << 5,
  LATENCY_FLAG_TIMEOUT = 1u << 6,
  LATENCY_FLAG_HOST_INTERNAL = 1u << 7,
  LATENCY_FLAG_OUTPUT_PREPARE = 1u << 8,
  LATENCY_FLAG_RAW_DETECTED = 1u << 9,
};

struct LatencySample {
  uint32_t seq;
  uint32_t button_mask;
  uint32_t stimulus_us;
  uint32_t raw_detect_us;
  uint32_t detect_us;
  uint32_t output_prepare_us;
  uint32_t usb_submit_us;
  uint32_t host_return_us;
  uint32_t complete_us;
  uint16_t usb_submit_sof;
  uint16_t usb_submit_sof_age_us;
  uint8_t input_mode;
  uint8_t configured_output_mode;
  uint8_t runtime_output_mode;
  uint8_t mode;
  uint16_t flags;
};

const char* latencyTestModeName(uint8_t mode);
const char* latencyHostTypeName(uint8_t hostType);

class LatencyTest {
private:
  uint8_t pin;
  uint8_t triggerPin;
  uint8_t returnPin;
  bool enabled;
  bool controllerInLoop;
  uint8_t hostType;
  uint8_t currentMode;
  bool triggerActiveHigh;
  uint32_t lastRawButtonState;
  uint32_t lastButtonState;
  uint32_t pulseEndTime;
  bool pulsing;
  bool pulsePinArmed;
  bool benchRunning;
  bool triggerHeld;
  bool syntheticActive;
  bool outputFrameBackedUp;
  bool pendingBenchStopLog;
  bool serialLogEnabled;
  volatile bool pendingReturnEdge;
  volatile uint32_t pendingReturnEdgeUs;
  uint32_t nextStimulusAtUs;
  uint32_t holdUntilUs;
  uint32_t sampleDeadlineUs;
  uint32_t activeTargetSamples;
  uint32_t completedRunSamples;
  uint32_t nextSequence;
  uint32_t runStartUs;
  uint32_t runId;
  uint8_t runInputMode;
  uint8_t runConfiguredOutputMode;
  uint8_t runRuntimeOutputMode;
  controller_state_t outputFrameBackup;
  LatencySample activeSample;
  bool activeSampleInUse;
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  static constexpr uint16_t kSampleRingSize = LATENCY_SAMPLE_RING_SIZE;
  LatencySample sampleRing[kSampleRingSize];
#else
  static constexpr uint16_t kSampleRingSize = 0;
#endif
  uint16_t sampleHead;
  uint16_t sampleCount;
  uint16_t pendingLogCount;

  bool isBenchMode(uint8_t mode) const;
  bool usesSyntheticStimulus() const;
  bool usesTriggerOutput() const;
  bool usesHostGpioReturn() const;
  bool usesPcAckReturn() const;
  bool usesInternalLoopReturn() const;
  uint32_t randomSampleDelayUs() const;
  void syncModeFromConfiguration();
  void armPulsePin();
  void releasePulsePin();
  void armNextBenchSample(uint32_t nowUs);
  void completeActiveSample(uint32_t nowUs, uint16_t extraFlags);
  void pushCompletedSample(const LatencySample& sample);
  void pressTrigger();
  void releaseTrigger();
  void clearActiveStimulusState();
  void handlePendingReturnEdge(uint32_t nowUs);
  void armPassiveInputSample(uint32_t nowUs, uint32_t buttonMask);
  void printSampleLine(Print& out, const LatencySample& sample) const;

public:
  LatencyTest();

  void begin();

  void setEnabled(bool en);
  void setMode(uint8_t mode);
  void setControllerInLoop(bool en);
  void setHostType(uint8_t host);
  void setTriggerActiveHigh(bool activeHigh);
  void setSerialLogEnabled(bool en) { serialLogEnabled = en; }

  #if defined(ADAPT_ENABLE_LATENCY_TEST)
  bool isEnabled() const { return enabled; }
  #else
  __attribute__((always_inline)) bool isEnabled() const { return false; }
  #endif
  bool isControllerInLoop() const { return controllerInLoop; }
  uint8_t getHostType() const { return hostType; }
  bool isTriggerActiveHigh() const { return triggerActiveHigh; }
  #if defined(ADAPT_ENABLE_LATENCY_TEST)
  bool isSerialLogEnabled() const { return serialLogEnabled; }
  #else
  __attribute__((always_inline)) bool isSerialLogEnabled() const { return false; }
  #endif
  uint8_t getMode() const { return currentMode; }
  bool isCurrentModeReady() const;
  uint8_t getPulsePin() const { return pin; }
  uint8_t getTriggerPin() const { return triggerPin; }
  uint8_t getReturnPin() const { return returnPin; }
  uint16_t getStoredSampleCount() const { return sampleCount; }
  uint16_t getStoredSampleCapacity() const { return kSampleRingSize; }

  void checkButtons(uint32_t buttonState);
  void observeRawButtons(uint8_t port, uint32_t rawState, bool connected);
  void observeButtons(uint8_t port, uint32_t buttonState, bool connected);

  void pulse();
  void update();
  void pulseBlocking();
  void reset();

  void startBench(uint32_t sampleCount = 0);
  void stopBench();
  #if defined(ADAPT_ENABLE_LATENCY_TEST)
  bool isBenchRunning() const { return benchRunning; }
  #else
  __attribute__((always_inline)) bool isBenchRunning() const { return false; }
  #endif
  uint32_t getCompletedRunSamples() const { return completedRunSamples; }
  uint32_t getRemainingRunSamples() const { return activeTargetSamples; }
  uint32_t getRunTargetSamples() const { return completedRunSamples + activeTargetSamples; }
  void clearSamples();
  void prepareOutputFrame();
  void restoreOutputFrame();
  void noteOutputFramePrepared();
  void noteUsbSubmit(uint8_t port, uint32_t buttonState);
  void acknowledgeHost(uint32_t seq = 0);
  void flushLog(Print& out);
  void writeStatus(Print& out) const;
  void dumpSamples(Print& out) const;
  size_t writeExportStatus(char* buffer, size_t capacity) const;
  size_t writeExportCsv(char* buffer, size_t capacity) const;
  uint32_t exportFileStamp() const;
  void handleReturnInterrupt();
};

extern LatencyTest latencyTest;
