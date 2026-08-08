#pragma once

#include <cstdint>

constexpr uint8_t XINPUTW_INVALID_PORT = 0xFF;

#ifdef __cplusplus
extern "C" {
#endif

void xinputw_slot_routing_reset();
bool xinputw_slot_routing_update(uint8_t sourceCount,
                                 uint32_t connectedMask,
                                 bool compactConnectedSources);
uint8_t xinputw_target_slot_for_source(uint8_t sourcePort);
uint8_t xinputw_source_port_for_target(uint8_t targetSlot);

#ifdef __cplusplus
}
#endif
