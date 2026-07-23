#pragma once

#include <stdint.h>

#include "../../core/settings_layout.h"
#include "../output_mode.h"

// Shared EEPROM region used for uploadable output authentication material.
// Kept in a small bridge header so boot/setup code does not need to include
// the full USB output stack just to size EEPROM storage.
#define AUTH_KEY_EEPROM_BASE        SETTINGS_STORAGE_REGION_SIZE
#define AUTH_KEY_PS4_OFFSET         0
#define AUTH_KEY_PS4_SIZE           1712
#define AUTH_KEY_RECORD_HEADER_SIZE 10
#define AUTH_KEY_SLOT_COUNT         2
#define AUTH_KEY_PS4_SLOT_SIZE      (AUTH_KEY_RECORD_HEADER_SIZE + AUTH_KEY_PS4_SIZE)
#define AUTH_KEY_XB360_OFFSET       (AUTH_KEY_SLOT_COUNT * AUTH_KEY_PS4_SLOT_SIZE)
#define AUTH_KEY_XB360_SIZE         0
#define AUTH_KEY_TOTAL_SIZE         (AUTH_KEY_SLOT_COUNT * AUTH_KEY_PS4_SLOT_SIZE)
#if defined(ARDUINO_ARCH_RP2040) && defined(PRODUCT_CLASSIC2USB) && !defined(ENABLE_BLUETOOTH_PAIRING) && !defined(REFLEX_HOST_TEST_EEPROM_AUTH)
#define AUTH_KEY_STORAGE_USES_RAW_FLASH 1
#define AUTH_KEY_EEPROM_RESERVED_SIZE   0
#else
#define AUTH_KEY_EEPROM_RESERVED_SIZE   AUTH_KEY_TOTAL_SIZE
#endif
#define AUTH_KEY_EEPROM_END         (AUTH_KEY_EEPROM_BASE + AUTH_KEY_EEPROM_RESERVED_SIZE)

#define AUTH_KEY_TYPE_PS4           0x01
#define AUTH_KEY_TYPE_XB360         0x02

#ifdef __cplusplus
extern "C" {
#endif

void webhid_check_auth_keys();
bool must_set_serial_string(outputMode_t mode);
const char* get_reflex_input_product_name();
const char* get_reflex_input_usb_serial_descriptor();
void configure_usb_output();
uint16_t auth_storage_active_payload_address(uint8_t keyType);
extern const uint16_t RZORD1_VID;
extern const uint16_t RZORD1_PID;
extern const uint16_t RZORD1_MISTER_PID;
extern const uint16_t RZORD1_VERSION;
extern const char* const RZORD1_VENDOR;
extern const char* const RZORD1_PPRODUCT;
extern const uint8_t bcd_input_revision;

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
void rp2040_usb_install_debug_irq();
#endif

#ifdef __cplusplus
}
#endif
