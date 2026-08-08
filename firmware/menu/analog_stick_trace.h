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
         strcmp(controllerType, "ClassicPro") == 0;
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
  AnalogStickTrace() {
    reset();
  }

  void reset() {
    for (uint8_t i = 0; i < directionCount(true); ++i) {
      points_[i] = { 0, 0, 0, 0, false };
    }
  }

  bool sample(int16_t x, int16_t y, bool octagonal, int16_t centerThreshold) {
    const int32_t ix = x;
    const int32_t iy = y;
    const int32_t threshold = centerThreshold < 0 ? -(int32_t)centerThreshold
                                                   : (int32_t)centerThreshold;
    if (absolute(ix) < threshold && absolute(iy) < threshold) {
      return false;
    }

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
      return false;
    }

    bool changed = false;
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
  AnalogTracePoint points_[(uint8_t)AnalogTraceDirection::Count];

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
