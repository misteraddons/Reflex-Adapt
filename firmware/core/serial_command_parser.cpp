#include "serial_command_parser.h"

#include <stdlib.h>
#include <stdint.h>

namespace {

char toUpperAscii(char value) {
  return (value >= 'a' && value <= 'z') ? (char)(value - ('a' - 'A')) : value;
}

}  // namespace

char* serialSkipSpaces(char* text) {
  while (*text == ' ' || *text == '\t') {
    ++text;
  }
  return text;
}

bool serialTokenEquals(const char* token, const char* expected) {
  size_t index = 0;
  while (expected[index] != '\0') {
    if (token[index] == '\0' ||
        token[index] == ' ' ||
        token[index] == '\t' ||
        toUpperAscii(token[index]) != expected[index]) {
      return false;
    }
    ++index;
  }
  return token[index] == '\0' || token[index] == ' ' || token[index] == '\t';
}

bool serialCommandStartsWith(const char* command, const char* expected, char** remainder) {
  size_t index = 0;
  while (expected[index] != '\0') {
    if (toUpperAscii(command[index]) != expected[index]) {
      return false;
    }
    ++index;
  }

  char* tail = const_cast<char*>(&command[index]);
  if (*tail == '\0') {
    *remainder = tail;
    return true;
  }
  if (*tail != ' ' && *tail != '\t') {
    return false;
  }
  *remainder = serialSkipSpaces(tail);
  return true;
}

bool serialParseLongToken(char*& text, long* value) {
  text = serialSkipSpaces(text);
  if (*text == '\0') {
    return false;
  }

  char* end = nullptr;
  const long parsed = strtol(text, &end, 0);
  if (end == text) {
    return false;
  }
  *value = parsed;
  text = serialSkipSpaces(end);
  return true;
}

bool serialParseUint32Token(char*& text, uint32_t* value) {
  text = serialSkipSpaces(text);
  if (*text == '\0') {
    return false;
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(text, &end, 0);
  if (end == text) {
    return false;
  }
  *value = (uint32_t)parsed;
  text = serialSkipSpaces(end);
  return true;
}
