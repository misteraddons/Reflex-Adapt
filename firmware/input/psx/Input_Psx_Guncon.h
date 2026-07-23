/*******************************************************************************
 * PlayStation (GUNCON) input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 *
 * Handles a single input port.
 *
 * Uses PsxNewLib
 * https://github.com/SukkoPera/PsxNewLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

const uint16_t maxMouseValue = 1023;//32767;

//min and max possible values
//from document at http://problemkaputt.de/psx-spx.htm#controllerslightgunsnamcoguncon
//x is 77 to 461
//y is 25 to 248 (ntsc). y is 32 to 295 (pal)
//from personal testing on a pvm
//x is 72 to 450. with underscan x 71 to 453
//y is 22 to 248. with underscan y is 13 to 254 (ntsc)

const uint8_t ANALOG_DEAD_ZONE = 25;

const uint8_t minPossibleX = 77;
const uint16_t maxPossibleX = 461;
const uint8_t minPossibleY = 25;
const uint16_t maxPossibleY = 295;

#if GUNCON_FORCE_MODE == 3
  const uint8_t maxNoLightCount = 254;//80
#else
  const uint8_t maxNoLightCount = 10;
#endif

// Runtime state is owned in input_psx_guncon_runtime_state.cpp.

// Minimum and maximum detected values. Varies from tv to tv.
// Values will be detected when pointing at the screen.
#if GUNCON_FORCE_MODE != 3
  uint16_t minX = 1000;
  uint16_t maxX = 0;
  uint16_t minY = 1000;
  uint16_t maxY = 0;
#endif

#ifdef ENABLE_PSX_GUNCON_MOUSE
  Guncon1_* AbsMouse;
#endif

uint16_t convertRange(const uint16_t gcMin, const uint16_t gcMax, const uint16_t value);
void moveToCoords(uint16_t x, uint16_t y);
void releaseAllButtons();
void readGuncon();
void analogDeadZone(byte& value);
void readDualShock();
void handleButtons();
void runCalibration();
void loopGuncon();
void gunconSetup();
void gunconSetup2();
