#include "../product_config.h"

#include "menu_pad_layouts_internal.h"

namespace menu_pad_layouts_internal {

const PadButton padLayoutNES[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 2, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_B, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_NES_COUNT = sizeof(padLayoutNES) / sizeof(PadButton);

const PadButton padLayoutPowerPad[] = {
  { GPAD_A, 0, 1 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 0, 3 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 0, 5 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 0, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L1, 1, 1 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R1, 1, 3 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L2, 1, 5 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R2, 1, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_SELECT, 2, 1 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_START, 2, 3 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L3, 2, 5 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R3, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_POWERPAD_COUNT = sizeof(padLayoutPowerPad) / sizeof(PadButton);

const PadButton padLayoutGeneric[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 2, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_B, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_GENERIC_COUNT = sizeof(padLayoutGeneric) / sizeof(PadButton);

#ifdef ENABLE_INPUT_PCE
const PadButton padLayoutPCE[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 2, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_B, 3, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R1, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L1, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 2, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_PCE_COUNT = sizeof(padLayoutPCE) / sizeof(PadButton);

const PadButton padLayoutPCE2[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 2, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_B, 3, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_PCE2_COUNT = sizeof(padLayoutPCE2) / sizeof(PadButton);
#endif

#ifdef ENABLE_INPUT_NEOGEO
const PadButton padLayoutNeoGeo[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, 4 * 6, PAD_DASH_ON, PAD_DASH_OFF },
  { GPAD_START, 3, 4 * 6, PAD_DASH_ON, PAD_DASH_OFF },
  { GPAD_A, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 1, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_NEOGEO_COUNT = sizeof(padLayoutNeoGeo) / sizeof(PadButton);
#endif

#ifdef ENABLE_INPUT_3DO
const PadButton padLayout3DO[] = {
  { GPAD_L1, 0, 1 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R1, 0, 7 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, (4 * 6) - 3, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_START, 2, (5 * 6) - 3, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_3DO_COUNT = sizeof(padLayout3DO) / sizeof(PadButton);
#endif

#ifdef ENABLE_INPUT_JAGUAR
const PadButton padLayoutJaguar[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_START, 0, 3 * 6, PAD_DASH_ON, PAD_DASH_OFF },
  { GPAD_SELECT, 0, 5 * 6, PAD_DASH_ON, PAD_DASH_OFF },
  { GPAD_L3, 1, 3 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_EXTRA1, 1, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_R3, 1, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_L2, 2, 3 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_EXTRA4, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_R2, 2, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_L1, 3, 3 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_Y, 3, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_R1, 3, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_EXTRA2, 4, 3 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_EXTRA0, 4, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_EXTRA3, 4, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_X, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_JAGUAR_COUNT = sizeof(padLayoutJaguar) / sizeof(PadButton);

const PadButton padLayoutJaguarRotary[] = {
  { GPAD_START, 2, 0, PAD_DASH_ON, PAD_DASH_OFF },
  { GPAD_SELECT, 2, 2 * 6, PAD_DASH_ON, PAD_DASH_OFF },
  { GPAD_X, 2, 5 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_JAGUAR_ROTARY_COUNT = sizeof(padLayoutJaguarRotary) / sizeof(PadButton);
#endif

#ifdef ENABLE_INPUT_INTV
const PadButton padLayoutIntv[] = {
  { GPAD_UP, 2, 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_LEFT, 3, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 3, 12, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_DOWN, 4, 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_A, 2, 48, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 3, 48, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 4, 48, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 1, 24, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_L1, 1, 30, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_R1, 1, 36, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_L2, 2, 24, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_R2, 2, 30, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_L3, 2, 36, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_R3, 3, 24, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_SELECT, 4, 24, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_HOME, 4, 30, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_START, 4, 36, PAD_RECT_ON, PAD_RECT_OFF },
};
const uint8_t PAD_LAYOUT_INTV_COUNT = sizeof(padLayoutIntv) / sizeof(PadButton);
#endif

#ifdef ENABLE_INPUT_DRIVING
const PadButton padLayoutDriving[] = {
  { GPAD_A, 2, 4 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_DRIVING_COUNT = sizeof(padLayoutDriving) / sizeof(PadButton);
#endif

}  // namespace menu_pad_layouts_internal
