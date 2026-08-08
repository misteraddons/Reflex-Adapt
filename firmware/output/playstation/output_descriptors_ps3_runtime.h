#pragma once

// PS3 descriptor family extracted from
// output_descriptors_playstation_runtime.h.

#include "output_descriptors_ps3_feature_reports_runtime.h"
#include "output_descriptors_ps3_hid_runtime.h"

typedef struct TU_ATTR_PACKED
{
//  // digital buttons, 0 = off, 1 = on
//
//  uint8_t report_id;
//  uint8_t : 8;
//
//  uint8_t square_btn : 1;
//  uint8_t cross_btn : 1;
//  uint8_t circle_btn : 1;
//  uint8_t triangle_btn : 1;
//
//  uint8_t l1_btn : 1;
//  uint8_t r1_btn : 1;
//  uint8_t l2_btn : 1;
//  uint8_t r2_btn : 1;
//
//  uint8_t select_btn : 1;
//  uint8_t start_btn : 1;
//  uint8_t l3_btn : 1;
//  uint8_t r3_btn : 1;
//  
//  uint8_t ps_btn : 1;
//
//  uint8_t : 3;
//
//  // digital direction, use the dir_* constants(enum)
//  // 8 = center, 0 = up, 1 = up/right, 2 = right, 3 = right/down
//  // 4 = down, 5 = down/left, 6 = left, 7 = left/up
//
//  uint8_t direction;
//
//  // left and right analog sticks, 0x00 left/up, 0x80 middle, 0xff right/down
//
//  uint8_t l_x_axis;
//  uint8_t l_y_axis;
//  uint8_t r_x_axis;
//  uint8_t r_y_axis;
//
//  // Gonna assume these are button analog values for the d-pad.
//  // NOTE: NOT EVEN SURE THIS IS RIGHT, OR IN THE CORRECT ORDER
//  uint8_t right_axis;
//  uint8_t left_axis;
//  uint8_t up_axis;
//  uint8_t down_axis;
//
//  // button axis, 0x00 = unpressed, 0xff = fully pressed
//
//  uint8_t triangle_axis;
//  uint8_t circle_axis;
//  uint8_t cross_axis;
//  uint8_t square_axis;
//
//  uint8_t l1_axis;
//  uint8_t r1_axis;
//  uint8_t l2_axis;
//  uint8_t r2_axis;

    //uint8_t report_id;
    uint8_t unk0;

    //uint8_t buttons[3];
    // 0
    uint8_t select_btn : 1;
    uint8_t l3_btn : 1;
    uint8_t r3_btn : 1;
    uint8_t start_btn : 1;
    uint8_t pad_up : 1;
    uint8_t pad_right : 1;
    uint8_t pad_down : 1;
    uint8_t pad_left : 1;

    // 1
    uint8_t l2_btn : 1;
    uint8_t r2_btn : 1;
    uint8_t l1_btn : 1;
    uint8_t r1_btn : 1;
    uint8_t triangle_btn : 1;
    uint8_t circle_btn : 1;
    uint8_t cross_btn : 1;
    uint8_t square_btn : 1;

    //2
    uint8_t ps_btn : 1;
    uint8_t : 3;

    uint8_t unk1;

    uint8_t joystick_lx;
    uint8_t joystick_ly;
    uint8_t joystick_rx;
    uint8_t joystick_ry;

    uint8_t unk2[2];
    uint8_t move_power_status;
    uint8_t unk3;

    uint8_t up_axis;
    uint8_t right_axis;
    uint8_t down_axis;
    uint8_t left_axis;

    uint8_t l2_axis;
    uint8_t r2_axis;
    uint8_t l1_axis;
    uint8_t r1_axis;

    uint8_t triangle_axis;
    uint8_t circle_axis;
    uint8_t cross_axis;
    uint8_t square_axis;

    uint8_t unk4[3];

    uint8_t plugged; // 0x02 usb connection?
    uint8_t power_status; // 0x05 battery charged?
    uint8_t rumble_status; // 0x10;

    uint8_t reserved5[9];

    //10 bits?
    uint16_t acceler_x; // roll? 
    uint16_t acceler_y; // pitch?
    uint16_t acceler_z;
    uint16_t gyro_z; // yaw?

} usbout_ps3_report_t;

static_assert(sizeof(usbout_ps3_report_t) == 48, "PS3 input report must stay donor-aligned");
