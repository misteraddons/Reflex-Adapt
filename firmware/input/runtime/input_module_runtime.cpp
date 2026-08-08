#include "../../product_config.h"

#include "input_module_runtime.h"

#include <Arduino.h>

#include "../../core/device_runtime_state.h"
#include "../mixed/input_mixed_runtime_state.h"
#include "../state/input_active_module_state.h"
#include "../base/RZInputModule.h"

#ifdef ENABLE_INPUT_PADDLE
  #include "../atari/Input_AtariPaddle.h"
#endif
#ifdef ENABLE_INPUT_DREAMCAST
  #include "../dreamcast/Input_Dreamcast.h"
#endif
#if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
  #include "../gc64/Input_GC64.h"
#endif
#include "../mixed/Input_Mixed.h"
#ifdef ENABLE_INPUT_PSX
  #include "../psx/Input_Psx.h"
#endif

namespace {

RZInputModule* currentInputModule() {
  return activeInputModule();
}

}  // namespace

bool hasCurrentInputModule() {
  return hasActiveInputModule();
}

RZInputPaddle* currentPaddleInputModule() {
  #ifdef ENABLE_INPUT_PADDLE
  if (inputMixedModeActive()) {
    return nullptr;
  }
  return static_cast<RZInputPaddle*>(currentInputModule());
  #else
  return nullptr;
  #endif
}

RZInputDreamcast* currentDreamcastInputModule() {
  #ifdef ENABLE_INPUT_DREAMCAST
  if (inputMixedModeActive() || deviceMode != RZORD_DREAMCAST || !hasCurrentInputModule()) {
    return nullptr;
  }
  return static_cast<RZInputDreamcast*>(currentInputModule());
  #else
  return nullptr;
  #endif
}

RZInputGC64* currentGc64InputModule() {
  #if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
  if (inputMixedModeActive() || !hasCurrentInputModule()) {
    return nullptr;
  }
  #ifdef ENABLE_INPUT_N64
  if (deviceMode == RZORD_N64) {
    return static_cast<RZInputGC64*>(currentInputModule());
  }
  #endif
  #ifdef ENABLE_INPUT_GAMECUBE
  if (deviceMode == RZORD_GAMECUBE) {
    return static_cast<RZInputGC64*>(currentInputModule());
  }
  #endif
  #ifdef ENABLE_INPUT_GBA
  if (deviceMode == RZORD_GBA) {
    return static_cast<RZInputGC64*>(currentInputModule());
  }
  #endif
  return nullptr;
  #else
  return nullptr;
  #endif
}

RZInputMixed* currentMixedInputModule() {
  if (!inputMixedModeActive() || !hasCurrentInputModule()) {
    return nullptr;
  }
  return static_cast<RZInputMixed*>(currentInputModule());
}

RZInputPSX* currentPsxInputModule() {
  #ifdef ENABLE_INPUT_PSX
  if (inputMixedModeActive() || !hasCurrentInputModule()) {
    return nullptr;
  }
  if (deviceMode == RZORD_PSX ||
      deviceMode == RZORD_PSX_JOG ||
      deviceMode == RZORD_PSX_DANCE) {
    return static_cast<RZInputPSX*>(currentInputModule());
  }
  return nullptr;
  #else
  return nullptr;
  #endif
}

const char* currentInputModuleDescriptionOr(const char* fallback) {
  return hasCurrentInputModule() ? currentInputModule()->getDescription() : fallback;
}

const char* currentInputModuleUsbIdOr(const char* fallback) {
  return hasCurrentInputModule() ? currentInputModule()->getUsbId() : fallback;
}

const char* currentInputModulePhysicalConnectionDisplayName() {
  return hasCurrentInputModule()
    ? currentInputModule()->physicalConnectionDisplayName()
    : nullptr;
}

uint32_t __not_in_flash_func(currentInputModulePollIntervalUs)() {
  return hasCurrentInputModule() ? currentInputModule()->pollInterval : 0;
}

bool currentInputModuleHasPhysicalConnectionForHotSwap() {
  return hasCurrentInputModule() &&
         currentInputModule()->hasPhysicalConnectionForHotSwap();
}

bool __not_in_flash_func(pollCurrentInputModule)(bool* updated) {
  if (updated != nullptr) {
    *updated = false;
  }

  if (!hasCurrentInputModule()) {
    return false;
  }

  const bool polled_updated = currentInputModule()->poll();
  if (updated != nullptr) {
    *updated = polled_updated;
  }
  return true;
}

bool __not_in_flash_func(pollCurrentInputModuleIfDue)(uint32_t now_us, uint32_t& last_poll_at_us,
                                                      bool* updated) {
  if (updated != nullptr) {
    *updated = false;
  }

  RZInputModule* const module = currentInputModule();
  if (module == nullptr) {
    return false;
  }

  const uint32_t interval_us = module->pollInterval;
  if (interval_us != 0 && now_us - last_poll_at_us < interval_us) {
    return false;
  }

  const bool polled_updated = module->poll();
  if (updated != nullptr) {
    *updated = polled_updated;
  }

  if (interval_us == 0 || last_poll_at_us == 0 ||
      now_us - last_poll_at_us > interval_us * 4) {
    last_poll_at_us = now_us;
  } else {
    last_poll_at_us += interval_us;
  }
  return true;
}

void runCurrentInputModuleAfterOutputFrameSent(bool polled, bool updated) {
  if (hasCurrentInputModule()) {
    currentInputModule()->afterOutputFrameSent(polled, updated);
  }
}

void setupCurrentInputModule() {
  if (hasCurrentInputModule()) {
    currentInputModule()->setup();
  }
}

void setupCurrentInputModuleStage2() {
  if (hasCurrentInputModule()) {
    currentInputModule()->setup2();
  }
}

void configureCurrentInputModuleBcdDeviceVersion() {
  if (hasCurrentInputModule()) {
    currentInputModule()->configureBcdDeviceVersion();
  }
}
