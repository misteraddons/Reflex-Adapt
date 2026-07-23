#pragma once

// Internal console-family USB mode setup helpers extracted from output_usb_mode_setup_runtime.h.

#include "output_usb_xinput2p_slots_runtime.h"

static void configure_xinput_output_runtime() {
  use_device_hid_class = false;
  xinput_composite_hid_enabled = false;
  xinput_seen_string_0xEE = false;
  xinput_seen_string_4 = false;
  xinput_seen_in_xfer = false;
  xinput_use_raw_descriptors = false;
  xinput_set_subtype(determine_xinput_subtype());
  xinput_register_descriptor_hooks();
  _xinput = new Adafruit_USBD_XInput();
  TinyUSBDevice.setID(0x045E, 0x028E);
  TinyUSBDevice.setDeviceClass(0xFF);
  TinyUSBDevice.setDeviceSubClass(0xFF);
  TinyUSBDevice.setDeviceProtocol(0xFF);
  TinyUSBDevice.setManufacturerDescriptor("\xc2\xa9Microsoft Corporation");
  TinyUSBDevice.setProductDescriptor("Controller");
  TinyUSBDevice.setSerialDescriptor("0000000000000000");
  TinyUSBDevice.setConfigurationMaxPower(500);
  TinyUSBDevice.setConfigurationAttribute(0xA0);
}

static void configure_xinput_wireless_output_runtime() {
  use_device_hid_class = false;
  _xinputw = new Adafruit_USBD_XInputW();
  TinyUSBDevice.setID(0x045E, 0x0719);
  TinyUSBDevice.setVersion(0x0200);
  TinyUSBDevice.setDeviceVersion(0x0100);
  TinyUSBDevice.setManufacturerDescriptor("\xc2\xa9Microsoft Corporation");
  TinyUSBDevice.setProductDescriptor("Xbox 360 Wireless Receiver for Windows");
  TinyUSBDevice.setDeviceClass(0xFF);
  TinyUSBDevice.setDeviceSubClass(0xFF);
  TinyUSBDevice.setDeviceProtocol(0xFF);
  TinyUSBDevice.setConfigurationAttribute(0xA0);
}

static uint8_t runtime_xinput_controller_count() {
#ifdef FORCE_XINPUT2P_ONE_CONTROLLER
  return 1;
#endif
  if (configuredOutputMode == OUTPUT_XINPUT ||
      autoDetectState == AUTO_STATE_XBOX_360 ||
      autoDetectProbeStage == AUTO_PROBE_XINPUT) {
    return 1;
  }

  return xinput2p_desired_usb_controller_count();
}

static void configure_xinput_2p_output_runtime() {
#ifdef FORCE_XINPUT2P_SINGLE_DRIVER
  configure_xinput_output_runtime();
  return;
#endif
  use_device_hid_class = false;
  xinput_set_subtype(determine_xinput_subtype());
#ifdef ENABLE_XINPUT2P_WIRELESS_TRANSPORT
  configure_xinput_wireless_output_runtime();
  return;
#endif
  // Two wired-XInput controller groups exceed Adafruit TinyUSB's default
  // 256-byte configuration buffer. Without this, addInterface() fails and
  // Windows sees a device descriptor with an empty 9-byte configuration.
  static uint8_t xinput2p_configuration_buffer[512] __attribute__((aligned(4)));
  TinyUSBDevice.setConfigurationBuffer(
    xinput2p_configuration_buffer,
    sizeof(xinput2p_configuration_buffer)
  );
  const uint8_t controllerCount = runtime_xinput_controller_count();
  _xinput2p = new Adafruit_USBD_XInputMulti(1, controllerCount);
  TinyUSBDevice.setID(0x045E, 0x028E);
  TinyUSBDevice.setVersion(0x0200);
  TinyUSBDevice.setManufacturerDescriptor("\xc2\xa9Microsoft Corporation");
  TinyUSBDevice.setProductDescriptor("Controller");
#ifdef ENABLE_XINPUT_MULTI_OLED_DIAG
  #ifndef XINPUT2P_DIAG_SERIAL_DESCRIPTOR
    #define XINPUT2P_DIAG_SERIAL_DESCRIPTOR "00000000000000D6"
  #endif
  TinyUSBDevice.setSerialDescriptor(XINPUT2P_DIAG_SERIAL_DESCRIPTOR);
#else
  // Keep XInput2P separate from the single Xbox 360 identity and give the
  // one-slot/two-slot shapes distinct IDs so Windows refreshes XUSB children.
  TinyUSBDevice.setSerialDescriptor(
    controllerCount > 1 ? "00000000000000D9" : "00000000000000D8");
#endif
#ifdef ENABLE_XINPUT_MULTI_OLED_DIAG
  #ifndef XINPUT2P_DIAG_DEVICE_VERSION
    #define XINPUT2P_DIAG_DEVICE_VERSION 0x0116
  #endif
  TinyUSBDevice.setDeviceVersion(XINPUT2P_DIAG_DEVICE_VERSION);
#else
  TinyUSBDevice.setDeviceVersion(controllerCount > 1 ? 0x0119 : 0x0118);
#endif
  TinyUSBDevice.setDeviceClass(0xFF);
  TinyUSBDevice.setDeviceSubClass(0xFF);
  TinyUSBDevice.setDeviceProtocol(0xFF);
  TinyUSBDevice.setConfigurationMaxPower(500);
  TinyUSBDevice.setConfigurationAttribute(0xA0);
}

static void configure_xid_output_runtime() {
  use_device_hid_class = false;
  _xid = new Adafruit_USBD_XID();
  TinyUSBDevice.setID(0x045E, 0x0289);
  TinyUSBDevice.setDeviceVersion(0x0100);
  TinyUSBDevice.setVersion(0x0110);
}

#ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
static void configure_xboxone_output_runtime() {
  use_device_hid_class = false;
  _xboxone = new Adafruit_USBD_XboxOne();
  TinyUSBDevice.setID(0x0E6F, 0x02A4);
  TinyUSBDevice.setVersion(0x0200);
  TinyUSBDevice.setDeviceVersion(0x0101);
  TinyUSBDevice.setMaxPacketSize0(64);
  TinyUSBDevice.setDeviceClass(0xFF);
  TinyUSBDevice.setDeviceSubClass(0xFF);
  TinyUSBDevice.setDeviceProtocol(0xFF);
  TinyUSBDevice.setManufacturerDescriptor("Reflex Adapt");
  TinyUSBDevice.setProductDescriptor("Xbox One Controller");
  TinyUSBDevice.setSerialDescriptor("0000000000000001");
  TinyUSBDevice.setConfigurationMaxPower(500);
  TinyUSBDevice.setConfigurationAttribute(0xA0);
}
#endif

static void configure_ps3_output_runtime() {
  use_device_hid_class = true;
  if (can_run_usb_detection()) {
    // AUTO uses the PS3-shaped descriptor only as a discriminator, but it must
    // include the 0x2621 trigger and F2/F5 feature reports. Runtime PS3 falls
    // through to the generic HID fallback below because that is the transport
    // tested PS3 consoles actually accept for controls.
    usb_device_hid[0] = new Adafruit_USBD_HID(
      desc_ps3_probe_report,
      sizeof(desc_ps3_probe_report),
      HID_ITF_PROTOCOL_NONE,
      1,
      true
    );
    TinyUSBDevice.setID(0x054c, 0x0268);
    TinyUSBDevice.setManufacturerDescriptor("Sony");
    TinyUSBDevice.setProductDescriptor("PLAYSTATION(R)3 Controller");
    TinyUSBDevice.setVersion(0x0200);
    TinyUSBDevice.setDeviceVersion(0x0100);
    return;
  }

  // Tested PS3 consoles accept the generic HID fallback shape while ignoring
  // both native DS3 and simplified PS3 reports. Keep AUTO's PS3 feature probe,
  // but runtime PS3 mode deliberately falls back to the known-working path.
  usb_device_hid[0] = new Adafruit_USBD_HID(
    desc_hidgeneric_clean_report,
    sizeof(desc_hidgeneric_clean_report),
    HID_ITF_PROTOCOL_NONE,
    1,
    false
  );
  TinyUSBDevice.setDeviceVersion(bcd_device_version.composite);
  TinyUSBDevice.setID(RZORD1_VID, RZORD1_PID);
  TinyUSBDevice.setManufacturerDescriptor("MiSTerAddons");
  TinyUSBDevice.setProductDescriptor(get_reflex_input_product_name());
  TinyUSBDevice.setSerialDescriptor(get_reflex_input_product_name());
}

static void configure_ps4_output_runtime() {
  use_device_hid_class = true;
  usb_device_hid[0] = new Adafruit_USBD_HID(ps4_desc_hid_report, sizeof(ps4_desc_hid_report), HID_ITF_PROTOCOL_NONE, 8, true);
  TinyUSBDevice.setID(0x03EB, 0x2043);
  TinyUSBDevice.setManufacturerDescriptor("Sony Computer Entertainment");
  TinyUSBDevice.setProductDescriptor("Wireless Controller");
  TinyUSBDevice.setVersion(0x0200);
  TinyUSBDevice.setDeviceVersion(0x0100);
}

static void configure_ps5_general_output_runtime() {
  use_device_hid_class = true;
  usb_device_hid[0] = new Adafruit_USBD_P5General();
  TinyUSBDevice.setID(P5GENERAL_VENDOR_ID, P5GENERAL_PRODUCT_ID);
  TinyUSBDevice.setDeviceClass(0x00);
  TinyUSBDevice.setDeviceSubClass(0x00);
  TinyUSBDevice.setDeviceProtocol(0x00);
  TinyUSBDevice.setMaxPacketSize0(64);
  TinyUSBDevice.setManufacturerDescriptor("Activtor");
  TinyUSBDevice.setProductDescriptor("P5General");
  TinyUSBDevice.ClearStringSerialIndex();
  TinyUSBDevice.setVersion(p5general_bcd_usb);
  TinyUSBDevice.setDeviceVersion(p5general_bcd_device);
  TinyUSBDevice.setConfigurationAttribute(0x80);
  TinyUSBDevice.setConfigurationMaxPower(500);
}

static void configure_switch_output_runtime() {
  use_device_hid_class = true;
  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(pokken_report_desc, sizeof(pokken_report_desc), HID_ITF_PROTOCOL_NONE, 1, true);
  }
  TinyUSBDevice.setID(0x0F0D, 0x0092);
  TinyUSBDevice.setManufacturerDescriptor("HORI CO.,LTD.");
  TinyUSBDevice.setProductDescriptor("POKKEN CONTROLLER");
  TinyUSBDevice.setSerialDescriptor("1.0");
  TinyUSBDevice.setLanguageDescriptor(0x0409);
  TinyUSBDevice.setVersion(0x02);
  TinyUSBDevice.setDeviceVersion(0x0210);
}

static void configure_switchpro_output_runtime() {
  use_device_hid_class = true;
  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(switch_usb_report_descriptor, sizeof(switch_usb_report_descriptor), HID_ITF_PROTOCOL_NONE, 1, true);
    switchpro[i] = new OutputSwitchPro();
  }

  switch (SwitchCommon::switchpro_mode) {
    case SWITCHPRO_PRO:
      TinyUSBDevice.setID(0x057E, 0x2009);
      TinyUSBDevice.setManufacturerDescriptor("Nintendo Co., Ltd.");
      TinyUSBDevice.setProductDescriptor("Pro Controller");
      TinyUSBDevice.setSerialDescriptor("000000000001");
      TinyUSBDevice.setLanguageDescriptor(0x0409);
      TinyUSBDevice.setVersion(0x0200);
      TinyUSBDevice.setDeviceVersion(0x0210);
      break;
    case SWITCHPRO_NES:
    case SWITCHPRO_SNES:
      TinyUSBDevice.setID(0x057E, 0x2017);
      TinyUSBDevice.setManufacturerDescriptor("Nintendo Co., Ltd.");
      TinyUSBDevice.setProductDescriptor("SNES Controller");
      TinyUSBDevice.setSerialDescriptor("000000000001");
      TinyUSBDevice.setLanguageDescriptor(0x0409);
      TinyUSBDevice.setVersion(0x0200);
      TinyUSBDevice.setDeviceVersion(0x0210);
      break;
    case SWITCHPRO_N64:
      TinyUSBDevice.setID(0x057E, 0x2019);
      TinyUSBDevice.setManufacturerDescriptor("Nintendo Co., Ltd.");
      TinyUSBDevice.setProductDescriptor("N64 Controller");
      TinyUSBDevice.setSerialDescriptor("000000000001");
      TinyUSBDevice.setLanguageDescriptor(0x0409);
      TinyUSBDevice.setVersion(0x0200);
      TinyUSBDevice.setDeviceVersion(0x0212);
      break;
  }
}

