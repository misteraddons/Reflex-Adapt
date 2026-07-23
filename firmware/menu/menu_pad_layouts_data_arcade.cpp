#include "../product_config.h"

#include "menu_pad_layouts_internal.h"

namespace menu_pad_layouts_internal {

#ifdef ENABLE_INPUT_DREAMCAST
const PadButton padLayoutDreamcast[] = {
  { GPAD_L2, 0, 1 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 8 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_START, 2, 4 * 6, PAD_WIDE_TRIANGLE_LEFT_ON, PAD_WIDE_TRIANGLE_LEFT_OFF },
  { GPAD_Y, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_DREAMCAST_COUNT = sizeof(padLayoutDreamcast) / sizeof(PadButton);

const PadButton padLayoutDreamcastWheel[] = {
  { GPAD_L2, 0, 0 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 10 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 2, 0 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_DOWN, 2, 2 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 2, 4 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_START, 2, 8 * 6, PAD_WIDE_TRIANGLE_LEFT_ON, PAD_WIDE_TRIANGLE_LEFT_OFF },
};
const uint8_t PAD_LAYOUT_DREAMCAST_WHEEL_COUNT = sizeof(padLayoutDreamcastWheel) / sizeof(PadButton);
#endif

const PadButton padLayoutPSXDigital[] = {
  { GPAD_L2, 0, 0 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_L1, 0, 2 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R1, 0, 7 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 9 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, (4 * 6) - 3, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 2, (5 * 6) + 3, PAD_PS_TRIANGLE_ON, PAD_PS_TRIANGLE_OFF },
  { GPAD_Y, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_PSX_DIGITAL_COUNT = sizeof(padLayoutPSXDigital) / sizeof(PadButton);

const PadButton padLayoutPopn[] = {
  { GPAD_SELECT, 0, 1 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 0, 3 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_B, 1, 1 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 1, 3 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 1, 5 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_UP, 1, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 2, 0 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R1, 2, 2 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L1, 2, 4 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R2, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L2, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_POPN_COUNT = sizeof(padLayoutPopn) / sizeof(PadButton);

const PadButton padLayoutGuitarFreaks[] = {
  { GPAD_L2, 1, 1 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_Y, 1, 4 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 1, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R2, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_UP | GPAD_DOWN, 2, 1 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_SELECT, 3, 1 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 3, 3 * 6, PAD_RECT_ON, PAD_RECT_OFF },
};
const uint8_t PAD_LAYOUT_GUITARFREAKS_COUNT = sizeof(padLayoutGuitarFreaks) / sizeof(PadButton);

const PadButton padLayoutDancePad[] = {
  { GPAD_SELECT, 0, 2 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 0, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_A, 1, 1 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_UP, 1, 3 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_B, 1, 5 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_LEFT, 2, 1 * 6, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 5 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_Y, 3, 1 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_DOWN, 3, 3 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_X, 3, 5 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_DANCEPAD_COUNT = sizeof(padLayoutDancePad) / sizeof(PadButton);

#define PAD_LAYOUT_JVS_COMMON_ENTRIES \
  { GPAD_START,  1, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF }, \
  { GPAD_EXTRA0, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF }, \
  { GPAD_HOME,   3, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF }, \
  { GPAD_UP,     1, 1 * 6, PAD_UP_ON, PAD_UP_OFF }, \
  { GPAD_LEFT,   2, 0, PAD_LEFT_ON, PAD_LEFT_OFF }, \
  { GPAD_RIGHT,  2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF }, \
  { GPAD_DOWN,   3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF }

#define PAD_LAYOUT_JVS_PANEL_6_ENTRIES \
  { GPAD_X,      1, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF }, \
  { GPAD_Y,      1, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF }, \
  { GPAD_R1,     1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF }, \
  { GPAD_A,      3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF }, \
  { GPAD_B,      3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF }, \
  { GPAD_R2,     3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF }

#define PAD_LAYOUT_JVS_PANEL_7_ENTRIES \
  PAD_LAYOUT_JVS_PANEL_6_ENTRIES, \
  { GPAD_L1,     1, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF }

#define PAD_LAYOUT_JVS_PANEL_8_ENTRIES \
  PAD_LAYOUT_JVS_PANEL_7_ENTRIES, \
  { GPAD_L2,     3, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF }

const PadButton padLayoutJvs6[] = {
  PAD_LAYOUT_JVS_COMMON_ENTRIES,
  PAD_LAYOUT_JVS_PANEL_6_ENTRIES
};
const uint8_t PAD_LAYOUT_JVS6_COUNT = sizeof(padLayoutJvs6) / sizeof(PadButton);

const PadButton padLayoutJvs7[] = {
  PAD_LAYOUT_JVS_COMMON_ENTRIES,
  PAD_LAYOUT_JVS_PANEL_7_ENTRIES
};
const uint8_t PAD_LAYOUT_JVS7_COUNT = sizeof(padLayoutJvs7) / sizeof(PadButton);

const PadButton padLayoutJvs8[] = {
  PAD_LAYOUT_JVS_COMMON_ENTRIES,
  PAD_LAYOUT_JVS_PANEL_8_ENTRIES
};
const uint8_t PAD_LAYOUT_JVS8_COUNT = sizeof(padLayoutJvs8) / sizeof(PadButton);

const PadButton padLayoutVirtualJvs[] = {
  PAD_LAYOUT_JVS_COMMON_ENTRIES,
  PAD_LAYOUT_JVS_PANEL_8_ENTRIES
};
const uint8_t PAD_LAYOUT_VIRTUAL_JVS_COUNT =
    sizeof(padLayoutVirtualJvs) / sizeof(PadButton);

#ifdef ENABLE_INPUT_JVS
const PadButton padLayoutJvs[] = {
  PAD_LAYOUT_JVS_COMMON_ENTRIES,
  PAD_LAYOUT_JVS_PANEL_8_ENTRIES
};
const uint8_t PAD_LAYOUT_JVS_COUNT = sizeof(padLayoutJvs) / sizeof(PadButton);
#endif

#undef PAD_LAYOUT_JVS_PANEL_8_ENTRIES
#undef PAD_LAYOUT_JVS_PANEL_7_ENTRIES
#undef PAD_LAYOUT_JVS_PANEL_6_ENTRIES
#undef PAD_LAYOUT_JVS_COMMON_ENTRIES

const PadButton padLayoutPSX[] = {
  { GPAD_L2, 0, 0 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_L1, 0, 2 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R1, 0, 7 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 9 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 1, (4 * 6) - 3, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 1, (5 * 6) + 3, PAD_PS_TRIANGLE_ON, PAD_PS_TRIANGLE_OFF },
  { GPAD_L3, 3, 3 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R3, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_PSX_COUNT = sizeof(padLayoutPSX) / sizeof(PadButton);

const PadButton padLayoutPSXDualShock[] = {
  { GPAD_L2, 0, 0 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_L1, 0, (3 * 6) + 3, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R1, 0, (6 * 6) - 3, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 9 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 2, (4 * 6) - 3, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 2, (5 * 6) + 3, PAD_PS_TRIANGLE_ON, PAD_PS_TRIANGLE_OFF },
  { GPAD_L3, 3, 3 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R3, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_PSX_DUALSHOCK_COUNT = sizeof(padLayoutPSXDualShock) / sizeof(PadButton);

const PadButton padLayoutPS3[] = {
  { GPAD_L2, 0, 0 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_L1, 0, 2 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R1, 0, 7 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 9 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 1, (3 * 6) - 3, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_HOME, 2, (4 * 6) + 1, PAD_WIDE_CIRCLE_LEFT_ON, PAD_WIDE_CIRCLE_LEFT_OFF },
  { GPAD_START, 1, (6 * 6) + 3, PAD_PS_TRIANGLE_ON, PAD_PS_TRIANGLE_OFF },
  { GPAD_L3, 3, 3 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R3, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 9 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_PS3_COUNT = sizeof(padLayoutPS3) / sizeof(PadButton);

}  // namespace menu_pad_layouts_internal
