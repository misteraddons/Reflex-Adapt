#include "Input_Saturn.h"

#include "../autodetect/input_autodetect_runtime_state.h"

namespace {
constexpr uint32_t kAutoDetectSaturnTapCarryMs = 4000;
}

RZInputSaturn::RZInputSaturn() : RZInputModule() {}

void RZInputSaturn::setPhysicalPortMask(uint8_t mask) {
  RZInputModule::setPhysicalPortMask(mask);
}

uint8_t RZInputSaturn::getInternalMode() {
  switch (deviceMode) {
    #if defined(ENABLE_INPUT_MEGADRIVE)
      case RZORD_MEGADRIVE: return 1;
    #endif
    #if defined(ENABLE_INPUT_SATURN)
      case RZORD_SATURN: return 2;
    #endif
    default: return 0;
  }
}

const char* RZInputSaturn::getUsbId() {
  switch (getInternalMode()) {
    case 1: return "RZRNMD";
    case 2: return "RZRNSAT";
  }
  return "";
}

void RZInputSaturn::configureBcdDeviceVersion() {
  switch (getInternalMode()) {
    case 1:
      bcd_device_version.platform = BCD_PLAT_MEGADRIVE;
      bcd_device_version.platform_sub = 0;
      break;
    case 2:
      bcd_device_version.platform = BCD_PLAT_SATURN;
      bcd_device_version.platform_sub = 0;
      break;
  }
}

const char* RZInputSaturn::getDescription() {
  switch (getInternalMode()) {
    case 1: return "GENESIS";
    case 2: return "SATURN";
  }
  return "SATURN/MD";
}

bool RZInputSaturn::recentControllerSlotPresent() const {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    if (dtype[i] == SAT_DEVICE_NONE ||
        dtype[i] == SAT_DEVICE_NOTSUPPORTED ||
        slotLastSeenMs[i] == 0) {
      continue;
    }
    if ((uint32_t)(now - slotLastSeenMs[i]) <= HOTSWAP_RECENT_SLOT_GRACE_MS) {
      return true;
    }
  }
  return false;
}

void RZInputSaturn::updateTapPresence(uint8_t port, uint8_t tap_ports, uint32_t now_ms) {
  if (port >= input_ports) {
    return;
  }
  if (tap_ports != 0) {
    lastTapPorts[port] = tap_ports;
    tapLastSeenMs[port] = now_ms;
  } else if (lastTapPorts[port] != 0 &&
             (int32_t)(now_ms - (tapLastSeenMs[port] + TAP_DISCONNECT_DEBOUNCE_MS)) >= 0) {
    lastTapPorts[port] = 0;
  }
}

uint8_t RZInputSaturn::refreshTapPresence(uint8_t port, uint8_t observed_tap_ports, uint32_t now_ms) {
  if (port >= input_ports || saturn[port] == nullptr) {
    return observed_tap_ports;
  }
  if (observed_tap_ports != 0) {
    tapLatchedFromAutoDetect[port] = false;
    updateTapPresence(port, observed_tap_ports, now_ms);
    return observed_tap_ports;
  }
  // SaturnLib's multitap probe is a startup/hotplug handshake, not a reliable
  // continuous removal test. Keep AUTO-detected bare taps latched so zero-child
  // taps do not bounce between Saturn mode and AUTO.
  if (tapLatchedFromAutoDetect[port] && lastTapPorts[port] != 0) {
    return lastTapPorts[port];
  }
  updateTapPresence(port, observed_tap_ports, now_ms);
  return observed_tap_ports;
}

bool RZInputSaturn::hasPhysicalConnectionForHotSwap() const {
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (!physical_port_enabled(i) || saturn[i] == nullptr) {
      continue;
    }
    if (lastTapPorts[i] != 0) {
      return true;
    }
  }
  return recentControllerSlotPresent();
}

const char* RZInputSaturn::physicalConnectionDisplayName() const {
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (lastTapPorts[i] != 0) {
      #if defined(ENABLE_INPUT_MEGADRIVE)
      if (deviceMode == RZORD_MEGADRIVE) {
        return "Mega Tap";
      }
      #endif
      return "Saturn Tap";
    }
  }
  if (!recentControllerSlotPresent()) {
    return nullptr;
  }
  #if defined(ENABLE_INPUT_MEGADRIVE)
  if (deviceMode == RZORD_MEGADRIVE) {
    return "Mega Pad";
  }
  #endif
  return "Saturn Pad";
}

void RZInputSaturn::setup() {
  setInputPortCount(input_ports);

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = inputFrame(i);
    dtype[i] = SAT_DEVICE_NONE;
    stableMegadriveType[i] = SAT_DEVICE_NONE;
    megadriveTypeDowngradeStartedMs[i] = 0;
    if (i < input_ports) {
      lastTapPorts[i] = 0;
      tapLastSeenMs[i] = 0;
      tapLatchedFromAutoDetect[i] = false;
    }
    slotLastSeenMs[i] = 0;
    setInputFrameConnected(frame, false);
    clearInputFrameTypeName(frame);
    frame.HAS_BTN_START = 1;
    frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
  }

  // Manual modes should only parse their selected protocol. Leaving both
  // enabled lets some MegaDrive button patterns look like Saturn IDs.
  switch (getInternalMode()) {
    case 1:
      satlibconfig = SatLibConfig_megadrive_only;
      break;
    case 2:
      satlibconfig = SatLibConfig_saturn_only;
      break;
    default:
      satlibconfig = SatLibConfig_default;
      break;
  }

  for (uint8_t i = 0; i < input_ports; ++i) {
    if (!physical_port_enabled(i)) {
      saturn[i] = nullptr;
      portSlotCapacity[i] = 1;
      continue;
    }
    input_saturn_config_t c = input_saturn_config[i];
    saturn[i] = new SaturnPort(c.d0, c.d1, c.d2, c.d3, c.th, c.tr, c.tl, satlibconfig);
    saturn[i]->begin();
    portSlotCapacity[i] = 1;
  }

  if (satlibconfig.enable_sattap || satlibconfig.enable_megatap) {
    bool found_mtap = false;
    delay(100);
    uint8_t total_controller_ports = 0;
    for (uint8_t i = 0; i < input_ports; ++i) {
      if (!physical_port_enabled(i) || saturn[i] == nullptr) {
        total_controller_ports += portSlotCapacity[i];
        continue;
      }
      saturn[i]->detectMultitap();
      uint8_t tap = saturn[i]->getMultitapPorts();
      if (tap == 0 && inputAutoDetectRecentSaturnTap(i, millis(), kAutoDetectSaturnTapCarryMs)) {
        tap = input_autodetect_saturn_tap_ports;
        tapLatchedFromAutoDetect[i] = true;
      }
      updateTapPresence(i, tap, millis());
      #if defined(AUTODETECT_DEBUG_CDC)
      printf("Saturn setup: port%d multitap_ports=%d\n", i, tap);
      #endif
      portSlotCapacity[i] = tap ? tap : 1;
      input_saturn_debug_update_port(i, tap, lastTapPorts[i], 0, portSlotCapacity[i]);
      total_controller_ports += portSlotCapacity[i];
      if (tap) {
        found_mtap = true;
      }
    }
    setInputPortCount(min(total_controller_ports, MAX_USB_OUT));
    input_saturn_debug_set_total_reserved_slots(inputPortCount());
    #if defined(AUTODETECT_DEBUG_CDC)
    printf("Saturn setup: total_controller_ports=%d port_count=%d MAX_USB_OUT=%d\n",
           total_controller_ports, inputPortCount(), MAX_USB_OUT);
    #endif
    if (found_mtap) {
      delay(50);
    }
  }
}

void RZInputSaturn::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i) {
    if (!physical_port_enabled(i)) {
      continue;
    }
    setPortLed(i, HIGH);
  }
}
