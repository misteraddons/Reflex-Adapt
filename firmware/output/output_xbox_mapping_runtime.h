#pragma once

// Internal Xbox-family output mapping helpers. This stays header-only so the
// helpers can reuse the shared runtime flags, output reports, and conversion
// helpers still owned by out_usb.h.
#include "xinput/output_xinput_mapping_runtime.h"
#include "xinputw/output_xinputw_mapping_runtime.h"
#include "xid/output_xid_mapping_runtime.h"
