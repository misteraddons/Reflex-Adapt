#pragma once

#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
namespace input_autodetect_passive_internal {

inline constexpr uint32_t ASSISTED_ROUTE_STABLE_MS = 60;

AutoDetectResult probePassiveOnlyAssistedRoute(uint8_t port);
bool passiveHoldRoutePresent(uint8_t port);

#ifdef ENABLE_INPUT_JAGUAR
bool jaguarAssistHoldPresent(uint8_t port);
AutoDetectResult probeJaguarAssistHeldInternal(uint8_t port);
#endif

}  // namespace input_autodetect_passive_internal
#endif
