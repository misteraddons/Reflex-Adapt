#include "Input_GC64.h"

RZInputGC64::RZInputGC64() : RZInputModule() {}

uint8_t RZInputGC64::getInternalMode() {
  switch (deviceMode) {
    #if defined(ENABLE_INPUT_N64)
      case RZORD_N64: return 1;
    #endif
    #if defined(ENABLE_INPUT_GBA)
      case RZORD_GBA: return 2;
    #endif
    #if defined(ENABLE_INPUT_GAMECUBE)
      case RZORD_GAMECUBE: return 3;
    #endif
    default: return 0;
  }
}

const char* RZInputGC64::getUsbId() {
  switch (getInternalMode()) {
    case 1: return "RZRN64";
    case 2: return "RZRGBA";
    case 3: return "RZRGC";
  }
  return "";
}

void RZInputGC64::configureBcdDeviceVersion() {
  switch (getInternalMode()) {
    case 1:
      bcd_device_version.platform = BCD_PLAT_N64;
      bcd_device_version.platform_sub = 0;
      break;
    case 2:
      bcd_device_version.platform = BCD_PLAT_GBA;
      bcd_device_version.platform_sub = 0;
      break;
    case 3:
      bcd_device_version.platform = BCD_PLAT_GC;
      bcd_device_version.platform_sub = 0;
      break;
  }
}

const char* RZInputGC64::getDescription() {
  switch (getInternalMode()) {
    case 1: return "N64";
    case 2: return "GBA";
    case 3: return "GAMECUBE";
  }
  return "JOYBUS";
}

bool RZInputGC64::is_port_connected(const uint8_t index) {
  return dtype[index] != JOYBUS_DEVICE_NONE;
}

void RZInputGC64::setup() {
  setInputPortCount(input_ports);

  empty_port_behaviour = EMPTY_PORT_USE_INTERVAL;
  polling_empty_interval_ms = 500;

  joybus_pio_set_timeout_ms(1);

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    setInputFrameConnected(i, false);
    clearInputFrameTypeName(i);
  }
  for (uint8_t i = 0; i < input_ports; ++i) {
    dtype[i] = JOYBUS_DEVICE_NONE;
    if (i < GC64_DEBUG_PORTS) {
      gc64_debug_device_type[i] = JOYBUS_DEVICE_NONE;
      gc64_n64_accessory_aux[i] = 0;
      gc64_n64_rumble_pak_detected[i] = false;
      gc64_n64_rumble_command_pending[i] = false;
      gc64_n64_rumble_probe_attempts[i] = 0;
      gc64_n64_rumble_probe_result[i] = 0;
      gc64_n64_rumble_probe_byte[i] = 0;
      gc64_n64_rumble_motor_result[i] = 0;
      gc64_n64_rumble_motor_transport[i] = 0;
      gc64_n64_rumble_motor_expected[i] = 0;
      gc64_n64_rumble_motor_response[i] = 0;
    }
  }

  for (uint8_t i = 0; i < input_ports; ++i) {
    if (!physical_port_enabled(i)) {
      joybus[i] = nullptr;
      continue;
    }
    input_gc64_config_t c = input_gc64_config[i];
    joybus[i] = new JoybusPort(c.pio, c.sm, c.dat);
    joybus[i]->begin();
    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
      frame.HAS_BTN_START = 1;
      frame.HAS_ANALOG_STICK_MAIN = 1;
    }
  }
}

void RZInputGC64::setup2() {
  polling_empty_interval_ms = 500;

  for (uint8_t i = 0; i < input_ports; ++i) {
    if (!physical_port_enabled(i)) {
      continue;
    }
    setPortLed(i, HIGH);
  }

  output_apply_switch_profile_for_joybus_input_mode(getInternalMode(), is_nso_special_active());
}
