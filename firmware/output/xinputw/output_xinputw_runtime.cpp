#include <device/usbd_pvt.h>
#include <tusb_option.h>

#include "out_xinputw.h"
#include "output_xinputw_runtime.h"

namespace {

constexpr uint64_t kConnectionStatusRetryUs = 250000;

// Input protocols can discover physical ports in different frames. Give a
// higher child enough time for every lower physical slot to settle before its
// Windows connection announcement. This changes hot-plug time, not poll rate.
constexpr uint64_t kHigherChildOrderSettleUs = 750000;

}  // namespace

bool Adafruit_USBD_XInputW::ready(uint8_t itf) {
    return tud_xinputw_ready(itf);
}

bool Adafruit_USBD_XInputW::sendRawReport(uint8_t itf, uint8_t *report, size_t size) {
    return send_xinputw_report(itf, report, size);
}

bool Adafruit_USBD_XInputW::sendReport(uint8_t itf, xinputw_report_t *report) {
    checkConnectionState(itf);
    const bool sent = send_xinputw_report(itf, report);
    if (sent && itf < XINPUT_WIRELESS_CONTROLLERS) {
        ++xinputw_diag.controller_report_count[itf];
    }
    return sent;
}

bool Adafruit_USBD_XInputW::connectedChildMayStart(uint8_t itf) const {
    if (!_xinput_dev || itf >= XINPUT_WIRELESS_CONTROLLERS) {
        return false;
    }
    const xinputw_itf_t& interface = _xinput_dev->interfaces[itf];
    if (interface.connection_order_deadline_us != 0 &&
        time_us_64() < interface.connection_order_deadline_us) {
        return false;
    }
    for (uint8_t lower = 0; lower < itf; ++lower) {
        const xinputw_itf_t& lower_interface = _xinput_dev->interfaces[lower];
        if (lower_interface.connected && lower_interface.info_state != NONE) {
            return false;
        }
    }
    return true;
}

void Adafruit_USBD_XInputW::checkConnectionState(uint8_t itf) {
    if (_xinput_dev) {
        xinputw_itf_t& interface = _xinput_dev->interfaces[itf];
        if (interface.connected) {
            // Windows assigns XInput user numbers in child-handshake order,
            // not strictly by the receiver interface number. When several
            // pads are already present at boot, finish every lower connected
            // child first so physical P1 cannot be assigned after P2.
            if (interface.info_state == DISCONNECTED ||
                (interface.info_state == UNKNOWN1 &&
                 interface.connection_retry_deadline_us != 0 &&
                 time_us_64() >= interface.connection_retry_deadline_us)) {
                if (!connectedChildMayStart(itf)) {
                    return;
                }
                _xinput_dev->send_connection_status(itf, true);
            }
        } else if (interface.info_state != DISCONNECTED) {
            _xinput_dev->send_connection_status(itf, false);
        }
    }
}

ControllerInfoState Adafruit_USBD_XInputW::getControllerState(uint8_t itf) {
    return _xinput_dev ? _xinput_dev->interfaces[itf].info_state : DISCONNECTED;
}

void Adafruit_USBD_XInputW::setControllerState(uint8_t itf, ControllerInfoState state) {
    if (_xinput_dev)
        _xinput_dev->interfaces[itf].info_state = state;
}

void Adafruit_USBD_XInputW::handle_connection_status(uint8_t itf) {
    if (!_xinput_dev || itf >= XINPUT_WIRELESS_CONTROLLERS) {
        return;
    }
    // The host is asking about physical child presence, not whether an earlier
    // handshake packet happened to survive a USB reset/reconfiguration. Keep
    // the reply ordered too, so a host query cannot announce P2 before P1.
    const bool connected = _xinput_dev->interfaces[itf].connected &&
                           connectedChildMayStart(itf);
    send_connection_status(itf, connected);
}

void Adafruit_USBD_XInputW::send_connection_status(uint8_t itf, bool connected) {
    uint8_t data[2];
    data[0] = 0x08;
    data[1] = connected ? 0x80 : 0x00;
    bool ret = _xinput_dev->sendRawReport(itf, data, sizeof(data));

    if (ret) {
        xinputw_itf_t& interface = _xinput_dev->interfaces[itf];
        interface.info_state = connected ? UNKNOWN1 : DISCONNECTED;
        interface.connection_retry_deadline_us =
            connected ? time_us_64() + kConnectionStatusRetryUs : 0;
        if (itf < XINPUT_WIRELESS_CONTROLLERS) {
            ++xinputw_diag.connection_status_count[itf];
        }
    }
}

void Adafruit_USBD_XInputW::reset_transport_state() {
    for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS; ++i) {
        xinputw_itf_t& interface = interfaces[i];
        interface._endpoint_in = 0;
        interface._endpoint_out = 0;
        memset(interface._xinput_out_buffer, 0, sizeof(interface._xinput_out_buffer));
        interface.info_state = DISCONNECTED;
        interface.idle_msg_deadline_us = 0;
        interface.connection_retry_deadline_us = 0;
        interface.connection_order_deadline_us =
            (interface.connected && i != 0)
                ? time_us_64() + kHigherChildOrderSettleUs : 0;
    }
}

void Adafruit_USBD_XInputW::driver_task() {
    if (!_xinput_dev || !tud_mounted())
        return;
    for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS; ++i)
        interface_task(i);
}

void Adafruit_USBD_XInputW::interface_task(uint8_t itf) {
    if (!_xinput_dev)
        return;

    receive_xinputw_report(itf);
    checkConnectionState(itf);

    if (_xinput_dev->interfaces[itf].idle_msg_deadline_us != 0 &&
        time_us_64() >= _xinput_dev->interfaces[itf].idle_msg_deadline_us) {
        uint8_t data[29] = {0};
        data[3] = 0xF0;
        if (_xinput_dev->ready(itf)) {
            _xinput_dev->sendRawReport(itf, data, sizeof(data));
        }
        _xinput_dev->interfaces[itf].idle_msg_deadline_us = 0;
    }
}

void Adafruit_USBD_XInputW::set_connected(uint8_t itf, bool state) {
    if (!_xinput_dev)
        return;
    xinputw_itf_t& interface = _xinput_dev->interfaces[itf];
    if (interface.connected == state) {
        return;
    }
    interface.connected = state;
    if (state && itf != 0) {
        interface.connection_order_deadline_us =
            time_us_64() + kHigherChildOrderSettleUs;
    } else {
        interface.connection_order_deadline_us = 0;
    }
}

bool tud_xinputw_ready(uint8_t itf) {
    return _xinput_dev && _xinput_dev->interfaces[itf]._endpoint_in && tud_ready() &&
           !usbd_edpt_busy(xinputw_out_rhport, _xinput_dev->interfaces[itf]._endpoint_in);
}

void receive_xinputw_report(uint8_t itf) {
    if (_xinput_dev && _xinput_dev->interfaces[itf]._endpoint_out && tud_ready() &&
        !usbd_edpt_busy(xinputw_out_rhport, _xinput_dev->interfaces[itf]._endpoint_out)) {
        usbd_edpt_claim(xinputw_out_rhport, _xinput_dev->interfaces[itf]._endpoint_out);
        usbd_edpt_xfer(
            xinputw_out_rhport,
            _xinput_dev->interfaces[itf]._endpoint_out,
            _xinput_dev->interfaces[itf]._xinput_out_buffer,
            XINPUTW_EPSIZE
        );
        usbd_edpt_release(xinputw_out_rhport, _xinput_dev->interfaces[itf]._endpoint_out);
    }
}

bool send_xinputw_report(uint8_t itf, uint8_t *report, size_t size) {
    bool sent = false;

    if (tud_xinputw_ready(itf)) {
        const uint8_t endpoint_in = _xinput_dev->interfaces[itf]._endpoint_in;
        if (!usbd_edpt_claim(xinputw_out_rhport, endpoint_in)) {
            return false;
        }
        sent = usbd_edpt_xfer(
            xinputw_out_rhport,
            endpoint_in,
            (uint8_t *)report,
            size
        );
        usbd_edpt_release(xinputw_out_rhport, endpoint_in);
        if (sent && itf < XINPUT_WIRELESS_CONTROLLERS) {
            ++xinputw_diag.in_xfer_count[itf];
        }
    }

    return sent;
}

bool send_xinputw_report(uint8_t itf, xinputw_report_t *report) {
    uint8_t data[29] = {0};
    data[0] = 0x00;
    data[1] = 0x01;
    data[3] = 0xF0;
    data[4] = 0x00;
    data[5] = 0x13;
    memcpy(&data[6], report, sizeof(xinputw_report_t));

    bool sent = send_xinputw_report(itf, data, sizeof(data));

    if (sent)
        _xinput_dev->interfaces[itf].idle_msg_deadline_us = time_us_64() + 11000;

    return sent;
}
