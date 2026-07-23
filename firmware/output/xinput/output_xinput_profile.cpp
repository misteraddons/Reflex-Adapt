#include <string.h>

#include "../../core/controller_frame_state.h"
#include "out_xinput.h"
#include "output_xinput_profile.h"

uint8_t determine_xinput_subtype() {
  const char* type = controllerFrameConst(0).controller_type_name;
  if (type && type[0] != '\0') {
    // Wheels/steering controllers
    if (strstr(type, "JogCon") || strstr(type, "NeGcon") || strstr(type, "Wheel")) {
      return XINPUT_SUBTYPE_WHEEL;
    }
    // Dance pads
    if (strstr(type, "Dance")) {
      return XINPUT_SUBTYPE_DANCEPAD;
    }
    // Guitar controllers
    if (strstr(type, "Guitar")) {
      return XINPUT_SUBTYPE_GUITAR;
    }
    // Arcade pads (Pop'n Music, etc.)
    if (strstr(type, "Pop")) {
      return XINPUT_SUBTYPE_ARCADEPAD;
    }
    // Flight sticks
    if (strstr(type, "Flight")) {
      return XINPUT_SUBTYPE_FLIGHTSTICK;
    }
  }
  return XINPUT_SUBTYPE_GAMEPAD;
}
