#pragma once

#include <stdint.h>
#include <string.h>

#include "../core/controller_state.h"
#include "../core/device_mode.h"
#include "../firmware_platform_config.h"

enum class AnalogDiagnosticKind : uint8_t {
  None = 0,
  Stick,
  Wheel,
  NeGcon,
  Paddle,
};

enum class AnalogDiagnosticStick : uint8_t {
  Main = 0,
  Aux,
};

struct AnalogDiagnosticTarget {
  uint8_t port;
  AnalogDiagnosticStick stick;
  AnalogDiagnosticKind kind;
};

constexpr uint8_t kAnalogDiagnosticNoPort = MAX_USB_OUT;

inline AnalogDiagnosticTarget analogDiagnosticNoTarget() {
  return {
    kAnalogDiagnosticNoPort,
    AnalogDiagnosticStick::Main,
    AnalogDiagnosticKind::None,
  };
}

inline bool analogDiagnosticTargetIsValid(const AnalogDiagnosticTarget& target) {
  return target.port < MAX_USB_OUT &&
         target.kind != AnalogDiagnosticKind::None;
}

inline bool analogDiagnosticTypeEquals(const controller_state_t& frame,
                                       const char* expected) {
  return expected != nullptr &&
         strcmp(frame.controller_type_name, expected) == 0;
}

inline bool analogDiagnosticTypeStartsWith(const controller_state_t& frame,
                                           const char* prefix,
                                           uint8_t prefixLength) {
  return prefix != nullptr &&
         strncmp(frame.controller_type_name, prefix, prefixLength) == 0;
}

inline bool analogDiagnosticFrameHasStick(
    const controller_state_t& frame, AnalogDiagnosticStick stick) {
  return stick == AnalogDiagnosticStick::Aux
           ? frame.HAS_ANALOG_STICK_AUX
           : frame.HAS_ANALOG_STICK_MAIN;
}

inline AnalogDiagnosticKind analogDiagnosticKindForFrame(
    DeviceEnum mode, const controller_state_t& frame) {
  if (!frame.connected) {
    return AnalogDiagnosticKind::None;
  }

  const bool hasStick =
      frame.HAS_ANALOG_STICK_MAIN || frame.HAS_ANALOG_STICK_AUX;

  if (analogDiagnosticTypeEquals(frame, "VB Pad")) {
    return AnalogDiagnosticKind::None;
  }
  if (hasStick && analogDiagnosticTypeEquals(frame, "Driving")) {
    return AnalogDiagnosticKind::Wheel;
  }
  if (mode == RZORD_PADDLE) {
    return AnalogDiagnosticKind::Paddle;
  }
  if (mode == RZORD_DRIVING) {
    return AnalogDiagnosticKind::Wheel;
  }
  if (mode == RZORD_GAMEPORT) {
    return AnalogDiagnosticKind::Stick;
  }

  if (mode == RZORD_N64 || mode == RZORD_GAMECUBE) {
    if (hasStick &&
        (analogDiagnosticTypeEquals(frame, "N64 Pad") ||
         analogDiagnosticTypeEquals(frame, "N64 Rumble") ||
         analogDiagnosticTypeEquals(frame, "GC Pad") ||
         analogDiagnosticTypeEquals(frame, "WaveBird"))) {
      return AnalogDiagnosticKind::Stick;
    }
    return AnalogDiagnosticKind::None;
  }

  if (mode == RZORD_WII) {
    if (hasStick &&
        (analogDiagnosticTypeEquals(frame, "Classic") ||
         analogDiagnosticTypeEquals(frame, "ClassicPro") ||
         analogDiagnosticTypeEquals(frame, "Nunchuk"))) {
      return AnalogDiagnosticKind::Stick;
    }
    return AnalogDiagnosticKind::None;
  }

  if (mode == RZORD_SATURN) {
    if (!hasStick) {
      return AnalogDiagnosticKind::None;
    }
    if (analogDiagnosticTypeEquals(frame, "Wheel")) {
      return AnalogDiagnosticKind::Wheel;
    }
    if (analogDiagnosticTypeEquals(frame, "3D Pad") ||
        analogDiagnosticTypeEquals(frame, "Mission") ||
        analogDiagnosticTypeEquals(frame, "Mission6")) {
      return AnalogDiagnosticKind::Stick;
    }
    return AnalogDiagnosticKind::None;
  }

  if (mode == RZORD_DREAMCAST) {
    if (!hasStick) {
      return AnalogDiagnosticKind::None;
    }
    if (analogDiagnosticTypeEquals(frame, "Wheel") ||
        analogDiagnosticTypeStartsWith(frame, "Whl+", 4)) {
      return AnalogDiagnosticKind::Wheel;
    }
    if (analogDiagnosticTypeEquals(frame, "Pad") ||
        analogDiagnosticTypeEquals(frame, "Pad + VMU") ||
        analogDiagnosticTypeEquals(frame, "Pad + Rmb") ||
        analogDiagnosticTypeEquals(frame, "Pad + Both") ||
        analogDiagnosticTypeEquals(frame, "Mission") ||
        analogDiagnosticTypeEquals(frame, "Mission+VMU")) {
      return AnalogDiagnosticKind::Stick;
    }
    return AnalogDiagnosticKind::None;
  }

  if (mode == RZORD_PSX || mode == RZORD_PSX_JOG) {
    if (analogDiagnosticTypeEquals(frame, "neGcon")) {
      return AnalogDiagnosticKind::NeGcon;
    }
    if (analogDiagnosticTypeStartsWith(frame, "JogCon", 6)) {
      return frame.HAS_ANALOG_STICK_MAIN
               ? AnalogDiagnosticKind::Wheel
               : AnalogDiagnosticKind::None;
    }
    if (hasStick &&
        (analogDiagnosticTypeEquals(frame, "DualShock") ||
         analogDiagnosticTypeEquals(frame, "DualShock2") ||
         analogDiagnosticTypeEquals(frame, "FlightStick"))) {
      return AnalogDiagnosticKind::Stick;
    }
    return AnalogDiagnosticKind::None;
  }

  return hasStick ? AnalogDiagnosticKind::Stick
                  : AnalogDiagnosticKind::None;
}

inline uint8_t analogDiagnosticFrameLimit(uint8_t frameCount) {
  return frameCount < MAX_USB_OUT ? frameCount : MAX_USB_OUT;
}

inline AnalogDiagnosticTarget analogDiagnosticTargetForPort(
    DeviceEnum mode, const controller_state_t* frames, uint8_t frameCount,
    uint8_t port,
    AnalogDiagnosticStick preferredStick = AnalogDiagnosticStick::Main) {
  const uint8_t limit = analogDiagnosticFrameLimit(frameCount);
  if (frames == nullptr || port >= limit) {
    return analogDiagnosticNoTarget();
  }

  const controller_state_t& frame = frames[port];
  const AnalogDiagnosticKind kind =
      analogDiagnosticKindForFrame(mode, frame);
  if (kind == AnalogDiagnosticKind::None) {
    return analogDiagnosticNoTarget();
  }

  AnalogDiagnosticStick stick = AnalogDiagnosticStick::Main;
  if (kind == AnalogDiagnosticKind::Stick) {
    if (analogDiagnosticFrameHasStick(frame, preferredStick)) {
      stick = preferredStick;
    } else if (frame.HAS_ANALOG_STICK_MAIN) {
      stick = AnalogDiagnosticStick::Main;
    } else if (frame.HAS_ANALOG_STICK_AUX) {
      stick = AnalogDiagnosticStick::Aux;
    } else if (mode == RZORD_GAMEPORT) {
      stick = AnalogDiagnosticStick::Main;
    } else {
      return analogDiagnosticNoTarget();
    }
  }

  return { port, stick, kind };
}

inline AnalogDiagnosticTarget analogDiagnosticDefaultTarget(
    DeviceEnum mode, const controller_state_t* frames, uint8_t frameCount) {
  const uint8_t limit = analogDiagnosticFrameLimit(frameCount);
  if (frames == nullptr) {
    return analogDiagnosticNoTarget();
  }

  for (uint8_t port = 0; port < limit; ++port) {
    const AnalogDiagnosticTarget target =
        analogDiagnosticTargetForPort(mode, frames, limit, port);
    if (analogDiagnosticTargetIsValid(target)) {
      return target;
    }
  }
  return analogDiagnosticNoTarget();
}

inline AnalogDiagnosticTarget analogDiagnosticNextPortTarget(
    DeviceEnum mode, const controller_state_t* frames, uint8_t frameCount,
    const AnalogDiagnosticTarget& current, int8_t direction = 1) {
  const uint8_t limit = analogDiagnosticFrameLimit(frameCount);
  if (frames == nullptr || limit == 0) {
    return analogDiagnosticNoTarget();
  }

  uint8_t cursor = current.port < limit
                     ? current.port
                     : (direction < 0 ? 0 : (uint8_t)(limit - 1));
  for (uint8_t scanned = 0; scanned < limit; ++scanned) {
    cursor = direction < 0
               ? (uint8_t)((cursor + limit - 1) % limit)
               : (uint8_t)((cursor + 1) % limit);
    const AnalogDiagnosticTarget target = analogDiagnosticTargetForPort(
        mode, frames, limit, cursor, current.stick);
    if (analogDiagnosticTargetIsValid(target)) {
      return target;
    }
  }
  return analogDiagnosticNoTarget();
}

inline AnalogDiagnosticTarget analogDiagnosticNextStickTarget(
    DeviceEnum mode, const controller_state_t* frames, uint8_t frameCount,
    const AnalogDiagnosticTarget& current) {
  if (frames == nullptr) {
    return analogDiagnosticNoTarget();
  }

  AnalogDiagnosticTarget target = analogDiagnosticTargetForPort(
      mode, frames, frameCount, current.port, current.stick);
  if (!analogDiagnosticTargetIsValid(target)) {
    return analogDiagnosticDefaultTarget(mode, frames, frameCount);
  }
  if (target.kind != AnalogDiagnosticKind::Stick) {
    return target;
  }

  const controller_state_t& frame = frames[target.port];
  if (frame.HAS_ANALOG_STICK_MAIN && frame.HAS_ANALOG_STICK_AUX) {
    target.stick = target.stick == AnalogDiagnosticStick::Main
                     ? AnalogDiagnosticStick::Aux
                     : AnalogDiagnosticStick::Main;
  }
  return target;
}
