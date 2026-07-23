#pragma once

// Simple PlayStation 3 descriptor and report definitions extracted from
// output_descriptors_playstation_runtime.h.

// Minimal HID gamepad descriptor WITH feature reports for AUTO mode detection
// PS3 requests feature reports when it sees Sony VID/PID - used for detection
static const uint8_t ps3_simple_report_descriptor[] =
{
	0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,        // USAGE (Gamepad)
	0xa1, 0x01,        // COLLECTION (Application)
	// Buttons (14 buttons + 2 padding = 16 bits = 2 bytes)
	0x15, 0x00,        //   LOGICAL_MINIMUM (0)
	0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
	0x35, 0x00,        //   PHYSICAL_MINIMUM (0)
	0x45, 0x01,        //   PHYSICAL_MAXIMUM (1)
	0x75, 0x01,        //   REPORT_SIZE (1)
	0x95, 0x0e,        //   REPORT_COUNT (14)
	0x05, 0x09,        //   USAGE_PAGE (Button)
	0x19, 0x01,        //   USAGE_MINIMUM (Button 1)
	0x29, 0x0e,        //   USAGE_MAXIMUM (Button 14)
	0x81, 0x02,        //   INPUT (Data,Var,Abs)
	0x95, 0x02,        //   REPORT_COUNT (2)
	0x81, 0x01,        //   INPUT (Cnst,Ary,Abs)
	// Hat switch (4 bits + 4 bits padding = 1 byte)
	0x05, 0x01,        //   USAGE_PAGE (Generic Desktop)
	0x25, 0x07,        //   LOGICAL_MAXIMUM (7)
	0x46, 0x3b, 0x01,  //   PHYSICAL_MAXIMUM (315)
	0x75, 0x04,        //   REPORT_SIZE (4)
	0x95, 0x01,        //   REPORT_COUNT (1)
	0x65, 0x14,        //   UNIT (Eng Rot:Angular Pos)
	0x09, 0x39,        //   USAGE (Hat switch)
	0x81, 0x42,        //   INPUT (Data,Var,Abs,Null)
	0x65, 0x00,        //   UNIT (None)
	0x95, 0x01,        //   REPORT_COUNT (1)
	0x81, 0x01,        //   INPUT (Cnst,Ary,Abs)
	// Analog sticks (4 axes x 8 bits = 4 bytes)
	0x26, 0xff, 0x00,  //   LOGICAL_MAXIMUM (255)
	0x46, 0xff, 0x00,  //   PHYSICAL_MAXIMUM (255)
	0x09, 0x30,        //   USAGE (X)
	0x09, 0x31,        //   USAGE (Y)
	0x09, 0x32,        //   USAGE (Z)
	0x09, 0x35,        //   USAGE (Rz)
	0x75, 0x08,        //   REPORT_SIZE (8)
	0x95, 0x04,        //   REPORT_COUNT (4)
	0x81, 0x02,        //   INPUT (Data,Var,Abs)
	// Feature reports for PS3 detection
	0x85, 0xF2,        //   REPORT_ID (0xF2) - PS3 requests this for pairing info
	0x09, 0x22,        //   USAGE (0x22)
	0x95, 0x11,        //   REPORT_COUNT (17)
	0xb1, 0x02,        //   FEATURE (Data,Var,Abs)
	0x85, 0xF5,        //   REPORT_ID (0xF5) - PS3 requests this for host address
	0x09, 0x21,        //   USAGE (0x21)
	0x95, 0x08,        //   REPORT_COUNT (8)
	0xb1, 0x02,        //   FEATURE (Data,Var,Abs)
	0xc0               // END_COLLECTION
};

// Minimal PS3 report structure (7 bytes) - matches minimal descriptor
typedef struct TU_ATTR_PACKED {
	// Buttons (16 bits = 2 bytes) - 14 buttons + 2 padding
	uint8_t square_btn : 1;
	uint8_t cross_btn : 1;
	uint8_t circle_btn : 1;
	uint8_t triangle_btn : 1;
	uint8_t l1_btn : 1;
	uint8_t r1_btn : 1;
	uint8_t l2_btn : 1;
	uint8_t r2_btn : 1;
	uint8_t select_btn : 1;
	uint8_t start_btn : 1;
	uint8_t l3_btn : 1;
	uint8_t r3_btn : 1;
	uint8_t ps_btn : 1;
	uint8_t : 3;  // padding (buttons 14 implicit + 2 padding = 16 bits)
	// Hat switch (1 byte, lower 4 bits used)
	uint8_t hat;
	// Analog sticks (4 bytes)
	uint8_t lx;
	uint8_t ly;
	uint8_t rx;
	uint8_t ry;
} usbout_ps3_minimal_report_t;

// Simple PS3 report structure (19 bytes) - matches ps3_simple_report_descriptor
typedef struct TU_ATTR_PACKED {
	// Buttons (16 bits = 2 bytes)
	uint8_t square_btn : 1;
	uint8_t cross_btn : 1;
	uint8_t circle_btn : 1;
	uint8_t triangle_btn : 1;
	uint8_t l1_btn : 1;
	uint8_t r1_btn : 1;
	uint8_t l2_btn : 1;
	uint8_t r2_btn : 1;
	uint8_t select_btn : 1;
	uint8_t start_btn : 1;
	uint8_t l3_btn : 1;
	uint8_t r3_btn : 1;
	uint8_t ps_btn : 1;
	uint8_t : 3;  // padding

	// Hat switch (0-7 = directions, 8 = center)
	uint8_t hat;

	// Analog sticks
	uint8_t lx;
	uint8_t ly;
	uint8_t rx;
	uint8_t ry;

	// Pressure axes (d-pad + face buttons + shoulders)
	uint8_t right_axis;
	uint8_t left_axis;
	uint8_t up_axis;
	uint8_t down_axis;
	uint8_t triangle_axis;
	uint8_t circle_axis;
	uint8_t cross_axis;
	uint8_t square_axis;
	uint8_t l1_axis;
	uint8_t r1_axis;
	uint8_t l2_axis;
	uint8_t r2_axis;
} usbout_ps3_simple_report_t;
