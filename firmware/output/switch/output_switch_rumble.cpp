#include "output_switch_rumble.h"

#include <string.h>

namespace {

const uint16_t rumble_freq_lut[] = {
    0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031,
    0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0039, 0x003a, 0x003b,
    0x003c, 0x003e, 0x003f, 0x0040, 0x0042, 0x0043, 0x0045, 0x0046, 0x0048,
    0x0049, 0x004b, 0x004d, 0x004e, 0x0050, 0x0052, 0x0054, 0x0055, 0x0057,
    0x0059, 0x005b, 0x005d, 0x005f, 0x0061, 0x0063, 0x0066, 0x0068, 0x006a,
    0x006c, 0x006f, 0x0071, 0x0074, 0x0076, 0x0079, 0x007b, 0x007e, 0x0081,
    0x0084, 0x0087, 0x0089, 0x008d, 0x0090, 0x0093, 0x0096, 0x0099, 0x009d,
    0x00a0, 0x00a4, 0x00a7, 0x00ab, 0x00ae, 0x00b2, 0x00b6, 0x00ba, 0x00be,
    0x00c2, 0x00c7, 0x00cb, 0x00cf, 0x00d4, 0x00d9, 0x00dd, 0x00e2, 0x00e7,
    0x00ec, 0x00f1, 0x00f7, 0x00fc, 0x0102, 0x0107, 0x010d, 0x0113, 0x0119,
    0x011f, 0x0125, 0x012c, 0x0132, 0x0139, 0x0140, 0x0147, 0x014e, 0x0155,
    0x015d, 0x0165, 0x016c, 0x0174, 0x017d, 0x0185, 0x018d, 0x0196, 0x019f,
    0x01a8, 0x01b1, 0x01bb, 0x01c5, 0x01ce, 0x01d9, 0x01e3, 0x01ee, 0x01f8,
    0x0203, 0x020f, 0x021a, 0x0226, 0x0232, 0x023e, 0x024b, 0x0258, 0x0265,
    0x0272, 0x0280, 0x028e, 0x029c, 0x02ab, 0x02ba, 0x02c9, 0x02d9, 0x02e9,
    0x02f9, 0x030a, 0x031b, 0x032c, 0x033e, 0x0350, 0x0363, 0x0376, 0x0389,
    0x039d, 0x03b1, 0x03c6, 0x03db, 0x03f1, 0x0407, 0x041d, 0x0434, 0x044c,
    0x0464, 0x047d, 0x0496, 0x04af, 0x04ca, 0x04e5
};
constexpr size_t rumble_freq_lut_size = sizeof(rumble_freq_lut) / sizeof(uint16_t);

const float rumble_amp_lut_f[] = {
    0.000000, 0.120576, 0.137846, 0.146006, 0.154745, 0.164139, 0.174246,
    0.185147, 0.196927, 0.209703, 0.223587, 0.238723, 0.255268, 0.273420,
    0.293398, 0.315462, 0.321338, 0.327367, 0.333557, 0.339913, 0.346441,
    0.353145, 0.360034, 0.367112, 0.374389, 0.381870, 0.389564, 0.397476,
    0.405618, 0.413996, 0.422620, 0.431501, 0.436038, 0.440644, 0.445318,
    0.450062, 0.454875, 0.459764, 0.464726, 0.469763, 0.474876, 0.480068,
    0.485342, 0.490694, 0.496130, 0.501649, 0.507256, 0.512950, 0.518734,
    0.524609, 0.530577, 0.536639, 0.542797, 0.549055, 0.555413, 0.561872,
    0.568436, 0.575106, 0.581886, 0.588775, 0.595776, 0.602892, 0.610127,
    0.617482, 0.624957, 0.632556, 0.640283, 0.648139, 0.656126, 0.664248,
    0.672507, 0.680906, 0.689447, 0.698135, 0.706971, 0.715957, 0.725098,
    0.734398, 0.743857, 0.753481, 0.763273, 0.773235, 0.783370, 0.793684,
    0.804178, 0.814858, 0.825726, 0.836787, 0.848044, 0.859502, 0.871165,
    0.883035, 0.895119, 0.907420, 0.919943, 0.932693, 0.945673, 0.958889,
    0.972345, 0.986048, 1.000000
};
constexpr size_t rumble_amp_lut_f_size = sizeof(rumble_amp_lut_f) / sizeof(float);

}

void decodeSwitchRumbleValues(const uint8_t enc[], SwitchRumbleData* dec) {
  uint8_t hi_freq_ind = 0x20 + (enc[0] >> 2) + ((enc[1] & 0x01) * 0x40) - 1;
  uint8_t hi_amp_ind = (enc[1] & 0xfe) >> 1;
  uint8_t lo_freq_ind = (enc[2] & 0x7f) - 1;
  uint8_t lo_amp_ind = ((enc[3] - 0x40) << 1) + ((enc[2] & 0x80) >> 7);

  if (!((hi_freq_ind < rumble_freq_lut_size) &&
        (hi_amp_ind < rumble_amp_lut_f_size) &&
        (lo_freq_ind < rumble_freq_lut_size) &&
        (lo_amp_ind < rumble_amp_lut_f_size))) {
    memset(dec, 0, sizeof(SwitchRumbleData));
    return;
  }

  dec->high_band_freq = float(rumble_freq_lut[hi_freq_ind]);
  dec->high_band_amp = rumble_amp_lut_f[hi_amp_ind];
  dec->low_band_freq = float(rumble_freq_lut[lo_freq_ind]);
  dec->low_band_amp = rumble_amp_lut_f[lo_amp_ind];
}
