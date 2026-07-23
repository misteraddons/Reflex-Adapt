#ifndef OUTPUT_XINPUTW_RUNTIME_H_
#define OUTPUT_XINPUTW_RUNTIME_H_

#include <stdint.h>

class Adafruit_USBD_XInputW;

extern uint8_t xinputw_out_rhport;
extern Adafruit_USBD_XInputW *_xinput_dev;

#endif  // OUTPUT_XINPUTW_RUNTIME_H_
