#pragma once

// Internal per-mode USB output setup helpers. Keep this as the umbrella include
// so existing output code can pull in the family-specific setup groups from one
// stable place while the actual setup paths live in smaller owned headers.

#include "output_usb_player_count.h"
#include "output_usb_mode_setup_hid_runtime.h"
#include "output_usb_mode_setup_console_runtime.h"
#include "output_usb_mode_setup_specialized_runtime.h"
