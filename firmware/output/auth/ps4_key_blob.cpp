#include "../../product_config.h"

#ifdef ADAPT_OUTPUT_USB_DEVICE

#include "ps4_key_blob.h"

#include "../runtime/output_boot_bridge.h"
#include "ps4_key_layout.h"

namespace {

bool regionHasMeaningfulData(const uint8_t* data, size_t length) {
  bool anyNonZero = false;
  bool anyNonFF = false;
  for (size_t i = 0; i < length; ++i) {
    anyNonZero |= (data[i] != 0x00u);
    anyNonFF |= (data[i] != 0xFFu);
  }
  return anyNonZero && anyNonFF;
}

bool exponentIs65537(const uint8_t* data) {
  for (size_t i = 0; i + 3u < PS4_KEY_E_SZ; ++i) {
    if (data[i] != 0x00u) {
      return false;
    }
  }
  return data[PS4_KEY_E_SZ - 3u] == 0x01u &&
         data[PS4_KEY_E_SZ - 2u] == 0x00u &&
         data[PS4_KEY_E_SZ - 1u] == 0x01u;
}

}  // namespace

uint32_t ps4KeyBlobCrc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
    }
  }
  return ~crc;
}

PS4KeyBlobValidation ps4ValidateAuthBlob(const uint8_t* data, size_t length) {
  if (data == nullptr || length != AUTH_KEY_PS4_SIZE) {
    return PS4_KEY_BLOB_INVALID_LENGTH;
  }
  if (!regionHasMeaningfulData(data + PS4_KEY_SERIAL_OFF, PS4_KEY_SERIAL_SZ)) {
    return PS4_KEY_BLOB_INVALID_SERIAL;
  }
  if (!regionHasMeaningfulData(data + PS4_KEY_SIG_OFF, PS4_KEY_SIG_SZ)) {
    return PS4_KEY_BLOB_INVALID_SIGNATURE;
  }
  if (!regionHasMeaningfulData(data + PS4_KEY_N_OFF, PS4_KEY_N_SZ) ||
      ((data[PS4_KEY_N_OFF + PS4_KEY_N_SZ - 1u] & 0x01u) == 0)) {
    return PS4_KEY_BLOB_INVALID_MODULUS;
  }
  if (!exponentIs65537(data + PS4_KEY_E_OFF)) {
    return PS4_KEY_BLOB_INVALID_EXPONENT;
  }
  if (!regionHasMeaningfulData(data + PS4_KEY_D_OFF, PS4_KEY_D_SZ)) {
    return PS4_KEY_BLOB_INVALID_PRIVATE;
  }
  if (!regionHasMeaningfulData(data + PS4_KEY_P_OFF, PS4_KEY_P_SZ) ||
      !regionHasMeaningfulData(data + PS4_KEY_Q_OFF, PS4_KEY_Q_SZ) ||
      !regionHasMeaningfulData(data + PS4_KEY_DP_OFF, PS4_KEY_DP_SZ) ||
      !regionHasMeaningfulData(data + PS4_KEY_DQ_OFF, PS4_KEY_DQ_SZ) ||
      !regionHasMeaningfulData(data + PS4_KEY_QP_OFF, PS4_KEY_QP_SZ) ||
      ((data[PS4_KEY_P_OFF + PS4_KEY_P_SZ - 1u] & 0x01u) == 0) ||
      ((data[PS4_KEY_Q_OFF + PS4_KEY_Q_SZ - 1u] & 0x01u) == 0)) {
    return PS4_KEY_BLOB_INVALID_CRT;
  }
  return PS4_KEY_BLOB_VALID;
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
