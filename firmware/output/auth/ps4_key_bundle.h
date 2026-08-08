#pragma once

#include <stddef.h>
#include <stdint.h>

enum PS4SplitBundleStatus : uint8_t {
  PS4_SPLIT_BUNDLE_OK = 0,
  PS4_SPLIT_BUNDLE_INVALID_PEM,
  PS4_SPLIT_BUNDLE_INVALID_BASE64,
  PS4_SPLIT_BUNDLE_INVALID_ASN1,
  PS4_SPLIT_BUNDLE_INVALID_SERIAL,
  PS4_SPLIT_BUNDLE_INVALID_SIGNATURE,
  PS4_SPLIT_BUNDLE_FIELD_TOO_LARGE,
  PS4_SPLIT_BUNDLE_INVALID_EXPONENT,
  PS4_SPLIT_BUNDLE_INVALID_OUTPUT
};

bool ps4BuildAuthBlobFromSplitBundle(const char* pemText,
                                     size_t pemLength,
                                     const uint8_t* serialData,
                                     size_t serialLength,
                                     const uint8_t* sigData,
                                     size_t sigLength,
                                     uint8_t* outBlob,
                                     size_t outBlobLength,
                                     PS4SplitBundleStatus* outStatus);

const char* ps4SplitBundleStatusLabel(PS4SplitBundleStatus status);
