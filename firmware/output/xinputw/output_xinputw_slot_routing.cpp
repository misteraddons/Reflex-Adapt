#include "output_xinputw_slot_routing.h"

#include <cstring>

#include "out_xinputw.h"

namespace {

uint8_t sourceToTarget[XINPUT_WIRELESS_CONTROLLERS] = {
  XINPUTW_INVALID_PORT, XINPUTW_INVALID_PORT,
  XINPUTW_INVALID_PORT, XINPUTW_INVALID_PORT,
};
uint8_t targetToSource[XINPUT_WIRELESS_CONTROLLERS] = {
  XINPUTW_INVALID_PORT, XINPUTW_INVALID_PORT,
  XINPUTW_INVALID_PORT, XINPUTW_INVALID_PORT,
};

}  // namespace

void xinputw_slot_routing_reset() {
  memset(sourceToTarget, XINPUTW_INVALID_PORT, sizeof(sourceToTarget));
  memset(targetToSource, XINPUTW_INVALID_PORT, sizeof(targetToSource));
}

bool xinputw_slot_routing_update(uint8_t sourceCount,
                                 uint32_t connectedMask,
                                 bool compactConnectedSources) {
  uint8_t nextSourceToTarget[XINPUT_WIRELESS_CONTROLLERS];
  uint8_t nextTargetToSource[XINPUT_WIRELESS_CONTROLLERS];
  memset(nextSourceToTarget, XINPUTW_INVALID_PORT, sizeof(nextSourceToTarget));
  memset(nextTargetToSource, XINPUTW_INVALID_PORT, sizeof(nextTargetToSource));

  if (sourceCount > XINPUT_WIRELESS_CONTROLLERS) {
    sourceCount = XINPUT_WIRELESS_CONTROLLERS;
  }

  uint8_t nextTarget = 0;
  for (uint8_t source = 0; source < sourceCount; ++source) {
    if ((connectedMask & (1UL << source)) == 0) {
      continue;
    }

    const uint8_t target = compactConnectedSources ? nextTarget++ : source;
    nextSourceToTarget[source] = target;
    nextTargetToSource[target] = source;
  }

  const bool changed =
    memcmp(sourceToTarget, nextSourceToTarget, sizeof(sourceToTarget)) != 0 ||
    memcmp(targetToSource, nextTargetToSource, sizeof(targetToSource)) != 0;
  memcpy(sourceToTarget, nextSourceToTarget, sizeof(sourceToTarget));
  memcpy(targetToSource, nextTargetToSource, sizeof(targetToSource));
  return changed;
}

uint8_t xinputw_target_slot_for_source(uint8_t sourcePort) {
  if (sourcePort >= XINPUT_WIRELESS_CONTROLLERS) {
    return XINPUTW_INVALID_PORT;
  }
  return sourceToTarget[sourcePort];
}

uint8_t xinputw_source_port_for_target(uint8_t targetSlot) {
  if (targetSlot >= XINPUT_WIRELESS_CONTROLLERS) {
    return XINPUTW_INVALID_PORT;
  }
  return targetToSource[targetSlot];
}
