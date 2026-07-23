#include "../product_config.h"

#include "menu_pad_layouts.h"

#include "../platform/display_runtime_state.h"

namespace {

inline bool isDirectionalPadGlyph(const PadButton& button, char onChar, char offChar) {
  return button.on == onChar && button.off == offChar;
}

}  // namespace

void drawDpadCentersForLayout(const PadButton* layout, uint8_t count, uint8_t colOffset, uint8_t rowOffset) {
  drawDpadCentersForLayoutState(layout, count, 0, colOffset, rowOffset);
}

void drawDpadCentersForLayoutState(const PadButton* layout, uint8_t count, uint32_t state, uint8_t colOffset, uint8_t rowOffset) {
#ifndef USE_I2C_DISPLAY
  (void)layout;
  (void)count;
  (void)state;
  (void)colOffset;
  (void)rowOffset;
  return;
#else
  if (!layout) return;

  for (uint8_t i = 0; i < count; ++i) {
    if (!isDirectionalPadGlyph(layout[i], PAD_UP_ON, PAD_UP_OFF)) continue;

    const uint8_t centerRow = layout[i].row + 1;
    const uint8_t centerCol = layout[i].col;
    bool hasDown = false;
    bool hasLeft = false;
    bool hasRight = false;
    uint32_t leftMask = 0;
    uint32_t rightMask = 0;

    for (uint8_t j = 0; j < count; ++j) {
      if (isDirectionalPadGlyph(layout[j], PAD_DOWN_ON, PAD_DOWN_OFF) &&
          layout[j].row == centerRow + 1 && layout[j].col == centerCol) {
        hasDown = true;
      } else if (isDirectionalPadGlyph(layout[j], PAD_LEFT_ON, PAD_LEFT_OFF) &&
                 layout[j].row == centerRow && layout[j].col + 6 == centerCol) {
        hasLeft = true;
        leftMask |= layout[j].mask;
      } else if (isDirectionalPadGlyph(layout[j], PAD_RIGHT_ON, PAD_RIGHT_OFF) &&
                 layout[j].row == centerRow && layout[j].col == centerCol + 6) {
        hasRight = true;
        rightMask |= layout[j].mask;
      }
    }

    if (hasDown && hasLeft && hasRight) {
      display.setCursor(centerCol + colOffset, centerRow + rowOffset);
      display.print(PAD_DPAD_CENTER);

      // Fill the single spacing columns between the 5x7 direction glyphs so the
      // d-pad outline reads as one continuous cross around the center dot.
      const uint8_t connectorRow = centerRow + rowOffset;
      const uint8_t centerDisplayCol = centerCol + colOffset;
      const uint8_t rightConnectorCol = centerDisplayCol + 5;
      const uint8_t leftConnector = (state & leftMask) ? 0xFF : 0x81;
      const uint8_t rightConnector = (state & rightMask) ? 0xFF : 0x81;
      if (centerDisplayCol > 0) {
        display.setCursor(centerDisplayCol - 1, connectorRow);
        display.ssd1306WriteRamBuf(leftConnector);
      }
      if (rightConnectorCol < 128) {
        display.setCursor(rightConnectorCol, connectorRow);
        display.ssd1306WriteRamBuf(rightConnector);
      }
    }
  }
#endif
}
