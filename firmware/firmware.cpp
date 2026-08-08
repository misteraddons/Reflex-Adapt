// Product configuration - must be first (sets ENABLE_INPUT_*, pin defines, etc.)
// The release repository builds the Classic2USB product profile.
#include "product_config.h"

#include <Arduino.h>

#include "core/runtime/firmware_runtime.h"

void setup() {
  runFirmwareSetup();
}

void loop() {
  runFirmwareLoop();
}
