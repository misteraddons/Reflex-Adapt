#include "../../../product_config.h"

#include <string.h>

#include "../../auth/webhid_auth_reports.h"
#include "webhid_basic_reports.h"
#include "webhid_command_runtime.h"
#include "webhid_debug_reports.h"
#include "webhid_protocol.h"
#include "webhid_report_dispatch.h"
#include "webhid_runtime_reports.h"
#include "webhid_settings_reports.h"
#include "webhid_write_reports.h"

namespace {

uint16_t build_webhid_report(uint8_t report_id, uint8_t* buffer) {
  switch (report_id) {
    case WEBHID_REPORT_DEVICE_INFO: {
      return webhid_get_device_info_report(buffer);
    }

    case WEBHID_REPORT_KEY_STATUS: {
      return webhid_get_key_status_report(buffer);
    }

    case WEBHID_REPORT_INPUT_STATE: {
      return webhid_get_input_state_report(buffer);
    }

    case WEBHID_REPORT_GPIO_STATE: {
      return webhid_get_gpio_state_report(buffer);
    }

    case WEBHID_REPORT_RAW_DATA: {
      return webhid_get_raw_data_report(buffer);
    }

    case WEBHID_REPORT_SETTINGS: {
      return webhid_get_settings_report(buffer);
    }

    case WEBHID_REPORT_INPUT_MODE: {
      return webhid_get_input_mode_report(buffer);
    }

    case WEBHID_REPORT_TURBO: {
      return webhid_get_turbo_report(buffer);
    }

    case WEBHID_REPORT_REMAP: {
      return webhid_get_remap_report(buffer);
    }

    case WEBHID_REPORT_INPUT_HISTORY: {
      return webhid_get_input_history_report(buffer);
    }

    case WEBHID_REPORT_STATS: {
      return webhid_get_stats_report(buffer);
    }

    default: {
      buffer[0] = WEBHID_MAGIC_BYTE;
      return WEBHID_REPORT_SIZE;
    }
  }
}

}  // namespace

uint16_t webhid_get_report(uint8_t report_id, uint8_t* buffer, uint16_t reqlen) {
  if (buffer == nullptr || reqlen == 0) {
    return 0;
  }

  // Every report builder owns a complete 63-byte feature report. Build into a
  // correctly sized local buffer, then honor the host's requested length so a
  // short control request can never make a builder write past TinyUSB's buffer.
  uint8_t report[WEBHID_REPORT_SIZE] = {};
  const uint16_t builtReportLen = build_webhid_report(report_id, report);
  const uint16_t builtLen = builtReportLen < WEBHID_REPORT_SIZE
      ? builtReportLen
      : (uint16_t)WEBHID_REPORT_SIZE;
  const uint16_t responseLen = builtLen < reqlen ? builtLen : reqlen;
  memcpy(buffer, report, responseLen);
  return responseLen;
}

void webhid_set_report(uint8_t report_id, const uint8_t* buffer, uint16_t bufsize) {
  if (bufsize < 1) return;

  switch (report_id) {
    case WEBHID_REPORT_COMMAND: {
      webhid_handle_command_report(buffer, bufsize);
      break;
    }

    case WEBHID_REPORT_KEY_WRITE: {
      webhid_write_auth_key_report(buffer, bufsize);
      break;
    }

    case WEBHID_REPORT_KEY_CLEAR: {
      webhid_clear_auth_key_report(buffer, bufsize);
      break;
    }

    case WEBHID_REPORT_SETTINGS: {
      webhid_write_settings_report(buffer, bufsize);
      break;
    }

    case WEBHID_REPORT_INPUT_MODE: {
      if (buffer[0] == WEBHID_MAGIC_BYTE && bufsize >= 2) {
        webhid_set_input_mode_report_page(buffer[1], (bufsize >= 3) ? buffer[2] : 0);
      }
      break;
    }

    case WEBHID_REPORT_INPUT_STATE: {
      webhid_set_input_state_player(buffer, bufsize);
      break;
    }

    case WEBHID_REPORT_TURBO: {
      webhid_write_turbo_report(buffer, bufsize);
      break;
    }

    case WEBHID_REPORT_REMAP: {
      webhid_write_remap_report(buffer, bufsize);
      break;
    }

    case WEBHID_REPORT_STATS: {
      webhid_handle_stats_report(buffer, bufsize);
      break;
    }
  }
}
