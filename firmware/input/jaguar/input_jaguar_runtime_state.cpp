#include "../../product_config.h"

#include "input_jaguar_runtime_state.h"

bool jaguarRotaryActive = false;
bool jaguarRotaryActivePorts[MAX_USB_OUT] = { false };

void resetJaguarRotaryRuntimeState() {
  jaguarRotaryActive = false;
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    jaguarRotaryActivePorts[i] = false;
  }
}
