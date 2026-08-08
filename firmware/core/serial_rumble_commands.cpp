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
    out.print(F(" UPDATES="));
    out.println(diag.update_count);
  }
}

}  // namespace

bool handleSerialRumbleCommand(char* text, Print& out) {
  if (*text == '\0' || serialTextEqualsExact(text, "STATUS")) {
    printRumbleStatus(out);
    return true;
  }
  if (serialTextEqualsExact(text, "HELP")) {
    out.println(F("RUMBLE CMDS:RUMBLE STATUS,RUMBLE TEST <LEFT> <RIGHT> [MS],RUMBLE TEST PLAYER <1-N> <LEFT> <RIGHT> [MS],RUMBLE STOP"));
    return true;
  }
  if (serialTextEqualsExact(text, "STOP")) {
    rumbleTestStop();
    printRumbleStatus(out);
    return true;
  }

  char* remainder = nullptr;
  if (serialCommandStartsWith(text, "PLAYER", &remainder) ||
      serialCommandStartsWith(text, "TEST PLAYER", &remainder)) {
    long rawPlayer = 0;
    long rawLeft = 0;
    long rawRight = 0;
    long rawMs = 3000;
    if (!serialParseLongToken(remainder, &rawPlayer) ||
        !serialParseLongToken(remainder, &rawLeft) ||
        !serialParseLongToken(remainder, &rawRight) ||
        rawPlayer < 1 || rawPlayer > MAX_USB_OUT ||
        rawLeft < 0 || rawLeft > 255 ||
        rawRight < 0 || rawRight > 255) {
      out.println(F("ERR:BAD_RUMBLE_PLAYER_TEST"));
      return true;
    }
    if (*serialSkipSpaces(remainder) != '\0' &&
        (!serialParseLongToken(remainder, &rawMs) ||
         rawMs < 1 || rawMs > 30000)) {
      out.println(F("ERR:BAD_RUMBLE_PLAYER_TEST"));
      return true;
    }
    if (*serialSkipSpaces(remainder) != '\0') {
      out.println(F("ERR:BAD_RUMBLE_PLAYER_TEST"));
      return true;
    }
    rumbleRuntimeStartTest(
        1UL << static_cast<uint8_t>(rawPlayer - 1),
        static_cast<uint8_t>(rawLeft),
        static_cast<uint8_t>(rawRight),
        static_cast<uint16_t>(rawMs));
    out.print(F("OK:RUMBLE_PLAYER="));
    out.print(rawPlayer);
    out.print(F(" LEFT="));
    out.print(rawLeft);
    out.print(F(" RIGHT="));
    out.print(rawRight);
    out.print(F(" MS="));
    out.println(rawMs);
    return true;
  }
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
    if (*serialSkipSpaces(remainder) != '\0') {
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
