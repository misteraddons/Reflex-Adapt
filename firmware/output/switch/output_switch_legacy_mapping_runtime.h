#pragma once

// Internal Switch and legacy console output mapping helpers. These stay
// header-only so they can reuse the shared runtime flags, report objects, and
// conversion helpers still owned by out_usb.h.

#include "../specialized/output_pokken_mapping_runtime.h"
#include "output_switchpro_mapping_runtime.h"
#include "../specialized/output_pantherlord_mapping_runtime.h"
#include "../specialized/output_gcwiiu_mapping_runtime.h"
#include "../specialized/output_mdmini_mapping_runtime.h"
