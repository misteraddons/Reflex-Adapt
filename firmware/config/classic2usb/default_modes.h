#pragma once

// Default input mode on first boot / EEPROM reset.
#if defined(ENABLE_INPUT_AUTODETECT)
  #define DEFAULT_INPUT_MODE  RZORD_AUTODETECT
#else
  #define DEFAULT_INPUT_MODE  RZORD_SNES
#endif
