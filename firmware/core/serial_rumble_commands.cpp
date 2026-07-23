#include "serial_rumble_commands.h"
#include "serial_command_parser.h"

#include <Arduino.h>

#include "rumble_test_runtime.h"
#include "../firmware_platform_config.h"

namespace {

void printRumbleStatus(Print& out) {
  out.print(F("RUMBLE ACTIVE="));
  out.println(rumbleTestActive() ? 1 : 0);
  for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
    RumbleRuntimePortDiag diag{};
    if (!rumbleRuntimeGetPortDiag(port, &diag)) {
      continue;
    }
    out.print(F("RUMBLE I="));
    out.print((int)port);
    out.print(F(" RAW="));
    out.print((int)diag.raw_left);
    out.print('/');
    out.print((int)diag.raw_right);
    out.print(F(" SCALED="));
    out.print((int)diag.scaled_left);
    out.print('/');
    out.print((int)diag.scaled_right);
    out.print(F(" TEST="));
    out.print(diag.test_active ? 1 : 0);
    out.print(':');
    out.print((int)diag.test_left);
    out.print('/');
    out.print((int)diag.test_right);
    out.print(F(" ZERO="));
    out.print(diag.zero_pending ? 1 : 0);
    out.print(F(" UPDATES="));
    out.println(diag.update_count);
  }
}

}  // namespace

bool handleSerialRumbleCommand(char* text, Print& out) {
  if (*text == '\0' || serialTokenEquals(text, "STATUS")) {
    printRumbleStatus(out);
    return true;
  }
  if (serialTokenEquals(text, "HELP")) {
    out.println(F("RUMBLE CMDS:RUMBLE STATUS,RUMBLE TEST <LEFT> <RIGHT> [MS],RUMBLE STOP"));
    return true;
  }
  if (serialTokenEquals(text, "STOP")) {
    rumbleTestStop();
    printRumbleStatus(out);
    return true;
  }

  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "TEST", &remainder)) {
    long rawLeft = 0;
    long rawRight = 0;
    long rawMs = 3000;
    if (!serialParseLongToken(remainder, &rawLeft) ||
        !serialParseLongToken(remainder, &rawRight) ||
        rawLeft < 0 || rawLeft > 255 ||
        rawRight < 0 || rawRight > 255) {
      out.println(F("ERR:BAD_RUMBLE_TEST"));
      return true;
    }
    if (*serialSkipSpaces(remainder) != '\0' &&
        (!serialParseLongToken(remainder, &rawMs) ||
         rawMs < 1 || rawMs > 30000)) {
      out.println(F("ERR:BAD_RUMBLE_TEST"));
      return true;
    }
    rumbleTestStart((uint8_t)rawLeft, (uint8_t)rawRight, (uint16_t)rawMs);
    printRumbleStatus(out);
    return true;
  }

  out.println(F("ERR:BAD_RUMBLE_CMD"));
  return true;
}
