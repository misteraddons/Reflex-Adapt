#pragma once

#include <cstddef>
#include <stdint.h>

// XInput report/layout types extracted from out_xinput.h.

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t report_id;    // byte 0: always 0x00
    uint8_t report_size;  // byte 1: always 0x14 (20)

    // Buttons byte 2 (separate struct guarantees exactly 1 byte)
    struct __attribute__((packed)) {
        uint8_t dpad_up : 1;
        uint8_t dpad_down : 1;
        uint8_t dpad_left : 1;
        uint8_t dpad_right : 1;
        uint8_t start : 1;
        uint8_t back : 1;
        uint8_t ls : 1;
        uint8_t rs : 1;
    };

    // Buttons byte 3 (separate struct guarantees exactly 1 byte)
    struct __attribute__((packed)) {
        uint8_t lb : 1;
        uint8_t rb : 1;
        uint8_t home : 1;
        uint8_t _reserved0 : 1;
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t x : 1;
        uint8_t y : 1;
    };

    uint8_t lt;            // byte 4: left trigger (0-255)
    uint8_t rt;            // byte 5: right trigger (0-255)
    int16_t lx;            // bytes 6-7: left stick X
    int16_t ly;            // bytes 8-9: left stick Y
    int16_t rx;            // bytes 10-11: right stick X
    int16_t ry;            // bytes 12-13: right stick Y
    uint8_t _reserved1[6]; // bytes 14-19
} xinput_report_t;

// Verify Xbox 360 report struct matches expected 20-byte layout
static_assert(sizeof(xinput_report_t) == 20, "xinput_report_t must be exactly 20 bytes");
static_assert(offsetof(xinput_report_t, lt) == 4, "lt must be at byte 4");
static_assert(offsetof(xinput_report_t, rt) == 5, "rt must be at byte 5");
static_assert(offsetof(xinput_report_t, lx) == 6, "lx must be at byte 6");
static_assert(offsetof(xinput_report_t, ly) == 8, "ly must be at byte 8");
static_assert(offsetof(xinput_report_t, rx) == 10, "rx must be at byte 10");
static_assert(offsetof(xinput_report_t, ry) == 12, "ry must be at byte 12");

struct XInputDebugInfo {
    bool initialized;
    bool authenticated;
    uint8_t auth_state;       // 0=idle, 1=id_sent, 2=challenge_pending, 3=response_ready, 4=verify_pending, 5=verified
    uint8_t last_request;     // Last bRequest received (0x81-0x87)
    uint8_t last_stage;       // Last control stage (0=SETUP, 1=DATA, 2=ACK)
    uint16_t request_count;   // Total requests received
    uint16_t data_stage_count; // DATA stage calls (for OUT transfers)
    // Per-request counters
    uint8_t req_81_count;     // 0x81 ID requests
    uint8_t req_82_count;     // 0x82 Challenge requests
    uint8_t req_83_count;     // 0x83 Response requests
    uint8_t req_84_count;     // 0x84 Misc requests
    uint8_t req_85_count;     // 0x85 Misc requests
    uint8_t req_86_count;     // 0x86 Status requests
    uint8_t req_87_count;     // 0x87 Verify requests
    uint8_t req_other_count;  // Other vendor requests
    uint8_t last_other_req;   // Last non-XSM3 vendor request
    uint8_t challenge_byte0;  // First byte of challenge packet
    uint8_t response_byte0;   // First byte of response (should be 0x49 after init)
    uint8_t last_wIndex;      // wIndex of last auth request (interface number)
    uint8_t last_wLength;     // wLength of last auth request
    uint8_t pending_req;      // Pending request (0x82 or 0x87)
    uint16_t process_count;   // Times xinput_auth_process was called
    uint8_t interfaces_opened; // Number of interfaces opened
    uint8_t endpoint_in;      // Actual IN endpoint address
    uint8_t endpoint_out;     // Actual OUT endpoint address
    bool raw_descriptors;     // Whether raw descriptors mode is active
    bool hooks_registered;    // Whether descriptor hooks are registered
    uint8_t desc_device_calls;  // Number of device descriptor requests
    uint8_t desc_config_calls;  // Number of config descriptor requests
    uint16_t control_count;     // XInput class control callback calls
    uint8_t standard_control_count; // Standard interface control requests
    uint8_t vendor_control_count;   // Vendor interface control requests
    uint8_t desc_21_count;      // Xbox class descriptor requests
    uint8_t desc_41_count;      // Xbox security descriptor requests
    uint8_t last_control_request; // Last class-control bRequest
    uint8_t last_control_type;  // Last class-control bmRequestType.type
    uint8_t last_control_recipient; // Last class-control recipient
    uint8_t last_control_desc_type; // Last descriptor type requested
    uint8_t last_control_itf;   // Last interface index requested
    bool string_4_seen;         // Xbox security string descriptor requested
    uint8_t security_mismatch_count; // Auth requests aimed at the wrong interface
    uint8_t reset_count;        // USB reset count
    // Low-level USB status
    bool tud_inited;          // TinyUSB stack initialized
    bool tud_connected;       // USB cable connected
    bool tud_mounted;         // Device enumerated by host
    bool tud_suspended;       // Device suspended
    // Adafruit USB debug
    uint16_t cfg_desc_len;    // Config descriptor length from Adafruit
    uint8_t itf_count;        // Interface count
    bool begin_result;        // Result of begin() call
};
