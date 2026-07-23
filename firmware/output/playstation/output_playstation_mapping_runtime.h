#pragma once

// Internal PlayStation-family output mapping helpers. This stays header-only so
// it can reuse the shared runtime flags, output reports, and conversion helpers
// still owned by out_usb.h.

inline uint32_t playstationFaceButtons(const controller_state_t& frame) {
  uint32_t buttons = applyNintendoPositionButtonMap(frame.digital_buttons, isPositionButtonMapActive());
  return output_apply_n64_c_buttons_to_face_buttons(buttons, frame);
}

#include "output_ps3_mapping_runtime.h"
#include "output_ps4_mapping_runtime.h"
#include "output_ps5_general_mapping_runtime.h"
