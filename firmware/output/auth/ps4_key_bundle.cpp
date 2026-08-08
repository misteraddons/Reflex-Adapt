#include "../../product_config.h"

#ifdef ADAPT_OUTPUT_USB_DEVICE

#include "ps4_key_bundle.h"

#include <string.h>

#include "../runtime/output_boot_bridge.h"
#include "ps4_key_layout.h"

namespace {

constexpr char kPemHeader[] = "-----BEGIN RSA PRIVATE KEY-----";
constexpr char kPemFooter[] = "-----END RSA PRIVATE KEY-----";
constexpr size_t kMaxBase64Chars = 4096;
constexpr size_t kMaxDerBytes = 3072;

struct IntegerView {
  const uint8_t* data;
  size_t length;
};

bool isAsciiWhitespace(uint8_t value) {
  return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

int8_t decodeBase64Char(char c) {
  if (c >= 'A' && c <= 'Z') return (int8_t)(c - 'A');
  if (c >= 'a' && c <= 'z') return (int8_t)(c - 'a' + 26);
  if (c >= '0' && c <= '9') return (int8_t)(c - '0' + 52);
  if (c == '+') return 62;
  if (c == '/') return 63;
  if (c == '=') return -2;
  return -1;
}

bool extractPemPayload(const char* pemText, size_t pemLength, char* base64Out, size_t base64OutSize, size_t& base64Length) {
  base64Length = 0;
  if (pemText == nullptr || pemLength == 0 || base64Out == nullptr || base64OutSize == 0) {
    return false;
  }

  const char* header = strstr(pemText, kPemHeader);
  if (header == nullptr) {
    return false;
  }
  header += strlen(kPemHeader);

  const char* footer = strstr(header, kPemFooter);
  if (footer == nullptr || footer <= header) {
    return false;
  }

  for (const char* cursor = header; cursor < footer; ++cursor) {
    const uint8_t value = (uint8_t)(*cursor);
    if (isAsciiWhitespace(value)) {
      continue;
    }
    if (base64Length + 1 >= base64OutSize) {
      return false;
    }
    base64Out[base64Length++] = (char)value;
  }
  base64Out[base64Length] = '\0';
  return base64Length > 0;
}

bool decodeBase64(const char* base64Text, size_t base64Length, uint8_t* out, size_t outCapacity, size_t& outLength) {
  outLength = 0;
  if (base64Text == nullptr || out == nullptr) {
    return false;
  }

  uint8_t quartet[4] = {};
  uint8_t quartetCount = 0;

  for (size_t i = 0; i < base64Length; ++i) {
    const int8_t decoded = decodeBase64Char(base64Text[i]);
    if (decoded < -1) {
      quartet[quartetCount++] = 0xFFu;
    } else if (decoded >= 0) {
      quartet[quartetCount++] = (uint8_t)decoded;
    } else {
      return false;
    }

    if (quartetCount != 4) {
      continue;
    }

    const bool pad2 = quartet[2] == 0xFFu;
    const bool pad3 = quartet[3] == 0xFFu;
    if (pad2 && !pad3) {
      return false;
    }

    if (outLength + 3 > outCapacity) {
      return false;
    }

    out[outLength++] = (uint8_t)((quartet[0] << 2) | (quartet[1] >> 4));
    if (!pad2) {
      out[outLength++] = (uint8_t)((quartet[1] << 4) | (quartet[2] >> 2));
    }
    if (!pad3) {
      out[outLength++] = (uint8_t)((quartet[2] << 6) | quartet[3]);
    }

    quartetCount = 0;
  }

  return quartetCount == 0 && outLength > 0;
}

bool readDerLength(const uint8_t* der, size_t derLength, size_t& offset, size_t& valueLength) {
  if (offset >= derLength) {
    return false;
  }

  uint8_t first = der[offset++];
  if ((first & 0x80u) == 0) {
    valueLength = first;
    return offset + valueLength <= derLength;
  }

  const uint8_t width = first & 0x7Fu;
  if (width == 0 || width > 4 || offset + width > derLength) {
    return false;
  }

  valueLength = 0;
  for (uint8_t i = 0; i < width; ++i) {
    valueLength = (valueLength << 8) | der[offset++];
  }
  return offset + valueLength <= derLength;
}

bool readDerInteger(const uint8_t* der, size_t derLength, size_t& offset, IntegerView& outValue) {
  if (offset >= derLength || der[offset++] != 0x02u) {
    return false;
  }

  size_t valueLength = 0;
  if (!readDerLength(der, derLength, offset, valueLength) || offset + valueLength > derLength) {
    return false;
  }

  const uint8_t* value = der + offset;
  offset += valueLength;

  while (valueLength > 1u && value[0] == 0x00u) {
    ++value;
    --valueLength;
  }

  outValue.data = value;
  outValue.length = valueLength;
  return true;
}

bool copyFixedWidthInteger(const IntegerView& value, uint8_t* dest, size_t width) {
  if (dest == nullptr || value.length == 0 || value.length > width) {
    return false;
  }

  memset(dest, 0, width);
  memcpy(dest + (width - value.length), value.data, value.length);
  return true;
}

bool exponentIs65537(const IntegerView& exponent) {
  return exponent.length == 3u &&
         exponent.data[0] == 0x01u &&
         exponent.data[1] == 0x00u &&
         exponent.data[2] == 0x01u;
}

bool normalizeSerialText(const uint8_t* serialData, size_t serialLength, uint8_t* serialOut) {
  if (serialData == nullptr || serialOut == nullptr) {
    return false;
  }

  size_t start = 0;
  size_t end = serialLength;
  while (start < end && isAsciiWhitespace(serialData[start])) {
    ++start;
  }
  while (end > start && isAsciiWhitespace(serialData[end - 1u])) {
    --end;
  }

  const size_t trimmedLength = end - start;
  if (trimmedLength != PS4_KEY_SERIAL_SZ) {
    return false;
  }

  memcpy(serialOut, serialData + start, PS4_KEY_SERIAL_SZ);
  return true;
}

}  // namespace

bool ps4BuildAuthBlobFromSplitBundle(const char* pemText,
                                     size_t pemLength,
                                     const uint8_t* serialData,
                                     size_t serialLength,
                                     const uint8_t* sigData,
                                     size_t sigLength,
                                     uint8_t* outBlob,
                                     size_t outBlobLength,
                                     PS4SplitBundleStatus* outStatus) {
  auto setStatus = [&](PS4SplitBundleStatus status) {
    if (outStatus != nullptr) {
      *outStatus = status;
    }
    return false;
  };

  if (outBlob == nullptr || outBlobLength < AUTH_KEY_PS4_SIZE) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_OUTPUT);
  }
  if (sigData == nullptr || sigLength != PS4_KEY_SIG_SZ) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_SIGNATURE);
  }

  uint8_t serialBytes[PS4_KEY_SERIAL_SZ] = {};
  if (!normalizeSerialText(serialData, serialLength, serialBytes)) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_SERIAL);
  }

  char base64Text[kMaxBase64Chars + 1] = {};
  size_t base64Length = 0;
  if (!extractPemPayload(pemText, pemLength, base64Text, sizeof(base64Text), base64Length)) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_PEM);
  }

  uint8_t der[kMaxDerBytes] = {};
  size_t derLength = 0;
  if (!decodeBase64(base64Text, base64Length, der, sizeof(der), derLength)) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_BASE64);
  }

  size_t offset = 0;
  if (offset >= derLength || der[offset++] != 0x30u) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_ASN1);
  }

  size_t sequenceLength = 0;
  if (!readDerLength(der, derLength, offset, sequenceLength) || offset + sequenceLength != derLength) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_ASN1);
  }

  IntegerView version = {};
  IntegerView modulus = {};
  IntegerView exponent = {};
  IntegerView privateExponent = {};
  IntegerView primeP = {};
  IntegerView primeQ = {};
  IntegerView exponentP = {};
  IntegerView exponentQ = {};
  IntegerView coefficient = {};

  if (!readDerInteger(der, derLength, offset, version) ||
      !readDerInteger(der, derLength, offset, modulus) ||
      !readDerInteger(der, derLength, offset, exponent) ||
      !readDerInteger(der, derLength, offset, privateExponent) ||
      !readDerInteger(der, derLength, offset, primeP) ||
      !readDerInteger(der, derLength, offset, primeQ) ||
      !readDerInteger(der, derLength, offset, exponentP) ||
      !readDerInteger(der, derLength, offset, exponentQ) ||
      !readDerInteger(der, derLength, offset, coefficient) ||
      offset != derLength) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_ASN1);
  }

  if (!exponentIs65537(exponent)) {
    return setStatus(PS4_SPLIT_BUNDLE_INVALID_EXPONENT);
  }

  memset(outBlob, 0, outBlobLength);
  memcpy(outBlob + PS4_KEY_SERIAL_OFF, serialBytes, PS4_KEY_SERIAL_SZ);
  memcpy(outBlob + PS4_KEY_SIG_OFF, sigData, PS4_KEY_SIG_SZ);

  if (!copyFixedWidthInteger(modulus, outBlob + PS4_KEY_N_OFF, PS4_KEY_N_SZ) ||
      !copyFixedWidthInteger(exponent, outBlob + PS4_KEY_E_OFF, PS4_KEY_E_SZ) ||
      !copyFixedWidthInteger(privateExponent, outBlob + PS4_KEY_D_OFF, PS4_KEY_D_SZ) ||
      !copyFixedWidthInteger(primeP, outBlob + PS4_KEY_P_OFF, PS4_KEY_P_SZ) ||
      !copyFixedWidthInteger(primeQ, outBlob + PS4_KEY_Q_OFF, PS4_KEY_Q_SZ) ||
      !copyFixedWidthInteger(exponentP, outBlob + PS4_KEY_DP_OFF, PS4_KEY_DP_SZ) ||
      !copyFixedWidthInteger(exponentQ, outBlob + PS4_KEY_DQ_OFF, PS4_KEY_DQ_SZ) ||
      !copyFixedWidthInteger(coefficient, outBlob + PS4_KEY_QP_OFF, PS4_KEY_QP_SZ)) {
    return setStatus(PS4_SPLIT_BUNDLE_FIELD_TOO_LARGE);
  }

  if (outStatus != nullptr) {
    *outStatus = PS4_SPLIT_BUNDLE_OK;
  }
  return true;
}

const char* ps4SplitBundleStatusLabel(PS4SplitBundleStatus status) {
  switch (status) {
    case PS4_SPLIT_BUNDLE_OK:
      return "ready";
    case PS4_SPLIT_BUNDLE_INVALID_PEM:
      return "invalid key.pem";
    case PS4_SPLIT_BUNDLE_INVALID_BASE64:
      return "invalid key.pem base64";
    case PS4_SPLIT_BUNDLE_INVALID_ASN1:
      return "invalid key.pem ASN.1";
    case PS4_SPLIT_BUNDLE_INVALID_SERIAL:
      return "invalid serial.txt";
    case PS4_SPLIT_BUNDLE_INVALID_SIGNATURE:
      return "invalid sig.bin";
    case PS4_SPLIT_BUNDLE_FIELD_TOO_LARGE:
      return "RSA field exceeds GP2040 limits";
    case PS4_SPLIT_BUNDLE_INVALID_EXPONENT:
      return "RSA exponent must be 65537";
    case PS4_SPLIT_BUNDLE_INVALID_OUTPUT:
    default:
      return "internal output buffer error";
  }
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
