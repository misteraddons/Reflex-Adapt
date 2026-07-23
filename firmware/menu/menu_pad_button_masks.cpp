#include "../product_config.h"

#include "menu_pad_button_masks.h"

#include "../core/controller_frame_state.h"
#include "../core/controller_output_cache_state.h"
#include "../core/device_runtime_state.h"
#include "../input/shared/input_button_bits.h"
#include "../output/output_runtime_state.h"

#include <cstring>

namespace {

void addVirtualBoyOledMask(const controller_state_t& r, uint32_t& mask) {
#ifdef ENABLE_INPUT_VBOY
  const bool isVirtualBoyFrame =
    deviceMode == RZORD_VBOY ||
    std::strcmp(r.controller_type_name, "VB Pad") == 0;
  if (!isVirtualBoyFrame) {
    return;
  }

  if (r.A)  mask |= 0x0010;
  if (r.B)  mask |= 0x0020;
  if (r.RY < 0) mask |= 0x0040;
  if (r.RX < 0) mask |= 0x0080;
  if (r.RY > 0) mask |= 0x0400;
  if (r.RX > 0) mask |= 0x0800;
#else
  (void)r;
  (void)mask;
#endif
}

}  // namespace

uint32_t buildButtonMaskFromDigitalButtons(uint32_t buttons) {
  uint32_t mask = 0;
  if (buttons & INPUT_PAD_U) mask |= 0x0001;
  if (buttons & INPUT_PAD_D) mask |= 0x0002;
  if (buttons & INPUT_PAD_L) mask |= 0x0004;
  if (buttons & INPUT_PAD_R) mask |= 0x0008;
  if (buttons & INPUT_A)     mask |= 0x0010;
  if (buttons & INPUT_B)     mask |= 0x0020;
  if (buttons & INPUT_X)     mask |= 0x0040;
  if (buttons & INPUT_Y)     mask |= 0x0080;
  if (buttons & INPUT_L1)    mask |= 0x0100;
  if (buttons & INPUT_R1)    mask |= 0x0200;
  if (buttons & INPUT_L2)    mask |= 0x0400;
  if (buttons & INPUT_R2)    mask |= 0x0800;
  if (buttons & INPUT_START) mask |= 0x1000;
  if (buttons & INPUT_SELECT) mask |= 0x2000;
  if (buttons & INPUT_L3)    mask |= 0x4000;
  if (buttons & INPUT_R3)    mask |= 0x8000;
  if (buttons & INPUT_HOME)  mask |= 0x10000;
  if (buttons & INPUT_CAPTURE) mask |= 0x20000;
  return mask;
}

uint32_t buildButtonMaskFromReport(uint8_t index) {
  if (index >= MAX_USB_OUT) return 0;
  const controller_state_t& r = controllerFrameConst(index);
  uint32_t mask = buildButtonMaskFromDigitalButtons(r.digital_buttons);

  #ifdef ENABLE_INPUT_N64
  if (deviceMode == RZORD_N64) {
    if (is_nso_special_active()) {
      if (r.X)      mask |= 0x0040;
      if (r.R2)     mask |= 0x0800;
      if (r.Y)      mask |= 0x0080;
      if (r.SELECT) mask |= 0x2000;
    } else {
      if (r.RY < 0) mask |= 0x0040;
      if (r.RY > 0) mask |= 0x0800;
      if (r.RX < 0) mask |= 0x0080;
      if (r.RX > 0) mask |= 0x2000;
      // In N64 C-button-as-button mode the input frame carries C-down/C-right
      // on L3/R3 to avoid colliding with A/B. The OLED N64 glyph still draws
      // those positions as R2/Select, so mirror them here for display only.
      if (r.L3) mask |= 0x0800;
      if (r.R3) mask |= 0x2000;
    }
  }
  #endif

  #ifdef ENABLE_INPUT_GAMECUBE
  if (deviceMode == RZORD_GAMECUBE && r.R1) {
    // GameCube Z is carried as R1 in the input frame. The compact GameCube
    // layout reserves the Select display slot for its dedicated Z glyph.
    mask |= 0x2000;
  }
  #endif

  if (r.EXTRA & 0x01) mask |= 0x20000;
  if (r.EXTRA & 0x02) mask |= 0x40000;
  if (r.EXTRA & 0x04) mask |= 0x80000;
  if (r.EXTRA & 0x08) mask |= 0x100000;
  if (r.EXTRA & 0x10) mask |= 0x200000;
  addVirtualBoyOledMask(r, mask);

  return mask;
}

uint32_t buildButtonMaskFromOutputState(uint8_t index) {
  return buildButtonMaskFromDigitalButtons(post_remap_buttons[index]);
}
