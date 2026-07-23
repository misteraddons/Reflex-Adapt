#include "../output_runtime_state.h"
#include "output_autodetect_reporting.h"

#include <stdio.h>

const char* auto_detect_state_name() {
  switch (autoDetectState) {
    case AUTO_STATE_PS3:          return "PS3";
    case AUTO_STATE_WINDOWS:      return "Windows";
    case AUTO_STATE_PS4:          return "PS4";
    case AUTO_STATE_PS5:          return "PS5";
    case AUTO_STATE_PLAYSTATION:  return "PlayStation";
    case AUTO_STATE_OG_XBOX:      return "Xbox OG";
    case AUTO_STATE_XBOX_360:     return "Xbox 360";
    case AUTO_STATE_SWITCH:       return "Switch";
    case AUTO_STATE_FALLBACK_HID: return "Fallback";
    default:                      return "Auto";
  }
}

void auto_trace_format_oled_line(char* buf, size_t len) {
  if (len == 0) return;
  buf[0] = '\0';
  if (!is_auto_output_mode_selected()) return;
  snprintf(buf, len, "A %s", auto_detect_state_name());
}
