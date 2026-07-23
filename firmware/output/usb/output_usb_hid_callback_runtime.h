#pragma once

// Internal HID callback trampolines and AUTO XID helper wiring for the USB
// device output stack. This stays header-only so it can share the existing
// callback bodies and output object state defined in out_usb.h.

uint16_t hid_device_get_report_callback_0(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return hid_device_get_report_callback(0, report_id, report_type, buffer, reqlen);
}

uint16_t hid_device_get_report_callback_1(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return hid_device_get_report_callback(1, report_id, report_type, buffer, reqlen);
}

uint16_t hid_device_get_report_callback_2(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return hid_device_get_report_callback(2, report_id, report_type, buffer, reqlen);
}

uint16_t hid_device_get_report_callback_3(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return hid_device_get_report_callback(3, report_id, report_type, buffer, reqlen);
}

uint16_t hid_device_get_report_callback_4(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return hid_device_get_report_callback(4, report_id, report_type, buffer, reqlen);
}

uint16_t hid_device_get_report_callback_5(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return hid_device_get_report_callback(5, report_id, report_type, buffer, reqlen);
}

uint16_t (*hid_device_get_report_callback_arr[]) (uint8_t, hid_report_type_t, uint8_t*, uint16_t) = {
  hid_device_get_report_callback_0,
  hid_device_get_report_callback_1,
  hid_device_get_report_callback_2,
  hid_device_get_report_callback_3,
  hid_device_get_report_callback_4,
  hid_device_get_report_callback_5,
};

void hid_device_set_report_callback_0(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  hid_device_set_report_callback(0, report_id, report_type, buffer, bufsize);
}

void hid_device_set_report_callback_1(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  hid_device_set_report_callback(1, report_id, report_type, buffer, bufsize);
}

void hid_device_set_report_callback_2(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  hid_device_set_report_callback(2, report_id, report_type, buffer, bufsize);
}

void hid_device_set_report_callback_3(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  hid_device_set_report_callback(3, report_id, report_type, buffer, bufsize);
}

void hid_device_set_report_callback_4(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  hid_device_set_report_callback(4, report_id, report_type, buffer, bufsize);
}

void hid_device_set_report_callback_5(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  hid_device_set_report_callback(5, report_id, report_type, buffer, bufsize);
}

void (*hid_device_set_report_callback_arr[]) (uint8_t, hid_report_type_t, uint8_t const*, uint16_t) = {
  hid_device_set_report_callback_0,
  hid_device_set_report_callback_1,
  hid_device_set_report_callback_2,
  hid_device_set_report_callback_3,
  hid_device_set_report_callback_4,
  hid_device_set_report_callback_5,
};

inline void auto_detect_init_xid_interface(uint8_t rhport) {
  if (!_usb_detect_xid)
    return;

  uint8_t ep_in = (_usb_detect_xid->stored_ep_in != 0) ? _usb_detect_xid->stored_ep_in : 0x81;
  uint8_t ep_out = (_usb_detect_xid->stored_ep_out != 0) ? _usb_detect_xid->stored_ep_out : 0x02;
  xid_manual_init(rhport, _usb_detect_xid->stored_itfnum, ep_in, ep_out);
}
