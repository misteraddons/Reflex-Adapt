#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <Arduino.h>
#include <cstring>

#include "out_xinput.h"
#include "output_xinput_auth_runtime.h"
#include "output_xinput_blob_identity.h"
#include "output_xinput_auth_runtime_state.h"
#include "output_xinput_descriptor_runtime.h"

extern "C" {
#include <libxsm3/xsm3.h>
}

namespace {

template <typename DeviceT>
auto tinyusb_get_cfg_desc_len(DeviceT& device, int)
    -> decltype(device.getDescCfgLen(), uint16_t{}) {
    return device.getDescCfgLen();
}

inline uint16_t tinyusb_get_cfg_desc_len(...) {
    return 0;
}

template <typename DeviceT>
auto tinyusb_get_itf_count(DeviceT& device, int)
    -> decltype(device.getItfCount(), uint8_t{}) {
    return device.getItfCount();
}

inline uint8_t tinyusb_get_itf_count(...) {
    return 0;
}

constexpr uint16_t X360_AUTHLEN_DONGLE_SERIAL = 29;
constexpr uint16_t X360_AUTHLEN_DONGLE_INIT = 46;
constexpr uint16_t X360_AUTHLEN_CHALLENGE = 22;

void process_pending_xsm3_auth_request(XInputAuthRuntimeState& state) {
    if (state.pending_request == XSM3_REQUEST_CHALLENGE && state.auth_state == 2) {
        xsm3_do_challenge_init(state.challenge_buffer);
        memcpy(state.response_buffer, xsm3_challenge_response, X360_AUTHLEN_DONGLE_INIT);
        state.response_len = X360_AUTHLEN_DONGLE_INIT;
        state.auth_state = 3;
        state.pending_request = 0;
    } else if (state.pending_request == XSM3_REQUEST_VERIFY && state.auth_state == 4) {
        xsm3_do_challenge_verify(state.challenge_buffer);
        memcpy(state.response_buffer, xsm3_challenge_response, X360_AUTHLEN_CHALLENGE);
        state.response_len = X360_AUTHLEN_CHALLENGE;
        state.auth_state = 5;
        state.pending_request = 0;
    }
}

}  // namespace

extern Adafruit_USBD_XInput *xinput_device;

void xinput_note_begin_result(bool success) {
    xinputAuthRuntimeState().debug_begin_result = success;
}

void xinput_note_interface_opened(void) {
    xinputAuthRuntimeState().debug_interfaces_opened++;
}

void xinput_auth_invalidate(void) {
    auto& state = xinputAuthRuntimeState();
    state.initialized = false;
    state.authenticated = false;
    state.auth_state = 0;
    state.pending_request = 0;
    state.challenge_len = 0;
    state.response_len = 0;
    memset(state.challenge_buffer, 0, sizeof(state.challenge_buffer));
    memset(state.response_buffer, 0, sizeof(state.response_buffer));
    memset(state.misc_buffer, 0, sizeof(state.misc_buffer));
}

void xinput_auth_reset_usb_runtime(void) {
    auto& state = xinputAuthRuntimeState();
    state.debug_reset_count++;
    xinput_auth_invalidate();
}

void xinput_auth_init(void) {
    auto& state = xinputAuthRuntimeState();
    if (state.initialized) {
        return;
    }

    // The XSM3 library is global/singleton and mirrors the known-working donor
    // Xbox 360 controller flow. Keep Xbox 360 on the single-controller
    // OUTPUT_XINPUT path; XInput2P may expose security-shaped descriptors for
    // Windows compatibility, but it must not become the console auth runtime.
    xinputApplyBlobIdentity();
    xsm3_initialise_state();
    xsm3_set_identification_data(xsm3_id_data_ms_controller);
    memcpy(state.id_buffer, xsm3_id_data_ms_controller, sizeof(state.id_buffer));

    state.initialized = true;
    state.authenticated = false;
    state.auth_state = 0;
    state.pending_request = 0;
    state.challenge_len = 0;
    state.response_len = 0;
}

void xinput_auth_process(void) {
    auto& state = xinputAuthRuntimeState();
    state.debug_process_count++;
    process_pending_xsm3_auth_request(state);
}

bool xinput_is_authenticated(void) {
    return xinputAuthRuntimeState().authenticated;
}

void xinput_get_debug_info(XInputDebugInfo* info) {
    if (!info) {
        return;
    }

    auto& state = xinputAuthRuntimeState();
    info->initialized = state.initialized;
    info->authenticated = state.authenticated;
    info->auth_state = state.auth_state;
    info->last_request = state.debug_last_request;
    info->last_stage = state.debug_last_stage;
    info->request_count = state.debug_request_count;
    info->data_stage_count = state.debug_data_stage_count;
    info->req_81_count = state.debug_req_81;
    info->req_82_count = state.debug_req_82;
    info->req_83_count = state.debug_req_83;
    info->req_84_count = state.debug_req_84;
    info->req_85_count = state.debug_req_85;
    info->req_86_count = state.debug_req_86;
    info->req_87_count = state.debug_req_87;
    info->req_other_count = state.debug_req_other;
    info->last_other_req = state.debug_last_other_req;
    info->challenge_byte0 = state.challenge_buffer[0];
    info->response_byte0 = xsm3_challenge_response[0];
    info->last_wIndex = state.debug_last_wIndex;
    info->last_wLength = state.debug_last_wLength;
    info->pending_req = state.pending_request;
    info->process_count = state.debug_process_count;
    info->interfaces_opened = state.debug_interfaces_opened;
    info->endpoint_in = xinput_device ? xinput_device->_endpoint_in : 0;
    info->endpoint_out = xinput_device ? xinput_device->_endpoint_out : 0;
    xinput_fill_descriptor_debug_info(info);
    info->control_count = state.debug_control_count;
    info->standard_control_count = state.debug_standard_control_count;
    info->vendor_control_count = state.debug_vendor_control_count;
    info->desc_21_count = state.debug_desc_21_count;
    info->desc_41_count = state.debug_desc_41_count;
    info->last_control_request = state.debug_last_control_request;
    info->last_control_type = state.debug_last_control_type;
    info->last_control_recipient = state.debug_last_control_recipient;
    info->last_control_desc_type = state.debug_last_control_desc_type;
    info->last_control_itf = state.debug_last_control_itf;
    info->string_4_seen = xinput_seen_string_4;
    info->security_mismatch_count =
        (state.debug_security_mismatch_count > 0xFFu) ? 0xFFu : (uint8_t)state.debug_security_mismatch_count;
    info->reset_count = (state.debug_reset_count > 0xFFu) ? 0xFFu : (uint8_t)state.debug_reset_count;
    info->tud_inited = tud_inited();
    info->tud_connected = tud_connected();
    info->tud_mounted = tud_mounted();
    info->tud_suspended = tud_suspended();
    info->cfg_desc_len = tinyusb_get_cfg_desc_len(TinyUSBDevice, 0);
    info->itf_count = tinyusb_get_itf_count(TinyUSBDevice, 0);
    info->begin_result = state.debug_begin_result;
}

bool xinput_auth_handle_control_for_security_interface(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request,
    uint8_t security_itf
) {
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR) {
        return false;
    }

    auto& state = xinputAuthRuntimeState();

    if ((request->wIndex & 0xFF) != security_itf) {
        state.debug_security_mismatch_count++;
        state.debug_last_bad_wIndex = request->wIndex & 0xFF;
        return false;
    }

    state.debug_last_request = request->bRequest;
    state.debug_last_stage = stage;
    state.debug_request_count++;
    state.debug_last_wIndex = request->wIndex & 0xFF;
    state.debug_last_wLength = request->wLength & 0xFF;

    if (!state.initialized) {
        xinput_auth_init();
    }

    uint16_t len = 0;

    if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
        if (stage != CONTROL_STAGE_SETUP) {
            return true;
        }

        switch (request->bRequest) {
            case XSM3_REQUEST_INIT:
                state.debug_req_81++;
                break;
            case 0x84:
                state.debug_req_84++;
                break;
            case 0x85:
                state.debug_req_85++;
                break;
            case XSM3_REQUEST_RESPONSE:
                state.debug_req_83++;
                break;
            case XSM3_REQUEST_STATUS:
                state.debug_req_86++;
                break;
            default:
                state.debug_req_other++;
                state.debug_last_other_req = request->bRequest;
                return false;
        }

        switch (request->bRequest) {
            case XSM3_REQUEST_INIT:
                state.auth_state = 1;
                len = X360_AUTHLEN_DONGLE_SERIAL;
                return tud_control_xfer(rhport, request, state.id_buffer, len);

            case XSM3_REQUEST_RESPONSE:
                if (state.auth_state == 3) {
                    len = X360_AUTHLEN_DONGLE_INIT;
                } else if (state.auth_state == 5) {
                    len = X360_AUTHLEN_CHALLENGE;
                } else {
                    return false;
                }
                return tud_control_xfer(rhport, request, state.response_buffer, len);

            case 0x84:
            case 0x85:
                if (request->wLength == 0) {
                    return tud_control_status(rhport, request);
                }
                return tud_control_xfer(rhport, request, state.misc_buffer, request->wLength);

            case XSM3_REQUEST_STATUS: {
                static uint16_t status = 1;
                if (state.auth_state == 3 || state.auth_state == 5) {
                    status = 2;
                } else {
                    status = 1;
                }
                if (state.auth_state == 5) {
                    state.authenticated = true;
                }
                len = sizeof(status);
                return tud_control_xfer(rhport, request, &status, len);
            }

            default:
                return false;
        }
    }

    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if (stage == CONTROL_STAGE_SETUP) {
            switch (request->bRequest) {
                case XSM3_REQUEST_CHALLENGE:
                    state.debug_req_82++;
                    memset(state.challenge_buffer, 0, sizeof(state.challenge_buffer));
                    return tud_control_xfer(rhport, request, state.challenge_buffer, request->wLength);

                case 0x84:
                    state.debug_req_84++;
                    if (request->wLength == 0) {
                        return tud_control_status(rhport, request);
                    }
                    memset(state.misc_buffer, 0, sizeof(state.misc_buffer));
                    return tud_control_xfer(rhport, request, state.misc_buffer, request->wLength);

                case 0x85:
                    state.debug_req_85++;
                    if (request->wLength == 0) {
                        return tud_control_status(rhport, request);
                    }
                    memset(state.misc_buffer, 0, sizeof(state.misc_buffer));
                    return tud_control_xfer(rhport, request, state.misc_buffer, request->wLength);

                case XSM3_REQUEST_VERIFY:
                    state.debug_req_87++;
                    memset(state.challenge_buffer, 0, sizeof(state.challenge_buffer));
                    return tud_control_xfer(rhport, request, state.challenge_buffer, request->wLength);

                default:
                    state.debug_req_other++;
                    state.debug_last_other_req = request->bRequest;
                    return false;
            }
        }

        if (stage == CONTROL_STAGE_DATA) {
            state.debug_data_stage_count++;

            switch (request->bRequest) {
                case XSM3_REQUEST_CHALLENGE:
                    if (state.auth_state == 0 || state.auth_state == 1) {
                        state.challenge_len = request->wLength;
                        state.pending_request = XSM3_REQUEST_CHALLENGE;
                        state.auth_state = 2;
                    }
                    return true;

                case XSM3_REQUEST_VERIFY:
                    state.challenge_len = request->wLength;
                    state.pending_request = XSM3_REQUEST_VERIFY;
                    state.auth_state = 4;
                    return true;

                case 0x84:
                case 0x85:
                    return true;

                default:
                    state.debug_req_other++;
                    state.debug_last_other_req = request->bRequest;
                    return false;
            }
        }

        return true;
    }

    return false;
}

bool xinput_auth_handle_control(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request) {
    if (!xinput_device) {
        return false;
    }
    return xinput_auth_handle_control_for_security_interface(
        rhport,
        stage,
        request,
        xinput_device->_security_itfnum
    );
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
