#pragma once

#include <string.h>

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include <Adafruit_TinyUSB.h>
#endif

// Native PS5 diagnostic descriptor/report shape ported from:
// Adapt-Dev retains the P5General donor snapshot. Keep donor IDs/strings/report layout unchanged
// while the PS5 path is validated on real hardware.

#define P5GENERAL_ENDPOINT_SIZE 64
#define P5GENERAL_TP_X_MIN 0
#define P5GENERAL_TP_X_MAX 1920
#define P5GENERAL_TP_Y_MIN 0
#define P5GENERAL_TP_Y_MAX 943
// GP2040-CE's public macro says 0x28B1, but its raw descriptor enumerates as
// 2B81:0101. Use the actual wire identity for PS5 game compatibility.
#define P5GENERAL_VENDOR_ID  0x2B81
#define P5GENERAL_PRODUCT_ID 0x0101

constexpr uint16_t p5general_bcd_usb = 0x0200;
constexpr uint16_t p5general_bcd_device = 0x0001;

struct Ps5GeneralTouchpadXY {
  uint8_t counter : 7;
  uint8_t unpressed : 1;
  uint8_t data[3];

  void set_x(uint16_t x) {
    data[0] = x & 0xff;
    data[1] = (data[1] & 0xf0) | ((x >> 8) & 0x0f);
  }

  void set_y(uint16_t y) {
    data[1] = (data[1] & 0x0f) | ((y & 0x0f) << 4);
    data[2] = y >> 4;
  }
};

struct Ps5GeneralTouchpadData {
  Ps5GeneralTouchpadXY p1;
  Ps5GeneralTouchpadXY p2;
};

struct Ps5GeneralSensor {
  int16_t x;
  int16_t y;
  int16_t z;
} __attribute__((packed));

typedef struct __attribute__((packed)) {
  uint8_t report_id;
  uint8_t left_stick_x;
  uint8_t left_stick_y;
  uint8_t right_stick_x;
  uint8_t right_stick_y;
  uint8_t left_trigger;
  uint8_t right_trigger;
  uint8_t data_7;
  uint8_t dpad : 4;
  uint8_t button_west : 1;
  uint8_t button_south : 1;
  uint8_t button_east : 1;
  uint8_t button_north : 1;
  uint8_t button_l1 : 1;
  uint8_t button_r1 : 1;
  uint8_t button_l2 : 1;
  uint8_t button_r2 : 1;
  uint8_t button_select : 1;
  uint8_t button_start : 1;
  uint8_t button_l3 : 1;
  uint8_t button_r3 : 1;
  uint8_t button_home : 1;
  uint8_t button_touchpad : 1;
  uint8_t : 6;
  uint8_t data_11;
  uint32_t auth_seq_number;
  Ps5GeneralSensor gyroscope;
  Ps5GeneralSensor accelerometer;
  uint16_t data_28_29;
  uint16_t data_30_31_0x001a;
  Ps5GeneralTouchpadData touchpad_data;
  uint8_t data_40_55[16];
  uint8_t hash[8];
} usbout_ps5_general_report_t;

static const uint8_t p5general_report_descriptor[] = {
  0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01,
  0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35,
  0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xFF,
  0x00, 0x75, 0x08, 0x95, 0x06, 0x81, 0x02, 0x06,
  0x00, 0xFF, 0x09, 0x20, 0x95, 0x01, 0x81, 0x02,
  0x05, 0x01, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07,
  0x35, 0x00, 0x46, 0x3B, 0x01, 0x65, 0x14, 0x75,
  0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00, 0x05,
  0x09, 0x19, 0x01, 0x29, 0x0E, 0x15, 0x00, 0x25,
  0x01, 0x75, 0x01, 0x95, 0x0E, 0x81, 0x02, 0x06,
  0x00, 0xFF, 0x09, 0x21, 0x95, 0x0E, 0x81, 0x02,
  0x06, 0x00, 0xFF, 0x09, 0x22, 0x15, 0x00, 0x26,
  0xFF, 0x00, 0x75, 0x08, 0x95, 0x34, 0x81, 0x02,
  0x85, 0x02, 0x09, 0x23, 0x95, 0x2F, 0x91, 0x02,
  0x85, 0x03, 0x0A, 0x21, 0x28, 0x95, 0x2F, 0xB1,
  0x02, 0x06, 0x80, 0xFF, 0x85, 0xE0, 0x09, 0x57,
  0x95, 0x02, 0xB1, 0x02, 0xC0, 0x06, 0xF0, 0xFF,
  0x09, 0x40, 0xA1, 0x01, 0x85, 0xF0, 0x09, 0x47,
  0x95, 0x3F, 0xB1, 0x02, 0x85, 0xF1, 0x09, 0x48,
  0x95, 0x3F, 0xB1, 0x02, 0x85, 0xF2, 0x09, 0x49,
  0x95, 0x0F, 0xB1, 0x02, 0xC0,
};

static constexpr uint8_t p5general_configuration_descriptor[] = {
  0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x00, 0x80, 0xFA,
  0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
  0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0xA5, 0x00,
  0x07, 0x05, 0x82, 0x03, 0x40, 0x00, 0x01,
  0x07, 0x05, 0x01, 0x03, 0x40, 0x00, 0x06,
};

static_assert(sizeof(p5general_configuration_descriptor) == 41,
              "P5General config descriptor must stay donor-sized");
static_assert(p5general_configuration_descriptor[29] == 0x82,
              "P5General interrupt IN endpoint address must stay 0x82");
static_assert(p5general_configuration_descriptor[30] == 0x03,
              "P5General interrupt IN endpoint must stay interrupt type");
static_assert(p5general_configuration_descriptor[33] == 0x01,
              "P5General interrupt IN endpoint must poll every 1 ms");
static_assert(p5general_configuration_descriptor[36] == 0x01,
              "P5General interrupt OUT endpoint address must stay 0x01");
static_assert(p5general_configuration_descriptor[37] == 0x03,
              "P5General interrupt OUT endpoint must stay interrupt type");
static_assert(p5general_configuration_descriptor[40] == 0x06,
              "P5General interrupt OUT endpoint must stay donor interval");

#ifdef ADAPT_OUTPUT_USB_DEVICE
class Adafruit_USBD_P5General final : public Adafruit_USBD_HID {
public:
  Adafruit_USBD_P5General()
    : Adafruit_USBD_HID(
        p5general_report_descriptor,
        sizeof(p5general_report_descriptor),
        HID_ITF_PROTOCOL_NONE,
        1,
        true
      ) {}

  uint16_t getInterfaceDescriptor(
    uint8_t itfnum_deprecated,
    uint8_t *buf,
    uint16_t bufsize
  ) override {
    (void) itfnum_deprecated;

    constexpr uint16_t CONFIG_HEADER_SIZE = 9;
    constexpr uint16_t len = sizeof(p5general_configuration_descriptor) - CONFIG_HEADER_SIZE;

    if (!buf) {
      return len;
    }
    if (bufsize < len) {
      return 0;
    }

    memcpy(buf, p5general_configuration_descriptor + CONFIG_HEADER_SIZE, len);

    // Preserve GP2040/P5General's fixed endpoint addresses. Only patch the
    // interface number allocated by Adafruit's composite builder.
    const uint8_t itfnum = TinyUSBDevice.allocInterface(1);
    buf[2] = itfnum;
    return len;
  }
};
#endif
