#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <Arduino.h>

#include "usb_detect_runtime_state.h"

#ifdef __cplusplus
extern "C" {
#endif
bool can_run_usb_detection(void);
#ifdef __cplusplus
}
#endif

host_detection_t host_detection { };
uint32_t host_detection_last_signal_ms = 0;

void usb_detect_mark_result(uint8_t detect_flag) {
  if (!can_run_usb_detection())
    return;

  host_detection.any |= detect_flag;
  host_detection_last_signal_ms = millis();
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
