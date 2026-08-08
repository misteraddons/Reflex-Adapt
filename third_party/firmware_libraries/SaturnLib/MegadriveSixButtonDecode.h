#pragma once

#include <stdint.h>

extern "C++" {

namespace saturnlib_megadrive {

inline bool six_button_id_phase(uint8_t page) {
  const uint8_t id = page & 0x0F;
  // The connected OEM MK-1653 can leave D1 high on this page.
  return id == 0x00 || id == 0x02;
}

inline bool six_button_marker_valid(uint8_t page, bool allow_m30_auxiliary) {
  // D3 and D1 are fixed high. Standard pads also keep D2 and D0 high, while
  // the 8BitDo M30 drives D2 low for Star and D0 low for Home.
  const uint8_t fixed_mask = allow_m30_auxiliary ? 0x0A : 0x0F;
  return (page & fixed_mask) == fixed_mask;
}

inline uint8_t m30_aux_control_page(uint8_t marker_page) {
  // Preserve the validated active-low marker nibble so D0 reaches Home and
  // D2 reaches Star without aliasing either control to a Saturn shoulder.
  return marker_page & 0x0F;
}

}  // namespace saturnlib_megadrive

}  // extern "C++"
