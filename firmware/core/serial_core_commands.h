#pragma once

#include <Arduino.h>

bool handleSerialSetCommand(char* text, Print& out);
bool handleSerialHotkeyCommand(char* text, Print& out);
bool handleSerialChordCommand(char* text, Print& out);
bool handleSerialUiCommand(char* text, Print& out);
bool handleSerialBootCommand(const char* command, Print& out);
bool handleSerialStateCommand(const char* command, Print& out);
bool handleSerialGpioCommand(const char* command, Print& out);
bool handleSerialDreamcastCommand(const char* command, Print& out);
