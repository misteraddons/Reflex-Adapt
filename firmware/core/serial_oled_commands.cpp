#include "serial_oled_commands.h"

#include "oled_serial_runtime.h"
#include "serial_command_parser.h"

bool serialIsOledCommand(const char* command) {
  char* remainder = nullptr;
  return serialCommandStartsWith(command, "OLED", &remainder) ||
         serialCommandStartsWith(command, "OLEDMIRROR", &remainder) ||
         serialCommandStartsWith(command, "OLEDSTREAM", &remainder);
}

bool handleSerialOledCommand(const char* command, Print& out) {
  char* remainder = nullptr;
  if (!(serialCommandStartsWith(command, "OLED", &remainder) ||
        serialCommandStartsWith(command, "OLEDMIRROR", &remainder) ||
        serialCommandStartsWith(command, "OLEDSTREAM", &remainder))) {
    return false;
  }

  if (*remainder == '\0' || serialTokenEquals(remainder, "STATUS")) {
    oledSerialWriteStatus(out);
    return true;
  }
  if (serialTokenEquals(remainder, "ON") ||
      serialTokenEquals(remainder, "START")) {
    oledSerialSetEnabled(true);
    out.println(F("OK:OLED=1"));
    return true;
  }
  if (serialTokenEquals(remainder, "OFF") ||
      serialTokenEquals(remainder, "STOP")) {
    oledSerialSetEnabled(false);
    out.println(F("OK:OLED=0"));
    return true;
  }
  if (serialTokenEquals(remainder, "FRAME") ||
      serialTokenEquals(remainder, "SNAP")) {
    oledSerialWriteFrame(out);
    return true;
  }
  if (serialCommandStartsWith(command, "OLED RATE", &remainder) ||
      serialCommandStartsWith(command, "OLEDMIRROR RATE", &remainder) ||
      serialCommandStartsWith(command, "OLEDSTREAM RATE", &remainder)) {
    long rateHz = 0;
    if (!serialParseLongToken(remainder, &rateHz) ||
        rateHz < OLED_SERIAL_MIN_RATE_HZ ||
        rateHz > OLED_SERIAL_MAX_RATE_HZ) {
      out.println(F("ERR:BAD_OLED_RATE"));
    } else {
      oledSerialSetRateHz((uint16_t)rateHz);
      out.print(F("OK:OLED_RATE="));
      out.println((int)oledSerialRateHz());
    }
    return true;
  }

  out.println(F("ERR:BAD_OLED_CMD"));
  return true;
}
