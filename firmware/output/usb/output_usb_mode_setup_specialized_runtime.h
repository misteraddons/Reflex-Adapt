#pragma once

// Internal specialized USB mode setup helpers extracted from output_usb_mode_setup_runtime.h.

static void configure_pantherlord_output_runtime() {
  use_device_hid_class = true;
  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(pantherlord_report_descriptor, sizeof(pantherlord_report_descriptor), HID_ITF_PROTOCOL_NONE, 1, false);
  }

  TinyUSBDevice.setID(0x0810, 0x0001);
  TinyUSBDevice.setManufacturerDescriptor("PantherLord");
  TinyUSBDevice.setProductDescriptor("Twin USB Joystick");
  TinyUSBDevice.setLanguageDescriptor(0x0409);
  TinyUSBDevice.setVersion(0x0100);
  TinyUSBDevice.setDeviceVersion(0x0106);
}

static void configure_gcwiiu_output_runtime() {
  use_device_hid_class = true;
  grouped_devices = 1;
  gcwiiu_initialized = false;
  usb_device_hid[0] = new Adafruit_USBD_HID(gcwiiu_report_desc, sizeof(gcwiiu_report_desc), HID_ITF_PROTOCOL_NONE, 4, true);
  TinyUSBDevice.setID(0x057E, 0x0337);
  TinyUSBDevice.setManufacturerDescriptor("Nintendo");
  TinyUSBDevice.setProductDescriptor("WUP-028");
  TinyUSBDevice.setLanguageDescriptor(0x0409);
  TinyUSBDevice.setVersion(0x0200);
  TinyUSBDevice.setDeviceVersion(0x0100);
}

static void configure_mdmini_output_runtime() {
  use_device_hid_class = true;
  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(mdmini_report_descriptor, sizeof(mdmini_report_descriptor), HID_ITF_PROTOCOL_NONE, 1, false);
  }
  TinyUSBDevice.setID(0x0CA3, 0x0024);
  TinyUSBDevice.setManufacturerDescriptor("SEGA CORP.");
  TinyUSBDevice.setProductDescriptor("MEGA DRIVE mini GAMEPAD");
  TinyUSBDevice.setSerialDescriptor("1.0");
  TinyUSBDevice.setVersion(0x0200);
  TinyUSBDevice.setDeviceVersion(0x0206);
}
