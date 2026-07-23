#ifndef _USB_DETECT_H_
#define _USB_DETECT_H_

#include "usb_detect_runtime_state.h"
#include "usb_detect_xinput_auth.h"

#ifdef __cplusplus
extern "C" {
#endif
bool can_run_usb_detection(void);
#ifdef __cplusplus
}
#endif

uint16_t const* usb_detect_handle_ms_os_10(void);
void usb_detect_handle_manufacturer_read(void);
void usb_detect_handle_descriptor_read(void);
void usb_detect_handle_edpt_clear_stall(uint8_t ep_addr);
void usb_detect_handle_hid_get(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
void usb_detect_handle_hid_set(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
bool usb_detect_handle_vendor_control_xfer(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request);
bool usb_detect_has_mounted_generic_hid_activity(void);
uint8_t usb_detect_get_aux_flags(void);
uint8_t usb_host_detection_task(void);

extern "C" uint16_t const* get_ms_os_10_string_ptr(void);
void string_manufacturer_read_cb(void);
extern "C" void hid_descriptor_report_read_cb(void);
extern "C" void usbd_edpt_clear_stall_cb(uint8_t ep_addr);

#endif  // _USB_DETECT_H_
