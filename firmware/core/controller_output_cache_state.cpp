#include "controller_output_cache_state.h"

uint32_t raw_input_buttons[MAX_USB_OUT] = { 0 };
uint32_t pre_remap_buttons[MAX_USB_OUT] = { 0 };
uint32_t pre_transform_hotkey_buttons[MAX_USB_OUT] = { 0 };
uint32_t post_remap_buttons[MAX_USB_OUT] = { 0 };
int16_t post_output_lx[MAX_USB_OUT] = { 0 };
int16_t post_output_ly[MAX_USB_OUT] = { 0 };
int16_t post_output_rx[MAX_USB_OUT] = { 0 };
int16_t post_output_ry[MAX_USB_OUT] = { 0 };
uint8_t post_output_l2[MAX_USB_OUT] = { 0 };
uint8_t post_output_r2[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_x[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_y[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_z[MAX_USB_OUT] = { 0 };
uint8_t debug_hid_rx[MAX_USB_OUT] = { 0 };
