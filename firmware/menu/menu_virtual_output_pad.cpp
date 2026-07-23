#include "../product_config.h"

#include "menu_virtual_output_pad.h"
#include "menu_pad_layouts_internal.h"
#ifdef ENABLE_INPUT_JVS
#include "../input/jvs/jvs_host_runtime.h"
#endif

namespace {

using namespace menu_pad_layouts_internal;

const PadButton padLayoutModernOutput[] = {
  { GPAD_L2, 0, 0*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_L1, 0, 3*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R1, 0, 6*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_R2, 0, 9*6, PAD_SHOULDER_ON, PAD_SHOULDER_OFF },
  { GPAD_UP, 1, 1*6, PAD_UP_ON, PAD_UP_OFF },
  { GPAD_DOWN, 3, 1*6, PAD_DOWN_ON, PAD_DOWN_OFF },
  { GPAD_LEFT, 2, 0, PAD_LEFT_ON, PAD_LEFT_OFF },
  { GPAD_RIGHT, 2, 2*6, PAD_RIGHT_ON, PAD_RIGHT_OFF },
  { GPAD_SELECT, 1, 3*6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_HOME, 1, (4*6) + 1, PAD_WIDE_CIRCLE_LEFT_ON, PAD_WIDE_CIRCLE_LEFT_OFF },
  { GPAD_START, 1, 6*6, PAD_RECT_ON, PAD_RECT_OFF },
  { GPAD_L3, 3, 3*6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_R3, 3, 6*6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_Y, 1, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_X, 2, 7*6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_B, 2, 9*6, PAD_FACE_ON, PAD_FACE_OFF },
  { GPAD_A, 3, 8*6, PAD_FACE_ON, PAD_FACE_OFF },
};

const uint8_t PAD_LAYOUT_MODERN_OUTPUT_COUNT =
    sizeof(padLayoutModernOutput) / sizeof(PadButton);

bool isModernVirtualOutputMode(outputMode_t outputMode) {
  switch (outputMode) {
    case OUTPUT_HID:
    case OUTPUT_MISTER:
    case OUTPUT_MISTER_JOGCON:
    case OUTPUT_RESERVED_JOGCON:
    case OUTPUT_MISTER_NEGCON:
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XBOXONE:
    case OUTPUT_XID:
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
    case OUTPUT_PANTHERLORD:
    case OUTPUT_GCWIIU:
    case OUTPUT_KEYBOARD:
    #ifdef ENABLE_OUTPUT_JVS
    case OUTPUT_JVS:
    #endif
      return true;
    default:
      return false;
  }
}

bool shouldUseArcadeVirtualOutputPad(DeviceEnum inputMode, outputMode_t outputMode) {
  return (inputMode == RZORD_JVS)
      #ifdef ENABLE_OUTPUT_JVS
      || (outputMode == OUTPUT_JVS)
      #endif
      ;
}

char lowerAscii(char value) {
  return (value >= 'A' && value <= 'Z') ? (char)(value + ('a' - 'A')) : value;
}

void getArcadeVirtualOutputPadLayout(DeviceEnum inputMode, const PadButton** layout, uint8_t* layoutCount) {
  #ifdef ENABLE_INPUT_JVS
  if (inputMode == RZORD_JVS) {
    switch (getJvsPhysicalButtonsPerPlayer(getJvsBoardInfo())) {
      case 6:
        *layout = padLayoutJvs6;
        *layoutCount = PAD_LAYOUT_JVS6_COUNT;
        return;
      case 7:
        *layout = padLayoutJvs7;
        *layoutCount = PAD_LAYOUT_JVS7_COUNT;
        return;
      case 0:
      case 8:
      default:
        *layout = padLayoutJvs8;
        *layoutCount = PAD_LAYOUT_JVS8_COUNT;
        return;
    }
  }
  #endif

  *layout = padLayoutJvs8;
  *layoutCount = PAD_LAYOUT_JVS8_COUNT;
}

char genericButtonLabel(uint32_t mask) {
  switch (mask) {
    case GPAD_A: return 'A';
    case GPAD_B: return 'B';
    case GPAD_X: return 'X';
    case GPAD_Y: return 'Y';
    case GPAD_L1: return 'L';
    case GPAD_R1: return 'R';
    case GPAD_L2: return 'Z';
    case GPAD_R2: return 'T';
    case GPAD_L3: return '3';
    case GPAD_R3: return '4';
    case GPAD_SELECT: return 'S';
    case GPAD_START: return '+';
    case GPAD_HOME: return 'H';
    case GPAD_EXTRA0: return 'C';
    default: return '\0';
  }
}

char xboxButtonLabel(uint32_t mask) {
  switch (mask) {
    case GPAD_L2: return 'L';
    case GPAD_R2: return 'R';
    case GPAD_SELECT: return 'B';
    case GPAD_HOME: return 'G';
    default: return genericButtonLabel(mask);
  }
}

char xidButtonLabel(uint32_t mask) {
  switch (mask) {
    case GPAD_L1: return 'K';  // Black
    case GPAD_R1: return 'W';  // White
    case GPAD_L2: return 'L';
    case GPAD_R2: return 'R';
    case GPAD_SELECT: return 'B';
    default: return genericButtonLabel(mask);
  }
}

char playstationButtonLabel(uint32_t mask) {
  switch (mask) {
    case GPAD_A: return 'X';   // Cross
    case GPAD_B: return 'O';   // Circle
    case GPAD_X: return 'S';   // Square
    case GPAD_Y: return 'T';   // Triangle
    case GPAD_L2: return '2';
    case GPAD_R2: return '3';
    case GPAD_SELECT: return 'S';
    case GPAD_START: return '+';
    case GPAD_HOME: return 'P';
    default: return genericButtonLabel(mask);
  }
}

char switchButtonLabel(uint32_t mask) {
  switch (mask) {
    case GPAD_A: return 'B';
    case GPAD_B: return 'A';
    case GPAD_X: return 'Y';
    case GPAD_Y: return 'X';
    case GPAD_L2: return 'Z';
    case GPAD_R2: return 'R';
    case GPAD_SELECT: return '-';
    case GPAD_START: return '+';
    case GPAD_HOME: return 'H';
    case GPAD_EXTRA0: return 'C';
    default: return genericButtonLabel(mask);
  }
}

char keyboardButtonLabel(uint32_t mask) {
  switch (mask) {
    case GPAD_X: return 'C';   // Left Ctrl
    case GPAD_Y: return 'A';   // Left Alt
    case GPAD_R1: return 'S';  // Space
    case GPAD_L1: return 'L';  // Left Shift
    case GPAD_A: return 'Z';
    case GPAD_B: return 'X';
    case GPAD_L2: return 'V';
    case GPAD_R2: return 'C';
    case GPAD_START: return '1';
    case GPAD_EXTRA0: return '5';
    case GPAD_SELECT: return '9';
    case GPAD_HOME: return 'F';
    default: return genericButtonLabel(mask);
  }
}

}  // namespace

bool shouldShowVirtualOutputPad(DeviceEnum inputMode, outputMode_t outputMode) {
  return shouldUseArcadeVirtualOutputPad(inputMode, outputMode) ||
         isModernVirtualOutputMode(outputMode);
}

void getVirtualOutputPadLayout(DeviceEnum inputMode, outputMode_t outputMode,
                               const PadButton** layout, uint8_t* layoutCount) {
  if (shouldUseArcadeVirtualOutputPad(inputMode, outputMode)) {
    getArcadeVirtualOutputPadLayout(inputMode, layout, layoutCount);
    return;
  }

  if (isModernVirtualOutputMode(outputMode)) {
    *layout = padLayoutModernOutput;
    *layoutCount = PAD_LAYOUT_MODERN_OUTPUT_COUNT;
    return;
  }

  *layout = nullptr;
  *layoutCount = 0;
}

char getVirtualOutputButtonLabel(outputMode_t outputMode, uint32_t mask) {
  switch (outputMode) {
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XBOXONE:
    case OUTPUT_HID:
    case OUTPUT_MISTER:
      return xboxButtonLabel(mask);
    case OUTPUT_XID:
      return xidButtonLabel(mask);
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
      return playstationButtonLabel(mask);
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
      return switchButtonLabel(mask);
    case OUTPUT_KEYBOARD:
      return keyboardButtonLabel(mask);
    default:
      return genericButtonLabel(mask);
  }
}

char getVirtualOutputButtonGlyph(outputMode_t outputMode, uint32_t mask, bool pressed) {
  switch (outputMode) {
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
      switch (mask) {
        case GPAD_X:
          return (char)(pressed ? PAD_PS_SQUARE_ON : PAD_PS_SQUARE_OFF);
        case GPAD_Y:
          return (char)(pressed ? PAD_PS_TRIANGLE_ON : PAD_PS_TRIANGLE_OFF);
        default:
          break;
      }
      break;
    default:
      break;
  }

  const char label = getVirtualOutputButtonLabel(outputMode, mask);
  if (label == '\0') {
    return '\0';
  }
  return pressed ? label : lowerAscii(label);
}
