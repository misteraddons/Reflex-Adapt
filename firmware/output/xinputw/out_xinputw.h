/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//https://github.com/JonnyHaystack/Adafruit_TinyUSB_XInput/tree/master
//https://github.com/timokroeger/psx-usb

#ifndef ADAFRUIT_USBD_XINPUTW_HPP_
#define ADAFRUIT_USBD_XINPUTW_HPP_

#include <arduino/Adafruit_USBD_Device.h>
#include <device/usbd_pvt.h>
#include <tusb_option.h>

#define XINPUTW_SUBCLASS_DEFAULT 0x5D
#define XINPUTW_PROTOCOL_DEFAULT 0x81 //1

//#define EPOUT 0x01 //0x01
//#define EPIN 0x81
#define XINPUTW_EPSIZE 32
#define XINPUT_WIRELESS_CONTROLLERS 4

static_assert(XINPUT_WIRELESS_CONTROLLERS > 0 && XINPUT_WIRELESS_CONTROLLERS < 5, "Invalid value for XINPUT_WIRELESS_CONTROLLERS");


extern void setLed(uint8_t state);

// clang-format off

//--------------------------------------------------------------------+
// MSC Descriptor TemplatesVENDOR_REQUEST_MICROSOFT 
//--------------------------------------------------------------------+

// Length of template descriptor: 40 bytes
#define TUD_XINPUTW_DESC_LEN    (9 + 17 + 7 + 7)

#define TUD_XINPUTW_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize, _ep_interval) \
    /* Cotroller 1 */\
    9, TUSB_DESC_INTERFACE, (_itfnum), 0, 2, 0xFF, XINPUTW_SUBCLASS_DEFAULT, XINPUTW_PROTOCOL_DEFAULT, 0,\
    17, 0x21, 0x00, 0x01, 0x01, 0x25, _epin, 0x14, 0x00, 0x00, 0x00, 0x00, 0x13, _epout, 0x08, 0x00, 0x00,\
    7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(_epsize), 1,\
    7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(_epsize), 8


// clang-format on

enum ControllerInfoState {
    DISCONNECTED,
    NONE,
    UNKNOWN1,
    UNKNOWN2
};

extern void rumble_callback(uint8_t index, uint8_t left, uint8_t right);

typedef struct __attribute((packed, aligned(1))) {
    // Buttons first byte
    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;
    bool start : 1;
    bool back : 1;
    bool ls : 1;
    bool rs : 1;

    // Buttons second byte
    bool lb : 1;
    bool rb : 1;
    bool home : 1;
    bool _reserved0 : 1;
    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    uint8_t lt;
    uint8_t rt;
    int16_t lx;
    int16_t ly;
    int16_t rx;
    int16_t ry;
} xinputw_report_t;

typedef struct {
    uint8_t _endpoint_in;
    uint8_t _endpoint_out;
    uint8_t _xinput_out_buffer[XINPUTW_EPSIZE];
    ControllerInfoState info_state;
    uint64_t idle_msg_deadline_us;
    bool connected;
} xinputw_itf_t;

struct XInputWDiagInfo {
    uint8_t controller_count;
    uint16_t out_xfer_count[XINPUT_WIRELESS_CONTROLLERS];
    uint8_t endpoint_in[XINPUT_WIRELESS_CONTROLLERS];
    uint8_t endpoint_out[XINPUT_WIRELESS_CONTROLLERS];
    uint8_t last_out_size[XINPUT_WIRELESS_CONTROLLERS];
    uint8_t last_out_packet[XINPUT_WIRELESS_CONTROLLERS][8];
    uint8_t last_rumble_left[XINPUT_WIRELESS_CONTROLLERS];
    uint8_t last_rumble_right[XINPUT_WIRELESS_CONTROLLERS];
};
extern XInputWDiagInfo xinputw_diag;

bool tud_xinputw_ready(uint8_t itf);
void receive_xinputw_report(uint8_t itf);
bool send_xinputw_report(uint8_t itf, uint8_t *report, size_t size);
bool send_xinputw_report(uint8_t itf, xinputw_report_t *report);
void xinputw_get_diag_info(XInputWDiagInfo *info);
uint16_t xinputw_open(
    uint8_t rhport,
    const tusb_desc_interface_t *itf_descriptor,
    uint16_t max_length
);
bool xinputw_xfer_callback(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
);

class Adafruit_USBD_XInputW : public Adafruit_USBD_Interface {
  public:
    Adafruit_USBD_XInputW(uint8_t interval_ms = 1);

    bool begin(void);

    bool ready(uint8_t itf);
    bool sendRawReport(uint8_t itf, uint8_t *report, size_t size);
    bool sendReport(uint8_t itf, xinputw_report_t *report);
    ControllerInfoState getControllerState(uint8_t itf);
    void setControllerState(uint8_t itf, ControllerInfoState state);
    void driver_task();
    void set_connected(uint8_t itf, bool state);

    void handle_connection_status(uint8_t itf);
    void send_connection_status(uint8_t itf, bool connected);
    void interface_task(uint8_t itf);
    void checkConnectionState(uint8_t itf);

    // from Adafruit_USBD_Interface
    virtual uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize);

  private:
    uint8_t _interval_ms;
    uint8_t _itf_base = 0xFF;
    //uint8_t _endpoint_in = 0;
    //uint8_t _endpoint_out = 0;
    //uint8_t _xinput_out_buffer[XINPUTW_EPSIZE] = {};

    xinputw_itf_t interfaces[XINPUT_WIRELESS_CONTROLLERS] = {};

    friend bool tud_xinputw_ready(uint8_t itf);
    friend void receive_xinputw_report(uint8_t itf);
    friend bool send_xinputw_report(uint8_t itf, uint8_t *report, size_t size);
    friend bool send_xinputw_report(uint8_t itf, xinputw_report_t *report);
    friend void xinputw_get_diag_info(XInputWDiagInfo *info);
    
    friend uint16_t xinputw_open(
        uint8_t rhport,
        const tusb_desc_interface_t *itf_descriptor,
        uint16_t max_length
    );
    friend bool xinputw_xfer_callback(
        uint8_t rhport,
        uint8_t ep_addr,
        xfer_result_t result,
        uint32_t xferred_bytes
    );
    friend const usbd_class_driver_t *usbd_app_driver_get_cb(uint8_t *driver_count);
    friend bool _xinputw_tud_vendor_control_xfer_cb(
        uint8_t rhport,
        uint8_t stage,
        const tusb_control_request_t *request
    );
};

#ifdef __cplusplus
extern "C"
{
#endif
  const usbd_class_driver_t *xinputw_get_driver();
#ifdef __cplusplus
}
#endif

#endif /* ADAFRUIT_USBD_XINPUTW_HPP_ */
