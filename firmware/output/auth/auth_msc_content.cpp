#include "../../product_config.h"

#include "auth_msc_content.h"

namespace {

constexpr uint8_t kFatVolumeLabelLen = 11;

#if defined(PRODUCT_CLASSIC2USB)
constexpr char kVolumeLabel[kFatVolumeLabelLen] = {
  'C','L','A','S','S','I','C','2','U','S','B'
};
constexpr const char* kProductName = "CLASSIC2USB";
#else
constexpr char kVolumeLabel[kFatVolumeLabelLen] = {
  'A','D','A','P','T','A','U','T','H',' ',' '
};
constexpr const char* kProductName = "AdaptAuth";
#endif

}  // namespace

const char* auth_msc_product_name() {
  return kProductName;
}

const char* auth_msc_volume_label() {
  return kVolumeLabel;
}

uint8_t auth_msc_volume_label_len() {
  return kFatVolumeLabelLen;
}
