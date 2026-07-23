#include "out_SwitchCommon.h"

#include "output_switch_report_normalize.h"
#include "output_switch_rumble.h"

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
      SwitchRumbleData r0 = {};
      SwitchRumbleData r1 = {};
      decodeSwitchRumbleValues(&normalized[rumble_off], &r0);
      decodeSwitchRumbleValues(&normalized[rumble_off + 4], &r1);

      float left_f = (r0.low_band_amp > r1.low_band_amp) ? r0.low_band_amp : r1.low_band_amp;
      float right_f = (r0.high_band_amp > r1.high_band_amp) ? r0.high_band_amp : r1.high_band_amp;
      uint8_t left_lut = static_cast<uint8_t>(255 * left_f);
      uint8_t right_lut = static_cast<uint8_t>(255 * right_f);

      bool lValid = (normalized[rumble_off] & 0x03) == 0x00 &&
                    (normalized[rumble_off + 3] & 0x40) == 0x40;
      bool rValid = (normalized[rumble_off + 4] & 0x03) == 0x00 &&
                    (normalized[rumble_off + 7] & 0x40) == 0x40;

      uint8_t left_raw = lValid ? ((normalized[rumble_off + 3] & 0x3F) << 2) : 0;
      uint8_t right_raw = rValid ? ((normalized[rumble_off + 7] & 0x3F) << 2) : 0;

      uint8_t best_left = left_lut ? left_lut : left_raw;
      uint8_t best_right = right_lut ? right_lut : right_raw;

      inst->set_controller_rumble(itf, best_left, best_right);
    }
  }
  inst->setSwitchRequestReport(normalized, normalized_size);
}
