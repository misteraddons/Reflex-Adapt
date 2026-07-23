#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include "input_autodetect_support.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
#ifdef AUTODETECT_DEBUG
void getAutoDetectDebugStr(char* buf, size_t len) {
  char jag_state = lastDebug[0].jag_skipped_due_saturn_activity ? 'K' : (lastDebug[0].jag_detected ? 'Y' : 'N');
  snprintf(buf, len, "JB:%04X PS:%02X%02X%02X W:%c J:%c",
    lastDebug[0].joybus_type,
    lastDebug[0].psx_recv1,
    lastDebug[0].psx_type,
    lastDebug[0].psx_sync,
    lastDebug[0].wii_detected ? 'Y' : 'N',
    jag_state);
}

void getAutoDetectOledDebugLine(uint8_t port, char* buf, size_t len) {
  if (port >= kAutoDetectDebugPortCount) {
    snprintf(buf, len, "P? --");
    return;
  }

  const AutoDetectDebug& d = lastDebug[port];
  char jag_state = d.jag_skipped_due_saturn_activity ? 'K' : (d.jag_detected ? 'Y' : 'N');
  char wii_state = d.wii_detected ? 'Y' : 'N';
  snprintf(buf, len, "P%u R%u J%c%u W%c PS%02X%02X%02X",
    (unsigned)(port + 1),
    (unsigned)d.final_result,
    jag_state,
    (unsigned)d.jag_diff_bits,
    wii_state,
    (unsigned)d.psx_recv1,
    (unsigned)d.psx_type,
    (unsigned)d.psx_sync);
}

void getAutoDetectSerialDebugLine(uint8_t port, char* buf, size_t len) {
  if (port >= kAutoDetectDebugPortCount) {
    snprintf(buf, len, "P? invalid");
    return;
  }

  const AutoDetectDebug& d = lastDebug[port];
  snprintf(buf, len,
    "P%u R=%u PR=0x%03X SNES[d=%04X e=%04X id=%X tr=%u r=%u s=%u n=%u v=%u]"
    " PCE[n0=%X dp=%X bt=%X cand=%u lib=%u tap=%u cnt=%u typ=%u]"
    " SAT[s=%u m=%u a=%u iv=%u f=%X/%X l=%X/%X t0=%u%u t1=%u%u SL[p=%u h=%u tap=%u c=%u t=%u]"
    " JAG[sk=%u det=%u c2=%u/%u ni=%u/%u df=%u a=%02X,%02X,%02X,%02X b=%02X,%02X,%02X,%02X]"
    " WII[52=%u/%u 51=%u 53=%u sig=%u ty=%u id=%02X%02X]"
    " PSX[%02X,%02X,%02X MT=%u] JB[%04X]"
    " MS[jb=%u sn=%u pce=%u drv=%u sat=%u tdo=%u psx=%u wii=%u dc=%u neo=%u pdl=%u]",
    (unsigned)(port + 1),
    (unsigned)d.final_result,
    (unsigned)d.probes_run,
    (unsigned)d.snes_data,
    (unsigned)d.snes_extended,
    (unsigned)d.snes_id,
    (unsigned)d.snes_transitions,
    (unsigned)d.snes_result,
    (unsigned)d.snes_hits,
    (unsigned)d.nes_hits,
    (unsigned)d.vboy_hits,
    (unsigned)d.pce_nibble0,
    (unsigned)d.pce_nibble_dpad,
    (unsigned)d.pce_nibble_btn,
    (unsigned)d.pce_raw_candidate,
    (unsigned)d.pce_lib_confirmed,
    (unsigned)d.pce_tap_ports,
    (unsigned)d.pce_last_count,
    (unsigned)d.pce_last_type,
    (unsigned)d.saturn_hits,
    (unsigned)d.megadrive_hits,
    (unsigned)d.saturn_activity_hits,
    (unsigned)d.saturn_inv_fallback,
    (unsigned)d.sat_nibble0_first, (unsigned)d.sat_nibble1_first,
    (unsigned)d.sat_nibble0_last, (unsigned)d.sat_nibble1_last,
    (unsigned)d.sat_tl0_first, (unsigned)d.sat_tr0_first,
    (unsigned)d.sat_tl1_first, (unsigned)d.sat_tr1_first,
    (unsigned)d.satlib_passes,
    (unsigned)d.satlib_hits,
    (unsigned)d.satlib_tap_ports,
    (unsigned)d.satlib_last_count,
    (unsigned)d.satlib_last_type,
    (unsigned)d.jag_skipped_due_saturn_activity,
    (unsigned)d.jag_detected,
    (unsigned)d.jag_c2c3_a, (unsigned)d.jag_c2c3_b,
    (unsigned)d.jag_non_idle_a, (unsigned)d.jag_non_idle_b,
    (unsigned)d.jag_diff_bits,
    (unsigned)d.jag_a[0], (unsigned)d.jag_a[1], (unsigned)d.jag_a[2], (unsigned)d.jag_a[3],
    (unsigned)d.jag_b[0], (unsigned)d.jag_b[1], (unsigned)d.jag_b[2], (unsigned)d.jag_b[3],
    (unsigned)d.wii_ack52_a, (unsigned)d.wii_ack52_b,
    (unsigned)d.wii_ack51, (unsigned)d.wii_ack53,
    (unsigned)d.wii_sig_ok,
    (unsigned)d.wii_type,
    (unsigned)d.wii_id2, (unsigned)d.wii_id3,
    (unsigned)d.psx_recv1, (unsigned)d.psx_type, (unsigned)d.psx_sync,
    (unsigned)d.psx_multitap,
    (unsigned)d.joybus_type,
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_JOYBUS],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_SNES],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_PCE],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_ATARI_DRIVING],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_SATURN],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_3DO],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_PSX],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_WII],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_DREAMCAST],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_NEOGEO],
    (unsigned)d.probe_ms[ADDBG_PROBE_INDEX_ATARI_PADDLE]);
}

bool getAutoDetectJaguarSerialDebugLine(uint8_t port, char* buf, size_t len) {
  if (port >= kAutoDetectDebugPortCount) {
    snprintf(buf, len, "P? JAG invalid");
    return false;
  }

  const AutoDetectDebug& d = lastDebug[port];
  if ((d.probes_run & ADDBG_PROBE_JAGUAR) == 0) {
    snprintf(buf, len, "P%u JAG not-run", (unsigned)(port + 1));
    return false;
  }

  uint8_t low_a[4];
  uint8_t low_b[4];
  for (uint8_t i = 0; i < 4; ++i) {
    low_a[i] = (uint8_t)((~d.jag_a[i]) & 0x3F);
    low_b[i] = (uint8_t)((~d.jag_b[i]) & 0x3F);
  }

  bool row_variant_a = !(d.jag_a[0] == d.jag_a[1] && d.jag_a[0] == d.jag_a[2] && d.jag_a[0] == d.jag_a[3]);
  bool row_variant_b = !(d.jag_b[0] == d.jag_b[1] && d.jag_b[0] == d.jag_b[2] && d.jag_b[0] == d.jag_b[3]);
  bool idle_a = (d.jag_a[0] == 0x3F && d.jag_a[1] == 0x3F && d.jag_a[2] == 0x3F && d.jag_a[3] == 0x3F);
  bool idle_b = (d.jag_b[0] == 0x3F && d.jag_b[1] == 0x3F && d.jag_b[2] == 0x3F && d.jag_b[3] == 0x3F);

  snprintf(buf, len,
    "P%u JAGX det=%u c2c3=%u/%u rv=%u/%u idle=%u/%u diff=%u "
    "raw=%02X,%02X,%02X,%02X/%02X,%02X,%02X,%02X "
    "low=%02X,%02X,%02X,%02X/%02X,%02X,%02X,%02X "
    "SATact=%u SAT=%X/%X->%X/%X TLTR=%u%u/%u%u",
    (unsigned)(port + 1),
    (unsigned)d.jag_detected,
    (unsigned)d.jag_c2c3_a, (unsigned)d.jag_c2c3_b,
    (unsigned)row_variant_a, (unsigned)row_variant_b,
    (unsigned)idle_a, (unsigned)idle_b,
    (unsigned)d.jag_diff_bits,
    (unsigned)d.jag_a[0], (unsigned)d.jag_a[1], (unsigned)d.jag_a[2], (unsigned)d.jag_a[3],
    (unsigned)d.jag_b[0], (unsigned)d.jag_b[1], (unsigned)d.jag_b[2], (unsigned)d.jag_b[3],
    (unsigned)low_a[0], (unsigned)low_a[1], (unsigned)low_a[2], (unsigned)low_a[3],
    (unsigned)low_b[0], (unsigned)low_b[1], (unsigned)low_b[2], (unsigned)low_b[3],
    (unsigned)d.saturn_activity_hits,
    (unsigned)d.sat_nibble0_first, (unsigned)d.sat_nibble1_first,
    (unsigned)d.sat_nibble0_last, (unsigned)d.sat_nibble1_last,
    (unsigned)d.sat_tl0_first, (unsigned)d.sat_tr0_first,
    (unsigned)d.sat_tl1_first, (unsigned)d.sat_tr1_first);
  return true;
}
#endif

DeviceEnum autoDetectResultToDeviceMode(AutoDetectResult result) {
  switch (result) {
    case AUTODETECT_JOYBUS_N64:
      #ifdef ENABLE_INPUT_N64
        return RZORD_N64;
      #endif
      break;
    case AUTODETECT_JOYBUS_GC:
      #ifdef ENABLE_INPUT_GAMECUBE
        return RZORD_GAMECUBE;
      #endif
      break;
    case AUTODETECT_JOYBUS_GBA:
      #ifdef ENABLE_INPUT_GBA
        return RZORD_GBA;
      #endif
      break;
    case AUTODETECT_DREAMCAST:
      #ifdef ENABLE_INPUT_DREAMCAST
        return RZORD_DREAMCAST;
      #endif
      break;
    case AUTODETECT_PSX:
      #ifdef ENABLE_INPUT_PSX
        return RZORD_PSX;
      #endif
      break;
    case AUTODETECT_SNES:
      #ifdef ENABLE_INPUT_SNES
        return RZORD_SNES;
      #endif
      break;
    case AUTODETECT_NES:
      #ifdef ENABLE_INPUT_NES
        return RZORD_NES;
      #endif
      break;
    case AUTODETECT_VBOY:
      #ifdef ENABLE_INPUT_VBOY
        return RZORD_VBOY;
      #else
        #ifdef ENABLE_INPUT_SNES
          return RZORD_SNES;
        #endif
      #endif
      break;
    case AUTODETECT_SATURN:
      #ifdef ENABLE_INPUT_SATURN
        return RZORD_SATURN;
      #endif
      break;
    case AUTODETECT_MEGADRIVE:
      #ifdef ENABLE_INPUT_MEGADRIVE
        return RZORD_MEGADRIVE;
      #else
        #ifdef ENABLE_INPUT_SATURN
          return RZORD_SATURN;
        #endif
      #endif
      break;
    case AUTODETECT_PCE:
      #ifdef ENABLE_INPUT_PCE
        return RZORD_PCE;
      #endif
      break;
    case AUTODETECT_3DO:
      #ifdef ENABLE_INPUT_3DO
        return RZORD_3DO;
      #endif
      break;
    case AUTODETECT_WII:
      #ifdef ENABLE_INPUT_WII
        return RZORD_WII;
      #endif
      break;
    case AUTODETECT_JAGUAR:
      #ifdef ENABLE_INPUT_JAGUAR
        return RZORD_JAGUAR;
      #endif
      break;
    case AUTODETECT_NEOGEO:
      #ifdef ENABLE_INPUT_NEOGEO
        return RZORD_NEOGEO;
      #endif
      break;
    case AUTODETECT_ATARI_DRIVING:
      #ifdef ENABLE_INPUT_DRIVING
        return RZORD_DRIVING;
      #endif
      break;
    case AUTODETECT_ATARI_PADDLE:
      #ifdef ENABLE_INPUT_PADDLE
        return RZORD_PADDLE;
      #endif
      break;
    case AUTODETECT_SMS:
      #ifdef ENABLE_INPUT_SMS
        return RZORD_SMS;
      #endif
      break;
    case AUTODETECT_JPC:
      #ifdef ENABLE_INPUT_JPC
        return RZORD_JPC;
      #endif
      break;
    default:
      break;
  }
  return RZORD_NONE;
}

const char* autoDetectResultName(AutoDetectResult result) {
  switch (result) {
    case AUTODETECT_JOYBUS_N64: return "N64";
    case AUTODETECT_JOYBUS_GC: return "GameCube";
    case AUTODETECT_JOYBUS_GBA: return "GBA";
    case AUTODETECT_DREAMCAST: return "Dreamcast";
    case AUTODETECT_PSX: return "PSX";
    case AUTODETECT_SNES: return "SNES";
    case AUTODETECT_NES: return "NES";
    case AUTODETECT_VBOY: return "Virtual Boy";
    case AUTODETECT_SATURN: return "Saturn";
    case AUTODETECT_MEGADRIVE: return "Genesis";
    case AUTODETECT_PCE: return "PC-Engine";
    case AUTODETECT_3DO: return "3DO";
    case AUTODETECT_WII: return "Wii";
    case AUTODETECT_JAGUAR: return "Jaguar";
    case AUTODETECT_NEOGEO: return "Neo Geo";
    case AUTODETECT_ATARI_DRIVING: return "Driving";
    case AUTODETECT_ATARI_PADDLE: return "Paddle";
    case AUTODETECT_SMS: return "Atari/C64/SMS";
    case AUTODETECT_JPC: return "JPN PC";
    default: return "None";
  }
}
#endif
