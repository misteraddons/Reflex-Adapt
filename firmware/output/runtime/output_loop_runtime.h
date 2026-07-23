#pragma once

#include <stdint.h>

// Output-owned runtime services for transport housekeeping, queued host
// transfers, and output-send completion.

void runOutputBackgroundTasks();
void runOutputTransportSyncTasks();
void processPendingOutputRuntimeTasks();
void sendPreparedOutputFrame();

struct UsbDeviceRuntimeDiagnostics {
  uint32_t task_count;
  uint32_t mount_count;
  uint32_t umount_count;
  uint32_t suspend_count;
  uint32_t resume_count;
  uint32_t max_task_gap_us;
  uint32_t last_task_gap_us;
  uint32_t last_task_ms;
};

const UsbDeviceRuntimeDiagnostics& usbDeviceRuntimeDiagnostics();
