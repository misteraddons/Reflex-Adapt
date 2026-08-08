#pragma once

#include "../auth/auth_msc_runtime.h"
#include "../auth/auth_status.h"

// Internal USB output configuration/runtime helpers. This stays header-only so
// it can reuse the shared TinyUSB objects and output configuration state still
// declared in out_usb.h.

inline bool output_mode_requires_home(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XBOXONE:
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
      return true;
    default:
      return false;
  }
}

inline void configure_usb_output() {
  #if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  const bool lateTinyUsbInit = !tud_inited();
  #endif

  TinyUSBDevice.clearConfiguration();

  #if defined(ENABLE_INPUT_AUTODETECT) && defined(AUTODETECT_DEBUG_CDC)
  Serial.begin(115200);
  #endif

#if defined(ENABLE_INPUT_DREAMCAST)
  dreamcastDebugCdcEnabled = false;
#endif

  auto_detect_clear_scratch_state();

  use_device_hid_class = false;
  grouped_devices = 0;
  outputMode_t effectiveOutputMode = get_effective_output_mode();
  if (!authOutputModeCanRun(effectiveOutputMode)) {
    outputMode = OUTPUT_MISTER;
    effectiveOutputMode = OUTPUT_MISTER;
  }
  const bool managementCdcEnabled =
    output_allows_management_cdc(effectiveOutputMode);

  #ifdef ENABLE_RUNTIME_SERIAL_DEBUG_CDC
  runtimeDebugCdcEnabled = managementCdcEnabled;
  if (runtimeDebugCdcEnabled) {
    configure_management_composite_buffer_runtime();
    runtimeDebugCdc.begin(115200);
    // Adafruit_USBD_CDC::begin() installs the default "TinyUSB Serial" string.
    // Override it after begin so Web Serial users can identify the control port.
    runtimeDebugCdc.setStringDescriptor("Reflex Adapt Control");
  }
  #endif

  auth_msc_configure(effectiveOutputMode);

  if (output_is_generic_mister_hid_mode(effectiveOutputMode)) {
    configure_generic_mister_hid_output_runtime();
  } else if (output_effective_mode_is(OUTPUT_MISTER_JOGCON)) {
    configure_mister_jogcon_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_MISTER_NEGCON) {
    configure_mister_negcon_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_MISTER_GUNCON) {
    configure_mister_guncon_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_KEYBOARD) {
    configure_keyboard_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_XINPUT) {
    configure_xinput_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_XINPUTW) {
    configure_xinput_wireless_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_XINPUT2P) {
    configure_xinput_2p_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_XID) {
    configure_xid_output_runtime();
  #ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
  } else if (effectiveOutputMode == OUTPUT_XBOXONE) {
    configure_xboxone_output_runtime();
  #endif
  } else if (effectiveOutputMode == OUTPUT_PS3) {
    configure_ps3_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_PS4) {
    configure_ps4_output_runtime();
  #ifdef ENABLE_OUTPUT_PS5
  } else if (effectiveOutputMode == OUTPUT_PS5) {
    configure_ps5_general_output_runtime();
  #endif
  } else if (effectiveOutputMode == OUTPUT_SWITCH) {
    configure_switch_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_SWITCHPRO) {
    configure_switchpro_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_PANTHERLORD) {
    configure_pantherlord_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_GCWIIU) {
    configure_gcwiiu_output_runtime();
  } else if (effectiveOutputMode == OUTPUT_MDMINI) {
    configure_mdmini_output_runtime();
#ifdef ENABLE_OUTPUT_JVS
  } else if (effectiveOutputMode == OUTPUT_JVS) {
    use_device_hid_class = false;
    jvsOutput.begin();
    return;
#endif
  }

  prepare_manual_tinyusb_output_init_runtime();
  begin_output_dreamcast_debug_cdc_runtime();
  begin_output_usb_interfaces_runtime();

  #if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  if (lateTinyUsbInit) {
    TinyUSB_Port_InitDevice(0);
    tud_disconnect();
  }
  #endif
}
