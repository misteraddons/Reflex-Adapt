#pragma once

#include <Arduino.h>

bool handleSerialManagementCommand(const char* command, Print& out);
void appendSerialManagementHelp(Print& out);
