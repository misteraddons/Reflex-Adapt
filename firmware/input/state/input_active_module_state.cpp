#include "../../product_config.h"

#include "input_active_module_state.h"

#include "../base/RZInputModule.h"

namespace {

RZInputModule* active_input_module = nullptr;

}  // namespace

RZInputModule* activeInputModule() {
  return active_input_module;
}

void setActiveInputModule(RZInputModule* module) {
  if (active_input_module != nullptr && active_input_module != module) {
    delete active_input_module;
  }
  active_input_module = module;
}

bool hasActiveInputModule() {
  return activeInputModule() != nullptr;
}
