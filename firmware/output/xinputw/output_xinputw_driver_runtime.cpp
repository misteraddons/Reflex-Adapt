#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <cstdint>
#include <cstring>

#include <device/usbd_pvt.h>
#include <tusb_option.h>

#include "out_xinputw.h"
#include "../xinput/output_xinput_rumble_parser.h"
#include "output_xinputw_runtime.h"

uint8_t xinputw_out_rhport = 0;

bool _xinputw_tud_vendor_control_xfer_cb(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    if (!_xinput_dev) {
        return false;
    }

    // Handle vendor request 1 (serial number query)
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
        request->bRequest == 1 &&
        request->wValue == 1 &&
        request->wIndex == 0) {

        if (stage == CONTROL_STAGE_SETUP) {
            uint8_t serial_data[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x89, 0xB7};
            return tud_control_xfer(rhport, request, serial_data, sizeof(serial_data));
        }
    }

    return false;
}

static void xinputw_init(void) { }

static void xinputw_reset(uint8_t rhport) {
    (void)rhport;
}

uint16_t xinputw_open(
    uint8_t rhport,
    const tusb_desc_interface_t *itf_descriptor,
    uint16_t max_length
) {
    if (itf_descriptor->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC ||
        itf_descriptor->bInterfaceSubClass != XINPUTW_SUBCLASS_DEFAULT ||
        itf_descriptor->bInterfaceProtocol != XINPUTW_PROTOCOL_DEFAULT) {
        return false;
    }

    xinputw_out_rhport = rhport;

    uint8_t slot = itf_descriptor->bInterfaceNumber;
    if (_xinput_dev) {
        if (_xinput_dev->_itf_base != 0xFF) {
            TU_VERIFY(slot >= _xinput_dev->_itf_base, 0);
            slot -= _xinput_dev->_itf_base;
        }
        TU_VERIFY(slot < XINPUT_WIRELESS_CONTROLLERS, 0);
    }

    uint16_t driver_length = sizeof(tusb_desc_interface_t) +
                             (itf_descriptor->bNumEndpoints * sizeof(tusb_desc_endpoint_t)) + 17;

    TU_VERIFY(max_length >= driver_length, 0);

    const uint8_t *current_descriptor = tu_desc_next(itf_descriptor);
    uint8_t found_endpoints = 0;
    while ((found_endpoints < itf_descriptor->bNumEndpoints) && (driver_length <= max_length)) {
        const tusb_desc_endpoint_t *endpoint_descriptor =
            (const tusb_desc_endpoint_t *)current_descriptor;
        if (TUSB_DESC_ENDPOINT == tu_desc_type(endpoint_descriptor)) {
            TU_ASSERT(usbd_edpt_open(rhport, endpoint_descriptor));
            if (tu_edpt_dir(endpoint_descriptor->bEndpointAddress) == TUSB_DIR_IN)
                _xinput_dev->interfaces[slot]._endpoint_in = endpoint_descriptor->bEndpointAddress;
            else
                _xinput_dev->interfaces[slot]._endpoint_out = endpoint_descriptor->bEndpointAddress;
            xinputw_diag.endpoint_in[slot] = _xinput_dev->interfaces[slot]._endpoint_in;
            xinputw_diag.endpoint_out[slot] = _xinput_dev->interfaces[slot]._endpoint_out;

            ++found_endpoints;
        }

        current_descriptor = tu_desc_next(current_descriptor);
    }
    return driver_length;
}

bool xinputw_control_xfer_callback(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    (void)rhport;
    (void)stage;
    (void)request;

    return true;
}

bool xinputw_xfer_callback(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
) {
    (void)result;

    for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS; ++i) {
        xinputw_itf_t *xitf = &_xinput_dev->interfaces[i];

        if (ep_addr == xitf->_endpoint_out) {
            xinputw_diag.out_xfer_count[i]++;
            const uint8_t out_size =
                (xferred_bytes > XINPUTW_EPSIZE) ? XINPUTW_EPSIZE : (uint8_t)xferred_bytes;
            const size_t parse_size = xinputRumbleParseSize(out_size, XINPUTW_EPSIZE);
            xinputw_diag.last_out_size[i] = (parse_size > UINT8_MAX) ? UINT8_MAX : (uint8_t)parse_size;
            memset(xinputw_diag.last_out_packet[i], 0, sizeof(xinputw_diag.last_out_packet[i]));
            memcpy(xinputw_diag.last_out_packet[i],
                   xitf->_xinput_out_buffer,
                   parse_size > sizeof(xinputw_diag.last_out_packet[i])
                     ? sizeof(xinputw_diag.last_out_packet[i])
                     : parse_size);

            if (xitf->_xinput_out_buffer[0] == 0x08 && xitf->_xinput_out_buffer[1] == 0x00 &&
                xitf->_xinput_out_buffer[2] == 0x0F && xitf->_xinput_out_buffer[3] == 0xC0) {
                _xinput_dev->handle_connection_status(i);
            } else if (xitf->_xinput_out_buffer[0] == 0x00 && xitf->_xinput_out_buffer[1] == 0x00 &&
                       xitf->_xinput_out_buffer[2] == 0x00 && xitf->_xinput_out_buffer[3] == 0x40) {

                switch (xitf->info_state) {
                    case DISCONNECTED:
                    case NONE:
                        break;
                    case UNKNOWN1: {
                        xitf->info_state = UNKNOWN2;
                        uint8_t controller_info[29] = {
                            0x00, 0x0F, 0x00, 0xF0,
                            0xF0,
                            0xCC,
                            0xFF, 0xFF, 0xFF, 0xFF,
                            0x58, 0x91, 0xb3, 0xf0, 0x00, 0x09,
                            0x13,
                            0xA3,
                            0x20, 0x1D, 0x30, 0x03, 0x40, 0x01, 0x50,
                            0x01,
                            0xFF, 0xFF, 0xFF
                        };
                        _xinput_dev->sendRawReport(i, controller_info, sizeof(controller_info));
                        break;
                    }
                    case UNKNOWN2:
                        xitf->info_state = NONE;
                        break;
                }
            } else if (xitf->_xinput_out_buffer[0] == 0x00 && xitf->_xinput_out_buffer[1] == 0x00 &&
                       xitf->_xinput_out_buffer[2] == 0x08 && (xitf->_xinput_out_buffer[3] & 0x40) == 0x40) {
                //handle_led(_xinput_dev->_xinput_out_buffer[3] & 0x0F);
            } else {
                XInputRumblePacket rumble{};
                if (parseXinputRumblePacket(xitf->_xinput_out_buffer, parse_size, &rumble)) {
                    xinputw_diag.last_rumble_left[i] = rumble.left;
                    xinputw_diag.last_rumble_right[i] = rumble.right;
                    rumble_callback(i, rumble.left, rumble.right);
                } else {
                    // Unknown message
                }
            }

            usbd_edpt_xfer(
                rhport,
                xitf->_endpoint_out,
                xitf->_xinput_out_buffer,
                XINPUTW_EPSIZE
            );
            break;
        }
    }

    return true;
}

void xinputw_get_diag_info(XInputWDiagInfo *info) {
    if (!info) {
        return;
    }
    *info = xinputw_diag;
    info->controller_count = XINPUT_WIRELESS_CONTROLLERS;
    if (_xinput_dev) {
        for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS; ++i) {
            info->endpoint_in[i] = _xinput_dev->interfaces[i]._endpoint_in;
            info->endpoint_out[i] = _xinput_dev->interfaces[i]._endpoint_out;
        }
    }
}

extern "C" {

static const usbd_class_driver_t xinputw_driver = {
#if CFG_TUSB_DEBUG >= 2
    .name = "XINPUTW",
#endif
    .init = xinputw_init,
    .reset = xinputw_reset,
    .open = xinputw_open,
    .control_xfer_cb = xinputw_control_xfer_callback,
    .xfer_cb = xinputw_xfer_callback,
    .sof = NULL
};

const usbd_class_driver_t *xinputw_get_driver()
{
    return &xinputw_driver;
}

} // extern "C"

#endif  // ADAPT_OUTPUT_USB_DEVICE
