#pragma once

#include "../../core/controller_frame_state.h"
#include "../state/input_frame_runtime_state.h"

// Input-facing accessors for the shared controller-frame pipeline. Inputs own
// frame mutation, while delivery bookkeeping stays hidden behind this boundary.
inline controller_state_t& inputFrame(uint8_t index) {
  return controllerFrame(index);
}

inline const controller_state_t& inputFrameConst(uint8_t index) {
  return controllerFrameConst(index);
}

inline uint8_t inputPortCount() {
  return inputFramePortCount();
}

inline void setInputPortCount(uint8_t port_count) {
  setInputFramePortCount(port_count);
}

bool anyInputFrameConnected(uint8_t port_count);
uint8_t connectedInputFrameCount(uint8_t port_count);

bool inputPollUpdated();

void beginInputPollCycle();

void markInputPollUpdated();

void clearInputFrameButtons(uint8_t index);

void setInputFrameConnected(uint8_t index, bool connected);

void setInputFrameConnected(controller_state_t& frame, bool connected);

void clearInputFrameTypeName(uint8_t index);

void clearInputFrameTypeName(controller_state_t& frame);

void setInputFrameTypeName(controller_state_t& frame, const char* name);

void setInputFrameTypeName(uint8_t index, const char* name);

void formatInputFrameTypeName(controller_state_t& frame,
                              const char* format, ...);

void formatInputFrameTypeName(uint8_t index, const char* format, ...);

void markInputFrameUpdated(uint8_t index);
