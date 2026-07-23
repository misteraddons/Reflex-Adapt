#pragma once

#include <stddef.h>
#include <stdint.h>

enum PS4KeyBlobValidation : uint8_t {
  PS4_KEY_BLOB_VALID = 0,
  PS4_KEY_BLOB_INVALID_LENGTH = 1,
  PS4_KEY_BLOB_INVALID_SERIAL = 2,
  PS4_KEY_BLOB_INVALID_SIGNATURE = 3,
  PS4_KEY_BLOB_INVALID_MODULUS = 4,
  PS4_KEY_BLOB_INVALID_EXPONENT = 5,
  PS4_KEY_BLOB_INVALID_PRIVATE = 6,
  PS4_KEY_BLOB_INVALID_CRT = 7,
};

uint32_t ps4KeyBlobCrc32(const uint8_t* data, size_t length);
PS4KeyBlobValidation ps4ValidateAuthBlob(const uint8_t* data, size_t length);
