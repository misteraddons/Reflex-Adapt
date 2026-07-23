#pragma once

// Internal USB send/runtime helpers for the USB device output stack.
// This header is included from out_usb.h after the report mappers and shared
// USB output objects have been defined.

#include "../../platform/latency_test.h"
#include "output_usb_player_count.h"
#include "output_usb_xinput2p_slots_runtime.h"

void send_usb_grouped_report(uint8_t group) {
  uint8_t device_index = group;
  group = group + 1;
  if (outputMode == OUTPUT_GCWIIU) {
    for (uint8_t i = 0; i < 4; ++i) {
      uint8_t input_index = group * i;
      map_gcwiiu_output(i, input_index);
    }
    bool hid_ready = usb_device_hid[device_index]->ready();
    if (hid_ready && usb_device_hid[device_index]->sendReport(0, &_gcwiiu, sizeof(_gcwiiu))) {
      for (uint8_t i = 0; i < 4; ++i) {
        uint8_t input_index = group * i;
        completeControllerFrameDelivery(input_index);
      }
    }
  }
}

bool keyboard_report_needs_delivery(uint8_t portCount) {
  const uint8_t checkedPorts = min((uint8_t)2, portCount);
  for (uint8_t i = 0; i < checkedPorts; ++i) {
    if (controllerFrameNeedsDelivery(i)) {
      return true;
    }
  }
  return false;
}

void complete_keyboard_report_delivery(uint8_t portCount) {
  const uint8_t checkedPorts = min((uint8_t)2, portCount);
  for (uint8_t i = 0; i < checkedPorts; ++i) {
    if (controllerFrameNeedsDelivery(i)) {
      if (latencyTest.isEnabled()) {
        latencyTest.noteUsbSubmit(i, controllerFrameConst(i).digital_buttons);
      }
    }
    completeControllerFrameDelivery(i);
  }
}

void send_usb_keyboard_report() {
  const uint8_t keyboardPorts = min((uint8_t)2, max_devices);
  if (!keyboard_report_needs_delivery(keyboardPorts)) {
    return;
  }
  if (!usb_device_hid[0] || !usb_device_hid[0]->ready()) {
    return;
  }

  map_keyboard_output_combined(keyboardPorts);
  if (usb_device_hid[0]->keyboardReport(0, _keyboard.modifier, _keyboard.keycode)) {
    complete_keyboard_report_delivery(keyboardPorts);
  }
}

void service_usb_feedback_reports() {
  switch (get_effective_output_mode()) {
    case OUTPUT_XINPUT:
      if (_xinput) {
        receive_xinput_report();
      }
      break;

    case OUTPUT_XINPUT2P:
    #ifdef FORCE_XINPUT2P_SINGLE_DRIVER
      if (_xinput) {
        receive_xinput_report();
      }
    #elif defined(ENABLE_XINPUT2P_WIRELESS_TRANSPORT)
      if (_xinputw) {
        const uint8_t outputPlayers = output_usb_player_count();
        for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS && i < outputPlayers; ++i) {
          receive_xinputw_report(i);
        }
      }
    #else
      if (_xinput2p) {
        const uint8_t controllerCount = _xinput2p->controllerCount();
        for (uint8_t i = 0; i < controllerCount; ++i) {
          receive_xinput_multi_report(i);
        }
      }
    #endif
      break;

    case OUTPUT_XID: {
      if (_xid && tud_mounted()) {
        const uint8_t ep_in = (_xid->stored_ep_in != 0) ? _xid->stored_ep_in : 0x81;
        const uint8_t ep_out = (_xid->stored_ep_out != 0) ? _xid->stored_ep_out : 0x02;
        xid_manual_init(0, _xid->stored_itfnum, ep_in, ep_out);
      }
      const int8_t xid_index = xid_get_index_by_type(0, XID_TYPE_WHEEL);
      if (xid_index >= 0) {
        USB_XboxWheel_OutReport_t xpad_rumble = {};
        if (xid_get_report(xid_index, &xpad_rumble, sizeof(xpad_rumble))) {
          uint8_t xid_left = xpad_rumble.lValue >> 8;
          uint8_t xid_right = xpad_rumble.rValue >> 8;
          if ((xid_left | xid_right) == 0 &&
              ((xpad_rumble.lValue & 0xFF) | (xpad_rumble.rValue & 0xFF))) {
            xid_left = xpad_rumble.lValue & 0xFF;
            xid_right = xpad_rumble.rValue & 0xFF;
          }
          rumble_callback(0, xid_left, xid_right);
        }
      }
      break;
    }

    default:
      break;
  }
}

bool output_mode_uses_usb_feedback_reports() {
  switch (get_effective_output_mode()) {
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XINPUTW:
    case OUTPUT_XID:
      return true;
    default:
      return false;
  }
}

void __not_in_flash_func(send_usb_single_report)(uint8_t port) {
  bool force_update = (outputMode == OUTPUT_PS3);
  #ifdef ENABLE_OUTPUT_PS5
  force_update = force_update || (get_effective_output_mode() == OUTPUT_PS5);
  #endif
  if (!controllerFrameNeedsDelivery(port) && !force_update)
    return;

  if (use_device_hid_class) {
    const uint8_t hidInterface = output_usb_hid_interface_for_source_port(port);
    if (!usb_device_hid[hidInterface]) {
      return;
    }
    bool hidReady = false;
    {
      LatencyPhaseTraceScope readyTrace(LATENCY_TRACE_PHASE_USB_READY);
      hidReady = usb_device_hid[hidInterface]->ready();
    }
    if (!hidReady && !force_update) {
      LatencyPhaseTraceScope notReadyTrace(LATENCY_TRACE_PHASE_USB_NOT_READY);
      return;
    }

    uint8_t report_id = 0;
    uint16_t report_size = 0;
    uint8_t report_data[CFG_TUD_HID_EP_BUFSIZE];
    bool ps5_prepared_hash_report = false;
    outputMode_t effectiveOutputMode = get_effective_output_mode();
    {
      LatencyPhaseTraceScope buildTrace(LATENCY_TRACE_PHASE_USB_BUILD);
      switch (effectiveOutputMode) {
        case OUTPUT_HID:
        case OUTPUT_MISTER:
          report_size = sizeof(_hidgp);
          map_hid_output(port);
          memcpy(report_data, &_hidgp, report_size);
          break;
        case OUTPUT_MISTER_JOGCON:
        case OUTPUT_RESERVED_JOGCON:
          report_size = sizeof(_jogcon);
          map_hidjogconmister_output(port);
          memcpy(report_data, &_jogcon, report_size);
          break;
        case OUTPUT_MISTER_NEGCON:
          report_size = sizeof(_negcon);
          map_hidnegconmister_output(port);
          memcpy(report_data, &_negcon, report_size);
          break;
        case OUTPUT_MISTER_GUNCON:
          report_size = sizeof(_guncon);
          map_hidgunconmister_output(port);
          memcpy(report_data, &_guncon, report_size);
          break;
        case OUTPUT_PS3:
          report_size = sizeof(_hidgp);
          map_hid_output(port);
          memcpy(report_data, &_hidgp, report_size);
          break;
        case OUTPUT_PS4:
          report_size = sizeof(_ps4_report);
          map_ps4_output(port);
          memcpy(report_data, &_ps4_report, report_size);
          break;
        #ifdef ENABLE_OUTPUT_PS5
        case OUTPUT_PS5: {
          if (!usb_device_hid[hidInterface]->ready()) {
            return;
          }
          map_ps5_general_output(port);
          uint16_t ps5_report_size = sizeof(_ps5_general_report);
          if (!ps_auth_dongle_prepare_ps5_output_report(
                reinterpret_cast<const uint8_t*>(&_ps5_general_report),
                sizeof(_ps5_general_report),
                report_data,
                &ps5_report_size,
                sizeof(report_data))) {
            return;
          }
          report_size = ps5_report_size;
          ps5_prepared_hash_report = true;
          break;
        }
        #endif
        case OUTPUT_SWITCH:
          report_size = sizeof(_pokken);
          map_switch_output(port);
          memcpy(report_data, &_pokken, report_size);
          break;
        case OUTPUT_SWITCHPRO:
          report_size = 64;
          map_switchpro_output(port);
          memcpy(report_data, switchpro[port]->switchCommon->generate_usb_report(), report_size);
          break;
        case OUTPUT_PANTHERLORD:
          report_size = sizeof(_pantherlord);
          map_pantherlord_output(port);
          memcpy(report_data, &_pantherlord, report_size);
          break;
        case OUTPUT_MDMINI:
          report_size = sizeof(_mdmini);
          map_mdmini_output(port);
          memcpy(report_data, &_mdmini, report_size);
          break;
        default:
          break;
      }
    }

    bool submitReady = hidReady;
    if ((controllerFrameNeedsDelivery(port) || force_update) && submitReady) {
      bool submitted = false;
      {
        LatencyPhaseTraceScope submitTrace(LATENCY_TRACE_PHASE_USB_SUBMIT);
        submitted = usb_device_hid[hidInterface]->sendReport(report_id, report_data, report_size);
      }
      if (!submitted) {
        return;
      }
      #ifdef ENABLE_OUTPUT_PS5
      if (ps5_prepared_hash_report) {
        ps_auth_dongle_complete_ps5_output_report();
      }
      #endif
      if (latencyTest.isEnabled()) {
        latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
      }
      completeControllerFrameDelivery(port);
    }

  } else {
    switch (outputMode) {
      case OUTPUT_XINPUT:
        if (_xinput->ready()) {
          {
            LatencyPhaseTraceScope buildTrace(LATENCY_TRACE_PHASE_USB_BUILD);
            map_xinput_output(port);
          }
          receive_xinput_report();
          {
            LatencyPhaseTraceScope submitTrace(LATENCY_TRACE_PHASE_USB_SUBMIT);
            _xinput->sendReport(&_xinput_report);
          }
          if (latencyTest.isEnabled()) {
            latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
          }
          completeControllerFrameDelivery(port);
        }
        break;
      case OUTPUT_XINPUT2P:
      #ifdef FORCE_XINPUT2P_SINGLE_DRIVER
        if (port == 0 && _xinput && _xinput->ready()) {
          {
            LatencyPhaseTraceScope buildTrace(LATENCY_TRACE_PHASE_USB_BUILD);
            map_xinput_output(port);
          }
          receive_xinput_report();
          {
            LatencyPhaseTraceScope submitTrace(LATENCY_TRACE_PHASE_USB_SUBMIT);
            _xinput->sendReport(&_xinput_report);
          }
          if (latencyTest.isEnabled()) {
            latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
          }
          completeControllerFrameDelivery(port);
        }
      #elif defined(ENABLE_XINPUT2P_WIRELESS_TRANSPORT)
        if (_xinputw && port < XINPUT_WIRELESS_CONTROLLERS && _xinputw->ready(port)) {
          map_xinputw_output(port);
          _xinputw->sendReport(port, &_xinputw_report[port]);
          if (latencyTest.isEnabled()) {
            latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
          }
          completeControllerFrameDelivery(port);
        }
      #else
        if (_xinput2p && port < _xinput2p->controllerCount()) {
          const uint8_t target_slot = xinput_multi_target_slot_for_source_port(port);
          if (target_slot >= _xinput2p->controllerCount() || !_xinput2p->ready(target_slot)) {
            break;
          }
          {
            LatencyPhaseTraceScope buildTrace(LATENCY_TRACE_PHASE_USB_BUILD);
            map_xinput_output_to_report(port, _xinput2p_report[target_slot]);
          }
          receive_xinput_multi_report(target_slot);
          {
            LatencyPhaseTraceScope submitTrace(LATENCY_TRACE_PHASE_USB_SUBMIT);
            _xinput2p->sendReport(target_slot, &_xinput2p_report[target_slot]);
          }
          if (latencyTest.isEnabled()) {
            latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
          }
          completeControllerFrameDelivery(port);
        }
      #endif
        break;
      case OUTPUT_XINPUTW:
        if (_xinputw && port < XINPUT_WIRELESS_CONTROLLERS && _xinputw->ready(port)) {
          {
            LatencyPhaseTraceScope buildTrace(LATENCY_TRACE_PHASE_USB_BUILD);
            map_xinputw_output(port);
          }
          {
            LatencyPhaseTraceScope submitTrace(LATENCY_TRACE_PHASE_USB_SUBMIT);
            _xinputw->sendReport(port, &_xinputw_report[port]);
          }
          if (latencyTest.isEnabled()) {
            latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
          }
          completeControllerFrameDelivery(port);
        }
        break;
      case OUTPUT_XID: {
        if (_xid && tud_mounted()) {
          uint8_t ep_in = (_xid->stored_ep_in != 0) ? _xid->stored_ep_in : 0x81;
          uint8_t ep_out = (_xid->stored_ep_out != 0) ? _xid->stored_ep_out : 0x02;
          xid_manual_init(0, _xid->stored_itfnum, ep_in, ep_out);
        }
        int8_t xid_index = xid_get_index_by_type(0, XID_TYPE_WHEEL);
        if (xid_index >= 0 && xid_send_report_ready(xid_index)) {
          {
            LatencyPhaseTraceScope buildTrace(LATENCY_TRACE_PHASE_USB_BUILD);
            map_xid_output(port);
          }
          {
            LatencyPhaseTraceScope submitTrace(LATENCY_TRACE_PHASE_USB_SUBMIT);
            xid_send_report(xid_index, &xpad_data, sizeof(xpad_data));
          }
          if (latencyTest.isEnabled()) {
            latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
          }
          completeControllerFrameDelivery(port);
        }
        break;
      }
      #ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
      case OUTPUT_XBOXONE:
        if (_xboxone && _xboxone->sendReport(port)) {
          if (latencyTest.isEnabled()) {
            latencyTest.noteUsbSubmit(port, controllerFrameConst(port).digital_buttons);
          }
          completeControllerFrameDelivery(port);
        }
        break;
      #endif
      default:
        break;
    }
  }
}

void __not_in_flash_func(send_usb_report)() {
#ifdef ENABLE_OUTPUT_JVS
  if (outputMode == OUTPUT_JVS) {
    jvsOutput.update();
    return;
  }
#endif

  if (can_run_usb_detection()) {
    return;
  }

  service_xinput2p_dynamic_slot_reenumeration();

  // Host feedback (rumble, LEDs, etc.) can arrive while the controller is idle.
  // Keep OUT endpoints armed even when there is no new input report to send.
  {
    LatencyPhaseTraceScope feedbackTrace(LATENCY_TRACE_PHASE_USB_FEEDBACK);
    if (output_mode_uses_usb_feedback_reports()) {
      service_usb_feedback_reports();
    }
  }

  if (outputMode == OUTPUT_KEYBOARD) {
    send_usb_keyboard_report();
    return;
  }

  if (outputMode == OUTPUT_XINPUTW
  #ifdef ENABLE_XINPUT2P_WIRELESS_TRANSPORT
      || outputMode == OUTPUT_XINPUT2P
  #endif
      ) {
    if (_xinputw) {
      const uint8_t outputPlayers = output_usb_player_count();
      for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS && i < outputPlayers; ++i) {
        bool slotConnected = controllerFrameConst(i).connected;
        #ifdef ENABLE_XINPUT2P_WIRELESS_TRANSPORT
        // XInput2P over the wireless-receiver transport represents fixed
        // adapter slots, not user-paired wireless pads. Keep those logical
        // slots attached so Windows exposes them to XInput and sends rumble.
        if (outputMode == OUTPUT_XINPUT2P) {
          slotConnected = true;
        }
        #endif
        _xinputw->set_connected(i, slotConnected);
      }
      _xinputw->driver_task();
    }
  }

  if (grouped_devices) {
    for (uint8_t i = 0; i < grouped_devices; ++i) {
      send_usb_grouped_report(i);
    }
  } else {
    const uint8_t outputPlayers = output_usb_player_count();
    for (uint8_t i = 0; i < outputPlayers; ++i) {
      send_usb_single_report(i);
    }
  }
}
