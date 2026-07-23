#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include "output_xinput_driver_runtime.h"

#include "output_xinput_auth_runtime.h"
#include "output_xinput_auth_runtime_state.h"
#include "output_xinput_capabilities_runtime.h"
#include "output_xinput_descriptor_runtime.h"
#include "output_xinput_descriptor_state.h"

uint8_t xinput_out_rhport = 0;

enum {
    VENDOR_REQUEST_MICROSOFT = 1,
};

extern Adafruit_USBD_XInput *xinput_device;

namespace {

constexpr uint8_t kXinputDescriptorType = 0x21;
constexpr uint8_t kXinputSecurityDescriptorType = 0x41;

bool xinput_is_xsm3_request(uint8_t request) {
    switch (request) {
        case XSM3_REQUEST_INIT:
        case XSM3_REQUEST_CHALLENGE:
        case XSM3_REQUEST_RESPONSE:
        case 0x84:
        case 0x85:
        case XSM3_REQUEST_STATUS:
        case XSM3_REQUEST_VERIFY:
            return true;
        default:
            return false;
    }
}

bool get_xinput_interface_descriptor(uint8_t desc_type, uint8_t itf, const uint8_t** descriptor, uint16_t* length) {
    if (!descriptor || !length) {
        return false;
    }

    if (desc_type == kXinputDescriptorType) {
        switch (itf) {
            case 0:
                *descriptor = &xinput_config_descriptor_mutable[18];
                *length = 17;
                return true;
            case 1:
                *descriptor = &xinput_config_descriptor_mutable[58];
                *length = 27;
                return true;
            case 2:
                *descriptor = &xinput_config_descriptor_mutable[122];
                *length = 9;
                return true;
            default:
                return false;
        }
    }

    if (desc_type == kXinputSecurityDescriptorType && itf == 3) {
        *descriptor = &xinput_config_descriptor_mutable[147];
        *length = 6;
        return true;
    }

    return false;
}

}  // namespace

bool _xinput_tud_vendor_control_xfer_cb(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    auto& debug = xinputAuthRuntimeState();
    debug.debug_control_count++;
    debug.debug_last_control_request = request->bRequest;
    debug.debug_last_control_type = request->bmRequestType_bit.type;
    debug.debug_last_control_recipient = request->bmRequestType_bit.recipient;
    debug.debug_last_control_itf = request->wIndex & 0xFF;
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR) {
        debug.debug_vendor_control_count++;
        debug.debug_last_vendor_request = request->bRequest;
    }

    if (!xinput_device) {
        return false;
    }

    if (xinput_handle_capability_control(rhport, stage, request) ||
        xinput_handle_serial_control(rhport, stage, request)) {
        return true;
    }

    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
        xinput_is_xsm3_request(request->bRequest)) {
        return xinput_auth_handle_control(rhport, stage, request);
    }

    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    switch (request->bmRequestType_bit.type) {
        case TUSB_REQ_TYPE_VENDOR:
            switch (request->bRequest) {
                case VENDOR_REQUEST_MICROSOFT:
                    if (request->wIndex == 7) {
                        return xinput_handle_ms_os_descriptor_request(rhport, request);
                    }
                    return false;

                default:
                    break;
            }
            break;

        default:
            return false;
    }

    return true;
}

static void xinput_init(void) {}

static void xinput_reset(uint8_t rhport) {
    (void)rhport;
    xinput_auth_reset_usb_runtime();
}

uint16_t xinput_open(
    uint8_t rhport,
    const tusb_desc_interface_t *itf_descriptor,
    uint16_t max_length
) {
    if (itf_descriptor->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC) {
        return 0;
    }

    auto& debug = xinputAuthRuntimeState();

    if (itf_descriptor->bInterfaceSubClass == XINPUT_SECURITY_SUBCLASS &&
        itf_descriptor->bInterfaceProtocol == XINPUT_SECURITY_PROTOCOL) {
        uint16_t driver_length = sizeof(tusb_desc_interface_t) + 6;
        TU_VERIFY(max_length >= driver_length, 0);
        xinput_note_interface_opened();
        return driver_length;
    }

    if (itf_descriptor->bInterfaceSubClass != XINPUT_SUBCLASS_DEFAULT) {
        return 0;
    }

    xinput_note_interface_opened();

    uint16_t driver_length = 0;
    const uint8_t *current_descriptor = (const uint8_t *)itf_descriptor;

    switch (itf_descriptor->bInterfaceProtocol) {
        case 0x01:
        {
            xinput_out_rhport = rhport;
            driver_length = 9 + 17 + 14;
            TU_VERIFY(max_length >= driver_length, 0);

            current_descriptor = tu_desc_next(itf_descriptor);
            current_descriptor = tu_desc_next(current_descriptor);

            for (uint8_t i = 0; i < 2; i++) {
                const tusb_desc_endpoint_t *ep = (const tusb_desc_endpoint_t *)current_descriptor;
                if (TUSB_DESC_ENDPOINT == tu_desc_type(ep)) {
                    debug.debug_last_opened_ep = ep->bEndpointAddress;
                    if (usbd_edpt_open(rhport, ep)) {
                        debug.debug_ep_open_ok_count++;
                    } else {
                        debug.debug_ep_open_fail_count++;
                        return 0;
                    }
                    if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN) {
                        xinput_device->_endpoint_in = ep->bEndpointAddress;
                    } else {
                        xinput_device->_endpoint_out = ep->bEndpointAddress;
                    }
                }
                current_descriptor = tu_desc_next(current_descriptor);
            }
            break;
        }

        case 0x03:
        {
            driver_length = 9 + 27 + 28;
            TU_VERIFY(max_length >= driver_length, 0);

            current_descriptor = tu_desc_next(itf_descriptor);
            current_descriptor = tu_desc_next(current_descriptor);

            for (uint8_t i = 0; i < 4; i++) {
                const tusb_desc_endpoint_t *ep = (const tusb_desc_endpoint_t *)current_descriptor;
                if (TUSB_DESC_ENDPOINT == tu_desc_type(ep)) {
                    debug.debug_last_opened_ep = ep->bEndpointAddress;
                    if (usbd_edpt_open(rhport, ep)) {
                        debug.debug_ep_open_ok_count++;
                    } else {
                        debug.debug_ep_open_fail_count++;
                        return 0;
                    }
                }
                current_descriptor = tu_desc_next(current_descriptor);
            }
            break;
        }

        case 0x02:
        {
            driver_length = 9 + 9 + 7;
            TU_VERIFY(max_length >= driver_length, 0);

            current_descriptor = tu_desc_next(itf_descriptor);
            current_descriptor = tu_desc_next(current_descriptor);

            const tusb_desc_endpoint_t *ep = (const tusb_desc_endpoint_t *)current_descriptor;
            if (TUSB_DESC_ENDPOINT == tu_desc_type(ep)) {
                debug.debug_last_opened_ep = ep->bEndpointAddress;
                if (usbd_edpt_open(rhport, ep)) {
                    debug.debug_ep_open_ok_count++;
                } else {
                    debug.debug_ep_open_fail_count++;
                    return 0;
                }
            }
            break;
        }

        default:
            return 0;
    }

    return driver_length;
}

bool xinput_control_xfer_callback(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    auto& debug = xinputAuthRuntimeState();
    debug.debug_control_count++;
    debug.debug_last_control_request = request->bRequest;
    debug.debug_last_control_type = request->bmRequestType_bit.type;
    debug.debug_last_control_recipient = request->bmRequestType_bit.recipient;
    debug.debug_last_control_itf = request->wIndex & 0xFF;

    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    if (xinput_handle_capability_control(rhport, stage, request)) {
        return true;
    }

    if (request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_INTERFACE) {
        return false;
    }

    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD) {
        debug.debug_standard_control_count++;

        switch (request->bRequest) {
            case TUSB_REQ_GET_INTERFACE: {
                uint8_t alternate = 0;
                return tud_control_xfer(rhport, request, &alternate, sizeof(alternate));
            }

            case TUSB_REQ_SET_INTERFACE:
                return tud_control_status(rhport, request);

            case TUSB_REQ_GET_DESCRIPTOR: {
                const uint8_t itf = request->wIndex & 0xFF;
                const uint8_t desc_type = (request->wValue >> 8) & 0xFF;
                const uint8_t* descriptor = nullptr;
                uint16_t length = 0;

                debug.debug_last_control_desc_type = desc_type;

                if (desc_type == kXinputDescriptorType) {
                    debug.debug_desc_21_count++;
                } else if (desc_type == kXinputSecurityDescriptorType) {
                    debug.debug_desc_41_count++;
                }

                if (!descriptor_initialized) {
                    xinput_set_subtype(XINPUT_SUBTYPE_GAMEPAD);
                }

                if (get_xinput_interface_descriptor(desc_type, itf, &descriptor, &length)) {
                    return tud_control_xfer(rhport, request, (void*)descriptor, length);
                }
                return false;
            }

            default:
                return false;
        }
    }

    return false;
}

extern "C" {

static const usbd_class_driver_t xinput_driver = {
#if CFG_TUSB_DEBUG >= 2
    .name = "XINPUT",
#endif
    .init = xinput_init,
    .reset = xinput_reset,
    .open = xinput_open,
    .control_xfer_cb = xinput_control_xfer_callback,
    .xfer_cb = xinput_xfer_callback,
    .sof = NULL
};

const usbd_class_driver_t *xinput_get_driver()
{
    return &xinput_driver;
}

} // extern "C"

#endif  // ADAPT_OUTPUT_USB_DEVICE
