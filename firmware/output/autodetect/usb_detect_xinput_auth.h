#ifndef _USB_DETECT_XINPUT_AUTH_H_
#define _USB_DETECT_XINPUT_AUTH_H_

#include <Adafruit_TinyUSB.h>

class Adafruit_USBD_XInputAuth : public Adafruit_USBD_Interface {
 public:
  bool begin(void);
  uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize) override;
};

#endif  // _USB_DETECT_XINPUT_AUTH_H_
