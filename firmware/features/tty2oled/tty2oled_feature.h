#pragma once

#include "../feature_module.h"

#ifdef ENABLE_TTY2OLED_SERIAL
extern const FeatureModule kTty2OledFeatureModule;
bool tty2oledSerialDrainByte(uint8_t value, Print& out);
#endif
