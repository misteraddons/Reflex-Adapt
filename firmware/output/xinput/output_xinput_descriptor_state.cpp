#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <cstring>

#include "out_xinput.h"
#include "output_xinput_autodetect_descriptor_data_runtime.h"
#include "output_xinput_descriptor_state.h"

bool xinput_use_raw_descriptors = false;
bool xinput_composite_hid_enabled = false;
bool xinput_seen_string_0xEE = false;
bool xinput_seen_string_4 = false;
bool xinput_seen_in_xfer = false;

uint8_t xinput_hid_composite_config[TUD_XINPUT_HID_COMPOSITE_DESC_LEN];
uint8_t xinput_config_descriptor_mutable[sizeof(xinput_config_descriptor)];
static uint8_t current_xinput_subtype = XINPUT_SUBTYPE_GAMEPAD;
bool descriptor_initialized = false;
uint8_t autodetect_config_mutable[56];
bool autodetect_config_initialized = false;

uint8_t debug_desc_device_calls = 0;
uint8_t debug_desc_config_calls = 0;
bool debug_hooks_registered = false;

void xinput_set_subtype(uint8_t subtype) {
    current_xinput_subtype = subtype;
    if (!descriptor_initialized) {
        memcpy(xinput_config_descriptor_mutable, xinput_config_descriptor,
               sizeof(xinput_config_descriptor));
        descriptor_initialized = true;
    }
    xinput_config_descriptor_mutable[XINPUT_SUBTYPE_OFFSET] = subtype;
}

uint8_t xinput_get_subtype(void) {
    return current_xinput_subtype;
}

void xinput_build_composite_descriptor(const uint8_t* hid_report_desc, uint16_t hid_report_desc_len) {
    (void)hid_report_desc;
    if (!descriptor_initialized) {
        xinput_set_subtype(XINPUT_SUBTYPE_GAMEPAD);
    }

    memcpy(xinput_hid_composite_config, xinput_config_descriptor_mutable,
           sizeof(xinput_config_descriptor));
    xinput_hid_composite_config[2] = TUD_XINPUT_HID_COMPOSITE_DESC_LEN & 0xFF;
    xinput_hid_composite_config[3] = TUD_XINPUT_HID_COMPOSITE_DESC_LEN >> 8;
    xinput_hid_composite_config[4] = 5;

    uint8_t* p = &xinput_hid_composite_config[TUD_XINPUT_FULL_DESC_LEN];
    *p++ = 9;
    *p++ = TUSB_DESC_INTERFACE;
    *p++ = 4;
    *p++ = 0;
    *p++ = 1;
    *p++ = TUSB_CLASS_HID;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 9;
    *p++ = HID_DESC_TYPE_HID;
    *p++ = 0x11;
    *p++ = 0x01;
    *p++ = 0;
    *p++ = 1;
    *p++ = HID_DESC_TYPE_REPORT;
    *p++ = hid_report_desc_len & 0xFF;
    *p++ = hid_report_desc_len >> 8;
    *p++ = 7;
    *p++ = TUSB_DESC_ENDPOINT;
    *p++ = 0x87;
    *p++ = TUSB_XFER_INTERRUPT;
    *p++ = 64;
    *p++ = 0;
    *p++ = 1;

    xinput_composite_hid_enabled = true;
}

void xinput_init_autodetect_config(uint16_t hid_report_desc_len) {
    if (!autodetect_config_initialized) {
        memcpy(autodetect_config_mutable, autodetect_config_descriptor,
               sizeof(autodetect_config_descriptor));
        autodetect_config_initialized = true;
    }
    autodetect_config_mutable[25] = hid_report_desc_len & 0xFF;
    autodetect_config_mutable[26] = hid_report_desc_len >> 8;
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
