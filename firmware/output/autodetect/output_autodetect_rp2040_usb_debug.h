#pragma once

#include <stdint.h>

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
extern "C" void rp2040_usb_install_debug_irq();
void update_output_autodetect_rp2040_usb_debug(uint32_t ms_now, uint32_t auto_detect_elapsed_ms);
bool output_autodetect_usb_attach_retry_attempted();
#else
inline void update_output_autodetect_rp2040_usb_debug(uint32_t, uint32_t) {}
inline bool output_autodetect_usb_attach_retry_attempted() { return false; }
#endif
