#ifndef OUTPUT_XINPUT_CAPABILITIES_RUNTIME_H_
#define OUTPUT_XINPUT_CAPABILITIES_RUNTIME_H_

#include <stdint.h>
#include <tusb.h>

namespace xinput_capabilities_detail {

inline uint32_t xinput_serial_for_interface(uint8_t itf) {
    // Keep the high bytes stable and vary the low byte by interface group so
    // multi-controller composites do not report identical capability identity.
    return 0x00122860UL + (uint32_t)(itf & 0x0F);
}

inline bool is_xinput_vendor_get_report(
    const tusb_control_request_t *request
) {
    return request &&
           request->bmRequestType_bit.direction == TUSB_DIR_IN &&
           request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
           request->bRequest == 0x01;
}

}  // namespace xinput_capabilities_detail

inline bool xinput_handle_capability_control(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    if (stage != CONTROL_STAGE_SETUP ||
        !xinput_capabilities_detail::is_xinput_vendor_get_report(request) ||
        request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_INTERFACE ||
        (request->wValue != 0x0000 && request->wValue != 0x0100)) {
        return false;
    }

    static const uint8_t vibration_caps[] = {
        0x00, 0x08, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00
    };
    static const uint8_t input_caps[] = {
        0x00, 0x14,
        0x3F, 0xF7,
        0xFF, 0xFF,
        0x00, 0x00,
        0x00, 0x00,
        0xC0, 0xFF,
        0xC0, 0xFF,
        0x00, 0x00,
        0x00, 0x00,
        0x01, 0x00
    };

    switch (request->wValue) {
        case 0x0000:
            return tud_control_xfer(
                rhport,
                request,
                const_cast<uint8_t *>(vibration_caps),
                sizeof(vibration_caps)
            );
        case 0x0100:
            return tud_control_xfer(
                rhport,
                request,
                const_cast<uint8_t *>(input_caps),
                sizeof(input_caps)
            );
        default:
            return false;
    }
}

inline bool xinput_handle_serial_control(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    if (stage != CONTROL_STAGE_SETUP ||
        !xinput_capabilities_detail::is_xinput_vendor_get_report(request) ||
        request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_DEVICE ||
        request->wValue != 0x0000 ||
        request->wIndex != 0x0000 ||
        request->wLength < 4) {
        return false;
    }

    static uint8_t serial[4];
    const uint32_t value = xinput_capabilities_detail::xinput_serial_for_interface(0);
    serial[0] = (uint8_t)(value >> 24);
    serial[1] = (uint8_t)(value >> 16);
    serial[2] = (uint8_t)(value >> 8);
    serial[3] = (uint8_t)value;
    return tud_control_xfer(rhport, request, serial, sizeof(serial));
}

#endif  // OUTPUT_XINPUT_CAPABILITIES_RUNTIME_H_
