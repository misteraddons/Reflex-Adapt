#include "../product_config.h"

#include "menu.h"
#include "menu_main_display_internal.h"

#include "../output/output_capabilities.h"
#include "../output/output_runtime_state.h"
#include "../core/controller_settings_state.h"
#include "../core/rumble_test_runtime.h"

#if defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_VBOY)
#include "../input/snes/input_snes_runtime_state.h"
#endif

#ifdef ENABLE_INPUT_JVS
#include "../input/jvs/jvs_host_runtime.h"
#endif
#ifdef ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT
#include "../output/xinput/out_xinput_multi.h"
#ifdef FORCE_XINPUT2P_SINGLE_DRIVER
#include "../output/xinput/out_xinput.h"
#include "../output/xinput/output_xinput_auth_runtime_state.h"
#endif
#ifdef ENABLE_XINPUT2P_WIRELESS_TRANSPORT
#include "../output/xinputw/out_xinputw.h"
#endif
#endif

#include <Arduino.h>
#include <cstdio>
#include <cstring>

namespace menu_main_display_internal {


void renderModeButtonIndicator(bool force) {
  (void)force;
}

bool shouldShowAutoDetectScanning(uint8_t connectedCount) {
  #ifdef ENABLE_INPUT_AUTODETECT
  return isAutoDetectMode && (deviceMode == RZORD_AUTODETECT) && (connectedCount == 0);
  #else
  (void)connectedCount;
  return false;
  #endif
}

void renderAutoDetectScanningStatus() {
  display.setRow(3);
  display.setCol(0);
  display.print("Assisted detect hold...");

  display.setRow(4);
  display.setCol(0);
  #if AUTODETECT_ENABLE_PASSIVE_ASSIST
  display.print("Neo-Geo: Start");
  #else
  display.print("Active detect only");
  #endif

  display.setRow(5);
  display.setCol(0);
  #if AUTODETECT_ENABLE_PASSIVE_ASSIST
  display.print("Japanese PC: Left");
  #else
  display.print("Waiting for pad...");
  #endif

  display.setRow(6);
  display.setCol(0);
  #if AUTODETECT_ENABLE_PASSIVE_ASSIST
  display.print("2600/C64/SMS: Fire");
  #else
  display.print("Reconnect controller");
  #endif

  display.setRow(7);
  display.setCol(0);
  #if AUTODETECT_ENABLE_PASSIVE_ASSIST
    #ifdef ENABLE_INPUT_JAGUAR
  display.print("Jaguar: Pause");
    #else
  display.print("2600/C64/SMS: Fire");
    #endif
  #else
  display.print("for auto-detect");
  #endif
}

void renderConnectedPortNames() {
  display.setRow(7);
  display.setCol(0);

  bool port1_has_device = false;
  const char* port1_name = nullptr;

  #if defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_VBOY)
  if ((deviceMode == RZORD_SNES || deviceMode == RZORD_NES || deviceMode == RZORD_VBOY)
      && snes_debug_mtap_enabled0 && snes_debug_tap0 > 0) {
    port1_has_device = true;
    port1_name = "Multitap";
  } else
  #endif
  if (controllerFrameConst(0).connected) {
    const controller_state_t& port1 = controllerFrameConst(0);
    port1_has_device = true;
    port1_name = port1.controller_type_name[0] ? port1.controller_type_name :
                 (is_passive_controller_mode(deviceMode) ? nullptr : "Connected");
  }
  #ifdef ENABLE_INPUT_JAGUAR
  if (port1_has_device && port1_name && deviceMode != RZORD_JAGUAR) {
    display.print(port1_name);
  } else if (deviceMode != RZORD_JAGUAR) {
  #else
  if (port1_has_device && port1_name) {
    display.print(port1_name);
  } else {
  #endif
    display.setCol(20);
    display.print("--");
  }

  bool port2_has_device = false;
  const char* port2_name = nullptr;
  uint8_t port2_first_idx = 1;
  outputMode_t effectiveOutputMode = get_effective_output_mode();
  const bool mergedSinglePadView = classic_dual_merge_enabled != 0;
  bool hideEmptySecondPad = shouldHideEmptySecondPadInAutoInputMode();
  bool showOutputVirtualPad =
    !mergedSinglePadView &&
    controllerFrameConst(0).connected &&
    shouldShowVirtualOutputPad(deviceMode, effectiveOutputMode) &&
    ((max_devices == 1) || hideEmptySecondPad);
  bool showVirtualOutputName =
    !mergedSinglePadView &&
    (showOutputVirtualPad ||
    (controllerFrameConst(0).connected &&
     shouldShowVirtualOutputPad(deviceMode, effectiveOutputMode) &&
     !hideEmptySecondPad));

  if (!mergedSinglePadView && !hideEmptySecondPad) {
    #if defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_VBOY)
    if ((deviceMode == RZORD_SNES || deviceMode == RZORD_NES || deviceMode == RZORD_VBOY)
        && snes_debug_mtap_enabled1 && snes_debug_tap1 > 0) {
      port2_has_device = true;
      port2_name = "Multitap";
    } else {
      if ((deviceMode == RZORD_SNES || deviceMode == RZORD_NES || deviceMode == RZORD_VBOY)
          && snes_debug_mtap_enabled0 && snes_debug_tap0 > 0) {
        port2_first_idx = snes_port0_controller_count;
      }
    #else
    {
    #endif
      if (port2_first_idx < max_devices && port2_first_idx < MAX_USB_OUT && controllerFrameConst(port2_first_idx).connected) {
        const controller_state_t& port2 = controllerFrameConst(port2_first_idx);
        port2_has_device = true;
        port2_name = port2.controller_type_name[0] ?
                     port2.controller_type_name :
                     (is_passive_controller_mode(deviceMode) ? nullptr : "Connected");
      }
    }
  }

  if (!port2_has_device && showVirtualOutputName) {
    port2_has_device = true;
    port2_name = getVirtualOutputPadName(deviceMode, effectiveOutputMode);
  }
  const bool port2_is_virtual_output_name = showVirtualOutputName && port2_name;

  #ifdef ENABLE_INPUT_JAGUAR
  if (port2_has_device && port2_name && deviceMode != RZORD_JAGUAR) {
    const uint8_t name_pixels = (uint8_t)(strlen(port2_name) * 6);
    display.setCol(port2_is_virtual_output_name && name_pixels < 128
                     ? (uint8_t)(128 - name_pixels)
                     : 66);
    display.print(port2_name);
  } else if (!hideEmptySecondPad && deviceMode != RZORD_JAGUAR) {
  #else
  if (port2_has_device && port2_name) {
    const uint8_t name_pixels = (uint8_t)(strlen(port2_name) * 6);
    display.setCol(port2_is_virtual_output_name && name_pixels < 128
                     ? (uint8_t)(128 - name_pixels)
                     : 66);
    display.print(port2_name);
  } else if (!hideEmptySecondPad) {
  #endif
    display.setCol(91);
    display.print("--");
  }
}

void renderJvsRawDebugLine(bool force) {
  (void)force;
}

bool shouldShowXinputMultiDiagOverlay() {
#if defined(ENABLE_XINPUT_MULTI_OLED_DIAG) && defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  return get_effective_output_mode() == OUTPUT_XINPUT2P;
#else
  return false;
#endif
}

void renderXinputMultiDiagOverlay(bool force) {
#if defined(ENABLE_XINPUT_MULTI_OLED_DIAG) && defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  if (!shouldShowXinputMultiDiagOverlay()) {
    return;
  }

  char lines[4][22];
#ifdef FORCE_XINPUT2P_SINGLE_DRIVER
  XInputDebugInfo xin = {};
  xinput_get_debug_info(&xin);
  XInputAuthRuntimeState& state = xinputAuthRuntimeState();
  RumbleRuntimePortDiag rumbleDiag = {};
  rumbleRuntimeGetPortDiag(0, &rumbleDiag);
  std::snprintf(lines[0], sizeof(lines[0]), "S EP %02X%02X I%03u",
                xin.endpoint_in,
                xin.endpoint_out,
                (unsigned)(state.debug_xfer_in_complete_count % 1000u));
  std::snprintf(lines[1], sizeof(lines[1]), "A K%03u F%03u C%03u",
                (unsigned)(state.debug_xfer_out_submit_ok_count % 1000u),
                (unsigned)(state.debug_xfer_out_submit_fail_count % 1000u),
                (unsigned)(state.debug_control_count % 1000u));
  std::snprintf(lines[2], sizeof(lines[2]), "X0 %03u S%02u L%02X",
                (unsigned)(state.debug_xfer_out_count % 1000u),
                state.debug_last_out_size,
                state.debug_last_rumble_left);
  #if defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_VBOY)
  const uint8_t snesRumbleData = (snes_rumble_debug_data0 != 0)
    ? snes_rumble_debug_data0
    : snes_rumble_debug_data1;
  const uint32_t snesRumbleTx = snes_rumble_debug_tx0 + snes_rumble_debug_tx1;
  #else
  const uint8_t snesRumbleData = 0;
  const uint32_t snesRumbleTx = 0;
  #endif
  std::snprintf(lines[3], sizeof(lines[3]), "R%02X M%02X%02X B%02X Z%u Q%03u",
                state.debug_last_rumble_right,
                rumbleDiag.scaled_left,
                rumbleDiag.scaled_right,
                snesRumbleData,
                rumbleDiag.zero_pending ? 1 : 0,
                (unsigned)(snesRumbleTx % 1000u));
#elif defined(ENABLE_XINPUT2P_WIRELESS_TRANSPORT)
  XInputWDiagInfo winfo = {};
  xinputw_get_diag_info(&winfo);
  std::snprintf(lines[0], sizeof(lines[0]), "W0 %02X%02X X%03u",
                winfo.endpoint_in[0],
                winfo.endpoint_out[0],
                (unsigned)(winfo.out_xfer_count[0] % 1000u));
  std::snprintf(lines[1], sizeof(lines[1]), "P0 L%02X R%02X S%02u",
                winfo.last_rumble_left[0],
                winfo.last_rumble_right[0],
                winfo.last_out_size[0]);
  std::snprintf(lines[2], sizeof(lines[2]), "W1 %02X%02X X%03u",
                winfo.endpoint_in[1],
                winfo.endpoint_out[1],
                (unsigned)(winfo.out_xfer_count[1] % 1000u));
  std::snprintf(lines[3], sizeof(lines[3]), "P1 L%02X R%02X S%02u",
                winfo.last_rumble_left[1],
                winfo.last_rumble_right[1],
                winfo.last_out_size[1]);
#else
  XInputMultiDiagInfo info = {};
  xinput_multi_get_diag_info(&info);
  if (info.controller_count == 1) {
    std::snprintf(lines[0], sizeof(lines[0]), "A K%03u B%03u F%03u",
                  (unsigned)(info.out_arm_ok_count[0] % 1000u),
                  (unsigned)(info.out_arm_busy_count[0] % 1000u),
                  (unsigned)(info.out_arm_fail_count[0] % 1000u));
    std::snprintf(lines[1], sizeof(lines[1]), "C%03u O%03u R%02X W%02X",
                  (unsigned)(info.control_setup_count % 1000u),
                  (unsigned)(info.control_out_count % 1000u),
                  info.last_control_request,
                  info.last_control_wIndex);
    std::snprintf(lines[2], sizeof(lines[2]), "T%03u O%03u D%02X S%02u",
                  (unsigned)(info.xfer_callback_count % 1000u),
                  (unsigned)(info.xfer_override_count % 1000u),
                  info.last_xfer_driver,
                  info.last_xfer_size);
    std::snprintf(lines[3], sizeof(lines[3]), "X%03u P%02X L%02X R%02X",
                  (unsigned)(info.out_xfer_count[0] % 1000u),
                  info.last_out_endpoint[0],
                  info.last_rumble_left[0],
                  info.last_rumble_right[0]);
  } else {
    std::snprintf(lines[0], sizeof(lines[0]), "A0 X%03u R%03u N%03u",
                  (unsigned)(info.out_xfer_count[0] % 1000u),
                  (unsigned)(info.rumble_parse_count[0] % 1000u),
                  (unsigned)(info.rumble_nonzero_count[0] % 1000u));
    std::snprintf(lines[1], sizeof(lines[1]), "P0 %02X%02X%02X%02X%02X%02X",
                  info.last_out_packet[0][0],
                  info.last_out_packet[0][1],
                  info.last_out_packet[0][2],
                  info.last_out_packet[0][3],
                  info.last_out_packet[0][4],
                  info.last_out_packet[0][5]);
    std::snprintf(lines[2], sizeof(lines[2]), "A1 X%03u R%03u N%03u",
                  (unsigned)(info.out_xfer_count[1] % 1000u),
                  (unsigned)(info.rumble_parse_count[1] % 1000u),
                  (unsigned)(info.rumble_nonzero_count[1] % 1000u));
    std::snprintf(lines[3], sizeof(lines[3]), "P1 %02X%02X%02X%02X%02X%02X",
                  info.last_out_packet[1][0],
                  info.last_out_packet[1][1],
                  info.last_out_packet[1][2],
                  info.last_out_packet[1][3],
                  info.last_out_packet[1][4],
                  info.last_out_packet[1][5]);
  }
#endif

  static char lastLines[4][22] = {{0}};
  for (uint8_t i = 0; i < 4; ++i) {
    if (force || std::strcmp(lastLines[i], lines[i]) != 0) {
      const uint8_t row = 3 + i;
      clearRow(row);
      display.setRow(row);
      display.setCol(0);
      display.print(lines[i]);
      std::strncpy(lastLines[i], lines[i], sizeof(lastLines[i]) - 1);
      lastLines[i][sizeof(lastLines[i]) - 1] = '\0';
    }
  }
#else
  (void)force;
#endif
}

}  // namespace menu_main_display_internal
