#include "input_psx_negcon_runtime_state.h"
#include <PsxNewLib/PsxPublicTypes.h>

namespace {
constexpr uint8_t kPsxNegconPorts = 2;
}

bool negcon_origin_valid[kPsxNegconPorts] = { false };
uint8_t negcon_origin_twist[kPsxNegconPorts] = { 0x80 };
uint8_t negcon_last_x[kPsxNegconPorts] = { ANALOG_IDLE_VALUE };
uint8_t negcon_last_z[kPsxNegconPorts] = { ANALOG_MAX_VALUE };
uint8_t negcon_last_cross[kPsxNegconPorts] = { ANALOG_MAX_VALUE };
uint8_t negcon_last_square[kPsxNegconPorts] = { ANALOG_MAX_VALUE };
bool negcon_inited = false;
uint8_t negcon_maxpos[kPsxNegconPorts] = { 0 };
