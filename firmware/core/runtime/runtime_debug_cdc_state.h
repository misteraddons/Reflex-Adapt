#pragma once

#ifdef ENABLE_RUNTIME_SERIAL_DEBUG_CDC

  #include <Adafruit_TinyUSB.h>

extern Adafruit_USBD_CDC runtimeDebugCdc;
extern bool runtimeDebugCdcEnabled;

#endif
