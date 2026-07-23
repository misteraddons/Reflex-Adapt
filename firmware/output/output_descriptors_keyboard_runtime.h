#pragma once

// Plain boot-keyboard descriptor for the MAME-oriented arcade keyboard output.
// Management/WebHID stays in DInput mode; keyboard mode should enumerate as a
// boring keyboard so PC hosts and MAME see key events immediately.

const uint8_t keyboard_report_desc[] = {
  TUD_HID_REPORT_DESC_KEYBOARD()
};
