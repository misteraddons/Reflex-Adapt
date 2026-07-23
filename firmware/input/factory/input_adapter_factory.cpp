#include "../../product_config.h"

#include "input_adapter_factory.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "../../core/analog_calibration_state.h"
#include "../../core/device_runtime_state.h"
#include "../../core/settings_store.h"
#include "../../platform/display_runtime_state.h"
#include "../base/RZInputModule.h"
#include "../state/input_active_module_state.h"
#include "../autodetect/input_autodetect_runtime.h"
#include "../jaguar/input_jaguar_runtime_state.h"
#include "../mixed/input_mixed_runtime_state.h"
#include "../mixed/Input_Mixed.h"
#include "../runtime/input_module_runtime.h"

#if defined(ENABLE_INPUT_USB) || defined(ENABLE_PS4AUTH)
  #include "../usb_host/Input_UsbHost.h"
#endif
#ifdef ENABLE_INPUT_ESP32_SPI
  #include "../esp32_spi/Input_Esp32Spi.h"
#endif
#ifdef ENABLE_INPUT_JVS
  #include "../jvs/Input_Jvs.h"
#endif
#if defined(ENABLE_INPUT_DUMMY)
  #include "../dummy/Input_Dummy.h"
#endif
#if defined(ENABLE_INPUT_CUSTOM)
  #include "../custom/Input_Custom.h"
#endif
#if defined(ENABLE_INPUT_MEGADRIVE) || defined(ENABLE_INPUT_SATURN)
  #include "../saturn/Input_Saturn.h"
#endif
#if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
  #include "../gc64/Input_GC64.h"
#endif
#if defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_VBOY)
  #include "../snes/Input_Snes.h"
#endif
#if defined(ENABLE_INPUT_PCE)
  #include "../pce/Input_Pce.h"
#endif
#ifdef ENABLE_INPUT_WII
  #include "../wii/Input_Wii.h"
#endif
#ifdef ENABLE_INPUT_PSX
  #include "../psx/Input_Psx.h"
#endif
#ifdef ENABLE_INPUT_NEOGEO
  #include "../neogeo/Input_Neogeo.h"
#endif
#ifdef ENABLE_INPUT_3DO
  #include "../3do/Input_3do.h"
#endif
#ifdef ENABLE_INPUT_JAGUAR
  #include "../jaguar/Input_Jaguar.h"
#endif
#ifdef ENABLE_INPUT_DREAMCAST
  #include "../dreamcast/Input_Dreamcast.h"
#endif
#ifdef ENABLE_INPUT_INTV
  #include "../intellivision/Input_Intellivision.h"
#endif
#ifdef ENABLE_INPUT_PADDLE
  #include "../atari/Input_AtariPaddle.h"
#endif
#ifdef ENABLE_INPUT_DRIVING
  #include "../atari/Input_AtariDriving.h"
#endif
#ifdef ENABLE_INPUT_GAMEPORT
  #include "../gameport/Input_Gameport.h"
#endif
#ifdef ENABLE_INPUT_MEMCARD
  #include "../memcard/Input_MemCard.h"
#endif
#if defined(ENABLE_INPUT_SMS) || defined(ENABLE_INPUT_JPC)
  #include "../sms/Input_Sms.h"
#endif

namespace {

RZInputModule* createAutoFallbackModuleInstance(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_AUTODETECT
    case RZORD_AUTODETECT:
      #if defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_VBOY)
        return new RZInputSnes();
      #elif defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
        return new RZInputSaturn();
      #elif defined(ENABLE_INPUT_SMS) || defined(ENABLE_INPUT_JPC)
        return new RZInputSms();
      #elif defined(ENABLE_INPUT_NEOGEO)
        return new RZInputNeoGeo();
      #elif defined(ENABLE_INPUT_PSX)
        return new RZInputPSX();
      #endif
      break;
    #endif

    default:
      return nullptr;
  }
}

RZInputModule* createCoreInputModuleInstance(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_USB
    case RZORD_USB:       return new RZInputUsbHost();
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
    case RZORD_ESP32_SPI: return new RZInputEsp32Spi();
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:       return new RZInputJvs();
    #endif
    #ifdef ENABLE_INPUT_CUSTOM
    case RZORD_CUSTOM:    return new RZInputCustom();
    #endif
    #ifdef ENABLE_INPUT_DUMMY
    case RZORD_DUMMY:     return new RZInputDummy();
    #endif

    default:
      return nullptr;
  }
}

RZInputModule* createNintendoFamilyInputModuleInstance(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:       return new RZInputGC64();
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:  return new RZInputGC64();
    #endif
    #ifdef ENABLE_INPUT_GBA
    case RZORD_GBA:       return new RZInputGC64();
    #endif
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:       return new RZInputSnes();
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:      return new RZInputSnes();
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:      return new RZInputSnes();
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:       return new RZInputWii();
    #endif

    default:
      return nullptr;
  }
}

RZInputModule* createSegaAndHudsonInputModuleInstance(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:    return new RZInputSaturn();
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE: return new RZInputSaturn();
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:       return new RZInputSms();
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:       return new RZInputSms();
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:       return new RZInputPce();
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST: return new RZInputDreamcast();
    #endif

    default:
      return nullptr;
  }
}

RZInputModule* createArcadeAndMiscInputModuleInstance(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:    return new RZInputNeoGeo();
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:       return new RZInput3do();
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      resetJaguarRotaryRuntimeState();
      return new RZInputJaguar();
    #endif
    #ifdef ENABLE_INPUT_INTV
    case RZORD_INTV:      return new RZInputIntv();
    #endif
    #ifdef ENABLE_INPUT_PADDLE
    case RZORD_PADDLE:    return new RZInputPaddle();
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:   return new RZInputDriving();
    #endif
    #ifdef ENABLE_INPUT_GAMEPORT
    case RZORD_GAMEPORT:  return new RZInputGameport();
    #endif
    #ifdef ENABLE_INPUT_MEMCARD
    case RZORD_MEMCARD:   return new RZInputMemCard();
    #endif

    default:
      return nullptr;
  }
}

RZInputModule* createPlayStationFamilyInputModuleInstance(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:       return new RZInputPSX();
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG:   return new RZInputPSX();
    #endif
    #ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE: return new RZInputPSX();
    #endif

    default:
      return nullptr;
  }
}

}  // namespace

namespace {

RZInputModule* createConcreteModuleInstance(DeviceEnum mode) {
  RZInputModule* module = nullptr;

  module = createAutoFallbackModuleInstance(mode);
  if (module) return module;

  module = createCoreInputModuleInstance(mode);
  if (module) return module;

  module = createNintendoFamilyInputModuleInstance(mode);
  if (module) return module;

  module = createSegaAndHudsonInputModuleInstance(mode);
  if (module) return module;

  module = createPlayStationFamilyInputModuleInstance(mode);
  if (module) return module;

  module = createArcadeAndMiscInputModuleInstance(mode);
  if (module) return module;

  return nullptr;
}

DeviceEnum bootRecoveryInputMode() {
  #ifdef ENABLE_INPUT_AUTODETECT
  return RZORD_AUTODETECT;
  #elif defined(ENABLE_INPUT_JVS)
  return RZORD_JVS;
  #elif defined(ENABLE_INPUT_USB)
  return RZORD_USB;
  #elif defined(ENABLE_INPUT_ESP32_SPI)
  return RZORD_ESP32_SPI;
  #elif defined(ENABLE_INPUT_DUMMY)
  return RZORD_DUMMY;
  #elif defined(ENABLE_INPUT_SATURN)
  return RZORD_SATURN;
  #elif defined(ENABLE_INPUT_SNES)
  return RZORD_SNES;
  #elif defined(ENABLE_INPUT_PSX)
  return RZORD_PSX;
  #else
  return RZORD_NONE;
  #endif
}

}  // namespace

void createActiveInputAdapter() {
  prepareAutoDetectInputModeAtBoot();

  RZInputModule* module = inputMixedModeActive()
      ? static_cast<RZInputModule*>(new RZInputMixed())
      : createConcreteModuleInstance(deviceMode);
  if (module == nullptr) {
    const DeviceEnum recoveryMode = bootRecoveryInputMode();
    if (recoveryMode != RZORD_NONE && recoveryMode != deviceMode) {
      deviceMode = recoveryMode;
      savedDeviceMode = recoveryMode;
      persistConfiguredInputMode(recoveryMode);
      prepareAutoDetectInputModeAtBoot();
      module = inputMixedModeActive()
          ? static_cast<RZInputModule*>(new RZInputMixed())
          : createConcreteModuleInstance(deviceMode);
    }
  }

  #ifdef ENABLE_INPUT_AUTODETECT
  if (module != nullptr &&
      inputAutoDetectModeActive() &&
      input_auto_resolve_port < 2) {
    module->setPhysicalPortMask((uint8_t)(1u << input_auto_resolve_port));
  }
  #endif

  setActiveInputModule(module);
}
