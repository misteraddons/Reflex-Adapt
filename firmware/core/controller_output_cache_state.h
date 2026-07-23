#pragma once

#include <cstdint>

#include "../firmware_platform_config.h"

// Cached snapshots used by menu/debug/output inspection paths.
extern uint32_t raw_input_buttons[MAX_USB_OUT];
extern uint32_t pre_remap_buttons[MAX_USB_OUT];
extern uint32_t pre_transform_hotkey_buttons[MAX_USB_OUT];
extern uint32_t post_remap_buttons[MAX_USB_OUT];
extern int16_t post_output_lx[MAX_USB_OUT];
extern int16_t post_output_ly[MAX_USB_OUT];
extern int16_t post_output_rx[MAX_USB_OUT];
extern int16_t post_output_ry[MAX_USB_OUT];
extern uint8_t post_output_l2[MAX_USB_OUT];
extern uint8_t post_output_r2[MAX_USB_OUT];
extern uint8_t debug_hid_x[MAX_USB_OUT];
extern uint8_t debug_hid_y[MAX_USB_OUT];
extern uint8_t debug_hid_z[MAX_USB_OUT];
extern uint8_t debug_hid_rx[MAX_USB_OUT];
