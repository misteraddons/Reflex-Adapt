#pragma once

#include <Arduino.h>  // For rp2040, delay(), etc.
#include "../../core/analog_calibration_state.h"
#include "../../core/controller_delivery_state.h"
#include "../../core/controller_frame_state.h"
#include "../../core/controller_output_cache_state.h"
#include "../../core/controller_settings_state.h"
#include "../../core/device_runtime_state.h"
#include "../../core/rumble_test_runtime.h"
#include "../../input/autodetect/input_autodetect_runtime_state.h"
#include "../autodetect/output_autodetect_reporting.h"
#include "../autodetect/output_autodetect_support.h"
#include "../../firmware_build_info.h"
#include "../../firmware_platform_config.h"
#include "../../core/n64_output_helpers.h"
#include "../../core/runtime/runtime_debug_cdc_state.h"
#include "../../input/dreamcast/input_dreamcast_debug_runtime_state.h"
#include "../../input/jaguar/input_jaguar_runtime_state.h"
#include "../runtime/input_runtime_output_bridge.h"
#include "../runtime/output_boot_bridge.h"
#include "../output_capabilities.h"
#include "../output_mode.h"
#include "../output_runtime_state.h"
#include "webhid/webhid_basic_reports.h"
#include "webhid/webhid_debug_reports.h"
#include "webhid/webhid_input_modes.h"
#include "webhid/webhid_report_dispatch.h"
#include "webhid/webhid_protocol.h"
#include "webhid/webhid_runtime_reports.h"
#include "webhid/webhid_settings_reports.h"
#include "webhid/webhid_write_reports.h"
#include "../xinput/output_xinput_profile.h"
#include "../../platform/webhid_runtime.h"
#include "../../platform/latency_trace_gpio.h"

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  #include "hardware/irq.h"
  #include "hardware/regs/usb.h"
  #include "hardware/structs/usb.h"
#endif

void rumble_callback (uint8_t itf, uint8_t index, uint8_t left_large, uint8_t right_small);

#include "../out_report.h"
#include "webhid/webhid_command_runtime.h"
#include "../auth/webhid_auth_runtime.h"
#include "../auth/webhid_auth_reports.h"
#include "../descriptors.h"
#include "../autodetect/output_autodetect_runtime.h"
#include "../autodetect/output_autodetect_hid_bridge.h"

#ifdef ADAPT_OUTPUT_USB_DEVICE
  #include "../xinput/out_xinput.h"
  #include "../xinput/out_xinput_multi.h"
  #include "../xinputw/out_xinputw.h"
  #include "../switch/out_switchpro.h"
  #include "../xid/xid.h"
  #include "../xid/xid_driver.h"
  #ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
  #include "../xboxone/out_xboxone.h"
  #endif
#endif

// Fallback for XInput wireless controller count (defined in out_xinputw.h for USB device builds)
#ifndef XINPUT_WIRELESS_CONTROLLERS
  #define XINPUT_WIRELESS_CONTROLLERS 4
#endif
#ifndef XINPUT_MULTI_CONTROLLERS
  #define XINPUT_MULTI_CONTROLLERS 2
#endif

// MAX_USB_OUT comes from firmware_platform_config.h so non-output modules can
// size shared arrays without depending on this full output stack header.

//typedef struct __attribute((packed, aligned(1))) {
//  uint32_t hotkey;
//  outputMode_t output_mode;
//  switchpro_mode_enum switch_pro_mode; // optional
//} hotkey2_t;


bcd_device_version_t bcd_device_version;

//rumble received from host
void rumble_callback(uint8_t index, uint8_t left_large, uint8_t right_small) {
  rumbleRuntimeSetHostFeedback(index, left_large, right_small);
}

#ifdef ADAPT_OUTPUT_USB_DEVICE
// =============================================================================
// Auth Key Storage
// =============================================================================
// Keys are stored in a reserved EEPROM region starting at offset 512
// PS4 key: 1712 bytes (RSA private key + certificate)
// Xbox 360 identity/auth data is embedded in firmware (libxsm3), not uploaded via WebHID.

// Debug counters for WebHID troubleshooting
// WebHID function declarations (implementations are after necessary types are defined)
uint16_t webhid_get_report(uint8_t report_id, uint8_t* buffer, uint16_t reqlen);
void webhid_set_report(uint8_t report_id, const uint8_t* buffer, uint16_t bufsize);

// External declarations for WebHID access (defined in firmware.cpp)
#ifdef ENABLE_INPUT_JAGUAR
static inline bool jaguarRotaryActiveOnPort(uint8_t port) {
  return port < MAX_USB_OUT && jaguarRotaryActivePorts[port];
}
#endif

// Turbo and remap (defined in firmware.cpp / button_remap.h)
// Forward declaration of turbo enums (must match turbo.h)
#ifndef TURBO_ENUMS_DEFINED
enum TurboRate : uint8_t {
  TURBO_OFF = 0,
  TURBO_SLOW,
  TURBO_MEDIUM,
  TURBO_FAST,
  TURBO_MAX,
  TURBO_ULTRA,
  TURBO_SLOW_LATCHED,
  TURBO_MEDIUM_LATCHED,
  TURBO_FAST_LATCHED,
  TURBO_MAX_LATCHED,
  TURBO_ULTRA_LATCHED,
  TURBO_RATE_LAST
};
enum TurboButton : uint8_t {
  TURBO_BTN_0 = 0,
  TURBO_BTN_1,
  TURBO_BTN_2,
  TURBO_BTN_3,
  TURBO_BTN_4,
  TURBO_BTN_5,
  TURBO_BTN_6,
  TURBO_BTN_7,
  TURBO_BTN_8,
  TURBO_BTN_9,
  TURBO_BTN_10,
  TURBO_BTN_11,
  TURBO_BTN_12,
  TURBO_BTN_13,
  TURBO_BTN_14,
  TURBO_BTN_15,
  TURBO_BTN_16,
  TURBO_BTN_COUNT
};
#define TURBO_ENUMS_DEFINED
#endif
class TurboController;
extern TurboController turbo;
extern uint8_t active_remaps[];  // Button remap array
extern uint16_t g_eeprom_remap_index;  // Per-mode remap EEPROM address

// GPIO pin arrays for debug (defined in menu.h, but we access via gpio_get)
// Port 1 pins: HDMI_1_01 through HDMI_1_13
// Port 2 pins: HDMI_2_01 through HDMI_2_13

// =============================================================================
// End WebHID Configuration Interface (declarations)
// =============================================================================

//swich pro get_controller_rumble

#include "output_xid_interface_runtime.h"


extern bool _xinput_tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request);
extern bool _xinputw_tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request);
#else // !ADAPT_OUTPUT_USB_DEVICE
// WebHID input tracing stubs live in platform/webhid_runtime.h for non-USB builds.
#endif // ADAPT_OUTPUT_USB_DEVICE (WebHID + USB device externs)

// PSX peripheral MiSTer flag — outside extern "C" so RZInputModule.h can reference it
#ifdef __cplusplus
extern "C"
{
#endif

uint8_t grouped_devices = 0; // special case for OUTPUT_GCWIIU WUP-028

#ifdef ADAPT_OUTPUT_USB_DEVICE
// USB HID object. For ESP32 these values cannot be changed after this declaration
// desc report, desc len, protocol, interval, use out endpoint
Adafruit_USBD_HID *usb_device_hid[MAX_USB_OUT];
Adafruit_USBD_XInput *_xinput;
Adafruit_USBD_XInputMulti *_xinput2p;
Adafruit_USBD_XInputW *_xinputw;
Adafruit_USBD_XID *_xid;
#ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
Adafruit_USBD_XboxOne *_xboxone;
#endif
Adafruit_USBD_XInputAuth *_usb_detect_xinput_auth;
Adafruit_USBD_XID *_usb_detect_xid;

#endif // ADAPT_OUTPUT_USB_DEVICE (USB device objects and default reports)

#ifndef DREAMCAST_DEBUG_FORCE_CDC
// 0 = enable Dreamcast CDC debug only in HID/MiSTer output modes (recommended)
// 1 = force CDC debug COM port in all Dreamcast output modes (debug only)
#define DREAMCAST_DEBUG_FORCE_CDC 0
#endif

#ifndef DREAMCAST_SERIAL_DEBUG
#define DREAMCAST_SERIAL_DEBUG 0
#endif

#ifdef ENABLE_OUTPUT_JVS
// JVS output mode (RS-485 arcade I/O)
#include "../jvs/out_jvs.h"
#endif

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include "output_usb_report_state.h"
#include "output_usb_hid_port_order_runtime.h"
#include "output_usb_hid_report_runtime.h"

#include "output_usb_hid_callback_runtime.h"
#endif // ADAPT_OUTPUT_USB_DEVICE (report instances, HID callbacks)

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include "output_usb_mode_setup_runtime.h"
#include "output_usb_begin_runtime.h"
#include "output_usb_configure_runtime.h"
#include "output_mapping_support_runtime.h"

#include "output_hid_mapping_runtime.h"
#include "output_keyboard_mapping_runtime.h"

#include "../output_xbox_mapping_runtime.h"

#include "../playstation/output_playstation_mapping_runtime.h"
#include "../switch/output_switch_legacy_mapping_runtime.h"
#include "output_usb_send_runtime.h"
#include "output_usb_control_runtime.h"


#endif // ADAPT_OUTPUT_USB_DEVICE (USB device configuration, output, and TinyUSB callbacks)

#ifdef __cplusplus
}
#endif

#ifndef ADAPT_OUTPUT_USB_DEVICE
// =============================================================================
// Stubs for non-USB-device builds (USB2RF, USB2JVS, USB2Classic)
// =============================================================================

#ifdef ENABLE_OUTPUT_ESP32_BT
#include "../out_esp32_bt.h"
inline void configure_usb_output() { esp32_bt_init(); }
inline void send_usb_report() { esp32_bt_send(); }
#elif defined(ENABLE_OUTPUT_JVS)
inline void configure_usb_output() { jvsOutput.begin(); }
inline void send_usb_report() { jvsOutput.update(); }
#elif defined(ENABLE_OUTPUT_CONSOLE)
#include "../classic/out_classic.h"
inline void configure_usb_output() { classic_output_begin(); }
inline void send_usb_report() { classic_output_update(); }
#elif defined(ENABLE_OUTPUT_DB15)
#include "../db15/out_db15.h"
inline void configure_usb_output() { db15_output_begin(); }
inline void send_usb_report() { db15_output_update(); }
#else
inline void configure_usb_output() {}
inline void send_usb_report() {}
#endif

inline void auto_detect_process() {}
inline void webhid_process_commands() {}
inline bool webhid_process_rumble() { return false; }
#endif // !ADAPT_OUTPUT_USB_DEVICE
