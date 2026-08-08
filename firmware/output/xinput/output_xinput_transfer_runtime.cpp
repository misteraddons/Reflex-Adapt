#include "out_xinput.h"

#include <string.h>

#include "output_xinput_auth_runtime_state.h"
#include "output_xinput_driver_runtime.h"
#include "output_xinput_descriptor_runtime.h"
#include "output_xinput_rumble_parser.h"

extern Adafruit_USBD_XInput *xinput_device;

bool Adafruit_USBD_XInput::ready(void) {
    return tud_xinput_ready();
}

bool Adafruit_USBD_XInput::sendReport(xinput_report_t *report) {
    return send_xinput_report(report);
}

bool tud_xinput_ready() {
    return xinput_device && xinput_device->_endpoint_in && tud_ready() &&
           !usbd_edpt_busy(xinput_out_rhport, xinput_device->_endpoint_in);
}

void receive_xinput_report(void) {
    if (xinput_device && xinput_device->_endpoint_out && tud_ready() &&
        !usbd_edpt_busy(xinput_out_rhport, xinput_device->_endpoint_out)) {
        auto& state = xinputAuthRuntimeState();
        if (!usbd_edpt_claim(xinput_out_rhport, xinput_device->_endpoint_out)) {
            state.debug_xfer_out_submit_fail_count++;
            return;
        }

        if (usbd_edpt_xfer(
                xinput_out_rhport,
                xinput_device->_endpoint_out,
                xinput_device->_xinput_out_buffer,
                EPSIZE)) {
            state.debug_xfer_out_submit_ok_count++;
        } else {
            state.debug_xfer_out_submit_fail_count++;
        }

        usbd_edpt_release(xinput_out_rhport, xinput_device->_endpoint_out);
    }
}

bool send_xinput_report(xinput_report_t *report) {
    bool sent = false;
    auto& state = xinputAuthRuntimeState();

    if (tud_xinput_ready()) {
        if (!usbd_edpt_claim(xinput_out_rhport, xinput_device->_endpoint_in)) {
            state.debug_xfer_in_submit_fail_count++;
            return false;
        }

        if (usbd_edpt_xfer(
                xinput_out_rhport,
                xinput_device->_endpoint_in,
                (uint8_t *)report,
                sizeof(xinput_report_t))) {
            state.debug_xfer_in_count++;
            state.debug_xfer_in_submit_ok_count++;
            sent = true;
        } else {
            state.debug_xfer_in_submit_fail_count++;
        }

        usbd_edpt_release(xinput_out_rhport, xinput_device->_endpoint_in);
    }

    return sent;
}

bool xinput_xfer_callback(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
) {
    (void)result;
    auto& state = xinputAuthRuntimeState();

    if (ep_addr == xinput_device->_endpoint_in) {
        xinput_seen_in_xfer = true;
        state.debug_xfer_in_complete_count++;
    }

    if (ep_addr == xinput_device->_endpoint_out) {
        state.debug_xfer_out_count++;
        const uint8_t out_size = (xferred_bytes > EPSIZE) ? EPSIZE : (uint8_t)xferred_bytes;
        const size_t parse_size = xinputRumbleParseSize(out_size, EPSIZE);
        // Keep the raw OUT frame visible through STATE while tuning host rumble.
        state.debug_last_out_size = (parse_size > UINT8_MAX) ? UINT8_MAX : (uint8_t)parse_size;
        memset(state.debug_last_out_packet, 0, sizeof(state.debug_last_out_packet));
        memcpy(state.debug_last_out_packet,
               xinput_device->_xinput_out_buffer,
               parse_size > sizeof(state.debug_last_out_packet)
                 ? sizeof(state.debug_last_out_packet)
                 : parse_size);
        XInputRumblePacket rumble{};
        if (parseXinputRumblePacket(xinput_device->_xinput_out_buffer, parse_size, &rumble)) {
          state.debug_last_rumble_left = rumble.left;
          state.debug_last_rumble_right = rumble.right;
          rumble_callback(0, rumble.left, rumble.right);
        }
        usbd_edpt_xfer(
            rhport,
            xinput_device->_endpoint_out,
            xinput_device->_xinput_out_buffer,
            EPSIZE
        );
    }

    return true;
}
