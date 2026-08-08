#pragma once

#include <Arduino.h>

void serialDebugRuntimeSetup();
void serialDebugRuntimeTask();
bool handleSerialDebugCommand(const char* command, Print& out);
