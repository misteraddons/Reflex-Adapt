#include "out_SwitchCommon.h"

#include "output_switch_report_normalize.h"
#include "output_switch_rumble.h"

#include <string.h>

namespace {

bool is_neutral_switch_rumble(const uint8_t* encoded) {
  static const uint8_t kNeutralRumble[4] = {0x00, 0x01, 0x40, 0x40};
  return memcmp(encoded, kNeutralRumble, sizeof(kNeutralRumble)) == 0;
}

bool disables_switch_vibration(const uint8_t* report, int report_size) {
  return report_size > 11 &&
         report[0] == 0x01 &&
         report[10] == ENABLE_VIBRATION &&
         report[11] == 0;
}

}  // namespace

void hid_report_data_callback(uint8_t itf, SwitchCommon *inst, uint16_t report_id,
                              uint8_t *report, int report_size) {
  uint8_t normalized[100] = {};
  int normalized_size = normalize_switch_out_report(report_id, report, report_size,
                                                    normalized, sizeof(normalized));
  if (normalized_size <= 0) {
    return;
  }

  uint8_t effective_report_id = normalized[0];
  if (effective_report_id == 0x01 || effective_report_id == 0x10 || effective_report_id == 0x11) {
    const int rumble_off = (effective_report_id == 0x01 || effective_report_id == 0x10 || effective_report_id == 0x11) ? 2 : 1;
    if ((rumble_off + 7) < normalized_size) {
      if (disables_switch_vibration(normalized, normalized_size) ||
          !inst->vibration_enabled() ||
          (is_neutral_switch_rumble(&normalized[rumble_off]) &&
           is_neutral_switch_rumble(&normalized[rumble_off + 4]))) {
        inst->set_controller_rumble(itf, 0, 0);
        inst->setSwitchRequestReport(normalized, normalized_size);
        return;
      }

      SwitchRumbleData r0 = {};
      SwitchRumbleData r1 = {};
      decodeSwitchRumbleValues(&normalized[rumble_off], &r0);
      decodeSwitchRumbleValues(&normalized[rumble_off + 4], &r1);

      float left_f =
          (r0.low_band_amp > r0.high_band_amp) ? r0.low_band_amp : r0.high_band_amp;
      float right_f =
          (r1.low_band_amp > r1.high_band_amp) ? r1.low_band_amp : r1.high_band_amp;
      uint8_t left = static_cast<uint8_t>(255 * left_f);
      uint8_t right = static_cast<uint8_t>(255 * right_f);
      inst->set_controller_rumble(itf, left, right);
    }
  }
  inst->setSwitchRequestReport(normalized, normalized_size);
}
