#include "../../product_config.h"

// Override TinyUSB defaults before including the output transport stack.
#define CFG_TUD_HID 6  // 6-player multitap support (Saturn, etc.)

#include "output_loop_runtime.h"

#include <Adafruit_TinyUSB.h>

#include "../auth/webhid_auth_runtime.h"
#include "../auth/auth_msc_runtime.h"
#include "output_runtime_bridge.h"
#include "../output_runtime_state.h"

#if !defined(ADAPT_OUTPUT_USB_DEVICE)
  #ifdef ENABLE_OUTPUT_ESP32_BT
    #include "../out_esp32_bt.h"
  #elif defined(ENABLE_OUTPUT_JVS)
    #include "../jvs/out_jvs.h"
  #elif defined(ENABLE_OUTPUT_CONSOLE)
    #include "../classic/out_classic.h"
  #elif defined(ENABLE_OUTPUT_DB15)
    #include "../db15/out_db15.h"
  #endif
#endif

static_assert(CFG_TUD_HID == 6, "CFG_TUD_HID must be 6 for 6-player multitap support");

namespace {

UsbDeviceRuntimeDiagnostics usbDeviceDiag{};
uint32_t lastUsbTaskUs = 0;

void __not_in_flash_func(noteUsbDeviceTask)() {
  const uint32_t nowUs = micros();
  if (lastUsbTaskUs != 0) {
    const uint32_t gap = nowUs - lastUsbTaskUs;
    usbDeviceDiag.last_task_gap_us = gap;
    if (gap > usbDeviceDiag.max_task_gap_us) {
      usbDeviceDiag.max_task_gap_us = gap;
    }
  }
  lastUsbTaskUs = nowUs;
  usbDeviceDiag.task_count++;
  usbDeviceDiag.last_task_ms = millis();
}

}  // namespace

const UsbDeviceRuntimeDiagnostics& usbDeviceRuntimeDiagnostics() {
  return usbDeviceDiag;
}

#ifdef ADAPT_OUTPUT_USB_DEVICE
extern "C" void tud_mount_cb(void) {
  usbDeviceDiag.mount_count++;
}

extern "C" void tud_umount_cb(void) {
  usbDeviceDiag.umount_count++;
}

extern "C" void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
  usbDeviceDiag.suspend_count++;
}

extern "C" void tud_resume_cb(void) {
  usbDeviceDiag.resume_count++;
}
#endif

void __not_in_flash_func(runOutputTransportSyncTasks)() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  #if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  TinyUSB_Device_Task();
  #else
  TinyUSBDevice.task();
  #endif
  #endif
}

void runOutputBackgroundTasks() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  runOutputTransportSyncTasks();
  auth_msc_task();
  // Diagnostics are not transport-critical. Record them after the input frame
  // has been polled and submitted so clock reads and counter maintenance cannot
  // delay the controller-to-USB path.
  noteUsbDeviceTask();

  if (outputMode == OUTPUT_XINPUT || outputMode == OUTPUT_XINPUT2P) {
    xinput_auth_process();
  }

  auto_detect_process();
  if (!is_ps5_timing_quiet_mode_active()) {
    webhid_process_commands();
    webhid_process_rumble();
  }
  #endif
}

void processPendingOutputRuntimeTasks() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  // PS4 auth: perform RSA signing when nonce is ready (runs in main loop, ~150ms)
  if (!is_ps5_timing_quiet_mode_active() && ps4Auth.isReady()) {
    ps4Auth.process();
  }
  #endif
}

void __not_in_flash_func(sendPreparedOutputFrame)() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  send_usb_report();
  #elif defined(ENABLE_OUTPUT_ESP32_BT)
  esp32_bt_send();
  #elif defined(ENABLE_OUTPUT_JVS)
  jvsOutput.update();
  #elif defined(ENABLE_OUTPUT_CONSOLE)
  classic_output_update();
  #elif defined(ENABLE_OUTPUT_DB15)
  db15_output_update();
  #endif
}
