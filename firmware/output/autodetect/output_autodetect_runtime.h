#pragma once

#include <Adafruit_TinyUSB.h>
#include <stdint.h>

#include "usb_detect.h"

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
extern "C" void rp2040_usb_install_debug_irq();
#endif

bool auto_detect_is_final_state();
void auto_detect_hid_report_read_cb(uint8_t itf);
extern "C" bool tud_hid_set_idle_cb(uint8_t instance, uint8_t idle_rate);
extern "C" void tud_string_0xee_requested_cb(void);
void auto_detect_trigger_reenum(uint32_t next_output_mode);
extern "C" void auto_detect_process();
