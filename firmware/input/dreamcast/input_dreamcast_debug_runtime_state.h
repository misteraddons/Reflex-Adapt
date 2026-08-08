#pragma once

#ifdef ENABLE_INPUT_DREAMCAST

  #include <Adafruit_TinyUSB.h>

extern Adafruit_USBD_CDC dreamcastDebugCdc;
extern bool dreamcastDebugCdcEnabled;

#endif
