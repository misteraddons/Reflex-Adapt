#include "../product_config.h"

#include "menu_home_debug.h"

#include <cstdio>
#include <cstring>

#include "../core/controller_frame_state.h"
#include "../core/device_runtime_state.h"
#include "../input/autodetect/input_autodetect_flags.h"
#include "../input/autodetect/input_autodetect_runtime_state.h"
#include "../platform/display_runtime_state.h"
#include "menu_helpers.h"
#include "menu_pad_button_masks.h"

namespace {

bool isAutoDetectInputModeSelected() {
#ifdef ENABLE_INPUT_AUTODETECT
  return deviceMode == RZORD_AUTODETECT;
#else
  return false;
#endif
}

}  // namespace

bool shouldHideEmptySecondPadInAutoInputMode() {
  if (!isAutoDetectMode || isAutoDetectInputModeSelected() || max_devices <= 1)
    return false;

  // Assisted/passive AUTO routes cannot infer a missing P2, so keep the
  // second input pad visible for multi-pad outputs in that workflow.
  if (input_autodetect_last_flags & INPUT_AUTODETECT_TIMING_PASSIVE)
    return false;

  for (uint8_t i = 1; i < max_devices && i < MAX_USB_OUT; ++i) {
    if (controllerFrameConst(i).connected)
      return false;
  }

  return true;
}

void updateHomeDebugOverlay() {
#ifndef USE_I2C_DISPLAY
  return;
#else
  static char lastLines[4][22] = {{0}};
  auto axisForDisplay = [](int16_t raw) -> int16_t {
    // Normalize 16-bit signed axis formats to an 8-bit-style range for readability.
    if (raw < -255 || raw > 255) return raw / 256;
    return raw;
  };

  for (uint8_t p = 0; p < 2; ++p) {
    char lineA[22];
    char lineB[22];
    bool connected = (p < max_devices) && (p < MAX_USB_OUT) && controllerFrameConst(p).connected;

    if (!connected) {
      snprintf(lineA, sizeof(lineA), "P%u BTN ------", p + 1);
      snprintf(lineB, sizeof(lineB), "L----,---- R----,----");
    } else {
      const controller_state_t& frame = controllerFrameConst(p);
      uint32_t buttons = buildButtonMaskFromReport(p);
      int16_t lx = axisForDisplay(frame.LX);
      int16_t ly = axisForDisplay(frame.LY);
      int16_t rx = axisForDisplay(frame.RX);
      int16_t ry = axisForDisplay(frame.RY);
      snprintf(lineA, sizeof(lineA), "P%u BTN %06lX", p + 1, (unsigned long)(buttons & 0xFFFFFFu));
      snprintf(lineB, sizeof(lineB), "L%4d,%4d R%4d,%4d", lx, ly, rx, ry);
    }

    uint8_t rowA = 3 + (p * 2);
    uint8_t rowB = rowA + 1;
    uint8_t lineIndexA = p * 2;
    uint8_t lineIndexB = lineIndexA + 1;

    if (strcmp(lastLines[lineIndexA], lineA) != 0) {
      clearRow(rowA);
      display.setRow(rowA);
      display.setCol(0);
      display.print(lineA);
      strncpy(lastLines[lineIndexA], lineA, sizeof(lastLines[lineIndexA]) - 1);
      lastLines[lineIndexA][sizeof(lastLines[lineIndexA]) - 1] = '\0';
    }

    if (strcmp(lastLines[lineIndexB], lineB) != 0) {
      clearRow(rowB);
      display.setRow(rowB);
      display.setCol(0);
      display.print(lineB);
      strncpy(lastLines[lineIndexB], lineB, sizeof(lastLines[lineIndexB]) - 1);
      lastLines[lineIndexB][sizeof(lastLines[lineIndexB]) - 1] = '\0';
    }
  }
#endif
}
