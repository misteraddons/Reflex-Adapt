#include "output_xinput_auth_runtime_state.h"

namespace {

XInputAuthRuntimeState xinput_auth_runtime_state;

}

XInputAuthRuntimeState& xinputAuthRuntimeState() {
  return xinput_auth_runtime_state;
}
