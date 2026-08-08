#include "../product_config.h"

#include "analog_test_renderer.h"

#include <stdio.h>

#include "../platform/display_runtime_state.h"

#ifdef USE_I2C_DISPLAY
namespace {

constexpr int16_t kAnalogTraceCenterX = 29;
constexpr int16_t kAnalogTraceCenterY = 28;
constexpr int16_t kAnalogTraceRadius = AnalogStickTrace::kPlotRadius;

int16_t analogTracePlotCoordinate(int16_t center, int16_t raw, int16_t fullScale) {
  int32_t offset = ((int32_t)raw * kAnalogTraceRadius) / fullScale;
  if (offset < -kAnalogTraceRadius) offset = -kAnalogTraceRadius;
  if (offset > kAnalogTraceRadius) offset = kAnalogTraceRadius;
  return (int16_t)(center + offset);
}

const char* analogTraceDirectionLabel(AnalogTraceDirection direction) {
  switch (direction) {
    case AnalogTraceDirection::Up: return "U";
    case AnalogTraceDirection::UpRight: return "UR";
    case AnalogTraceDirection::Right: return "R";
    case AnalogTraceDirection::DownRight: return "DR";
    case AnalogTraceDirection::Down: return "D";
    case AnalogTraceDirection::DownLeft: return "DL";
    case AnalogTraceDirection::Left: return "L";
    case AnalogTraceDirection::UpLeft: return "UL";
    default: return "?";
  }
}

bool analogTraceDirectionIsVertical(AnalogTraceDirection direction) {
  return direction == AnalogTraceDirection::Up ||
         direction == AnalogTraceDirection::Down;
}

bool analogTraceDirectionIsCardinal(AnalogTraceDirection direction) {
  return analogTraceDirectionIsVertical(direction) ||
         direction == AnalogTraceDirection::Left ||
         direction == AnalogTraceDirection::Right;
}

void drawAnalogTracePlot(const AnalogStickTrace& trace, bool octagonal,
                         int16_t currentX, int16_t currentY,
                         int16_t fullScale, bool connected) {
  for (int16_t offset = -kAnalogTraceRadius;
       offset <= kAnalogTraceRadius; offset += 5) {
    u8g2.drawPixel(kAnalogTraceCenterX + offset, kAnalogTraceCenterY);
    u8g2.drawPixel(kAnalogTraceCenterX, kAnalogTraceCenterY + offset);
  }

  (void)octagonal;
  for (uint8_t y = 0; y < AnalogStickTrace::kPlotDiameter; ++y) {
    for (uint8_t x = 0; x < AnalogStickTrace::kPlotDiameter; ++x) {
      if (trace.dotAt(x, y)) {
        u8g2.drawPixel(
          kAnalogTraceCenterX - kAnalogTraceRadius + x,
          kAnalogTraceCenterY - kAnalogTraceRadius + y);
      }
    }
  }

  if (connected) {
    u8g2.drawDisc(
      analogTracePlotCoordinate(kAnalogTraceCenterX, currentX, fullScale),
      analogTracePlotCoordinate(kAnalogTraceCenterY, currentY, fullScale), 2);
  } else {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(17, 31, "NO PAD");
  }
}

void drawAnalogTraceValues(const AnalogStickTrace& trace, bool octagonal,
                           bool rightStick, bool gameCube, uint8_t port) {
  u8g2.setFont(u8g2_font_4x6_tr);
  char header[16];
  const char* stick = rightStick ? (gameCube ? "C" : "R") : "L";
  snprintf(header, sizeof(header), "P%u RAW %s", (unsigned)(port + 1), stick);
  u8g2.drawStr(64, 5, header);

  const uint8_t count = AnalogStickTrace::directionCount(octagonal);
  for (uint8_t i = 0; i < count; ++i) {
    const AnalogTraceDirection direction =
      AnalogStickTrace::orderedDirection(octagonal, i);
    const AnalogTracePoint& point = trace.point(direction);
    char buffer[20];
    if (!point.valid) {
      snprintf(buffer, sizeof(buffer), "%-2s --",
               analogTraceDirectionLabel(direction));
    } else if (analogTraceDirectionIsCardinal(direction)) {
      const int16_t value =
        analogTraceDirectionIsVertical(direction) ? point.y : point.x;
      snprintf(buffer, sizeof(buffer), "%-2s %d",
               analogTraceDirectionLabel(direction), value);
    } else {
      snprintf(buffer, sizeof(buffer), "%-2s %d,%d",
               analogTraceDirectionLabel(direction), point.x, point.y);
    }
    const uint8_t baseline = octagonal ? (uint8_t)(11 + i * 6)
                                       : (uint8_t)(15 + i * 10);
    u8g2.drawStr(64, baseline, buffer);
  }
}

}  // namespace
#endif

int16_t analogTraceFullScale(analog_stick_precision precision) {
  switch (precision) {
    case ANALOG_STICK_PRECISION_12: return INT16_MAX >> 4;
    case ANALOG_STICK_PRECISION_16: return INT16_MAX;
    case ANALOG_STICK_PRECISION_8:
    default: return INT8_MAX;
  }
}

int16_t analogTraceCenterThreshold(int16_t fullScale) {
  const int16_t threshold = fullScale / 16;
  return threshold < 2 ? 2 : threshold;
}

void renderAnalogStickTraceScreen(const AnalogStickTraceScreen& screen) {
#ifdef USE_I2C_DISPLAY
  if (screen.trace == nullptr) {
    return;
  }
  const int16_t fullScale = analogTraceFullScale(screen.precision);
  u8g2.clearBuffer();
  drawAnalogTracePlot(*screen.trace, screen.octagonal,
                      screen.current_x, screen.current_y,
                      fullScale, screen.connected);
  drawAnalogTraceValues(*screen.trace, screen.octagonal,
                        screen.right_stick, screen.gamecube, screen.port);
  u8g2.setFont(u8g2_font_4x6_tr);
  char footer[24];
  if (screen.multiple_ports && screen.multiple_sticks) {
    snprintf(footer, sizeof(footer), "UD:port <>:stick Mode");
  } else if (screen.multiple_ports) {
    snprintf(footer, sizeof(footer), "UD:port Mode:back");
  } else if (screen.multiple_sticks) {
    snprintf(footer, sizeof(footer), "<>:stick Mode:back");
  } else {
    snprintf(footer, sizeof(footer), "Mode:back");
  }
  u8g2.drawStr(0, 63, footer);
  u8g2.sendBuffer();
#else
  (void)screen;
#endif
}

void renderAnalogValueTestScreen(const char* title, uint8_t port,
                                 const AnalogTestValue* values,
                                 uint8_t valueCount, bool multiplePorts) {
#ifdef USE_I2C_DISPLAY
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);
  char header[24];
  snprintf(header, sizeof(header), "P%u %s",
           (unsigned)(port + 1), title ? title : "Analog Test");
  u8g2.drawStr(0, 7, header);

  u8g2.setFont(u8g2_font_5x7_tr);
  const uint8_t rows = valueCount > 5 ? 5 : valueCount;
  for (uint8_t i = 0; i < rows; ++i) {
    char row[24];
    snprintf(row, sizeof(row), "%-7s %6d",
             values[i].label ? values[i].label : "", values[i].value);
    u8g2.drawStr(0, (uint8_t)(18 + i * 9), row);
  }

  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(0, 63,
               multiplePorts ? "UD:port Mode:back" : "Mode:back");
  u8g2.sendBuffer();
#else
  (void)title;
  (void)port;
  (void)values;
  (void)valueCount;
  (void)multiplePorts;
#endif
}
