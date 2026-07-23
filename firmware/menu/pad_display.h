#pragma once

// Pad display system for visual controller representation
// Uses ReflexPad5x7 font characters for button icons

// Button character constants from ReflexPad5x7 font
#define PAD_UP_ON 34       // " - Up pressed
#define PAD_UP_OFF 35      // # - Up released
#define PAD_DOWN_ON 36     // $ - Down pressed
#define PAD_DOWN_OFF 37    // % - Down released
#define PAD_LEFT_ON 38     // & - Left pressed
#define PAD_LEFT_OFF 39    // ' - Left released
#define PAD_RIGHT_ON 40    // ( - Right pressed
#define PAD_RIGHT_OFF 41   // ) - Right released
#define PAD_DPAD_CENTER '*'// * - D-pad center tile
#define PAD_FACE_ON 59     // ; - Face button pressed
#define PAD_FACE_OFF 60    // < - Face button released
#define PAD_SHOULDER_ON 62 // > - Shoulder button pressed (4px tall)
#define PAD_SHOULDER_OFF 63// ? - Shoulder button released (4px tall)
#define PAD_DASH_ON '^'    // Dash button pressed
#define PAD_DASH_OFF '`'   // Dash button released
#define PAD_RECT_ON '{'    // Rectangle button pressed (3px tall)
#define PAD_RECT_OFF '}'   // Rectangle button released (3px tall)
#define PAD_PS_TRIANGLE_ON 131
#define PAD_PS_TRIANGLE_OFF 132
#define PAD_PS_SQUARE_ON 133
#define PAD_PS_SQUARE_OFF 134
#define PAD_WIDE_CIRCLE_LEFT_ON 135
#define PAD_WIDE_CIRCLE_RIGHT_ON 136
#define PAD_WIDE_CIRCLE_LEFT_OFF 137
#define PAD_WIDE_CIRCLE_RIGHT_OFF 138
#define PAD_VERTICAL_RECT_ON 139
#define PAD_VERTICAL_RECT_OFF 140
#define PAD_WIDE_RECT_LEFT_ON 141
#define PAD_WIDE_RECT_RIGHT_ON 142
#define PAD_WIDE_RECT_LEFT_OFF 143
#define PAD_WIDE_RECT_RIGHT_OFF 144
#define PAD_WIDE_TRIANGLE_LEFT_ON 145
#define PAD_WIDE_TRIANGLE_RIGHT_ON 146
#define PAD_WIDE_TRIANGLE_LEFT_OFF 147
#define PAD_WIDE_TRIANGLE_RIGHT_OFF 148
#define PAD_SATURN_START_LEFT_ON 149
#define PAD_SATURN_START_RIGHT_ON 150
#define PAD_SATURN_START_LEFT_OFF 151
#define PAD_SATURN_START_RIGHT_OFF 152

// Pad element definition for layout
struct Pad {
  uint32_t padvalue;  // Bitmask for button
  uint8_t row;        // Display row (0-5 within pad area)
  uint8_t col;        // Column offset in pixels
  char on;            // Character when pressed
  char off;           // Character when released
};

// Pad display division for multi-player layouts
struct PadDivision {
  uint8_t firstCol;
  uint8_t lastCol;
};

// Default pad divisions for 2 player display
const PadDivision padDivision[] = {
  { 0, 10 * 6 },     // Player 1: columns 0-59
  { 11 * 6, 127 }    // Player 2: columns 66-127
};

// Starting row for pad display (after header area)
const uint8_t padDisplayFirstRow = 2;

// Print a single character at specific position
inline void printPadChar(uint8_t col, uint8_t row, char value) {
  display.setCursor(col, row);
  display.print(value);
}

// Print pad button with state tracking to minimize redraws
void printPadButton(uint8_t padIndex, uint8_t startCol, const Pad& pad,
                    uint32_t buttonState, uint32_t* prevState, bool force = false) {
  bool isPressed = buttonState & pad.padvalue;
  bool wasPressed = prevState[padIndex] & pad.padvalue;

  if (isPressed == wasPressed && !force)
    return;

  printPadChar(startCol + pad.col, padDisplayFirstRow + pad.row,
               isPressed ? pad.on : pad.off);

  if (isPressed)
    prevState[padIndex] |= pad.padvalue;
  else
    prevState[padIndex] &= ~pad.padvalue;
}

// Render full pad layout (initial draw)
template<size_t N>
void renderPadLayout(uint8_t padIndex, const Pad (&pads)[N], uint32_t* prevState) {
  const uint8_t startCol = padDivision[padIndex].firstCol;

  // Clear pad display area
  display.clear(padDivision[padIndex].firstCol, padDivision[padIndex].lastCol,
                padDisplayFirstRow, 7);

  // Draw all buttons in released state
  for (uint8_t i = 0; i < N; ++i) {
    printPadChar(startCol + pads[i].col, padDisplayFirstRow + pads[i].row, pads[i].off);
  }

  // Clear previous state
  prevState[padIndex] = 0;
}

// Update pad display with current button state
template<size_t N>
void updatePadDisplay(uint8_t padIndex, const Pad (&pads)[N],
                      uint32_t buttonState, uint32_t* prevState) {
  const uint8_t startCol = padDivision[padIndex].firstCol;

  for (uint8_t i = 0; i < N; ++i) {
    printPadButton(padIndex, startCol, pads[i], buttonState, prevState);
  }
}
