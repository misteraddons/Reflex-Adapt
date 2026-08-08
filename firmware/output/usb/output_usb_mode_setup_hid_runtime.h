#pragma once

// Internal HID-family USB mode setup helpers extracted from output_usb_mode_setup_runtime.h.

static void configure_management_composite_buffer_runtime() {
  // CDC plus one or more HID interfaces can exceed Adafruit TinyUSB's default
  // 256-byte configuration buffer. One shared buffer covers DInput and the
  // specialized MiSTer PSX composites without changing their HID reports.
  static uint8_t management_configuration_buffer[512] __attribute__((aligned(4)));
  TinyUSBDevice.setConfigurationBuffer(
    management_configuration_buffer,
    sizeof(management_configuration_buffer)
  );
}

static void configure_generic_mister_hid_output_runtime() {
  use_device_hid_class = true;

  // DInput combines CDC, MSC, and one or more HID interfaces. The Auto probe
  // also adds the Xbox security interface. Either composite can exceed
  // Adafruit TinyUSB's default 256-byte configuration buffer, which causes
  // Windows to reject the device descriptor and leaves boot apparently hung.
  configure_management_composite_buffer_runtime();

  if (can_run_usb_detection()) {
    // Keep the RetroZord generic VID/PID and Xbox security side-interface.
    // The HID report adds local PS3 feature/output discriminators alongside
    // the donor PS4 trigger so mounted generic HID hosts can still time out.
    use_device_hid_class = true;
    xinput_seen_string_0xEE = false;
    xinput_seen_string_4 = false;
    xinput_seen_in_xfer = false;
    xinput_auth_invalidate();
    const uint8_t* autoDetectReportDesc = desc_hidgeneric_autodetect_report;
    const uint16_t autoDetectReportDescLen = sizeof(desc_hidgeneric_autodetect_report);
    autodetect_register_descriptor_hooks(autoDetectReportDescLen);
    usb_device_hid[0] = new Adafruit_USBD_HID(
      autoDetectReportDesc,
      autoDetectReportDescLen,
      HID_ITF_PROTOCOL_NONE,
      1,
      true
    );
    _usb_detect_xinput_auth = new Adafruit_USBD_XInputAuth();

    TinyUSBDevice.setID(0x0FFF, 0x0FF9);
    TinyUSBDevice.setVersion(0x0200);
    TinyUSBDevice.setDeviceVersion(usb_detect_probe_device_version());
    TinyUSBDevice.setDeviceClass(0x00);
    TinyUSBDevice.setDeviceSubClass(0x00);
    TinyUSBDevice.setDeviceProtocol(0x00);
    TinyUSBDevice.setConfigurationMaxPower(500);
    TinyUSBDevice.setConfigurationAttribute(0xA0);
    return;
  }

  TinyUSBDevice.setDeviceVersion(can_run_usb_detection()
    ? usb_detect_probe_device_version()
    : bcd_device_version.composite);
  TinyUSBDevice.setID(RZORD1_VID, RZORD1_PID);
  TinyUSBDevice.setManufacturerDescriptor("MiSTerAddons");
  TinyUSBDevice.setProductDescriptor(PRODUCT_NAME);
  TinyUSBDevice.setSerialDescriptor(get_reflex_input_usb_serial_descriptor());

  const bool managementEndpoints =
    output_allows_management_usb_endpoints(get_effective_output_mode());
  const uint8_t* reportDesc = managementEndpoints
    ? desc_hidgeneric_report
    : desc_hidgeneric_clean_report;
  const uint16_t reportDescSize = managementEndpoints
    ? sizeof(desc_hidgeneric_report)
    : sizeof(desc_hidgeneric_clean_report);

  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(
      reportDesc,
      reportDescSize,
      HID_ITF_PROTOCOL_NONE,
      1,
      managementEndpoints
    );
  }
}

// Keep the legacy MiSTer VID/PID and product-name compatibility checks, but
// give each specialized PSX report family its own SDL/GameControllerDB GUID.
static constexpr uint16_t kMisterJogConDeviceVersion = 0x0101;
static constexpr uint16_t kMisterNegConDeviceVersion = 0x0102;
static constexpr uint16_t kMisterGunConDeviceVersion = 0x0103;

static void configure_mister_jogcon_output_runtime() {
  use_device_hid_class = true;
  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(jogconmister_report_descriptor, sizeof(jogconmister_report_descriptor), HID_ITF_PROTOCOL_NONE, 1, false);
  }

  TinyUSBDevice.setID(RZORD1_VID, RZORD1_MISTER_PID);
  TinyUSBDevice.setDeviceVersion(kMisterJogConDeviceVersion);
  TinyUSBDevice.setManufacturerDescriptor(RZORD1_VENDOR);
  TinyUSBDevice.setProductDescriptor(get_reflex_input_product_name());
  TinyUSBDevice.setSerialDescriptor(get_reflex_input_product_name());
}

static void configure_mister_negcon_output_runtime() {
  use_device_hid_class = true;
  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(negconmister_report_descriptor, sizeof(negconmister_report_descriptor), HID_ITF_PROTOCOL_NONE, 1, false);
  }

  TinyUSBDevice.setID(RZORD1_VID, RZORD1_MISTER_PID);
  TinyUSBDevice.setDeviceVersion(kMisterNegConDeviceVersion);
  TinyUSBDevice.setManufacturerDescriptor(RZORD1_VENDOR);
  TinyUSBDevice.setProductDescriptor(get_reflex_input_product_name());
  TinyUSBDevice.setSerialDescriptor(get_reflex_input_product_name());
}

static void configure_mister_guncon_output_runtime() {
  use_device_hid_class = true;
  for (uint8_t i = 0; i < output_usb_player_count(); ++i) {
    usb_device_hid[i] = new Adafruit_USBD_HID(gunconmister_report_descriptor, sizeof(gunconmister_report_descriptor), HID_ITF_PROTOCOL_NONE, 1, false);
  }

  TinyUSBDevice.setID(RZORD1_VID, RZORD1_MISTER_PID);
  TinyUSBDevice.setDeviceVersion(kMisterGunConDeviceVersion);
  TinyUSBDevice.setManufacturerDescriptor(RZORD1_VENDOR);
  TinyUSBDevice.setProductDescriptor(get_reflex_input_product_name());
  TinyUSBDevice.setSerialDescriptor(get_reflex_input_product_name());
}

static void configure_keyboard_output_runtime() {
  use_device_hid_class = true;

  TinyUSBDevice.setID(RZORD1_VID, RZORD1_PID);
  TinyUSBDevice.setDeviceVersion(RZORD1_VERSION);
  TinyUSBDevice.setManufacturerDescriptor(RZORD1_VENDOR);
  TinyUSBDevice.setProductDescriptor(PRODUCT_NAME);
  TinyUSBDevice.setSerialDescriptor(PRODUCT_NAME);

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    usb_device_hid[i] = nullptr;
  }

  // One physical keyboard interface carries both MAME P1 and P2 key mappings.
  usb_device_hid[0] = new Adafruit_USBD_HID(
    keyboard_report_desc,
    sizeof(keyboard_report_desc),
    HID_ITF_PROTOCOL_KEYBOARD,
    1,
    false
  );
}

