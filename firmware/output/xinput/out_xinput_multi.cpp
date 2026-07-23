#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <cstring>

#include "out_xinput_multi.h"
#include "output_xinput_auth_runtime.h"
#include "output_xinput_capabilities_runtime.h"
#include "output_xinput_descriptor_runtime.h"
#include "output_xinput_rumble_parser.h"

Adafruit_USBD_XInputMulti *xinput_multi_device = nullptr;
uint8_t xinput_multi_rhport = 0;
bool xinput_multi_xsm3_auth_observed = false;

namespace {

constexpr uint8_t kXinputDescriptorType = 0x21;
constexpr uint8_t kXinputSecurityDescriptorType = 0x41;
constexpr uint8_t kXinputMultiInvalidItf = 0xFF;

constexpr uint16_t kControlInterfaceOffset = 0;
constexpr uint16_t kControlDescriptorOffset = 9;
constexpr uint16_t kControlBcdXusbOffset = 2;
constexpr uint16_t kControlEndpointInDescriptorOffset = 26;
constexpr uint16_t kControlEndpointOutDescriptorOffset = 33;

constexpr uint16_t kAudioInterfaceOffset = 40;
constexpr uint16_t kAudioDescriptorOffset = 49;
constexpr uint16_t kAudioEndpointIn1DescriptorOffset = 76;
constexpr uint16_t kAudioEndpointOut1DescriptorOffset = 83;
constexpr uint16_t kAudioEndpointIn2DescriptorOffset = 90;
constexpr uint16_t kAudioEndpointOut2DescriptorOffset = 97;

constexpr uint16_t kPluginInterfaceOffset = 104;
constexpr uint16_t kPluginDescriptorOffset = 113;
constexpr uint16_t kPluginEndpointDescriptorOffset = 122;

constexpr uint16_t kSecurityInterfaceOffset = 129;
constexpr uint16_t kSecurityInterfaceNumberOffset = 2;
constexpr uint16_t kSecurityInterfaceStringOffset = 8;
constexpr uint16_t kSecurityDescriptorOffset = 138;
constexpr uint16_t kXinputMultiBcdXusb11 = 0x0110;

XInputMultiDiagInfo xinput_multi_diag = {};

struct XInputMultiEndpointMap {
    uint8_t control_in;
    uint8_t control_out;
    uint8_t audio_in_1;
    uint8_t audio_out_1;
    uint8_t audio_in_2;
    uint8_t audio_out_2;
    uint8_t plugin_in;
};

constexpr XInputMultiEndpointMap kXinputMultiEndpointMap[XINPUT_MULTI_CONTROLLERS] = {
    // Slot 0 mirrors the known-working single Xbox 360 output descriptor.
    {0x81, 0x02, 0x83, 0x04, 0x85, 0x06, 0x86},
    // Slot 1 uses the remaining endpoint numbers so the USB device stays valid.
    {0x82, 0x01, 0x84, 0x03, 0x87, 0x05, 0x88},
};

// Keep each XUSB child shaped like a complete wired Xbox 360 controller function
// in the production Windows path. That costs one security interface per slot but
// better matches app-side haptics paths than the compact shared-security layout.

uint8_t clamp_controller_count(uint8_t controller_count) {
    if (controller_count == 0) {
        return 1;
    }
    if (controller_count > XINPUT_MULTI_CONTROLLERS) {
        return XINPUT_MULTI_CONTROLLERS;
    }
    return controller_count;
}

void patch_interface_number(uint8_t *descriptor, uint16_t interface_offset, uint8_t itfnum) {
    descriptor[interface_offset + 2] = itfnum;
}

void patch_endpoint_descriptor(uint8_t *descriptor, uint16_t endpoint_offset, uint8_t ep_addr) {
    descriptor[endpoint_offset + 2] = ep_addr;
}

void patch_le16(uint8_t *descriptor, uint16_t offset, uint16_t value) {
    descriptor[offset] = (uint8_t)(value & 0xFF);
    descriptor[offset + 1] = (uint8_t)(value >> 8);
}

void patch_xinput_descriptor_endpoint(uint8_t *descriptor, uint16_t descriptor_offset, uint8_t offset, uint8_t ep_addr) {
    descriptor[descriptor_offset + offset] = ep_addr;
}

void patch_xinput_1_10_capability_descriptor(uint8_t *descriptor, uint16_t descriptor_offset) {
    // XUSB 1.10 uses these report capability markers before Windows queries
    // the vendor capability packets that carry the force-feedback flag.
    descriptor[descriptor_offset + 8] = 0x03;
    descriptor[descriptor_offset + 9] = 0x03;
    descriptor[descriptor_offset + 10] = 0x03;
    descriptor[descriptor_offset + 11] = 0x04;
    descriptor[descriptor_offset + 15] = 0x03;
    descriptor[descriptor_offset + 16] = 0x03;
}

bool xinput_multi_interface_to_slot_part(uint8_t itf, uint8_t *slot, uint8_t *part) {
    return xinput_multi_device && xinput_multi_device->interfaceToSlotPart(itf, slot, part);
}

bool xinput_multi_is_security_interface(uint8_t itf) {
    if (!xinput_multi_device) {
        return false;
    }
#if XINPUT_MULTI_SHARED_SECURITY_INTERFACE
    return xinput_multi_device->isSecurityInterface(itf);
#else
    uint8_t slot = 0;
    uint8_t part = 0;
    return xinput_multi_device->interfaceToSlotPart(itf, &slot, &part) &&
           part == (XINPUT_MULTI_INTERFACES_PER_CONTROLLER - 1);
#endif
}

bool xinput_multi_interface_to_slot(uint8_t itf, uint8_t *slot) {
    uint8_t part = 0;
    return xinput_multi_interface_to_slot_part(itf, slot, &part);
}

bool xinput_multi_get_interface_descriptor(uint8_t desc_type, uint8_t itf, uint8_t slot, uint8_t part, const uint8_t **descriptor, uint16_t *length) {
    if (!xinput_multi_device || !descriptor || !length) {
        return false;
    }

    if (desc_type == kXinputDescriptorType) {
        if (slot >= xinput_multi_device->controllerCount()) {
            return false;
        }
        const uint8_t *group = xinput_multi_device->descriptorForSlot(slot);
        if (!group) {
            return false;
        }
        xinput_multi_diag.desc_21_count[slot]++;
        xinput_multi_diag.last_part[slot] = part;
        switch (part) {
            case 0:
                *descriptor = &group[kControlDescriptorOffset];
                *length = 17;
                return true;
            case 1:
                *descriptor = &group[kAudioDescriptorOffset];
                *length = 27;
                return true;
            case 2:
                *descriptor = &group[kPluginDescriptorOffset];
                *length = 9;
                return true;
            default:
                return false;
        }
    }

    if (desc_type == kXinputSecurityDescriptorType &&
        xinput_multi_is_security_interface(itf)) {
#if XINPUT_MULTI_SHARED_SECURITY_INTERFACE
        xinput_multi_diag.desc_41_count[0]++;
        xinput_multi_diag.last_part[0] = XINPUT_MULTI_INTERFACES_PER_CONTROLLER;
        *descriptor = xinput_multi_device->securityDescriptor() + (kSecurityDescriptorOffset - kSecurityInterfaceOffset);
#else
        if (slot >= xinput_multi_device->controllerCount()) {
            return false;
        }
        const uint8_t *group = xinput_multi_device->descriptorForSlot(slot);
        if (!group) {
            return false;
        }
        xinput_multi_diag.desc_41_count[slot]++;
        xinput_multi_diag.last_part[slot] = part;
        *descriptor = &group[kSecurityDescriptorOffset];
#endif
        *length = 6;
        return true;
    }

    return false;
}

bool xinput_multi_is_xsm3_request(uint8_t request) {
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

bool open_xinput_multi_endpoints(
    uint8_t rhport,
    const tusb_desc_interface_t *itf_descriptor,
    uint8_t endpoint_count,
    bool store_gamepad_endpoints,
    uint8_t *gamepad_in,
    uint8_t *gamepad_out,
    uint8_t *alt_out,
    uint8_t alt_out_capacity
) {
    const uint8_t *current_descriptor = tu_desc_next(itf_descriptor);
    current_descriptor = tu_desc_next(current_descriptor);
    uint8_t alt_out_count = 0;

    for (uint8_t i = 0; i < endpoint_count; ++i) {
        const tusb_desc_endpoint_t *ep = reinterpret_cast<const tusb_desc_endpoint_t *>(current_descriptor);
        if (TUSB_DESC_ENDPOINT == tu_desc_type(ep)) {
            TU_ASSERT(usbd_edpt_open(rhport, ep));
            if (store_gamepad_endpoints) {
                if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN) {
                    if (gamepad_in) {
                        *gamepad_in = ep->bEndpointAddress;
                    }
                } else {
                    if (gamepad_out) {
                        *gamepad_out = ep->bEndpointAddress;
                    }
                }
            } else if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_OUT &&
                       alt_out &&
                       alt_out_count < alt_out_capacity) {
                alt_out[alt_out_count++] = ep->bEndpointAddress;
            }
        }
        current_descriptor = tu_desc_next(current_descriptor);
    }

    return true;
}

bool arm_xinput_multi_out_endpoint(
    uint8_t slot,
    uint8_t ep_addr,
    uint8_t *buffer
) {
    if (!xinput_multi_device ||
        slot >= xinput_multi_device->controllerCount() ||
        !ep_addr ||
        !buffer ||
        !tud_ready()) {
        if (slot < XINPUT_MULTI_CONTROLLERS) {
            xinput_multi_diag.out_arm_fail_count[slot]++;
        }
        return false;
    }

    if (slot < XINPUT_MULTI_CONTROLLERS) {
        xinput_multi_diag.out_arm_attempt_count[slot]++;
    }

    if (usbd_edpt_busy(xinput_multi_rhport, ep_addr)) {
        if (slot < XINPUT_MULTI_CONTROLLERS) {
            xinput_multi_diag.out_arm_busy_count[slot]++;
        }
        return false;
    }

    if (!usbd_edpt_claim(xinput_multi_rhport, ep_addr)) {
        if (slot < XINPUT_MULTI_CONTROLLERS) {
            xinput_multi_diag.out_arm_fail_count[slot]++;
        }
        return false;
    }

    const bool armed = usbd_edpt_xfer(xinput_multi_rhport, ep_addr, buffer, EPSIZE);
    if (slot < XINPUT_MULTI_CONTROLLERS) {
        if (armed) {
            xinput_multi_diag.out_arm_ok_count[slot]++;
        } else {
            xinput_multi_diag.out_arm_fail_count[slot]++;
        }
    }
    usbd_edpt_release(xinput_multi_rhport, ep_addr);
    return armed;
}

uint8_t *xinput_multi_out_buffer_for_endpoint(
    xinput_multi_itf_t *xitf,
    uint8_t ep_addr,
    uint8_t *alt_index
) {
    if (!xitf) {
        return nullptr;
    }
    if (alt_index) {
        *alt_index = 0xFF;
    }
    if (ep_addr == xitf->endpoint_out) {
        return xitf->out_buffer;
    }
    for (uint8_t i = 0; i < XINPUT_MULTI_ALT_OUT_ENDPOINTS; ++i) {
        if (ep_addr == xitf->endpoint_out_alt[i]) {
            if (alt_index) {
                *alt_index = i;
            }
            return xitf->out_alt_buffer[i];
        }
    }
    return nullptr;
}

void note_xinput_multi_vendor_get_report(const tusb_control_request_t *request) {
    if (!request ||
        request->bmRequestType_bit.direction != TUSB_DIR_IN ||
        request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR ||
        request->bRequest != 0x01) {
        return;
    }

    if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
        request->wValue == 0x0000 &&
        request->wIndex == 0x0000) {
        xinput_multi_diag.serial_count++;
        return;
    }

    if (request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_INTERFACE ||
        (request->wValue != 0x0000 && request->wValue != 0x0100)) {
        return;
    }

    uint8_t slot = 0;
    uint8_t part = 0;
    if (xinput_multi_interface_to_slot_part(request->wIndex & 0xFF, &slot, &part) &&
        slot < XINPUT_MULTI_CONTROLLERS &&
        part == 0) {
        xinput_multi_diag.capability_count[slot]++;
    }
}

void note_xinput_multi_control_setup(
    uint8_t stage,
    const tusb_control_request_t *request
) {
    if (stage != CONTROL_STAGE_SETUP || !request) {
        return;
    }

    xinput_multi_diag.control_setup_count++;
    xinput_multi_diag.last_control_request = request->bRequest;
    xinput_multi_diag.last_control_type = request->bmRequestType_bit.type;
    xinput_multi_diag.last_control_recipient = request->bmRequestType_bit.recipient;
    xinput_multi_diag.last_control_direction = request->bmRequestType_bit.direction;
    xinput_multi_diag.last_control_wIndex = request->wIndex & 0xFF;
    xinput_multi_diag.last_control_wValue = request->wValue & 0xFF;
    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        xinput_multi_diag.control_out_count++;
    }
}

}  // namespace

bool xinput_multi_handle_xfer_complete(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes,
    uint8_t driver_id,
    bool from_override
) {
    if (!xinput_multi_device) {
        return false;
    }

    xinput_multi_diag.xfer_callback_count++;
    if (from_override) {
        xinput_multi_diag.xfer_override_count++;
    }
    xinput_multi_diag.last_xfer_endpoint = ep_addr;
    xinput_multi_diag.last_xfer_driver = driver_id;
    xinput_multi_diag.last_xfer_result = (uint8_t)result;
    xinput_multi_diag.last_xfer_size = (xferred_bytes > UINT8_MAX)
        ? UINT8_MAX
        : (uint8_t)xferred_bytes;

    for (uint8_t i = 0; i < xinput_multi_device->controllerCount(); ++i) {
        xinput_multi_itf_t *xitf = &xinput_multi_device->interfaces[i];
        if (ep_addr == xitf->endpoint_in) {
            xinput_seen_in_xfer = true;
            xinput_multi_diag.in_xfer_count[i]++;
            return true;
        }

        uint8_t *out_buffer = xinput_multi_out_buffer_for_endpoint(xitf, ep_addr, nullptr);
        if (out_buffer) {
            xinput_multi_diag.out_xfer_count[i]++;
            xinput_multi_diag.last_out_endpoint[i] = ep_addr;
            const uint8_t out_size = (xferred_bytes > EPSIZE) ? EPSIZE : (uint8_t)xferred_bytes;
            const size_t parse_size = xinputRumbleParseSize(out_size, EPSIZE);
            xinput_multi_diag.last_out_size[i] = (parse_size > UINT8_MAX) ? UINT8_MAX : (uint8_t)parse_size;
            memset(xinput_multi_diag.last_out_packet[i], 0, sizeof(xinput_multi_diag.last_out_packet[i]));
            memcpy(xinput_multi_diag.last_out_packet[i], out_buffer,
                   parse_size > sizeof(xinput_multi_diag.last_out_packet[i])
                     ? sizeof(xinput_multi_diag.last_out_packet[i])
                     : parse_size);
            XInputRumblePacket rumble{};
            if (parseXinputRumblePacket(out_buffer, parse_size, &rumble)) {
                xinput_multi_diag.last_rumble_left[i] = rumble.left;
                xinput_multi_diag.last_rumble_right[i] = rumble.right;
                xinput_multi_diag.rumble_parse_count[i]++;
                if ((rumble.left | rumble.right) != 0) {
                    xinput_multi_diag.rumble_nonzero_count[i]++;
                    xinput_multi_diag.last_nonzero_rumble_left[i] = rumble.left;
                    xinput_multi_diag.last_nonzero_rumble_right[i] = rumble.right;
                }
                rumble_callback(xinput_multi_source_port_for_target_slot(i),
                                rumble.left,
                                rumble.right);
            }
            arm_xinput_multi_out_endpoint(i, ep_addr, out_buffer);
            return true;
        }
    }

    xinput_multi_diag.unmatched_xfer_count++;
    return false;
}

Adafruit_USBD_XInputMulti::Adafruit_USBD_XInputMulti(uint8_t interval_ms, uint8_t controller_count) {
    _interval_ms = interval_ms;
    _controller_count = clamp_controller_count(controller_count);
}

bool Adafruit_USBD_XInputMulti::begin(void) {
    _itf_base = kXinputMultiInvalidItf;
    memset(&xinput_multi_diag, 0, sizeof(xinput_multi_diag));
    for (uint8_t i = 0; i < XINPUT_MULTI_CONTROLLERS; ++i) {
        interfaces[i].endpoint_in = 0;
        interfaces[i].endpoint_out = 0;
        interfaces[i].interface_base = kXinputMultiInvalidItf;
        memset(interfaces[i].out_buffer, 0, sizeof(interfaces[i].out_buffer));
        memset(interfaces[i].endpoint_out_alt, 0, sizeof(interfaces[i].endpoint_out_alt));
        memset(interfaces[i].out_alt_buffer, 0, sizeof(interfaces[i].out_alt_buffer));
        memset(interfaces[i].descriptor, 0, sizeof(interfaces[i].descriptor));
    }

    xinput_multi_device = this;
    if (!TinyUSBDevice.addInterface(*this)) {
        xinput_note_begin_result(false);
        return false;
    }

    TinyUSBDevice.setMaxPacketSize0(8);
    xinput_auth_init();
    xinput_note_begin_result(true);
    return true;
}

uint8_t Adafruit_USBD_XInputMulti::controllerCount() const {
    return _controller_count;
}

bool Adafruit_USBD_XInputMulti::interfaceToSlot(uint8_t itf, uint8_t *slot) const {
    uint8_t part = 0;
    return interfaceToSlotPart(itf, slot, &part);
}

bool Adafruit_USBD_XInputMulti::interfaceToSlotPart(uint8_t itf, uint8_t *slot, uint8_t *part) const {
    if (!slot || _itf_base == kXinputMultiInvalidItf || itf < _itf_base) {
        return false;
    }

    const uint8_t relative = itf - _itf_base;
    const uint8_t candidate = relative / XINPUT_MULTI_INTERFACES_PER_CONTROLLER;
    if (candidate >= _controller_count) {
        return false;
    }
    *slot = candidate;
    if (part) {
        *part = relative % XINPUT_MULTI_INTERFACES_PER_CONTROLLER;
    }
    return true;
}

bool Adafruit_USBD_XInputMulti::isSecurityInterface(uint8_t itf) const {
    return itf == _security_itfnum;
}

const uint8_t *Adafruit_USBD_XInputMulti::descriptorForSlot(uint8_t slot) const {
    if (slot >= _controller_count) {
        return nullptr;
    }
    return interfaces[slot].descriptor;
}

const uint8_t *Adafruit_USBD_XInputMulti::securityDescriptor() const {
    return security_descriptor;
}

bool Adafruit_USBD_XInputMulti::ready(uint8_t itf) {
    return tud_xinput_multi_ready(itf);
}

bool Adafruit_USBD_XInputMulti::sendReport(uint8_t itf, xinput_report_t *report) {
    return send_xinput_multi_report(itf, report);
}

uint16_t Adafruit_USBD_XInputMulti::getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize) {
    (void)itfnum;
    const uint16_t len =
        (XINPUT_MULTI_CONTROLLER_DESC_LEN * _controller_count) +
        XINPUT_MULTI_SECURITY_DESC_LEN;
    if (!buf) {
        return len;
    }
    if (bufsize < len) {
        return 0;
    }

    _itf_base = TinyUSBDevice.allocInterface(
        (_controller_count * XINPUT_MULTI_INTERFACES_PER_CONTROLLER)
#if XINPUT_MULTI_SHARED_SECURITY_INTERFACE
        + 1
#endif
    );
#if XINPUT_MULTI_SHARED_SECURITY_INTERFACE
    _security_itfnum = _itf_base + (_controller_count * XINPUT_MULTI_INTERFACES_PER_CONTROLLER);
#else
    _security_itfnum = kXinputMultiInvalidItf;
#endif
    for (uint8_t i = 0; i < _controller_count; ++i) {
        xinput_multi_itf_t *xitf = &interfaces[i];
        xitf->interface_base = _itf_base + (i * XINPUT_MULTI_INTERFACES_PER_CONTROLLER);
        xinput_multi_diag.interface_base[i] = xitf->interface_base;
        memcpy(xitf->descriptor, &xinput_config_descriptor[9], XINPUT_MULTI_CONTROLLER_DESC_LEN);
        // Advertise the XUSB 1.10 capability descriptor shape so hosts that
        // query report capabilities can see the force-feedback bit below.
        patch_le16(xitf->descriptor,
                   kControlDescriptorOffset + kControlBcdXusbOffset,
                   kXinputMultiBcdXusb11);
        patch_xinput_1_10_capability_descriptor(xitf->descriptor, kControlDescriptorOffset);

        const XInputMultiEndpointMap &endpoint_map = kXinputMultiEndpointMap[i];
        const uint8_t control_in = endpoint_map.control_in;
        const uint8_t control_out = endpoint_map.control_out;
        const uint8_t audio_in_1 = endpoint_map.audio_in_1;
        const uint8_t audio_out_1 = endpoint_map.audio_out_1;
        const uint8_t audio_in_2 = endpoint_map.audio_in_2;
        const uint8_t audio_out_2 = endpoint_map.audio_out_2;
        const uint8_t plugin_in = endpoint_map.plugin_in;

        xitf->endpoint_in = control_in;
        xitf->endpoint_out = control_out;
        xitf->endpoint_out_alt[0] = audio_out_1;
        xitf->endpoint_out_alt[1] = audio_out_2;

        patch_interface_number(xitf->descriptor, kControlInterfaceOffset, xitf->interface_base);
        patch_interface_number(xitf->descriptor, kAudioInterfaceOffset, xitf->interface_base + 1);
        patch_interface_number(xitf->descriptor, kPluginInterfaceOffset, xitf->interface_base + 2);
#if !XINPUT_MULTI_SHARED_SECURITY_INTERFACE
        patch_interface_number(xitf->descriptor, kSecurityInterfaceOffset, xitf->interface_base + 3);
        xitf->descriptor[kSecurityInterfaceOffset + kSecurityInterfaceStringOffset] =
            TinyUSBDevice.addStringDescriptor(
                "Xbox Security Method 3, Version 1.00, \xc2\xa9 2005 Microsoft Corporation. All rights reserved.");
#endif

        patch_xinput_descriptor_endpoint(xitf->descriptor, kControlDescriptorOffset, 6, control_in);
        patch_xinput_descriptor_endpoint(xitf->descriptor, kControlDescriptorOffset, 13, control_out);
        patch_endpoint_descriptor(xitf->descriptor, kControlEndpointInDescriptorOffset, control_in);
        patch_endpoint_descriptor(xitf->descriptor, kControlEndpointOutDescriptorOffset, control_out);
        xitf->descriptor[kControlEndpointInDescriptorOffset + 6] = _interval_ms;

        patch_xinput_descriptor_endpoint(xitf->descriptor, kAudioDescriptorOffset, 6, audio_in_1);
        patch_xinput_descriptor_endpoint(xitf->descriptor, kAudioDescriptorOffset, 9, audio_out_1);
        patch_xinput_descriptor_endpoint(xitf->descriptor, kAudioDescriptorOffset, 12, audio_in_2);
        patch_xinput_descriptor_endpoint(xitf->descriptor, kAudioDescriptorOffset, 20, audio_out_2);
        patch_endpoint_descriptor(xitf->descriptor, kAudioEndpointIn1DescriptorOffset, audio_in_1);
        patch_endpoint_descriptor(xitf->descriptor, kAudioEndpointOut1DescriptorOffset, audio_out_1);
        patch_endpoint_descriptor(xitf->descriptor, kAudioEndpointIn2DescriptorOffset, audio_in_2);
        patch_endpoint_descriptor(xitf->descriptor, kAudioEndpointOut2DescriptorOffset, audio_out_2);

        patch_xinput_descriptor_endpoint(xitf->descriptor, kPluginDescriptorOffset, 6, plugin_in);
        patch_endpoint_descriptor(xitf->descriptor, kPluginEndpointDescriptorOffset, plugin_in);

        memcpy(buf + (i * XINPUT_MULTI_CONTROLLER_DESC_LEN), xitf->descriptor, XINPUT_MULTI_CONTROLLER_DESC_LEN);
    }

#if XINPUT_MULTI_SHARED_SECURITY_INTERFACE
    memcpy(security_descriptor,
           &xinput_config_descriptor[9 + kSecurityInterfaceOffset],
           XINPUT_MULTI_SECURITY_DESC_LEN);
    patch_interface_number(security_descriptor, 0, _security_itfnum);
    security_descriptor[kSecurityInterfaceStringOffset] = TinyUSBDevice.addStringDescriptor(
        "Xbox Security Method 3, Version 1.00, \xc2\xa9 2005 Microsoft Corporation. All rights reserved.");
    memcpy(buf + (_controller_count * XINPUT_MULTI_CONTROLLER_DESC_LEN),
           security_descriptor,
           XINPUT_MULTI_SECURITY_DESC_LEN);
#endif

    xinput_set_ms_os_first_interface(_itf_base);
    return len;
}

bool tud_xinput_multi_ready(uint8_t itf) {
    return xinput_multi_device &&
           itf < xinput_multi_device->controllerCount() &&
           xinput_multi_device->interfaces[itf].endpoint_in &&
           tud_ready() &&
           !usbd_edpt_busy(xinput_multi_rhport, xinput_multi_device->interfaces[itf].endpoint_in);
}

void receive_xinput_multi_report(uint8_t itf) {
    if (!xinput_multi_device ||
        itf >= xinput_multi_device->controllerCount() ||
        !tud_ready()) {
        if (itf < XINPUT_MULTI_CONTROLLERS) {
            xinput_multi_diag.out_arm_fail_count[itf]++;
        }
        return;
    }

    xinput_multi_itf_t *xitf = &xinput_multi_device->interfaces[itf];
    arm_xinput_multi_out_endpoint(itf, xitf->endpoint_out, xitf->out_buffer);
    // Browser/WGI haptics can target secondary XUSB OUT endpoints. Keep them
    // armed; completion parsing remains strict and ignores non-rumble packets.
    for (uint8_t i = 0; i < XINPUT_MULTI_ALT_OUT_ENDPOINTS; ++i) {
        arm_xinput_multi_out_endpoint(
            itf,
            xitf->endpoint_out_alt[i],
            xitf->out_alt_buffer[i]
        );
    }
}

bool send_xinput_multi_report(uint8_t itf, xinput_report_t *report) {
    if (!tud_xinput_multi_ready(itf)) {
        return false;
    }

    usbd_edpt_claim(xinput_multi_rhport, xinput_multi_device->interfaces[itf].endpoint_in);
    const bool sent = usbd_edpt_xfer(
        xinput_multi_rhport,
        xinput_multi_device->interfaces[itf].endpoint_in,
        reinterpret_cast<uint8_t *>(report),
        sizeof(xinput_report_t)
    );
    usbd_edpt_release(xinput_multi_rhport, xinput_multi_device->interfaces[itf].endpoint_in);
    return sent;
}

bool xinput_multi_console_auth_observed() {
    return xinput_multi_xsm3_auth_observed;
}

uint8_t xinput_multi_controller_count() {
    return xinput_multi_device ? xinput_multi_device->controllerCount() : 0;
}

void xinput_multi_get_diag_info(XInputMultiDiagInfo *info) {
    if (!info) {
        return;
    }

    *info = xinput_multi_diag;
    info->controller_count = xinput_multi_controller_count();
    info->console_auth_observed = xinput_multi_xsm3_auth_observed;
    if (xinput_multi_device) {
        for (uint8_t i = 0; i < xinput_multi_device->controllerCount() && i < XINPUT_MULTI_CONTROLLERS; ++i) {
            info->interface_base[i] = xinput_multi_device->interfaces[i].interface_base;
            info->endpoint_in[i] = xinput_multi_device->interfaces[i].endpoint_in;
            info->endpoint_out[i] = xinput_multi_device->interfaces[i].endpoint_out;
            memcpy(info->endpoint_out_alt[i],
                   xinput_multi_device->interfaces[i].endpoint_out_alt,
                   sizeof(info->endpoint_out_alt[i]));
        }
    }
}

uint8_t xinput_multi_target_slot_for_source_port(uint8_t source_port) {
    if (!xinput_multi_console_auth_observed() ||
        !xinput_multi_device ||
        xinput_multi_device->controllerCount() < 2) {
        return source_port;
    }

    // Windows follows descriptor order. Retail Xbox 360 hardware, however,
    // has been observed binding the second composite XInput instance as system
    // player 1. XSM3 traffic is the console-only discriminator; Windows never
    // reaches this branch, so preserve Windows P1/P2 descriptor order there.
    if (source_port == 0) {
        return 1;
    }
    if (source_port == 1) {
        return 0;
    }
    return source_port;
}

uint8_t xinput_multi_source_port_for_target_slot(uint8_t target_slot) {
    if (!xinput_multi_console_auth_observed() ||
        !xinput_multi_device ||
        xinput_multi_device->controllerCount() < 2) {
        return target_slot;
    }
    if (target_slot == 1) {
        return 0;
    }
    if (target_slot == 0) {
        return 1;
    }
    return target_slot;
}

bool xinput_multi_auth_handle_control(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    if (stage == CONTROL_STAGE_SETUP) {
        note_xinput_multi_control_setup(stage, request);
        note_xinput_multi_vendor_get_report(request);
    }

    if (xinput_handle_capability_control(rhport, stage, request) ||
        xinput_handle_serial_control(rhport, stage, request)) {
        return true;
    }

    if (!xinput_multi_device ||
        request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR ||
        !xinput_multi_is_xsm3_request(request->bRequest)) {
        return false;
    }

    // XInput2P keeps security descriptors so Windows apps see complete XUSB
    // children, and diagnostics can observe late console-style requests. Retail
    // Xbox 360 validation must still reboot/promote to OUTPUT_XINPUT; this
    // handler is not a replacement for the dedicated Xbox 360 auth path.
    const uint8_t itf = request->wIndex & 0xFF;
    if (!xinput_multi_is_security_interface(itf)) {
        return false;
    }

    uint8_t slot = 0;
    uint8_t part = 0;
    xinput_multi_interface_to_slot_part(itf, &slot, &part);
#if XINPUT_MULTI_SHARED_SECURITY_INTERFACE
    slot = 0;
    part = XINPUT_MULTI_INTERFACES_PER_CONTROLLER;
#endif

    xinput_multi_xsm3_auth_observed = true;
    xinput_multi_diag.console_auth_observed = true;
    xinput_multi_diag.last_request[slot] = request->bRequest;
    xinput_multi_diag.last_stage[slot] = stage;
    xinput_multi_diag.last_wIndex[slot] = itf;
    xinput_multi_diag.last_part[slot] = part;
    if (stage == CONTROL_STAGE_SETUP) {
        xinput_multi_diag.auth_request_count[slot]++;
    } else if (stage == CONTROL_STAGE_DATA) {
        xinput_multi_diag.data_stage_count[slot]++;
    }

    // The donor XSM3 implementation is singleton/global. For the first pass,
    // route security requests by interface while preserving that known-good
    // state machine instead of cloning the crypto path.
    return xinput_auth_handle_control_for_security_interface(
        rhport,
        stage,
        request,
        itf
    );
}

uint16_t xinput_multi_open(
    uint8_t rhport,
    const tusb_desc_interface_t *itf_descriptor,
    uint16_t max_length
) {
    if (!xinput_multi_device ||
        itf_descriptor->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC) {
        return 0;
    }

    xinput_multi_rhport = rhport;

    if (itf_descriptor->bInterfaceSubClass == XINPUT_SECURITY_SUBCLASS &&
        itf_descriptor->bInterfaceProtocol == XINPUT_SECURITY_PROTOCOL) {
        constexpr uint16_t driver_length = 9 + 6;
        TU_VERIFY(xinput_multi_is_security_interface(itf_descriptor->bInterfaceNumber), 0);
        TU_VERIFY(max_length >= driver_length, 0);
#if !XINPUT_MULTI_SHARED_SECURITY_INTERFACE
        uint8_t slot = 0;
        uint8_t part = 0;
        if (xinput_multi_interface_to_slot_part(itf_descriptor->bInterfaceNumber, &slot, &part)) {
            xinput_multi_diag.opened_part_mask[slot] |= (uint8_t)(1u << part);
        }
#endif
        return driver_length;
    }

    uint8_t slot = 0;
    uint8_t part = 0;
    if (!xinput_multi_interface_to_slot_part(itf_descriptor->bInterfaceNumber, &slot, &part)) {
        return 0;
    }

    xinput_multi_diag.opened_part_mask[slot] |= (uint8_t)(1u << part);

    if (itf_descriptor->bInterfaceSubClass != XINPUT_SUBCLASS_DEFAULT) {
        return 0;
    }

    switch (itf_descriptor->bInterfaceProtocol) {
        case 0x01: {
            constexpr uint16_t driver_length = 9 + 17 + 14;
            TU_VERIFY(part == 0, 0);
            TU_VERIFY(max_length >= driver_length, 0);
            uint8_t gamepad_in = 0;
            uint8_t gamepad_out = 0;
            TU_VERIFY(open_xinput_multi_endpoints(
                rhport,
                itf_descriptor,
                2,
                true,
                &gamepad_in,
                &gamepad_out,
                nullptr,
                0
            ), 0);
            xinput_multi_device->interfaces[slot].endpoint_in = gamepad_in;
            xinput_multi_device->interfaces[slot].endpoint_out = gamepad_out;
            return driver_length;
        }
        case 0x03: {
            constexpr uint16_t driver_length = 9 + 27 + 28;
            TU_VERIFY(part == 1, 0);
            TU_VERIFY(max_length >= driver_length, 0);
            TU_VERIFY(open_xinput_multi_endpoints(
                rhport,
                itf_descriptor,
                4,
                false,
                nullptr,
                nullptr,
                xinput_multi_device->interfaces[slot].endpoint_out_alt,
                XINPUT_MULTI_ALT_OUT_ENDPOINTS
            ), 0);
            return driver_length;
        }
        case 0x02: {
            constexpr uint16_t driver_length = 9 + 9 + 7;
            TU_VERIFY(part == 2, 0);
            TU_VERIFY(max_length >= driver_length, 0);
            TU_VERIFY(open_xinput_multi_endpoints(
                rhport,
                itf_descriptor,
                1,
                false,
                nullptr,
                nullptr,
                nullptr,
                0
            ), 0);
            return driver_length;
        }
        default:
            return 0;
    }
}

bool xinput_multi_control_xfer_callback(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
) {
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    note_xinput_multi_control_setup(stage, request);
    note_xinput_multi_vendor_get_report(request);

    if (xinput_handle_capability_control(rhport, stage, request)) {
        return true;
    }

    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
        request->bRequest == 1 &&
        request->wIndex == 7) {
        return xinput_handle_ms_os_descriptor_request(rhport, request);
    }

    if (request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_INTERFACE ||
        request->bmRequestType_bit.type != TUSB_REQ_TYPE_STANDARD) {
        return false;
    }

    switch (request->bRequest) {
        case TUSB_REQ_GET_INTERFACE: {
            uint8_t alternate = 0;
            return tud_control_xfer(rhport, request, &alternate, sizeof(alternate));
        }

        case TUSB_REQ_SET_INTERFACE:
            return tud_control_status(rhport, request);

        case TUSB_REQ_GET_DESCRIPTOR: {
            const uint8_t desc_type = (request->wValue >> 8) & 0xFF;
            uint8_t slot = 0;
            uint8_t part = 0;
            const uint8_t itf = request->wIndex & 0xFF;
            if (!xinput_multi_interface_to_slot_part(itf, &slot, &part) &&
                !xinput_multi_is_security_interface(itf)) {
                return false;
            }

            const uint8_t *descriptor = nullptr;
            uint16_t length = 0;
            if (!xinput_multi_get_interface_descriptor(desc_type, itf, slot, part, &descriptor, &length)) {
                return false;
            }
            return tud_control_xfer(
                rhport,
                request,
                const_cast<uint8_t *>(descriptor),
                length
            );
        }

        default:
            return false;
    }
}

bool xinput_multi_xfer_callback(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
) {
    xinput_multi_handle_xfer_complete(rhport, ep_addr, result, xferred_bytes, 0, false);
    return true;
}

extern "C" {

bool tud_xfer_complete_override_cb(
    uint8_t rhport,
    uint8_t ep_addr,
    uint8_t driver_id,
    xfer_result_t result,
    uint32_t xferred_bytes
) {
    // Try the XInput2P endpoint table before TinyUSB dispatches by driver id.
    // If this is not one of our endpoints, TinyUSB continues normally.
    return xinput_multi_handle_xfer_complete(
        rhport,
        ep_addr,
        result,
        xferred_bytes,
        driver_id,
        true
    );
}

static void xinput_multi_reset(uint8_t rhport) {
    (void)rhport;
    xinput_multi_xsm3_auth_observed = false;
    memset(&xinput_multi_diag, 0, sizeof(xinput_multi_diag));
    xinput_auth_reset_usb_runtime();
}

static const usbd_class_driver_t xinput_multi_driver = {
#if CFG_TUSB_DEBUG >= 2
    .name = "XINPUT2P",
#endif
    .init = NULL,
    .reset = xinput_multi_reset,
    .open = xinput_multi_open,
    .control_xfer_cb = xinput_multi_control_xfer_callback,
    .xfer_cb = xinput_multi_xfer_callback,
    .sof = NULL
};

const usbd_class_driver_t *xinput_multi_get_driver()
{
    return &xinput_multi_driver;
}

}  // extern "C"

#endif  // ADAPT_OUTPUT_USB_DEVICE
