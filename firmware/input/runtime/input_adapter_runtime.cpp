#include "../../product_config.h"

#include "input_adapter_runtime.h"

#include <Arduino.h>

#include "input_module_runtime.h"

bool hasActiveInputAdapter() {
  return hasCurrentInputModule();
}

RZInputPaddle* activePaddleInputAdapter() {
  return currentPaddleInputModule();
}

const char* activeInputAdapterDescriptionOr(const char* fallback) {
  return currentInputModuleDescriptionOr(fallback);
}

const char* activeInputAdapterUsbIdOr(const char* fallback) {
  return currentInputModuleUsbIdOr(fallback);
}

const char* activeInputAdapterPhysicalConnectionDisplayName() {
  return currentInputModulePhysicalConnectionDisplayName();
}

bool activeInputAdapterHasPhysicalConnectionForHotSwap() {
  return currentInputModuleHasPhysicalConnectionForHotSwap();
}

bool __not_in_flash_func(pollActiveInputAdapter)(bool* updated) {
  return pollCurrentInputModule(updated);
}

bool __not_in_flash_func(pollActiveInputAdapterIfDue)(uint32_t now_us, uint32_t& last_poll_at_us,
                                                      bool* updated) {
  return pollCurrentInputModuleIfDue(now_us, last_poll_at_us, updated);
}

void runActiveInputAdapterAfterOutputFrameSent(bool polled, bool updated) {
  runCurrentInputModuleAfterOutputFrameSent(polled, updated);
}

void setupActiveInputAdapter() {
  setupCurrentInputModule();
}

void setupActiveInputAdapterStage2() {
  setupCurrentInputModuleStage2();
}

void configureActiveInputAdapterBcdDeviceVersion() {
  configureCurrentInputModuleBcdDeviceVersion();
}
