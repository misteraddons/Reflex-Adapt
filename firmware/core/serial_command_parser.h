#pragma once

#include <stdint.h>

char* serialSkipSpaces(char* text);
bool serialTokenEquals(const char* token, const char* expected);
bool serialCommandStartsWith(const char* command, const char* expected, char** remainder);
bool serialParseLongToken(char*& text, long* value);
bool serialParseUint32Token(char*& text, uint32_t* value);
