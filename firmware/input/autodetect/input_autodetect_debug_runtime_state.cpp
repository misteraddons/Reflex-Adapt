#ifndef AUTODETECT_DEBUG
  #define AUTODETECT_DEBUG
#endif

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef AUTODETECT_DEBUG
AutoDetectDebug lastDebug[kAutoDetectDebugPortCount];
#endif
