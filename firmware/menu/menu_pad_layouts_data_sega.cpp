#include "../product_config.h"

#include "menu_pad_layouts_internal.h"

namespace menu_pad_layouts_internal {

const PadButton padLayoutSaturn[] = {
  { GPAD_L2, 0, 1 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 7 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_START, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_X, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L1, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R1, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_SATURN_COUNT = sizeof(padLayoutSaturn) / sizeof(PadButton);

const PadButton padLayoutSaturn3D[] = {
  { GPAD_L2, 0, 1 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 7 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_START, 2, 4 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L1, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R1, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_SATURN3D_COUNT = sizeof(padLayoutSaturn3D) / sizeof(PadButton);

#ifdef ENABLE_INPUT_MEGADRIVE
const PadButton padLayoutGenesis6[] = {
  { GPAD_R2, 0, 7 * 6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_START, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_X, 2, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_L1, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R1, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_GENESIS6_COUNT = sizeof(padLayoutGenesis6) / sizeof(PadButton);

const PadButton padLayoutGenesis3[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_START, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_A, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R1, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_GENESIS3_COUNT = sizeof(padLayoutGenesis3) / sizeof(PadButton);
#endif

#ifdef ENABLE_INPUT_SMS
const PadButton padLayoutSMS[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_A, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
};
const uint8_t PAD_LAYOUT_SMS_COUNT = sizeof(padLayoutSMS) / sizeof(PadButton);
#endif

#ifdef ENABLE_INPUT_JPC
const PadButton padLayoutJPC[] = {
  { GPAD_UP, 1, 1 * 6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1 * 6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2 * 6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_A, 3, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 3, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_SELECT, 2, 4 * 6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_START, 2, 5 * 6, PAD_RECT_ON, PAD_RECT_OFF },
};
const uint8_t PAD_LAYOUT_JPC_COUNT = sizeof(padLayoutJPC) / sizeof(PadButton);
#endif

}  // namespace menu_pad_layouts_internal
