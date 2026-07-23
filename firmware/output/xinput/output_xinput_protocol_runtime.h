#pragma once

#include <stdint.h>

#define XINPUT_SUBCLASS_DEFAULT 0x5D
#define XINPUT_PROTOCOL_DEFAULT 1

// Security interface constants
#define XINPUT_SECURITY_SUBCLASS 0xFD
#define XINPUT_SECURITY_PROTOCOL 0x13

//#define EPOUT 0x01 //0x01
//#define EPIN 0x81
#define EPSIZE 32

// XSM3 Authentication request types
#define XSM3_REQUEST_INIT       0x81  // Get identification data
#define XSM3_REQUEST_CHALLENGE  0x82  // Challenge init
#define XSM3_REQUEST_RESPONSE   0x83  // Get challenge response
#define XSM3_REQUEST_STATUS     0x86  // Get status
#define XSM3_REQUEST_VERIFY     0x87  // Challenge verify

// clang-format off

//--------------------------------------------------------------------+
// MSC Descriptor TemplatesVENDOR_REQUEST_MICROSOFT
//--------------------------------------------------------------------+

// XInput Controller Subtypes (Microsoft XInput spec)
enum XInputSubtype : uint8_t {
    XINPUT_SUBTYPE_GAMEPAD     = 0x01,
    XINPUT_SUBTYPE_WHEEL       = 0x02,
    XINPUT_SUBTYPE_ARCADESTICK = 0x03,
    XINPUT_SUBTYPE_FLIGHTSTICK = 0x04,
    XINPUT_SUBTYPE_DANCEPAD    = 0x05,
    XINPUT_SUBTYPE_GUITAR      = 0x06,
    XINPUT_SUBTYPE_DRUMKIT     = 0x07,
    XINPUT_SUBTYPE_ARCADEPAD   = 0x08,
};

// Offset to subtype byte in config descriptor (byte 22)
#define XINPUT_SUBTYPE_OFFSET 22

// Runtime subtype configuration
void xinput_set_subtype(uint8_t subtype);
uint8_t xinput_get_subtype(void);

// Full Xbox 360 configuration descriptor length (all 4 interfaces)
// Based on official Xbox 360 wired controller descriptor
#define TUD_XINPUT_FULL_DESC_LEN  153

// Composite XInput+HID descriptor: XInput (153) + HID interface 4 (25 = 9+9+7)
#define TUD_XINPUT_HID_COMPOSITE_DESC_LEN  (TUD_XINPUT_FULL_DESC_LEN + 25)

// clang-format on

// Full Xbox 360 controller descriptor (all 4 interfaces required by console)
// Interface 0: Control (gamepad I/O) - 2 endpoints
// Interface 1: Audio - 4 endpoints (not used but required)
// Interface 2: Plugin - 1 endpoint (not used but required)
// Interface 3: Security - 0 endpoints (XSM3 auth)
static const uint8_t xinput_config_descriptor[] = {
  // Configuration Descriptor
  0x09, 0x02,                   // bLength, bDescriptorType
  0x99, 0x00,                   // wTotalLength (153 bytes)
  0x04,                         // bNumInterfaces (4)
  0x01,                         // bConfigurationValue
  0x00,                         // iConfiguration
  0xA0,                         // bmAttributes (bus powered, remote wakeup)
  0xFA,                         // bMaxPower (500mA)

  // Interface 0: Control (Gamepad)
  0x09, 0x04,                   // bLength, bDescriptorType (Interface)
  0x00,                         // bInterfaceNumber
  0x00,                         // bAlternateSetting
  0x02,                         // bNumEndpoints
  0xFF,                         // bInterfaceClass (Vendor)
  0x5D,                         // bInterfaceSubClass (XInput)
  0x01,                         // bInterfaceProtocol (Gamepad)
  0x00,                         // iInterface

  // Gamepad Descriptor
  0x11, 0x21,                   // bLength, bDescriptorType (0x21 = HID-like)
  0x00, 0x01, 0x01,
  0x25,                         // Input report size
  0x81,                         // Endpoint IN address (fixed - matches official)
  0x14,                         // Input report ID
  0x00, 0x00, 0x00, 0x00,
  0x13,                         // Output report ID
  0x02,                         // Endpoint OUT address (fixed - matches official)
  0x08,                         // Output report size
  0x00, 0x00,

  // Endpoint IN (Control)
  0x07, 0x05,                   // bLength, bDescriptorType (Endpoint)
  0x81,                         // bEndpointAddress (IN 1) - fixed
  0x03,                         // bmAttributes (Interrupt)
  0x20, 0x00,                   // wMaxPacketSize (32)
  0x01,                         // bInterval (1ms)

  // Endpoint OUT (Control)
  0x07, 0x05,                   // bLength, bDescriptorType (Endpoint)
  0x02,                         // bEndpointAddress (OUT 2) - fixed
  0x03,                         // bmAttributes (Interrupt)
  0x20, 0x00,                   // wMaxPacketSize (32)
  0x08,                         // bInterval (8ms)

  // Interface 1: Audio (required but not used)
  0x09, 0x04,                   // bLength, bDescriptorType (Interface)
  0x01,                         // bInterfaceNumber
  0x00,                         // bAlternateSetting
  0x04,                         // bNumEndpoints
  0xFF,                         // bInterfaceClass (Vendor)
  0x5D,                         // bInterfaceSubClass (XInput)
  0x03,                         // bInterfaceProtocol (Audio)
  0x00,                         // iInterface

  // Audio Descriptor
  0x1B, 0x21,                   // bLength, bDescriptorType
  0x00, 0x01, 0x01, 0x01,
  0x83, 0x40, 0x01,
  0x04, 0x20, 0x16,
  0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x16, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  // Audio endpoints (4)
  0x07, 0x05, 0x83, 0x03, 0x20, 0x00, 0x02,  // IN
  0x07, 0x05, 0x04, 0x03, 0x20, 0x00, 0x04,  // OUT
  0x07, 0x05, 0x85, 0x03, 0x20, 0x00, 0x40,  // IN
  0x07, 0x05, 0x06, 0x03, 0x20, 0x00, 0x10,  // OUT

  // Interface 2: Plugin Module (required but not used)
  0x09, 0x04,                   // bLength, bDescriptorType (Interface)
  0x02,                         // bInterfaceNumber
  0x00,                         // bAlternateSetting
  0x01,                         // bNumEndpoints
  0xFF,                         // bInterfaceClass (Vendor)
  0x5D,                         // bInterfaceSubClass (XInput)
  0x02,                         // bInterfaceProtocol (Plugin)
  0x00,                         // iInterface

  // Plugin Descriptor
  0x09, 0x21,                   // bLength, bDescriptorType
  0x00, 0x01, 0x01,
  0x22,
  0x86, 0x03, 0x00,

  // Plugin endpoint
  0x07, 0x05, 0x86, 0x03, 0x20, 0x00, 0x10,  // IN

  // Interface 3: Security (XSM3 Authentication)
  0x09, 0x04,                   // bLength, bDescriptorType (Interface)
  0x03,                         // bInterfaceNumber
  0x00,                         // bAlternateSetting
  0x00,                         // bNumEndpoints
  0xFF,                         // bInterfaceClass (Vendor)
  0xFD,                         // bInterfaceSubClass (Security)
  0x13,                         // bInterfaceProtocol
  0x04,                         // iInterface (String 4)

  // Security Descriptor (XSM3)
  0x06, 0x41,                   // bLength, bDescriptorType (Xbox 360)
  0x00, 0x01, 0x01, 0x03
};

