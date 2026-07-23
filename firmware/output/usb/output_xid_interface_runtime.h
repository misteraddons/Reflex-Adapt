#pragma once

// Internal XID interface helper for the USB device output stack.

#include <Adafruit_TinyUSB.h>

extern uint8_t xid_out_rhport;
extern uint8_t xid_begin_called;
extern uint8_t xid_begin_success;

class Adafruit_USBD_XID : public Adafruit_USBD_Interface {
  public:
    Adafruit_USBD_XID();
    bool begin(void);
    virtual uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize);

    uint8_t stored_itfnum = 0;
    uint8_t stored_ep_in = 0;
    uint8_t stored_ep_out = 0;
};
