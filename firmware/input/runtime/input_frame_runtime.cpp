#include "input_frame_runtime.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "../../core/controller_delivery_state.h"
#include "../state/input_poll_runtime_state.h"

bool anyInputFrameConnected(uint8_t port_count) {
  for (uint8_t i = 0; i < port_count; ++i) {
    if (inputFrameConst(i).connected) {
      return true;
    }
  }
  return false;
}

uint8_t connectedInputFrameCount(uint8_t port_count) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < port_count; ++i) {
    if (inputFrameConst(i).connected) {
      ++count;
    }
  }
  return count;
}

bool inputPollUpdated() {
  return inputPollCycleUpdated();
}

void beginInputPollCycle() {
  resetInputPollCycleUpdated();
}

void markInputPollUpdated() {
  markInputPollCycleUpdated();
}

void clearInputFrameButtons(uint8_t index) {
  inputFrame(index).digital_buttons = 0;
}

void setInputFrameConnected(uint8_t index, bool connected) {
  inputFrame(index).connected = connected;
}

void setInputFrameConnected(controller_state_t& frame, bool connected) {
  frame.connected = connected;
}

void clearInputFrameTypeName(uint8_t index) {
  inputFrame(index).controller_type_name[0] = '\0';
}

void clearInputFrameTypeName(controller_state_t& frame) {
  frame.controller_type_name[0] = '\0';
}

void setInputFrameTypeName(controller_state_t& frame, const char* name) {
  if (!name || !name[0]) {
    frame.controller_type_name[0] = '\0';
    return;
  }
  strncpy(frame.controller_type_name, name,
          sizeof(frame.controller_type_name) - 1);
  frame.controller_type_name[sizeof(frame.controller_type_name) - 1] = '\0';
}

void setInputFrameTypeName(uint8_t index, const char* name) {
  setInputFrameTypeName(inputFrame(index), name);
}

void formatInputFrameTypeName(controller_state_t& frame,
                              const char* format, ...) {
  if (!format || !format[0]) {
    clearInputFrameTypeName(frame);
    return;
  }

  va_list args;
  va_start(args, format);
  vsnprintf(frame.controller_type_name, sizeof(frame.controller_type_name),
            format, args);
  va_end(args);
}

void formatInputFrameTypeName(uint8_t index, const char* format, ...) {
  controller_state_t& frame = inputFrame(index);
  if (!format || !format[0]) {
    clearInputFrameTypeName(frame);
    return;
  }

  va_list args;
  va_start(args, format);
  vsnprintf(frame.controller_type_name, sizeof(frame.controller_type_name),
            format, args);
  va_end(args);
}

void markInputFrameUpdated(uint8_t index) {
  requestControllerFrameDelivery(index);
  markInputPollUpdated();
}
