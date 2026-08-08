#pragma once

#include <stddef.h>
#include <stdint.h>

struct XInputRumblePacket {
  uint8_t left;
  uint8_t right;
};

inline size_t xinputRumbleParseSize(size_t transferred, size_t endpoint_size) {
  if (endpoint_size == 0) {
    return transferred;
  }
  // Some hosts complete XInput OUT transfers with a short/zero byte count even
  // though TinyUSB has already filled the fixed endpoint buffer.
  if (transferred == 0 || transferred < 5) {
    return endpoint_size;
  }
  return (transferred > endpoint_size) ? endpoint_size : transferred;
}

inline bool parseXinputRumblePacket(const uint8_t* buffer,
                                    size_t size,
                                    XInputRumblePacket* out) {
  if (buffer == nullptr || out == nullptr) {
    return false;
  }

  if (size >= 5 && buffer[0] == 0x00 && buffer[1] == 0x08) {
    uint8_t left = buffer[3];
    uint8_t right = buffer[4];
    // Wireless-style packets can carry motor bytes later in the same frame.
    if ((left | right) == 0 && size >= 7 && (buffer[5] | buffer[6])) {
      left = buffer[5];
      right = buffer[6];
    }
    out->left = left;
    out->right = right;
    return true;
  }

  if (size >= 5 && buffer[1] == 0x08) {
    // Some composite XInput child paths expose the 0x08 rumble frame with a
    // nonzero leading byte. Keep the motor byte positions from the standard
    // wired packet, but only after the explicit 00 08 form above has failed.
    out->left = buffer[3];
    out->right = buffer[4];
    return true;
  }

  if (size >= 5 && buffer[0] == 0x00 && buffer[1] == 0x06) {
    // Windows' multi-interface XUSB path can emit the same motor-byte layout
    // with a 6-byte frame length instead of the 8-byte wired-controller frame.
    out->left = buffer[3];
    out->right = buffer[4];
    return true;
  }

  if (size >= 4 &&
      buffer[0] == 0x06 &&
      buffer[1] == 0x00) {
    out->left = buffer[2];
    out->right = buffer[3];
    return true;
  }

  if (size >= 4 &&
      buffer[0] == 0x08 &&
      buffer[1] == 0x00 &&
      !(buffer[2] == 0x0F && buffer[3] == 0xC0)) {
    // Some APIs strip the leading report byte and expose the 0x08 rumble frame.
    out->left = buffer[2];
    out->right = buffer[3];
    return true;
  }

  if (size >= 7 &&
      buffer[0] == 0x00 &&
      buffer[1] == 0x01 &&
      buffer[2] == 0x0F &&
      buffer[3] == 0xC0) {
    out->left = buffer[5];
    out->right = buffer[6];
    return true;
  }

  return false;
}
