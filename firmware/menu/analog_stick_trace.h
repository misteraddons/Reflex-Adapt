#pragma once

#include <stdint.h>
#include <string.h>

#include "../core/device_mode.h"

inline bool analogTraceUsesOctagonalGate(DeviceEnum mode,
                                         const char* controllerType) {
  if (mode == RZORD_N64 || mode == RZORD_GAMECUBE) {
    return true;
  }
  if (mode != RZORD_WII || controllerType == nullptr) {
    return false;
  }
  return strcmp(controllerType, "Classic") == 0 ||
         strcmp(controllerType, "ClassicPro") == 0 ||
         strcmp(controllerType, "Nunchuk") == 0;
}

enum class AnalogTraceDirection : uint8_t {
  Up = 0,
  UpRight,
  Right,
  DownRight,
  Down,
  DownLeft,
  Left,
  UpLeft,
  Count,
};

struct AnalogTracePoint {
  int16_t x;
  int16_t y;
  uint32_t score;
  int32_t perpendicular;
  bool valid;
};

class AnalogStickTrace {
public:
  static constexpr int16_t kPlotRadius = 25;
  static constexpr uint8_t kPlotDiameter = (uint8_t)(kPlotRadius * 2 + 1);
  static constexpr uint16_t kMaxPlotDots = 500;

  AnalogStickTrace() {
    reset();
  }

  void reset() {
    for (uint8_t i = 0; i < directionCount(true); ++i) {
      points_[i] = { 0, 0, 0, 0, false };
    }
    memset(dots_, 0, sizeof(dots_));
    dot_count_ = 0;
  }

  bool sample(int16_t x, int16_t y, bool octagonal,
              int16_t centerThreshold, int16_t fullScale) {
    const int32_t ix = x;
    const int32_t iy = y;
    const int32_t threshold = centerThreshold < 0 ? -(int32_t)centerThreshold
                                                   : (int32_t)centerThreshold;
    if (absolute(ix) < threshold && absolute(iy) < threshold) {
      return false;
    }

    bool changed = markDot(x, y, fullScale);
    if (octagonal) {
      const AnalogTraceDirection direction = classifyOctagonalDirection(ix, iy);
      const uint32_t score = radialScore(ix, iy);
      const int32_t perpendicular = perpendicularDistance(direction, ix, iy);
      AnalogTracePoint& point = points_[(uint8_t)direction];
      if (!point.valid || score > point.score ||
          (score == point.score && perpendicular < point.perpendicular)) {
        point = { x, y, score, perpendicular, true };
        return true;
      }
      return changed;
    }

    for (uint8_t i = 0; i < directionCount(false); ++i) {
      const AnalogTraceDirection direction = orderedDirection(false, i);
      const int32_t projected = projection(direction, ix, iy);
      if (projected <= threshold) {
        continue;
      }

      const uint32_t score = (uint32_t)projected;
      const int32_t perpendicular = perpendicularDistance(direction, ix, iy);
      AnalogTracePoint& point = points_[(uint8_t)direction];
      if (!point.valid || score > point.score ||
          (score == point.score && perpendicular < point.perpendicular)) {
        point = { x, y, score, perpendicular, true };
        changed = true;
      }
    }
    return changed;
  }

  bool dotAt(uint8_t x, uint8_t y) const {
    if (x >= kPlotDiameter || y >= kPlotDiameter) {
      return false;
    }
    const uint16_t index = (uint16_t)y * kPlotDiameter + x;
    return (dots_[index >> 3] & (uint8_t)(1u << (index & 7u))) != 0;
  }

  uint16_t dotCount() const {
    return dot_count_;
  }

  const AnalogTracePoint& point(AnalogTraceDirection direction) const {
    return points_[(uint8_t)direction];
  }

  static constexpr uint8_t directionCount(bool octagonal) {
    return octagonal ? 8 : 4;
  }

  static constexpr AnalogTraceDirection orderedDirection(bool octagonal, uint8_t index) {
    if (octagonal) {
      return (AnalogTraceDirection)(index & 7u);
    }
    switch (index & 3u) {
      case 0: return AnalogTraceDirection::Up;
      case 1: return AnalogTraceDirection::Right;
      case 2: return AnalogTraceDirection::Down;
      default: return AnalogTraceDirection::Left;
    }
  }

private:
  static constexpr uint16_t kPlotDotCount =
    (uint16_t)kPlotDiameter * kPlotDiameter;
  static constexpr uint16_t kPlotDotBytes = (kPlotDotCount + 7u) / 8u;

  AnalogTracePoint points_[(uint8_t)AnalogTraceDirection::Count];
  uint8_t dots_[kPlotDotBytes];
  uint16_t dot_count_;

  static constexpr uint16_t dotIndex(uint8_t x, uint8_t y) {
    return (uint16_t)y * kPlotDiameter + x;
  }

  void setDotAt(uint8_t x, uint8_t y) {
    const uint16_t index = dotIndex(x, y);
    dots_[index >> 3] |= (uint8_t)(1u << (index & 7u));
    ++dot_count_;
  }

  void clearDotAt(uint8_t x, uint8_t y) {
    const uint16_t index = dotIndex(x, y);
    const uint8_t mask = (uint8_t)(1u << (index & 7u));
    uint8_t& byte = dots_[index >> 3];
    if ((byte & mask) == 0) {
      return;
    }
    byte &= (uint8_t)~mask;
    --dot_count_;
  }

  static constexpr uint16_t plotRadiusSquared(uint8_t x, uint8_t y) {
    const int16_t centeredX = (int16_t)x - kPlotRadius;
    const int16_t centeredY = (int16_t)y - kPlotRadius;
    return (uint16_t)(centeredX * centeredX + centeredY * centeredY);
  }

  bool markDot(int16_t x, int16_t y, int16_t fullScale) {
    const int32_t scale = fullScale < 0 ? -(int32_t)fullScale
                                        : (int32_t)fullScale;
    if (scale == 0) {
      return false;
    }
    int32_t plotX = ((int32_t)x * kPlotRadius) / scale;
    int32_t plotY = ((int32_t)y * kPlotRadius) / scale;
    if (plotX < -kPlotRadius) plotX = -kPlotRadius;
    if (plotX > kPlotRadius) plotX = kPlotRadius;
    if (plotY < -kPlotRadius) plotY = -kPlotRadius;
    if (plotY > kPlotRadius) plotY = kPlotRadius;

    const uint8_t gridX = (uint8_t)(plotX + kPlotRadius);
    const uint8_t gridY = (uint8_t)(plotY + kPlotRadius);
    if (dotAt(gridX, gridY)) {
      return false;
    }

    const uint16_t newRadius = plotRadiusSquared(gridX, gridY);
    for (int8_t dy = -1; dy <= 1; ++dy) {
      for (int8_t dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) continue;
        const int16_t neighborX = (int16_t)gridX + dx;
        const int16_t neighborY = (int16_t)gridY + dy;
        if (neighborX < 0 || neighborX >= kPlotDiameter ||
            neighborY < 0 || neighborY >= kPlotDiameter) {
          continue;
        }
        if (dotAt((uint8_t)neighborX, (uint8_t)neighborY) &&
            plotRadiusSquared((uint8_t)neighborX, (uint8_t)neighborY) >= newRadius) {
          return false;
        }
      }
    }

    for (int8_t dy = -1; dy <= 1; ++dy) {
      for (int8_t dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) continue;
        const int16_t neighborX = (int16_t)gridX + dx;
        const int16_t neighborY = (int16_t)gridY + dy;
        if (neighborX >= 0 && neighborX < kPlotDiameter &&
            neighborY >= 0 && neighborY < kPlotDiameter) {
          clearDotAt((uint8_t)neighborX, (uint8_t)neighborY);
        }
      }
    }

    if (dot_count_ >= kMaxPlotDots) {
      return false;
    }
    setDotAt(gridX, gridY);
    return true;
  }

  static constexpr int32_t absolute(int32_t value) {
    return value < 0 ? -value : value;
  }

  static constexpr AnalogTraceDirection classifyOctagonalDirection(int32_t x,
                                                                   int32_t y) {
    const int32_t ax = absolute(x);
    const int32_t ay = absolute(y);
    if (ay * 256 <= ax * 106) {
      return x < 0 ? AnalogTraceDirection::Left : AnalogTraceDirection::Right;
    }
    if (ax * 256 <= ay * 106) {
      return y < 0 ? AnalogTraceDirection::Up : AnalogTraceDirection::Down;
    }
    if (x >= 0) {
      return y < 0 ? AnalogTraceDirection::UpRight
                   : AnalogTraceDirection::DownRight;
    }
    return y < 0 ? AnalogTraceDirection::UpLeft
                 : AnalogTraceDirection::DownLeft;
  }

  static constexpr uint32_t radialScore(int32_t x, int32_t y) {
    const uint64_t ax = (uint64_t)absolute(x);
    const uint64_t ay = (uint64_t)absolute(y);
    return (uint32_t)(ax * ax + ay * ay);
  }

  static constexpr int32_t projection(AnalogTraceDirection direction,
                                      int32_t x, int32_t y) {
    switch (direction) {
      case AnalogTraceDirection::Up: return -y;
      case AnalogTraceDirection::UpRight: return x - y;
      case AnalogTraceDirection::Right: return x;
      case AnalogTraceDirection::DownRight: return x + y;
      case AnalogTraceDirection::Down: return y;
      case AnalogTraceDirection::DownLeft: return -x + y;
      case AnalogTraceDirection::Left: return -x;
      case AnalogTraceDirection::UpLeft: return -x - y;
      default: return 0;
    }
  }

  static constexpr int32_t perpendicularDistance(AnalogTraceDirection direction,
                                                 int32_t x, int32_t y) {
    switch (direction) {
      case AnalogTraceDirection::Up:
      case AnalogTraceDirection::Down:
        return absolute(x);
      case AnalogTraceDirection::Right:
      case AnalogTraceDirection::Left:
        return absolute(y);
      case AnalogTraceDirection::UpRight:
      case AnalogTraceDirection::DownLeft:
        return absolute(x + y);
      case AnalogTraceDirection::DownRight:
      case AnalogTraceDirection::UpLeft:
        return absolute(x - y);
      default:
        return 0;
    }
  }
};
