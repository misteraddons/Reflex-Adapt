#pragma once

#include <Arduino.h>

bool serialIsOledCommand(const char* command);
bool handleSerialOledCommand(const char* command, Print& out);
