#include "../product_config.h"

#include "menu.h"
#include "menu_main_display_internal.h"
#include "pad_display.h"

#include "../core/controller_settings_state.h"
#include "../core/controller_output_cache_state.h"
#include "../core/turbo.h"
#include "../input/runtime/input_adapter_runtime.h"
#include "../input/runtime/input_frame_runtime.h"
#include "../input/shared/input_button_bits.h"
#include "../output/auth/auth_storage.h"
#include "../output/output_capabilities.h"
#include "menu_runtime_state.h"

#ifdef ENABLE_INPUT_JVS
#include "../input/jvs/jvs_host_runtime.h"
#endif

#include <cstdio>
#include <cstring>

namespace menu_main_display_internal {

namespace {

constexpr uint32_t kJoystickDirectionMask =
  GPAD_UP | GPAD_DOWN | GPAD_LEFT | GPAD_RIGHT;
constexpr uint8_t kPadGlyphWidth = 6;
constexpr uint8_t kWideCircleRightHalfOffset = 5;
constexpr uint8_t kHomePadRowOffset = 3;
constexpr uint8_t kSecondPadColOffset = 66;
constexpr uint8_t kCenteredPadColOffset = 34;
constexpr uint8_t kHome1pViewInputOutput = 1;
constexpr uint8_t kAnalogTriggerGaugeMaxWidth = 18;
constexpr uint8_t kAnalogTriggerGaugeMinWidth = 6;
constexpr uint8_t kAnalogTriggerGaugeEmpty = 0x22;
constexpr uint8_t kAnalogTriggerGaugeFilled = 0x3E;
constexpr uint8_t kAnalogTriggerGaugeRedrawMs = 16;
constexpr uint8_t kDreamcastWheelTriggerGaugeWidth = 26;
constexpr uint8_t kDreamcastWheelButtonPlusCol = 54;
constexpr uint8_t kDreamcastWheelRightEdge =
  kDreamcastWheelButtonPlusCol + kPadGlyphWidth - 1;
constexpr uint8_t kDreamcastWheelPanelWidth = kDreamcastWheelRightEdge + 1;
constexpr uint8_t kSpinnerPaddlePanelWidth =
  kDreamcastWheelPanelWidth - kPadGlyphWidth;
constexpr uint8_t kDreamcastWheelRightTriggerCol =
  kDreamcastWheelRightEdge + 1 - kDreamcastWheelTriggerGaugeWidth;
constexpr uint8_t kDreamcastWheelAxisRow = 1;
constexpr uint8_t kDreamcastWheelButtonsRow = 2;
constexpr uint8_t kSaturnWheelPanelWidth = 62;
constexpr uint8_t kSaturnWheelTriggerGaugeWidth = kDreamcastWheelTriggerGaugeWidth;
constexpr uint8_t kSaturnWheelRightTriggerCol =
  kSaturnWheelPanelWidth - kSaturnWheelTriggerGaugeWidth;
constexpr uint8_t kSaturnWheelAxisRow = 1;
constexpr uint8_t kSaturnWheelButtonsRow = 2;
constexpr uint8_t kTriggerBothGlyphInsetChars = 2;
constexpr uint16_t kTurboVisualSlowMs = 9 * FRAME_MS;
constexpr uint16_t kTurboVisualMediumMs = 6 * FRAME_MS;
constexpr uint16_t kTurboVisualFastMs = 3 * FRAME_MS;
constexpr uint16_t kTurboVisualMaxMs = 2 * FRAME_MS;

bool isJoystickDirectionMask(uint32_t mask) {
  return mask == GPAD_UP ||
         mask == GPAD_DOWN ||
         mask == GPAD_LEFT ||
         mask == GPAD_RIGHT;
}

bool isAnalogTriggerMask(uint32_t mask) {
  return mask == GPAD_L2 || mask == GPAD_R2;
}

bool suppressAnalogTriggerGaugeForFrame(const controller_state_t* frame) {
  (void)frame;
  return false;
}

uint32_t rawInputMaskForTurboButton(TurboButton btn) {
  switch (btn) {
    case TURBO_BTN_0: return INPUT_A;
    case TURBO_BTN_1: return INPUT_B;
    case TURBO_BTN_2: return INPUT_X;
    case TURBO_BTN_3: return INPUT_Y;
    case TURBO_BTN_4: return INPUT_L1;
    case TURBO_BTN_5: return INPUT_R1;
    case TURBO_BTN_6: return INPUT_L2;
    case TURBO_BTN_7: return INPUT_R2;
    case TURBO_BTN_8: return INPUT_START;
    case TURBO_BTN_9: return INPUT_SELECT;
    case TURBO_BTN_10: return INPUT_L3;
    case TURBO_BTN_11: return INPUT_R3;
    case TURBO_BTN_12: return INPUT_EXTRA1;
    case TURBO_BTN_13: return INPUT_EXTRA4;
    case TURBO_BTN_14: return INPUT_EXTRA2;
    case TURBO_BTN_15: return INPUT_EXTRA0;
    case TURBO_BTN_16: return INPUT_EXTRA3;
    default:          return 0;
  }
}

uint32_t gpadMaskForTurboButton(TurboButton btn) {
  switch (btn) {
    case TURBO_BTN_0: return GPAD_A;
    case TURBO_BTN_1: return GPAD_B;
    case TURBO_BTN_2: return GPAD_X;
    case TURBO_BTN_3: return GPAD_Y;
    case TURBO_BTN_4: return GPAD_L1;
    case TURBO_BTN_5: return GPAD_R1;
    case TURBO_BTN_6: return GPAD_L2;
    case TURBO_BTN_7: return GPAD_R2;
    case TURBO_BTN_8: return GPAD_START;
    case TURBO_BTN_9: return GPAD_SELECT;
    case TURBO_BTN_10: return GPAD_L3;
    case TURBO_BTN_11: return GPAD_R3;
    case TURBO_BTN_12: return GPAD_EXTRA1;
    case TURBO_BTN_13: return GPAD_EXTRA4;
    case TURBO_BTN_14: return GPAD_EXTRA2;
    case TURBO_BTN_15: return GPAD_EXTRA0;
    case TURBO_BTN_16: return GPAD_EXTRA3;
    default:          return 0;
  }
}

uint16_t turboVisualIntervalMs(TurboRate rate) {
  switch (getTurboBaseRate(rate)) {
    case TURBO_SLOW:   return kTurboVisualSlowMs;
    case TURBO_MEDIUM: return kTurboVisualMediumMs;
    case TURBO_FAST:   return kTurboVisualFastMs;
    case TURBO_MAX:    return kTurboVisualMaxMs;
    case TURBO_ULTRA:  return kTurboVisualMaxMs;
    default:           return 0;
  }
}

bool turboVisualPressed(TurboRate rate) {
  const uint16_t interval = turboVisualIntervalMs(rate);
  if (interval == 0) {
    return true;
  }
  return ((millis() / interval) & 1u) == 0u;
}

uint32_t applyTurboVisualState(uint8_t device, uint32_t state) {
  if (!turbo.hasAnyEnabled() || device >= MAX_USB_OUT) {
    return state;
  }

  const uint32_t held = turbo.getRawHeldButtons(device);
  if (held == 0) {
    return state;
  }

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    const TurboRate rate = turbo.getButtonRate((TurboButton)i);
    if (rate == TURBO_OFF) {
      continue;
    }

    const uint32_t rawMask = rawInputMaskForTurboButton((TurboButton)i);
    const uint32_t gpadMask = gpadMaskForTurboButton((TurboButton)i);
    if (rawMask == 0 || gpadMask == 0 || (held & rawMask) == 0) {
      continue;
    }

    if (turboVisualPressed(rate)) {
      state |= gpadMask;
    } else {
      state &= ~gpadMask;
    }
  }
  return state;
}

bool shouldDrawAnalogTriggerGauge(const controller_state_t* frame,
                                  uint32_t mask) {
  return frame != nullptr &&
         frame->connected &&
         frame->HAS_ANALOG_TRIGGERS &&
         trigger_mode != TRIGGER_MODE_DIGITAL &&
         !suppressAnalogTriggerGaugeForFrame(frame) &&
         isAnalogTriggerMask(mask);
}

bool isAnalogTriggerGaugeButton(const PadButton& button) {
  return isAnalogTriggerMask(button.mask) &&
         button.on == PAD_SHOULDER_ON &&
         button.off == PAD_SHOULDER_OFF;
}

bool hasSeparateDigitalAnalogTriggerButton(const PadButton& button,
                                           const PadButton* layout,
                                           uint8_t layoutCount) {
  if (!isAnalogTriggerGaugeButton(button) || layout == nullptr) {
    return false;
  }
  for (uint8_t i = 0; i < layoutCount; ++i) {
    const PadButton& candidate = layout[i];
    if (&candidate != &button &&
        candidate.mask == button.mask &&
        !isAnalogTriggerGaugeButton(candidate)) {
      return true;
    }
  }
  return false;
}

uint8_t analogTriggerValue(const controller_state_t* frame,
                           uint32_t mask) {
  if (!shouldDrawAnalogTriggerGauge(frame, mask)) {
    return 0;
  }
  return (mask == GPAD_L2) ? frame->ANALOG_L2 : frame->ANALOG_R2;
}

uint8_t analogTriggerGaugeInnerWidth(uint8_t gaugeWidth) {
  return (gaugeWidth > 2) ? (gaugeWidth - 2) : 1;
}

uint8_t analogTriggerGaugeColumns(const controller_state_t* frame,
                                  uint32_t mask,
                                  uint8_t gaugeWidth) {
  const uint8_t value = analogTriggerValue(frame, mask);
  if (value == 0) {
    return 0;
  }

  const uint8_t innerWidth = analogTriggerGaugeInnerWidth(gaugeWidth);
  if (value == 255) {
    return innerWidth;
  }
  uint8_t columns = ((uint16_t)value * innerWidth) / 256;
  return columns == 0 ? 1 : columns;
}

uint8_t analogTriggerGaugeWidthForButton(const PadButton* layout,
                                         uint8_t layoutCount,
                                         const PadButton& button) {
  uint8_t availableWidth = kAnalogTriggerGaugeMaxWidth;

  if (button.mask == GPAD_L2) {
    uint8_t nearestCol = 255;
    for (uint8_t i = 0; i < layoutCount; ++i) {
      if (layout[i].row == button.row && layout[i].col > button.col &&
          layout[i].col < nearestCol) {
        nearestCol = layout[i].col;
      }
    }
    if (nearestCol != 255) {
      availableWidth = nearestCol - button.col;
    }
  } else if (button.mask == GPAD_R2) {
    int16_t nearestEnd = -1;
    for (uint8_t i = 0; i < layoutCount; ++i) {
      if (layout[i].row == button.row && layout[i].col < button.col) {
        const int16_t candidateEnd = (int16_t)layout[i].col + kPadGlyphWidth;
        if (candidateEnd > nearestEnd) {
          nearestEnd = candidateEnd;
        }
      }
    }
    const int16_t buttonEnd = (int16_t)button.col + kPadGlyphWidth;
    if (nearestEnd >= 0 && nearestEnd < buttonEnd) {
      availableWidth = (uint8_t)(buttonEnd - nearestEnd);
    }
  }

  if (availableWidth > kAnalogTriggerGaugeMaxWidth) {
    availableWidth = kAnalogTriggerGaugeMaxWidth;
  }
  if (availableWidth < kAnalogTriggerGaugeMinWidth) {
    availableWidth = kAnalogTriggerGaugeMinWidth;
  }
  return availableWidth;
}

uint8_t analogTriggerGaugeWidthForMask(const PadButton* layout,
                                       uint8_t layoutCount,
                                       uint32_t mask) {
  if (layout == nullptr) {
    return kAnalogTriggerGaugeMinWidth;
  }
  for (uint8_t i = 0; i < layoutCount; ++i) {
    if (layout[i].mask == mask && isAnalogTriggerGaugeButton(layout[i])) {
      return analogTriggerGaugeWidthForButton(layout, layoutCount, layout[i]);
    }
  }
  return kAnalogTriggerGaugeMinWidth;
}

bool renderPsxTriggerRawDebugLine(const controller_state_t* frame, bool force) {
#ifndef USE_I2C_DISPLAY
  (void)frame;
  (void)force;
  return false;
#else
#if defined(ENABLE_INPUT_PSX) && defined(ENABLE_PSX_TRIGGER_RAW_OLED_DEBUG)
  static bool lastVisible = false;
  static uint8_t lastL2 = 0xFF;
  static uint8_t lastR2 = 0xFF;

  const bool visible =
    deviceMode == RZORD_PSX &&
    frame != nullptr &&
    frame->connected &&
    frame->HAS_ANALOG_TRIGGERS;
  if (!visible) {
    if (lastVisible || force) {
      clearRow(7);
    }
    lastVisible = false;
    lastL2 = 0xFF;
    lastR2 = 0xFF;
    return false;
  }

  if (!force && lastVisible &&
      frame->ANALOG_L2 == lastL2 &&
      frame->ANALOG_R2 == lastR2) {
    return false;
  }

  char line[22];
  std::snprintf(line, sizeof(line), "RAW L2:%03u R2:%03u",
                (unsigned)frame->ANALOG_L2,
                (unsigned)frame->ANALOG_R2);
  clearRow(7);
  display.setFont(System5x7);
  display.setRow(7);
  display.setCol(0);
  display.print(line);

  lastVisible = true;
  lastL2 = frame->ANALOG_L2;
  lastR2 = frame->ANALOG_R2;
  return true;
#else
  (void)frame;
  (void)force;
  return false;
#endif
#endif
}

bool isWidePadLeftGlyph(char glyph) {
  const uint8_t value = static_cast<uint8_t>(glyph);
  return value == PAD_WIDE_CIRCLE_LEFT_ON ||
         value == PAD_WIDE_CIRCLE_LEFT_OFF ||
         value == PAD_WIDE_RECT_LEFT_ON ||
         value == PAD_WIDE_RECT_LEFT_OFF ||
         value == PAD_WIDE_TRIANGLE_LEFT_ON ||
         value == PAD_WIDE_TRIANGLE_LEFT_OFF;
}

char widePadRightGlyph(char leftGlyph, bool pressed) {
  const uint8_t value = static_cast<uint8_t>(leftGlyph);
  if (value == PAD_WIDE_RECT_LEFT_ON ||
      value == PAD_WIDE_RECT_LEFT_OFF) {
    return static_cast<char>(pressed
      ? PAD_WIDE_RECT_RIGHT_ON
      : PAD_WIDE_RECT_RIGHT_OFF);
  }
  if (value == PAD_WIDE_TRIANGLE_LEFT_ON ||
      value == PAD_WIDE_TRIANGLE_LEFT_OFF) {
    return static_cast<char>(pressed
      ? PAD_WIDE_TRIANGLE_RIGHT_ON
      : PAD_WIDE_TRIANGLE_RIGHT_OFF);
  }
  return static_cast<char>(pressed
    ? PAD_WIDE_CIRCLE_RIGHT_ON
    : PAD_WIDE_CIRCLE_RIGHT_OFF);
}

char displayLabelChar(outputMode_t outputMode, uint32_t mask, bool pressed) {
  return getVirtualOutputButtonGlyph(outputMode, mask, pressed);
}

void drawAnalogTriggerGauge(const PadButton& button,
                            const controller_state_t* frame,
                            uint8_t colOffset,
                            uint8_t rowOffset,
                            uint8_t gaugeWidth) {
  const uint8_t filledColumns =
    analogTriggerGaugeColumns(frame, button.mask, gaugeWidth);

  int16_t startCol = (int16_t)button.col + colOffset;
  if (button.mask == GPAD_R2) {
    startCol += (int16_t)kPadGlyphWidth - gaugeWidth;
  }
  if (startCol < 0) {
    startCol = 0;
  }
  if (startCol > (int16_t)(128 - gaugeWidth)) {
    startCol = 128 - gaugeWidth;
  }

  display.setCursor((uint8_t)startCol, button.row + rowOffset);
  const uint8_t innerWidth = analogTriggerGaugeInnerWidth(gaugeWidth);
  for (uint8_t i = 0; i < gaugeWidth; ++i) {
    uint8_t column = kAnalogTriggerGaugeEmpty;
    if (i == 0 || i == gaugeWidth - 1) {
      column = kAnalogTriggerGaugeFilled;
    } else {
      const uint8_t interiorIndex = i - 1;
      const bool fillFromRight = button.mask == GPAD_L2;
      const bool filled =
        fillFromRight
          ? ((innerWidth - 1 - interiorIndex) < filledColumns)
          : (interiorIndex < filledColumns);
      if (filled) {
        column = kAnalogTriggerGaugeFilled;
      }
    }
    display.ssd1306WriteRamBuf(column);
  }
}

uint8_t wheelPanelClearEnd(uint8_t colOffset) {
  const uint16_t end =
    (uint16_t)colOffset + kDreamcastWheelPanelWidth + kPadGlyphWidth - 1;
  return end > 127 ? 127 : (uint8_t)end;
}

void clearWheelPanelRow(uint8_t colOffset, uint8_t row) {
  display.clear(colOffset, wheelPanelClearEnd(colOffset), row, row);
}

bool isDreamcastWheelFrame(const controller_state_t* frame) {
  if (frame == nullptr || !frame->connected) {
    return false;
  }
  return std::strncmp(frame->controller_type_name, "Whl", 3) == 0 ||
         std::strcmp(frame->controller_type_name, "Wheel") == 0;
}

bool isSaturnWheelFrame(const controller_state_t* frame) {
  return frame != nullptr &&
         frame->connected &&
         std::strcmp(frame->controller_type_name, "Wheel") == 0;
}

void drawDreamcastWheelLinearGauge(uint8_t col,
                                   uint8_t row,
                                   uint8_t gaugeWidth,
                                   uint8_t filledColumns,
                                   bool fillFromRight) {
  display.setCursor(col, row);
  const uint8_t innerWidth = analogTriggerGaugeInnerWidth(gaugeWidth);
  if (filledColumns > innerWidth) {
    filledColumns = innerWidth;
  }
  for (uint8_t i = 0; i < gaugeWidth; ++i) {
    uint8_t column = kAnalogTriggerGaugeEmpty;
    if (i == 0 || i == gaugeWidth - 1) {
      column = kAnalogTriggerGaugeFilled;
    } else {
      const uint8_t interiorIndex = i - 1;
      const bool filled =
        fillFromRight
          ? ((innerWidth - 1 - interiorIndex) < filledColumns)
          : (interiorIndex < filledColumns);
      if (filled) {
        column = kAnalogTriggerGaugeFilled;
      }
    }
    display.ssd1306WriteRamBuf(column);
  }
}

void drawCenterOutPaddleGauge(const controller_state_t* frame,
                              uint8_t col,
                              uint8_t row,
                              uint8_t panelWidth) {
  const uint8_t raw = frame != nullptr ? frame->paddle : 128;
  const int16_t delta = (int16_t)raw - 128;
  const uint16_t absDelta = delta < 0 ? (uint16_t)(-delta) : (uint16_t)delta;
  const uint8_t centerLeftColumn = (panelWidth - 1) / 2;
  const uint8_t centerRightColumn = panelWidth / 2;
  const uint8_t halfSpan = centerLeftColumn > 0 ? (uint8_t)(centerLeftColumn - 1) : 0;
  uint8_t magnitude = (uint8_t)((absDelta * (uint16_t)halfSpan + 63) / 128);
  if (magnitude > halfSpan) {
    magnitude = halfSpan;
  }

  display.setCursor(col, row);
  for (uint8_t i = 0; i < panelWidth; ++i) {
    uint8_t column = kAnalogTriggerGaugeEmpty;
    if (i == 0 || i == panelWidth - 1 || i == centerLeftColumn || i == centerRightColumn) {
      column = kAnalogTriggerGaugeFilled;
    } else if (delta < 0 && i < centerLeftColumn && i >= (centerLeftColumn - magnitude)) {
      column = kAnalogTriggerGaugeFilled;
    } else if (delta > 0 && i > centerRightColumn && i <= (centerRightColumn + magnitude)) {
      column = kAnalogTriggerGaugeFilled;
    }
    display.ssd1306WriteRamBuf(column);
  }
}

void drawWheelAxisGauge(const controller_state_t* frame,
                        uint8_t colOffset,
                        uint8_t rowOffset,
                        uint8_t panelWidth) {
  const uint8_t row = rowOffset + kDreamcastWheelAxisRow;
  clearWheelPanelRow(colOffset, row);
  drawCenterOutPaddleGauge(frame, colOffset, row, panelWidth);
}

void drawDreamcastWheelAxisGauge(const controller_state_t* frame,
                                 uint8_t colOffset,
                                 uint8_t rowOffset) {
  drawWheelAxisGauge(frame, colOffset, rowOffset, kDreamcastWheelPanelWidth);
}

void drawSaturnWheelAxisGauge(const controller_state_t* frame,
                              uint8_t colOffset,
                              uint8_t rowOffset) {
  drawWheelAxisGauge(frame, colOffset, rowOffset, kSaturnWheelPanelWidth);
}

void printDreamcastWheelButtonLabel(const char* label, bool pressed) {
  display.setInvertMode(pressed);
  display.print(label);
  display.setInvertMode(false);
}

void drawWheelFaceButton(uint8_t col, uint8_t row, bool pressed, char label, bool useLabels) {
  display.setCursor(col, row);
  if (useLabels) {
    char text[2] = { label, '\0' };
    display.setFont(System5x7);
    printDreamcastWheelButtonLabel(text, pressed);
    return;
  }
  display.setFont(ReflexPad5x7);
  display.print((char)(pressed ? PAD_FACE_ON : PAD_FACE_OFF));
  display.setFont(System5x7);
}

void drawDreamcastWheelStartGlyph(uint8_t col, uint8_t row, bool pressed) {
  display.setFont(ReflexPad5x7);
  display.setCursor(col, row);
  display.print((char)(pressed ? PAD_WIDE_TRIANGLE_LEFT_ON : PAD_WIDE_TRIANGLE_LEFT_OFF));
  display.setCursor(col + kWideCircleRightHalfOffset, row);
  display.print((char)(pressed ? PAD_WIDE_TRIANGLE_RIGHT_ON : PAD_WIDE_TRIANGLE_RIGHT_OFF));
  display.setFont(System5x7);
}

void drawSaturnWheelStartGlyph(uint8_t col, uint8_t row, bool pressed) {
  display.setFont(ReflexPad5x7);
  display.setCursor(col, row);
  display.print((char)(pressed ? PAD_SATURN_START_LEFT_ON : PAD_SATURN_START_LEFT_OFF));
  display.setCursor(col + kWideCircleRightHalfOffset, row);
  display.print((char)(pressed ? PAD_SATURN_START_RIGHT_ON : PAD_SATURN_START_RIGHT_OFF));
  display.setFont(System5x7);
}

void drawDreamcastWheelButtonRow(uint32_t state,
                                 uint8_t colOffset,
                                 uint8_t rowOffset,
                                 bool useLabels) {
  const uint8_t row = rowOffset + kDreamcastWheelButtonsRow;
  clearWheelPanelRow(colOffset, row);
  drawWheelFaceButton(colOffset, row, (state & GPAD_DOWN) != 0, '-', useLabels);
  drawWheelFaceButton(colOffset + 12, row, (state & GPAD_B) != 0, 'B', useLabels);
  drawDreamcastWheelStartGlyph(colOffset + 24, row, (state & GPAD_START) != 0);
  drawWheelFaceButton(colOffset + 42, row, (state & GPAD_A) != 0, 'A', useLabels);
  drawWheelFaceButton(colOffset + kDreamcastWheelButtonPlusCol, row,
                      (state & GPAD_UP) != 0, '+', useLabels);
}

void drawSaturnWheelPaddleGauges(uint32_t state,
                                 uint8_t colOffset,
                                 uint8_t rowOffset) {
  const uint8_t row = rowOffset;
  clearWheelPanelRow(colOffset, row);
  const uint8_t fullColumns =
    analogTriggerGaugeInnerWidth(kSaturnWheelTriggerGaugeWidth);
  const uint8_t leftColumns = (state & GPAD_UP) != 0 ? fullColumns : 0;
  const uint8_t rightColumns = (state & GPAD_DOWN) != 0 ? fullColumns : 0;
  drawDreamcastWheelLinearGauge(colOffset,
                                row,
                                kSaturnWheelTriggerGaugeWidth,
                                leftColumns,
                                true);
  drawDreamcastWheelLinearGauge(colOffset + kSaturnWheelRightTriggerCol,
                                row,
                                kSaturnWheelTriggerGaugeWidth,
                                rightColumns,
                                false);
}

void drawSaturnWheelButtonRow(uint32_t state,
                              uint8_t colOffset,
                              uint8_t rowOffset,
                              bool useLabels) {
  const uint8_t row = rowOffset + kSaturnWheelButtonsRow;
  clearWheelPanelRow(colOffset, row);
  drawWheelFaceButton(colOffset, row, (state & GPAD_L1) != 0, 'Z', useLabels);
  drawWheelFaceButton(colOffset + 8, row, (state & GPAD_Y) != 0, 'Y', useLabels);
  drawWheelFaceButton(colOffset + 16, row, (state & GPAD_X) != 0, 'X', useLabels);
  drawSaturnWheelStartGlyph(colOffset + 26, row, (state & GPAD_START) != 0);
  drawWheelFaceButton(colOffset + 40, row, (state & GPAD_A) != 0, 'A', useLabels);
  drawWheelFaceButton(colOffset + 48, row, (state & GPAD_B) != 0, 'B', useLabels);
  drawWheelFaceButton(colOffset + 56, row, (state & GPAD_R1) != 0, 'C', useLabels);
}

void drawDreamcastWheelHome(const controller_state_t* frame,
                            uint32_t state,
                            uint8_t colOffset,
                            uint8_t rowOffset,
                            bool redrawTriggers,
                            bool redrawButtons,
                            bool redrawAxis,
                            bool useLabels) {
  if (redrawTriggers) {
    clearWheelPanelRow(colOffset, rowOffset);
    const uint8_t leftColumns =
      analogTriggerGaugeColumns(frame, GPAD_L2, kDreamcastWheelTriggerGaugeWidth);
    const uint8_t rightColumns =
      analogTriggerGaugeColumns(frame, GPAD_R2, kDreamcastWheelTriggerGaugeWidth);
    drawDreamcastWheelLinearGauge(colOffset,
                                  rowOffset,
                                  kDreamcastWheelTriggerGaugeWidth,
                                  leftColumns,
                                  true);
    drawDreamcastWheelLinearGauge(colOffset + kDreamcastWheelRightTriggerCol,
                                  rowOffset,
                                  kDreamcastWheelTriggerGaugeWidth,
                                  rightColumns,
                                  false);
  }
  if (redrawAxis) {
    drawDreamcastWheelAxisGauge(frame, colOffset, rowOffset);
  }
  if (redrawButtons) {
    drawDreamcastWheelButtonRow(state, colOffset, rowOffset, useLabels);
  }
}

void drawSaturnWheelHome(const controller_state_t* frame,
                         uint32_t state,
                         uint8_t colOffset,
                         uint8_t rowOffset,
                         bool redrawPaddles,
                         bool redrawButtons,
                         bool redrawAxis,
                         bool useLabels) {
  clearWheelPanelRow(colOffset, rowOffset + 3);
  if (redrawPaddles) {
    drawSaturnWheelPaddleGauges(state, colOffset, rowOffset);
  }
  if (redrawAxis) {
    drawSaturnWheelAxisGauge(frame, colOffset, rowOffset);
  }
  if (redrawButtons) {
    drawSaturnWheelButtonRow(state, colOffset, rowOffset, useLabels);
  }
}

bool findJoystickCenter(const PadButton* layout,
                        uint8_t layoutCount,
                        uint8_t* centerRow,
                        uint8_t* centerCol) {
  if (layout == nullptr || centerRow == nullptr || centerCol == nullptr) {
    return false;
  }

  for (uint8_t i = 0; i < layoutCount; ++i) {
    if (layout[i].mask == GPAD_UP) {
      *centerRow = layout[i].row + 1;
      *centerCol = layout[i].col;
      return true;
    }
  }
  return false;
}

void drawJoystickBallForLayout(const PadButton* layout,
                               uint8_t layoutCount,
                               uint32_t state,
                               uint8_t colOffset,
                               uint8_t rowOffset) {
  uint8_t centerRow = 0;
  uint8_t centerCol = 0;
  if (!findJoystickCenter(layout, layoutCount, &centerRow, &centerCol)) {
    return;
  }

  int8_t x = 0;
  int8_t y = 0;
  const bool left = (state & GPAD_LEFT) != 0;
  const bool right = (state & GPAD_RIGHT) != 0;
  const bool up = (state & GPAD_UP) != 0;
  const bool down = (state & GPAD_DOWN) != 0;
  if (left != right) {
    x = left ? -1 : 1;
  }
  if (up != down) {
    y = up ? -1 : 1;
  }

  display.setFont(ReflexPad5x7);
  for (int8_t dy = -1; dy <= 1; ++dy) {
    for (int8_t dx = -1; dx <= 1; ++dx) {
      const int16_t col = (int16_t)centerCol + ((int16_t)dx * kPadGlyphWidth) + colOffset;
      const int16_t row = (int16_t)centerRow + dy + rowOffset;
      if (col < 0 || col > 127 || row < 0 || row > 7) {
        continue;
      }
      display.setCursor((uint8_t)col, (uint8_t)row);
      display.print(' ');
    }
  }

  const int16_t ballCol = (int16_t)centerCol + ((int16_t)x * kPadGlyphWidth) + colOffset;
  const int16_t ballRow = (int16_t)centerRow + y + rowOffset;
  if (ballCol >= 0 && ballCol <= 127 && ballRow >= 0 && ballRow <= 7) {
    display.setCursor((uint8_t)ballCol, (uint8_t)ballRow);
    display.print((char)PAD_FACE_ON);
  }
}

void drawPadSlotGlyph(const PadButton& button,
                      const PadButton* layout,
                      uint8_t layoutCount,
                      uint32_t state,
                      uint8_t colOffset,
                      uint8_t rowOffset,
                      bool useJoystickDirections,
                      bool useButtonLabels,
                      outputMode_t labelOutputMode,
                      const controller_state_t* analogFrame) {
  if (useJoystickDirections && isJoystickDirectionMask(button.mask)) {
    return;
  }

  const bool analogTriggerGaugeActive =
    isAnalogTriggerGaugeButton(button) &&
    shouldDrawAnalogTriggerGauge(analogFrame, button.mask);
  if (analogTriggerGaugeActive) {
    const uint8_t gaugeWidth =
      analogTriggerGaugeWidthForButton(layout, layoutCount, button);
    drawAnalogTriggerGauge(button, analogFrame, colOffset, rowOffset, gaugeWidth);
    if (trigger_mode != TRIGGER_MODE_BOTH ||
        hasSeparateDigitalAnalogTriggerButton(button, layout, layoutCount)) {
      return;
    }
  }

  const bool pressed = (state & button.mask) != 0;
  char glyph = pressed ? button.on : button.off;
  if (useButtonLabels) {
    const char label = displayLabelChar(labelOutputMode, button.mask, pressed);
    if (label != '\0') {
      glyph = label;
    }
  }

  display.setFont(ReflexPad5x7);
  int16_t glyphCol = (int16_t)button.col + colOffset;
  int16_t glyphRow = (int16_t)button.row + rowOffset;
  if (analogTriggerGaugeActive && trigger_mode == TRIGGER_MODE_BOTH) {
    glyphRow += 1;
    if (button.mask == GPAD_L2) {
      glyphCol += (int16_t)kPadGlyphWidth * kTriggerBothGlyphInsetChars;
    } else if (button.mask == GPAD_R2) {
      glyphCol -= (int16_t)kPadGlyphWidth * kTriggerBothGlyphInsetChars;
    }
  }
  if (glyphCol < 0) {
    glyphCol = 0;
  }
  if (glyphCol > 122) {
    glyphCol = 122;
  }
  if (glyphRow < 0) {
    glyphRow = 0;
  }
  if (glyphRow > 7) {
    glyphRow = 7;
  }

  display.setCursor((uint8_t)glyphCol, (uint8_t)glyphRow);
  display.print(glyph);
  if (isWidePadLeftGlyph(glyph)) {
    const uint8_t rightCol =
      (glyphCol + kWideCircleRightHalfOffset > 127)
        ? 127
        : (uint8_t)(glyphCol + kWideCircleRightHalfOffset);
    display.setCursor(rightCol, (uint8_t)glyphRow);
    display.print(widePadRightGlyph(glyph, pressed));
  }
}

void drawPadLayoutState(const PadButton* layout,
                        uint8_t layoutCount,
                        uint32_t state,
                        uint8_t colOffset,
                        uint8_t rowOffset,
                        bool useJoystickDirections,
                        bool useButtonLabels = false,
                        outputMode_t labelOutputMode = OUTPUT_MISTER,
                        const controller_state_t* analogFrame = nullptr) {
  if (layout == nullptr) {
    return;
  }

  for (uint8_t i = 0; i < layoutCount; ++i) {
    drawPadSlotGlyph(layout[i],
                     layout,
                     layoutCount,
                     state,
                     colOffset,
                     rowOffset,
                     useJoystickDirections,
                     useButtonLabels,
                     labelOutputMode,
                     analogFrame);
  }

  if (useJoystickDirections) {
    drawJoystickBallForLayout(layout, layoutCount, state, colOffset, rowOffset);
  } else {
    drawDpadCentersForLayoutState(layout, layoutCount, state, colOffset, rowOffset);
  }
}

bool shouldUseArcadeJoystickForPrimaryPad() {
  return deviceMode == RZORD_JVS;
}

bool shouldUseSinglePlayerViewSelector(bool showOutputVirtualPad,
                                       bool showPhysicalSecondPad,
                                       outputMode_t outputMode) {
  (void)showOutputVirtualPad;
  if (showPhysicalSecondPad) {
    return false;
  }
  if (controllerFrameConst(0).connected) {
    return true;
  }
  return !output_has_secondary_player_slot(outputMode);
}

bool shouldUseCenteredOutputButtonLabels(bool singlePlayerViewSelectorActive) {
  (void)singlePlayerViewSelectorActive;
  return menu_home_button_labels != 0;
}

bool shouldUseSideBySideOutputButtonLabels(bool singlePlayerViewSelectorActive) {
  (void)singlePlayerViewSelectorActive;
  return menu_home_button_labels != 0;
}

bool shouldUseArcadeJoystickForSecondaryPad(bool showOutputVirtualPad,
                                            bool showPhysicalSecondPad,
                                            outputMode_t outputMode) {
  if (showOutputVirtualPad) {
    if (deviceMode == RZORD_JVS) {
      return true;
    }
    #ifdef ENABLE_OUTPUT_JVS
    if (outputMode == OUTPUT_JVS) {
      return true;
    }
    #endif
  }
  return showPhysicalSecondPad && deviceMode == RZORD_JVS;
}

bool renderPhysicalOnlyInputHomeHint(bool forceRedraw, const char* name) {
  if (name == nullptr || name[0] == '\0') {
    return false;
  }

  static bool hintShown = false;
  static char lastPhysicalOnlyName[16] = {0};
  const bool physicalOnlyNameChanged =
    std::strncmp(lastPhysicalOnlyName, name, sizeof(lastPhysicalOnlyName)) != 0;

  if (!forceRedraw && hintShown && !physicalOnlyNameChanged) {
    return true;
  }

  for (uint8_t row = 3; row <= 6; ++row) {
    clearRow(row);
  }

  display.setFont(System5x7);
  const size_t rawNameWidth = std::strlen(name) * 6u;
  const uint8_t nameWidth =
    (rawNameWidth > 126u) ? 126u : (uint8_t)rawNameWidth;
  display.setRow(4);
  display.setCol((uint8_t)((128 - nameWidth) / 2));
  display.print(name);
  display.setRow(5);
  display.setCol(25);
  display.print("No controller");

  std::strncpy(lastPhysicalOnlyName, name, sizeof(lastPhysicalOnlyName) - 1);
  lastPhysicalOnlyName[sizeof(lastPhysicalOnlyName) - 1] = '\0';
  hintShown = true;
  return true;
}

}  // namespace


#ifdef ENABLE_INPUT_JVS
bool renderJvsDebugPanel(bool force) {
  (void)force;
  if (deviceMode != RZORD_JVS) {
    return false;
  }

  const JvsHostState& state = getJvsHostState();
  const JvsBoardInfo& info = getJvsBoardInfo();
  const uint32_t frameMask = buildButtonMaskFromReport(0) & 0xFFFFFFu;
  static char lastLines[5][24] = {{0}};
  char lines[5][24];

  snprintf(lines[0], sizeof(lines[0]), "H:%02u SN:%lu R:%u",
           getJvsHostMachineState(),
           (unsigned long)getJvsHostSyncedCount(),
           state.ready ? 1 : 0);
  snprintf(lines[1], sizeof(lines[1]), "P1:%02X %02X C:%02X",
           state.sw_state0[0], state.sw_state1[0], state.coin_state);
  snprintf(lines[2], sizeof(lines[2]), "P2:%02X %02X JP:%u",
           state.sw_state0[1], state.sw_state1[1], state.players);
  snprintf(lines[3], sizeof(lines[3]), "F:%06lX CF:%u",
           (unsigned long)frameMask,
           controllerFrameConst(0).connected ? 1 : 0);
  snprintf(lines[4], sizeof(lines[4]), "ID:%u FC:%u C:%u P:%u",
           info.id_received ? 1 : 0,
           info.function_check_received ? 1 : 0,
           state.connected ? 1 : 0,
           info.players);

  for (uint8_t i = 0; i < 5; ++i) {
    if (!force && std::strcmp(lastLines[i], lines[i]) == 0) {
      continue;
    }

    clearRow(3 + i);
    display.setRow(3 + i);
    display.setCol(0);
    display.print(lines[i]);

    std::strncpy(lastLines[i], lines[i], sizeof(lastLines[i]) - 1);
    lastLines[i][sizeof(lastLines[i]) - 1] = '\0';
  }

  return true;
}
#endif

bool didPrimaryControllerNameChange() {
  static char lastDisplayedControllerType[16] = {0};

  if (controllerFrameConst(0).connected &&
      std::strncmp(lastDisplayedControllerType, controllerFrameConst(0).controller_type_name, sizeof(lastDisplayedControllerType)) != 0) {
    std::strncpy(lastDisplayedControllerType, controllerFrameConst(0).controller_type_name, sizeof(lastDisplayedControllerType) - 1);
    lastDisplayedControllerType[sizeof(lastDisplayedControllerType) - 1] = '\0';
    return true;
  }

  return false;
}

void updateRealtimeButtons() {
  static uint32_t lastButtonState[2] = { 0, 0 };
  static const PadButton* currentLayout[2] = { nullptr, nullptr };
  static uint8_t currentLayoutCount[2] = { 0, 0 };
  static char lastControllerType[2][16] = { {0}, {0} };
  static DeviceEnum lastDeviceMode = RZORD_NONE;
  static outputMode_t lastOutputPadMode = OUTPUT_LAST;
  static bool lastShowOutputVirtualPad = false;
  static bool lastShowPhysicalSecondPad = false;
  static bool lastCenterOutputVirtualPad = false;
  static uint8_t lastButtonLabels = 0;
  static uint8_t lastSinglePlayerView = 0;
  static uint8_t lastAnalogL2[2] = { 0, 0 };
  static uint8_t lastAnalogR2[2] = { 0, 0 };
  static uint8_t lastWheelPaddle[2] = { 0xFF, 0xFF };
  static uint8_t lastTriggerRawDebugL2 = 0xFF;
  static uint8_t lastTriggerRawDebugR2 = 0xFF;
  static uint32_t lastAnalogGaugeRedrawMs = 0;
  static bool lastPhysicalOnlyInput = false;

  outputMode_t effectiveOutputMode = get_effective_output_mode();
  const bool mergedSinglePadView = classic_dual_merge_enabled != 0;
  bool showPhysicalSecondPad =
    !mergedSinglePadView &&
    (max_devices > 1) &&
    controllerFrameConst(1).connected &&
    !shouldHideEmptySecondPadInAutoInputMode();
  bool showOutputVirtualPad =
    !mergedSinglePadView &&
    controllerFrameConst(0).connected &&
    shouldShowVirtualOutputPad(deviceMode, effectiveOutputMode) &&
    ((max_devices == 1) || !showPhysicalSecondPad);
  bool centerOutputVirtualPad =
    showOutputVirtualPad &&
    !output_has_secondary_player_slot(effectiveOutputMode);
  const bool singlePlayerViewSelectorActive =
    shouldUseSinglePlayerViewSelector(showOutputVirtualPad,
                                      showPhysicalSecondPad,
                                      effectiveOutputMode);
  if (singlePlayerViewSelectorActive) {
    centerOutputVirtualPad = menu_home_jvs_view != kHome1pViewInputOutput;
  }
  bool showSecondPad = (showOutputVirtualPad && !centerOutputVirtualPad) || showPhysicalSecondPad;
  bool forceRedraw = padDisplayNeedsRedraw;
  if (forceRedraw) {
    padDisplayNeedsRedraw = false;
  }
  const char* physicalOnlyName = activeInputAdapterPhysicalConnectionDisplayName();
  const bool physicalOnlyInput =
    physicalOnlyName != nullptr &&
    physicalOnlyName[0] != '\0' &&
    !anyInputFrameConnected(max_devices);
  if (lastPhysicalOnlyInput != physicalOnlyInput) {
    forceRedraw = true;
  }
  lastPhysicalOnlyInput = physicalOnlyInput;
  bool secondPadVisibilityChanged =
    (lastOutputPadMode != effectiveOutputMode) ||
    (lastShowOutputVirtualPad != showOutputVirtualPad) ||
    (lastShowPhysicalSecondPad != showPhysicalSecondPad) ||
    (lastCenterOutputVirtualPad != centerOutputVirtualPad) ||
    (lastButtonLabels != menu_home_button_labels) ||
    (lastSinglePlayerView != menu_home_jvs_view);

#ifdef ENABLE_INPUT_JVS
  if (renderJvsDebugPanel(forceRedraw)) {
    return;
  }
#endif

  if (physicalOnlyInput &&
      renderPhysicalOnlyInputHomeHint(forceRedraw, physicalOnlyName)) {
    lastDeviceMode = deviceMode;
    return;
  }

  bool controllerTypeChanged = false;
  for (uint8_t p = 0; p < 2 && p < max_devices; ++p) {
    if (controllerFrameConst(p).connected) {
      if (std::strncmp(lastControllerType[p], controllerFrameConst(p).controller_type_name, sizeof(lastControllerType[p])) != 0) {
        std::strncpy(lastControllerType[p], controllerFrameConst(p).controller_type_name, sizeof(lastControllerType[p]) - 1);
        lastControllerType[p][sizeof(lastControllerType[p]) - 1] = '\0';
        controllerTypeChanged = true;
      }
    }
  }

  static bool popnMode = false;
  static bool guitarFreaksMode = false;
  bool popnModeChanged = false;
  bool guitarFreaksModeChanged = false;
  #ifdef ENABLE_INPUT_PSX
  if (deviceMode == RZORD_PSX) {
    bool isPopn = (std::strcmp(controllerFrameConst(0).controller_type_name, "Pop'n") == 0 ||
                   std::strcmp(controllerFrameConst(0).controller_type_name, "PopN") == 0);
    bool isGuitarFreaks = (std::strcmp(controllerFrameConst(0).controller_type_name, "GuitarFreaks") == 0);
    if (isPopn != popnMode) {
      popnMode = isPopn;
      popnModeChanged = true;
    }
    if (isGuitarFreaks != guitarFreaksMode) {
      guitarFreaksMode = isGuitarFreaks;
      guitarFreaksModeChanged = true;
    }
  } else {
    if (popnMode) {
      popnMode = false;
      popnModeChanged = true;
    }
    if (guitarFreaksMode) {
      guitarFreaksMode = false;
      guitarFreaksModeChanged = true;
    }
  }
  #endif

  static uint8_t lastJaguarRotaryMask = 0;
  bool jaguarRotaryModeChanged = false;
  #ifdef ENABLE_INPUT_JAGUAR
  uint8_t jaguarRotaryMask = 0;
  for (uint8_t p = 0; p < max_devices && p < 2; ++p) {
    if (jaguarRotaryActivePorts[p]) {
      jaguarRotaryMask |= (1 << p);
    }
  }
  if (jaguarRotaryMask != lastJaguarRotaryMask) {
    lastJaguarRotaryMask = jaguarRotaryMask;
    jaguarRotaryModeChanged = true;
  }
  #endif

  const bool homeLayoutNeedsRedraw =
    lastDeviceMode != deviceMode ||
    forceRedraw ||
    controllerTypeChanged ||
    popnModeChanged ||
    guitarFreaksModeChanged ||
    jaguarRotaryModeChanged ||
    secondPadVisibilityChanged;

  if (homeLayoutNeedsRedraw) {
    lastDeviceMode = deviceMode;
    lastOutputPadMode = effectiveOutputMode;
    lastShowOutputVirtualPad = showOutputVirtualPad;
    lastShowPhysicalSecondPad = showPhysicalSecondPad;
    lastCenterOutputVirtualPad = centerOutputVirtualPad;
    lastButtonLabels = menu_home_button_labels;
    lastSinglePlayerView = menu_home_jvs_view;

    #ifdef ENABLE_INPUT_PSX
    if (!centerOutputVirtualPad &&
        deviceMode == RZORD_PSX &&
        getSharedControllerTypePadLayout(controllerFrameConst(0).controller_type_name,
            &currentLayout[0], &currentLayoutCount[0])) {
      currentLayout[1] = currentLayout[0];
      currentLayoutCount[1] = currentLayoutCount[0];
    } else
    #endif
    {
      if (centerOutputVirtualPad) {
        getLayoutForPlayer(0, &currentLayout[0], &currentLayoutCount[0]);
        currentLayout[1] = nullptr;
        currentLayoutCount[1] = 0;
      } else {
        getLayoutForPlayer(0, &currentLayout[0], &currentLayoutCount[0]);
        if (showOutputVirtualPad) {
          getVirtualOutputPadLayout(deviceMode, effectiveOutputMode,
                                    &currentLayout[1], &currentLayoutCount[1]);
        } else if (showPhysicalSecondPad) {
          getLayoutForPlayer(1, &currentLayout[1], &currentLayoutCount[1]);
        } else {
          currentLayout[1] = nullptr;
          currentLayoutCount[1] = 0;
        }
      }
    }

    display.setFont(ReflexPad5x7);
    uint8_t maxRow = 6;
    #ifdef ENABLE_INPUT_JAGUAR
    if (deviceMode == RZORD_JAGUAR) {
      bool showJaguarKeypadRow = false;
      for (uint8_t p = 0; p < max_devices && p < 2; ++p) {
        if (!jaguarRotaryActivePorts[p]) {
          showJaguarKeypadRow = true;
          break;
        }
      }
      maxRow = showJaguarKeypadRow ? 7 : 6;
    }
    #endif
    for (uint8_t row = 3; row <= maxRow; ++row) {
      clearRow(row);
    }

    #ifdef ENABLE_INPUT_DREAMCAST
    const bool primaryLayoutDreamcastWheel =
      !centerOutputVirtualPad &&
      deviceMode == RZORD_DREAMCAST &&
      isDreamcastWheelFrame(max_devices > 0 ? &controllerFrameConst(0) : nullptr);
    const bool secondaryLayoutDreamcastWheel =
      showPhysicalSecondPad &&
      !showOutputVirtualPad &&
      deviceMode == RZORD_DREAMCAST &&
      isDreamcastWheelFrame(max_devices > 1 ? &controllerFrameConst(1) : nullptr);
    #else
    const bool primaryLayoutDreamcastWheel = false;
    const bool secondaryLayoutDreamcastWheel = false;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    const bool primaryLayoutSaturnWheel =
      !centerOutputVirtualPad &&
      deviceMode == RZORD_SATURN &&
      isSaturnWheelFrame(max_devices > 0 ? &controllerFrameConst(0) : nullptr);
    const bool secondaryLayoutSaturnWheel =
      showPhysicalSecondPad &&
      !showOutputVirtualPad &&
      deviceMode == RZORD_SATURN &&
      isSaturnWheelFrame(max_devices > 1 ? &controllerFrameConst(1) : nullptr);
    #else
    const bool primaryLayoutSaturnWheel = false;
    const bool secondaryLayoutSaturnWheel = false;
    #endif

    if (currentLayout[0] && !primaryLayoutDreamcastWheel) {
      if (!primaryLayoutSaturnWheel) {
        const bool useCenteredLabels =
          centerOutputVirtualPad &&
          shouldUseCenteredOutputButtonLabels(singlePlayerViewSelectorActive);
        drawPadLayoutState(currentLayout[0],
                           currentLayoutCount[0],
                           0,
                           centerOutputVirtualPad ? kCenteredPadColOffset : 0,
                           kHomePadRowOffset,
                           centerOutputVirtualPad ?
                             shouldUseArcadeJoystickForSecondaryPad(true, false, effectiveOutputMode) :
                             shouldUseArcadeJoystickForPrimaryPad(),
                           useCenteredLabels,
                           effectiveOutputMode,
                           max_devices > 0 ? &controllerFrameConst(0) : nullptr);
      }
    }

    if (showSecondPad && currentLayout[1]) {
      const bool useSecondaryLabels =
        showOutputVirtualPad &&
        shouldUseSideBySideOutputButtonLabels(singlePlayerViewSelectorActive);
      if (!secondaryLayoutDreamcastWheel) {
        if (!secondaryLayoutSaturnWheel) {
          drawPadLayoutState(currentLayout[1],
                             currentLayoutCount[1],
                             0,
                             kSecondPadColOffset,
                             kHomePadRowOffset,
                             shouldUseArcadeJoystickForSecondaryPad(showOutputVirtualPad,
                                                                   showPhysicalSecondPad,
                                                                   effectiveOutputMode),
                             useSecondaryLabels,
                             effectiveOutputMode,
                             showOutputVirtualPad ?
                               (max_devices > 0 ? &controllerFrameConst(0) : nullptr) :
                               (max_devices > 1 ? &controllerFrameConst(1) : nullptr));
        }
      }
    }

    display.setFont(System5x7);
    lastButtonState[0] = 0;
    lastButtonState[1] = 0;
    lastAnalogL2[0] = 0;
    lastAnalogR2[0] = 0;
    lastAnalogL2[1] = 0;
    lastAnalogR2[1] = 0;
    lastWheelPaddle[0] = 0xFF;
    lastWheelPaddle[1] = 0xFF;
    lastTriggerRawDebugL2 = 0xFF;
    lastTriggerRawDebugR2 = 0xFF;
  }

  if (currentLayout[0] == nullptr) {
    return;
  }

  #ifdef ENABLE_INPUT_DRIVING
  if (deviceMode == RZORD_DRIVING || (deviceMode == RZORD_SMS && isDrivingFallbackActive())) {
    static uint8_t lastAxisDisplay[2] = { 0xFF, 0xFF };
    const bool smsDrivingFallback = deviceMode == RZORD_SMS;
    uint8_t visiblePhysicalPlayers = max_devices;
    if (visiblePhysicalPlayers > 1 && !showPhysicalSecondPad) visiblePhysicalPlayers = 1;
    if (visiblePhysicalPlayers > 2) visiblePhysicalPlayers = 2;

    for (uint8_t p = 0; p < visiblePhysicalPlayers; ++p) {
      const controller_state_t* frame = &controllerFrameConst(p);
      if (smsDrivingFallback && std::strcmp(frame->controller_type_name, "Driving") != 0) {
        continue;
      }
      const bool axisChanged = frame->paddle != lastAxisDisplay[p];
      if (axisChanged || homeLayoutNeedsRedraw) {
        lastAxisDisplay[p] = frame->paddle;
        uint8_t colBase = (p == 0) ? 0 : kSecondPadColOffset;
        drawWheelAxisGauge(&controllerFrameConst(p),
                           colBase,
                           kHomePadRowOffset,
                           kSpinnerPaddlePanelWidth);
      }
    }
  }
  #endif

  #ifdef ENABLE_INPUT_JAGUAR
  uint8_t visiblePhysicalPlayers = max_devices;
  if (visiblePhysicalPlayers > 1 && !showPhysicalSecondPad) visiblePhysicalPlayers = 1;
  if (visiblePhysicalPlayers > 2) visiblePhysicalPlayers = 2;
  bool anyVisibleJaguarRotary = false;
  for (uint8_t p = 0; p < visiblePhysicalPlayers; ++p) {
    if (jaguarRotaryActivePorts[p]) {
      anyVisibleJaguarRotary = true;
      break;
    }
  }
  if (deviceMode == RZORD_JAGUAR && anyVisibleJaguarRotary) {
    static uint8_t lastAxisDisplay[2] = { 0xFF, 0xFF };

    for (uint8_t p = 0; p < visiblePhysicalPlayers; ++p) {
      if (!jaguarRotaryActivePorts[p]) {
        continue;
      }
      const controller_state_t* frame = &controllerFrameConst(p);
      const bool axisChanged = frame->paddle != lastAxisDisplay[p];
      if (axisChanged || homeLayoutNeedsRedraw) {
        lastAxisDisplay[p] = frame->paddle;
        uint8_t colBase = (p == 0) ? 0 : kSecondPadColOffset;
        drawWheelAxisGauge(&controllerFrameConst(p),
                           colBase,
                           kHomePadRowOffset,
                           kSpinnerPaddlePanelWidth);
      }
    }
  }
  #endif

  uint32_t state0 = buildButtonMaskFromReport(0);
  state0 = applyTurboVisualState(0, state0);
  uint32_t state1 = 0;
  if (showOutputVirtualPad && !centerOutputVirtualPad) {
    state1 = buildButtonMaskFromOutputState(0);
    state1 = applyTurboVisualState(0, state1);
  } else if (showPhysicalSecondPad) {
    state1 = buildButtonMaskFromReport(1);
    state1 = applyTurboVisualState(1, state1);
  }

  const controller_state_t* primaryAnalogFrame =
    (max_devices > 0) ? &controllerFrameConst(0) : nullptr;
  const controller_state_t* secondaryAnalogFrame = nullptr;
  if (showOutputVirtualPad && !centerOutputVirtualPad) {
    secondaryAnalogFrame = primaryAnalogFrame;
  } else if (showPhysicalSecondPad && max_devices > 1) {
    secondaryAnalogFrame = &controllerFrameConst(1);
  }
  #ifdef ENABLE_INPUT_DREAMCAST
  const bool primaryDreamcastWheel =
    deviceMode == RZORD_DREAMCAST && isDreamcastWheelFrame(primaryAnalogFrame);
  const bool secondaryDreamcastWheel =
    deviceMode == RZORD_DREAMCAST && isDreamcastWheelFrame(secondaryAnalogFrame);
  #else
  const bool primaryDreamcastWheel = false;
  const bool secondaryDreamcastWheel = false;
  #endif
  #ifdef ENABLE_INPUT_SATURN
  const bool primarySaturnWheel =
    deviceMode == RZORD_SATURN && isSaturnWheelFrame(primaryAnalogFrame);
  const bool secondarySaturnWheel =
    deviceMode == RZORD_SATURN && isSaturnWheelFrame(secondaryAnalogFrame);
  #else
  const bool primarySaturnWheel = false;
  const bool secondarySaturnWheel = false;
  #endif
  const bool primaryWheelAxisChanged =
    (primaryDreamcastWheel || primarySaturnWheel) &&
    primaryAnalogFrame != nullptr &&
    primaryAnalogFrame->paddle != lastWheelPaddle[0];
  const bool secondaryWheelAxisChanged =
    (secondaryDreamcastWheel || secondarySaturnWheel) &&
    secondaryAnalogFrame != nullptr &&
    secondaryAnalogFrame->paddle != lastWheelPaddle[1];
  const uint8_t analogL2GaugeWidth[2] = {
    primaryDreamcastWheel
      ? kDreamcastWheelTriggerGaugeWidth
      : analogTriggerGaugeWidthForMask(currentLayout[0], currentLayoutCount[0], GPAD_L2),
    secondaryDreamcastWheel
      ? kDreamcastWheelTriggerGaugeWidth
      : analogTriggerGaugeWidthForMask(currentLayout[1], currentLayoutCount[1], GPAD_L2),
  };
  const uint8_t analogR2GaugeWidth[2] = {
    primaryDreamcastWheel
      ? kDreamcastWheelTriggerGaugeWidth
      : analogTriggerGaugeWidthForMask(currentLayout[0], currentLayoutCount[0], GPAD_R2),
    secondaryDreamcastWheel
      ? kDreamcastWheelTriggerGaugeWidth
      : analogTriggerGaugeWidthForMask(currentLayout[1], currentLayoutCount[1], GPAD_R2),
  };
  const uint8_t analogL2Columns[2] = {
    analogTriggerGaugeColumns(primaryAnalogFrame, GPAD_L2, analogL2GaugeWidth[0]),
    analogTriggerGaugeColumns(secondaryAnalogFrame, GPAD_L2, analogL2GaugeWidth[1]),
  };
  const uint8_t analogR2Columns[2] = {
    analogTriggerGaugeColumns(primaryAnalogFrame, GPAD_R2, analogR2GaugeWidth[0]),
    analogTriggerGaugeColumns(secondaryAnalogFrame, GPAD_R2, analogR2GaugeWidth[1]),
  };
  const bool analogChanged0 =
    analogL2Columns[0] != lastAnalogL2[0] || analogR2Columns[0] != lastAnalogR2[0];
  const bool analogChanged1 =
    analogL2Columns[1] != lastAnalogL2[1] || analogR2Columns[1] != lastAnalogR2[1];
  const bool triggerRawDebugActive =
    deviceMode == RZORD_PSX &&
    primaryAnalogFrame != nullptr &&
    primaryAnalogFrame->connected &&
    primaryAnalogFrame->HAS_ANALOG_TRIGGERS;
  const bool triggerRawDebugChanged =
    triggerRawDebugActive &&
    (primaryAnalogFrame->ANALOG_L2 != lastTriggerRawDebugL2 ||
     primaryAnalogFrame->ANALOG_R2 != lastTriggerRawDebugR2);

  const bool buttonsChanged =
    state0 != lastButtonState[0] || state1 != lastButtonState[1];
  const bool analogChanged = analogChanged0 || analogChanged1;
  const bool wheelAxisChanged = primaryWheelAxisChanged || secondaryWheelAxisChanged;
  if (!buttonsChanged && !analogChanged && !triggerRawDebugChanged && !wheelAxisChanged) {
    return;
  }
  if (!buttonsChanged && !triggerRawDebugChanged && (analogChanged || wheelAxisChanged)) {
    const uint32_t now = millis();
    if ((uint32_t)(now - lastAnalogGaugeRedrawMs) <
        kAnalogTriggerGaugeRedrawMs) {
      return;
    }
    lastAnalogGaugeRedrawMs = now;
  } else if (analogChanged || wheelAxisChanged) {
    lastAnalogGaugeRedrawMs = millis();
  }

  display.setFont(ReflexPad5x7);
  const uint8_t primaryColOffset = centerOutputVirtualPad ? kCenteredPadColOffset : 0;
  const bool usePrimaryJoystick = shouldUseArcadeJoystickForPrimaryPad();
  const bool usePrimaryLabels =
    centerOutputVirtualPad &&
    shouldUseCenteredOutputButtonLabels(singlePlayerViewSelectorActive);
  const bool useSecondaryJoystick =
    shouldUseArcadeJoystickForSecondaryPad(showOutputVirtualPad,
                                           showPhysicalSecondPad,
                                           effectiveOutputMode);
  const bool useSecondaryLabels =
    showOutputVirtualPad &&
    shouldUseSideBySideOutputButtonLabels(singlePlayerViewSelectorActive);
  const bool useWheelButtonLabels = menu_home_button_labels != 0;

  if (primaryDreamcastWheel && currentLayout[0]) {
    drawDreamcastWheelHome(primaryAnalogFrame, state0, primaryColOffset, kHomePadRowOffset,
                           analogChanged0 || primaryWheelAxisChanged || primaryDreamcastWheel,
                           state0 != lastButtonState[0] || primaryWheelAxisChanged,
                           primaryWheelAxisChanged,
                           useWheelButtonLabels);
  }

  if (primarySaturnWheel && currentLayout[0]) {
    drawSaturnWheelHome(primaryAnalogFrame, state0, primaryColOffset, kHomePadRowOffset,
                        state0 != lastButtonState[0] || primaryWheelAxisChanged,
                        state0 != lastButtonState[0] || primaryWheelAxisChanged,
                        primaryWheelAxisChanged,
                        useWheelButtonLabels);
  }

  if (!primaryDreamcastWheel &&
      !primarySaturnWheel &&
      (state0 != lastButtonState[0] || analogChanged0) &&
      currentLayout[0]) {
    for (uint8_t i = 0; i < currentLayoutCount[0]; ++i) {
      uint32_t mask = currentLayout[0][i].mask;
      if (usePrimaryJoystick && isJoystickDirectionMask(mask)) {
        continue;
      }
      const bool triggerGaugeChanged =
        (mask == GPAD_L2 && analogL2Columns[0] != lastAnalogL2[0]) ||
        (mask == GPAD_R2 && analogR2Columns[0] != lastAnalogR2[0]);
      if (((state0 & mask) != (lastButtonState[0] & mask)) ||
          triggerGaugeChanged) {
        drawPadSlotGlyph(currentLayout[0][i],
                         currentLayout[0],
                         currentLayoutCount[0],
                         state0,
                         primaryColOffset,
                         kHomePadRowOffset,
                         usePrimaryJoystick,
                         usePrimaryLabels,
                         effectiveOutputMode,
                         primaryAnalogFrame);
      }
    }
    if (usePrimaryJoystick &&
        ((state0 ^ lastButtonState[0]) & kJoystickDirectionMask)) {
      drawJoystickBallForLayout(currentLayout[0],
                                currentLayoutCount[0],
                                state0,
                                primaryColOffset,
                                kHomePadRowOffset);
    }
  }

  if (showSecondPad && secondaryDreamcastWheel && currentLayout[1]) {
    drawDreamcastWheelHome(secondaryAnalogFrame, state1, kSecondPadColOffset, kHomePadRowOffset,
                           analogChanged1 || secondaryWheelAxisChanged || secondaryDreamcastWheel,
                           state1 != lastButtonState[1] || secondaryWheelAxisChanged,
                           secondaryWheelAxisChanged,
                           useWheelButtonLabels);
  }

  if (showSecondPad && secondarySaturnWheel && currentLayout[1]) {
    drawSaturnWheelHome(secondaryAnalogFrame, state1, kSecondPadColOffset, kHomePadRowOffset,
                        state1 != lastButtonState[1] || secondaryWheelAxisChanged,
                        state1 != lastButtonState[1] || secondaryWheelAxisChanged,
                        secondaryWheelAxisChanged,
                        useWheelButtonLabels);
  }

  if (showSecondPad &&
      !secondaryDreamcastWheel &&
      !secondarySaturnWheel &&
      (state1 != lastButtonState[1] || analogChanged1) &&
      currentLayout[1]) {
    for (uint8_t i = 0; i < currentLayoutCount[1]; ++i) {
      uint32_t mask = currentLayout[1][i].mask;
      if (useSecondaryJoystick && isJoystickDirectionMask(mask)) {
        continue;
      }
      const bool triggerGaugeChanged =
        (mask == GPAD_L2 && analogL2Columns[1] != lastAnalogL2[1]) ||
        (mask == GPAD_R2 && analogR2Columns[1] != lastAnalogR2[1]);
      if (((state1 & mask) != (lastButtonState[1] & mask)) ||
          triggerGaugeChanged) {
        drawPadSlotGlyph(currentLayout[1][i],
                         currentLayout[1],
                         currentLayoutCount[1],
                         state1,
                         kSecondPadColOffset,
                         kHomePadRowOffset,
                         useSecondaryJoystick,
                         useSecondaryLabels,
                         effectiveOutputMode,
                         secondaryAnalogFrame);
      }
    }
    if (useSecondaryJoystick &&
        ((state1 ^ lastButtonState[1]) & kJoystickDirectionMask)) {
      drawJoystickBallForLayout(currentLayout[1],
                                currentLayoutCount[1],
                                state1,
                                kSecondPadColOffset,
                                kHomePadRowOffset);
    }
  }

  renderPsxTriggerRawDebugLine(primaryAnalogFrame, forceRedraw || triggerRawDebugChanged);

  lastButtonState[0] = state0;
  lastButtonState[1] = state1;
  lastAnalogL2[0] = analogL2Columns[0];
  lastAnalogR2[0] = analogR2Columns[0];
  lastAnalogL2[1] = analogL2Columns[1];
  lastAnalogR2[1] = analogR2Columns[1];
  lastWheelPaddle[0] =
    ((primaryDreamcastWheel || primarySaturnWheel) && primaryAnalogFrame != nullptr)
      ? primaryAnalogFrame->paddle
      : 0xFF;
  lastWheelPaddle[1] =
    ((secondaryDreamcastWheel || secondarySaturnWheel) && secondaryAnalogFrame != nullptr)
      ? secondaryAnalogFrame->paddle
      : 0xFF;
  if (triggerRawDebugActive) {
    lastTriggerRawDebugL2 = primaryAnalogFrame->ANALOG_L2;
    lastTriggerRawDebugR2 = primaryAnalogFrame->ANALOG_R2;
  } else {
    lastTriggerRawDebugL2 = 0xFF;
    lastTriggerRawDebugR2 = 0xFF;
  }

  display.setFont(System5x7);
}

}  // namespace menu_main_display_internal
