#include "input_mixed_runtime_state.h"

#include "../../product_config.h"

namespace {

enum MixedInputFamily : uint8_t {
  MIXED_FAMILY_NONE = 0,
  MIXED_FAMILY_JOYBUS,
  MIXED_FAMILY_SNES,
  MIXED_FAMILY_SATURN,
  MIXED_FAMILY_SMS,
  MIXED_FAMILY_PCE,
  MIXED_FAMILY_PSX,
  MIXED_FAMILY_WII,
  MIXED_FAMILY_NEOGEO,
  MIXED_FAMILY_3DO,
  MIXED_FAMILY_JAGUAR,
  MIXED_FAMILY_DREAMCAST,
  MIXED_FAMILY_DRIVING,
};

DeviceEnum mixed_port_mode[INPUT_MIXED_PORT_COUNT] = { RZORD_NONE, RZORD_NONE };
bool mixed_mode_active = false;

// Cross-library mixed mode is intentionally off for production. Same-family
// combinations are handled by their native family drivers; true cross-library
// support needs a per-port neutral-frame pipeline that does not depend on the
// global deviceMode during output conversion.
constexpr bool kEnableCrossLibraryMixedSupervisor = false;

MixedInputFamily familyForMode(DeviceEnum mode) {
  switch (mode) {
    #if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
    case RZORD_N64:
    case RZORD_GAMECUBE:
    case RZORD_GBA:
      return MIXED_FAMILY_JOYBUS;
    #endif

    #if defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_VBOY)
    case RZORD_NES:
    case RZORD_SNES:
    case RZORD_VBOY:
      return MIXED_FAMILY_SNES;
    #endif

    #if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
    case RZORD_SATURN:
    case RZORD_MEGADRIVE:
      return MIXED_FAMILY_SATURN;
    #endif

    #if defined(ENABLE_INPUT_SMS) || defined(ENABLE_INPUT_JPC)
    case RZORD_SMS:
    case RZORD_JPC:
      return MIXED_FAMILY_SMS;
    #endif

    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return MIXED_FAMILY_PCE;
    #endif

    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    case RZORD_PSX_JOG:
    case RZORD_PSX_DANCE:
      return MIXED_FAMILY_PSX;
    #endif

    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      return MIXED_FAMILY_WII;
    #endif

    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      return MIXED_FAMILY_NEOGEO;
    #endif

    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      return MIXED_FAMILY_3DO;
    #endif

    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return MIXED_FAMILY_JAGUAR;
    #endif

    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return MIXED_FAMILY_DREAMCAST;
    #endif

    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      return MIXED_FAMILY_DRIVING;
    #endif

    default:
      return MIXED_FAMILY_NONE;
  }
}

bool mixedModeFamilyMaskSafe(MixedInputFamily family) {
  switch (family) {
    case MIXED_FAMILY_JOYBUS:
    case MIXED_FAMILY_SATURN:
    case MIXED_FAMILY_WII:
      return true;
    default:
      return false;
  }
}

}  // namespace

void clearInputMixedModeState() {
  mixed_port_mode[0] = RZORD_NONE;
  mixed_port_mode[1] = RZORD_NONE;
  mixed_mode_active = false;
}

bool inputMixedPortModeSupported(uint8_t port, DeviceEnum mode) {
  if (port >= INPUT_MIXED_PORT_COUNT) {
    return false;
  }

  return familyForMode(mode) != MIXED_FAMILY_NONE;
}

bool inputMixedModesNeedSupervisor(DeviceEnum port0Mode, DeviceEnum port1Mode) {
  if (!kEnableCrossLibraryMixedSupervisor) {
    return false;
  }

  if (port0Mode == RZORD_NONE || port1Mode == RZORD_NONE) {
    return false;
  }
  if (!inputMixedPortModeSupported(0, port0Mode) ||
      !inputMixedPortModeSupported(1, port1Mode)) {
    return false;
  }

  MixedInputFamily family0 = familyForMode(port0Mode);
  MixedInputFamily family1 = familyForMode(port1Mode);
  return family0 != MIXED_FAMILY_NONE &&
         family1 != MIXED_FAMILY_NONE &&
         mixedModeFamilyMaskSafe(family0) &&
         mixedModeFamilyMaskSafe(family1) &&
         family0 != family1;
}

void configureInputMixedAutoDetectModes(DeviceEnum port0Mode, DeviceEnum port1Mode) {
  mixed_port_mode[0] = port0Mode;
  mixed_port_mode[1] = port1Mode;
  mixed_mode_active = inputMixedModesNeedSupervisor(port0Mode, port1Mode);
  if (!mixed_mode_active) {
    if (port0Mode == RZORD_NONE) {
      mixed_port_mode[0] = RZORD_NONE;
    }
    if (port1Mode == RZORD_NONE) {
      mixed_port_mode[1] = RZORD_NONE;
    }
  }
}

bool inputMixedModeActive() {
  return mixed_mode_active;
}

DeviceEnum inputMixedPortMode(uint8_t port) {
  if (port >= INPUT_MIXED_PORT_COUNT) {
    return RZORD_NONE;
  }
  return mixed_port_mode[port];
}
