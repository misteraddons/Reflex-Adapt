#include <device/usbd_pvt.h>
#include <tusb_option.h>

#include "out_xinputw.h"
#include "output_xinputw_runtime.h"

bool Adafruit_USBD_XInputW::ready(uint8_t itf) {
    return tud_xinputw_ready(itf);
}

bool Adafruit_USBD_XInputW::sendRawReport(uint8_t itf, uint8_t *report, size_t size) {
    return send_xinputw_report(itf, report, size);
}

bool Adafruit_USBD_XInputW::sendReport(uint8_t itf, xinputw_report_t *report) {
    checkConnectionState(itf);
    return send_xinputw_report(itf, report);
}

void Adafruit_USBD_XInputW::checkConnectionState(uint8_t itf) {
    if (_xinput_dev) {
        if (_xinput_dev->interfaces[itf].connected) {
            if (_xinput_dev->interfaces[itf].info_state == DISCONNECTED) {
                _xinput_dev->send_connection_status(itf, true);
            }
        } else if (_xinput_dev->interfaces[itf].info_state != DISCONNECTED) {
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
    send_connection_status(itf, _xinput_dev->interfaces[itf].info_state != DISCONNECTED);
}

void Adafruit_USBD_XInputW::send_connection_status(uint8_t itf, bool connected) {
    uint8_t data[2];
    data[0] = 0x08;
    data[1] = connected ? 0x80 : 0x00;
    bool ret = _xinput_dev->sendRawReport(itf, data, sizeof(data));

    if (ret)
        _xinput_dev->interfaces[itf].info_state = connected ? UNKNOWN1 : DISCONNECTED;
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
    _xinput_dev->interfaces[itf].connected = state;
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
        usbd_edpt_claim(xinputw_out_rhport, _xinput_dev->interfaces[itf]._endpoint_in);
        usbd_edpt_xfer(
            xinputw_out_rhport,
            _xinput_dev->interfaces[itf]._endpoint_in,
            (uint8_t *)report,
            size
        );
        usbd_edpt_release(xinputw_out_rhport, _xinput_dev->interfaces[itf]._endpoint_in);
        sent = true;
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
