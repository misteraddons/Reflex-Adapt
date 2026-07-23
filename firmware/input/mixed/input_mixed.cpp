#include "Input_Mixed.h"

#include <cstring>

#include "../../product_config.h"
#include "../../core/controller_frame_state.h"
#include "../../core/device_runtime_state.h"
#include "../../output/runtime/input_runtime_output_bridge.h"
#include "../runtime/input_frame_runtime.h"

#if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
  #include "../gc64/Input_GC64.h"
#endif
#if defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_VBOY)
  #include "../snes/Input_Snes.h"
#endif
#if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
  #include "../saturn/Input_Saturn.h"
#endif
#if defined(ENABLE_INPUT_SMS) || defined(ENABLE_INPUT_JPC)
  #include "../sms/Input_Sms.h"
#endif
#ifdef ENABLE_INPUT_PCE
  #include "../pce/Input_Pce.h"
#endif
#ifdef ENABLE_INPUT_PSX
  #include "../psx/Input_Psx.h"
#endif
#ifdef ENABLE_INPUT_WII
  #include "../wii/Input_Wii.h"
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
#ifdef ENABLE_INPUT_DRIVING
  #include "../atari/Input_AtariDriving.h"
#endif

namespace {

void copyInputFrames(controller_state_t (&frames)[MAX_USB_OUT]) {
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    frames[i] = inputFrame(i);
  }
}

void restoreInputFrames(const controller_state_t (&frames)[MAX_USB_OUT]) {
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    inputFrame(i) = frames[i];
  }
}

void clearAllInputFrames() {
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    inputFrame(i) = controller_state_t{};
    clearInputFrameTypeName(i);
    setInputFrameConnected(i, false);
  }
}

class ScopedDeviceMode {
public:
  explicit ScopedDeviceMode(DeviceEnum mode) : previous(deviceMode) {
    deviceMode = mode;
  }

  ~ScopedDeviceMode() {
    deviceMode = previous;
  }

private:
  DeviceEnum previous;
};

}  // namespace

RZInputMixed::RZInputMixed() : RZInputModule() {
  portModes[0] = inputMixedPortMode(0);
  portModes[1] = inputMixedPortMode(1);
}

const char* RZInputMixed::getUsbId() {
  return "RZRMix";
}

const char* RZInputMixed::getDescription() {
  return "MIXED";
}

RZInputModule* RZInputMixed::moduleForPhysicalPort(uint8_t port) {
  if (port >= INPUT_MIXED_PORT_COUNT) {
    return nullptr;
  }
  ensureModules();
  return modules[port];
}

DeviceEnum RZInputMixed::modeForPhysicalPort(uint8_t port) const {
  return port < INPUT_MIXED_PORT_COUNT ? portModes[port] : RZORD_NONE;
}

RZInputModule* RZInputMixed::createModuleForMode(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return new RZInputGC64();
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return new RZInputGC64();
    #endif
    #ifdef ENABLE_INPUT_GBA
    case RZORD_GBA:
      return new RZInputGC64();
    #endif

    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      return new RZInputSnes();
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      return new RZInputSnes();
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      return new RZInputSnes();
    #endif

    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      return new RZInputSaturn();
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      return new RZInputSaturn();
    #endif

    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      return new RZInputSms();
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      return new RZInputSms();
    #endif

    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return new RZInputPce();
    #endif

    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    case RZORD_PSX_JOG:
    case RZORD_PSX_DANCE:
      return new RZInputPSX();
    #endif

    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      return new RZInputWii();
    #endif

    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      return new RZInputNeoGeo();
    #endif

    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      return new RZInput3do();
    #endif

    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return new RZInputJaguar();
    #endif

    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return new RZInputDreamcast();
    #endif

    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      return new RZInputDriving();
    #endif

    default:
      return nullptr;
  }
}

void RZInputMixed::ensureModules() {
  for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
    if (modules[port] != nullptr) {
      continue;
    }
    if (!inputMixedPortModeSupported(port, portModes[port])) {
      continue;
    }
    modules[port] = createModuleForMode(portModes[port]);
    if (modules[port] != nullptr) {
      // Mixed mode owns one physical connector per child module. Cross-family
      // AUTO only enables this supervisor for drivers that honor the mask.
      modules[port]->setPhysicalPortMask((uint8_t)(1u << port));
    }
  }
}

void RZInputMixed::configurePrimaryBcd() {
  switch (portModes[0]) {
    case RZORD_GAMECUBE: bcd_device_version.platform = BCD_PLAT_GC; break;
    case RZORD_GBA: bcd_device_version.platform = BCD_PLAT_GBA; break;
    case RZORD_MEGADRIVE: bcd_device_version.platform = BCD_PLAT_MEGADRIVE; break;
    case RZORD_SATURN: bcd_device_version.platform = BCD_PLAT_SATURN; break;
    case RZORD_PCE: bcd_device_version.platform = BCD_PLAT_PCE; break;
    case RZORD_NES: bcd_device_version.platform = BCD_PLAT_NES; break;
    case RZORD_SNES: bcd_device_version.platform = BCD_PLAT_SNES; break;
    case RZORD_VBOY: bcd_device_version.platform = BCD_PLAT_VBOY; break;
    case RZORD_NEOGEO: bcd_device_version.platform = BCD_PLAT_NEOGEO; break;
    case RZORD_WII: bcd_device_version.platform = BCD_PLAT_WII; break;
    case RZORD_3DO: bcd_device_version.platform = BCD_PLAT_3DO; break;
    case RZORD_JAGUAR: bcd_device_version.platform = BCD_PLAT_JAGUAR; break;
    case RZORD_DREAMCAST: bcd_device_version.platform = BCD_PLAT_DREAMCAST; break;
    case RZORD_DRIVING: bcd_device_version.platform = BCD_PLAT_DRIVING; break;
    case RZORD_SMS: bcd_device_version.platform = BCD_PLAT_SMS; break;
    case RZORD_JPC: bcd_device_version.platform = BCD_PLAT_JPC; break;
    case RZORD_PSX: bcd_device_version.platform = BCD_PLAT_PSX; break;
    case RZORD_N64:
    default:
      bcd_device_version.platform = BCD_PLAT_N64;
      break;
  }
  // MiSTer/SDL maps by the one USB VID/PID/bcdDevice identity visible for the
  // adapter. Mixed mode therefore advertises P1's normal platform GUID instead
  // of a separate mixed sub-variant; P2 is interpreted through that same map.
  bcd_device_version.platform_sub = 0;
}

void RZInputMixed::configureBcdDeviceVersion() {
  configurePrimaryBcd();
}

void RZInputMixed::setup() {
  ensureModules();

  for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
    if (modules[port] == nullptr) {
      continue;
    }
    ScopedDeviceMode scopedMode(portModes[port]);
    modules[port]->setup();
  }

  setInputPortCount(INPUT_MIXED_PORT_COUNT);
  clearAllInputFrames();
  pollInterval = 1000;
}

void RZInputMixed::setup2() {
  for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
    if (modules[port] == nullptr) {
      continue;
    }
    ScopedDeviceMode scopedMode(portModes[port]);
    modules[port]->setup2();
  }
  setInputPortCount(INPUT_MIXED_PORT_COUNT);
  pollInterval = 1000;
}

bool RZInputMixed::poll() {
  controller_state_t aggregate[MAX_USB_OUT];
  copyInputFrames(aggregate);

  bool anyUpdated = false;
  for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
    if (modules[port] == nullptr || port >= MAX_USB_OUT) {
      continue;
    }

    restoreInputFrames(aggregate);
    {
      ScopedDeviceMode scopedMode(portModes[port]);
      anyUpdated |= modules[port]->poll();
    }
    aggregate[port] = inputFrame(port);
  }

  restoreInputFrames(aggregate);
  setInputPortCount(INPUT_MIXED_PORT_COUNT);
  pollInterval = 1000;
  if (anyUpdated) {
    markPollUpdated();
  }
  return anyUpdated;
}
