#pragma once

// Narrow runtime bridge for output-side loop helpers that are still implemented
// inside the USB/output stack.

#ifdef __cplusplus
extern "C" {
#endif

void auto_detect_process();
void send_usb_report();

#ifdef __cplusplus
}
#endif

void webhid_process_commands();
bool webhid_process_rumble();
void xinput_auth_process(void);
