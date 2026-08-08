#include "Input_Pce.h"
#include "input_pce_runtime_state.h"

#include <cstring>

#include "../shared/input_button_bits.h"

namespace {

PceInputDebugSnapshot pce_debug_snapshot = {};

}  // namespace

void input_pce_debug_update_slot(uint8_t slot, uint8_t stable_type,
                                 uint8_t observed_type, uint8_t pending_type,
                                 uint16_t pending_count, uint8_t source_port,
                                 uint8_t source_index) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  PceInputDebugSlot& debug = pce_debug_snapshot.slots[slot];
  debug.stable_type = stable_type;
  debug.observed_type = observed_type;
  debug.pending_type = pending_type;
  debug.pending_count = pending_count;
  debug.source_port = source_port;
  debug.source_index = source_index;
  if (pce_debug_snapshot.slot_count <= slot) {
    pce_debug_snapshot.slot_count = slot + 1;
  }
}

void input_pce_debug_clear_slot(uint8_t slot) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  pce_debug_snapshot.slots[slot] = {};
  if (slot + 1 == pce_debug_snapshot.slot_count) {
    while (pce_debug_snapshot.slot_count > 0 &&
           pce_debug_snapshot.slots[pce_debug_snapshot.slot_count - 1].stable_type == 0 &&
           pce_debug_snapshot.slots[pce_debug_snapshot.slot_count - 1].observed_type == 0 &&
           pce_debug_snapshot.slots[pce_debug_snapshot.slot_count - 1].pending_type == 0) {
      --pce_debug_snapshot.slot_count;
    }
  }
}

PceInputDebugSnapshot input_pce_debug_snapshot() {
  return pce_debug_snapshot;
}

void __not_in_flash_func(RZInputPce::clearPendingDeviceType)(uint8_t slot) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  pendingDtype[slot] = PCE_DEVICE_NONE;
  pendingDtypeCount[slot] = 0;
}

PceDeviceType_Enum __not_in_flash_func(RZInputPce::stabilizeDeviceType)(
    uint8_t slot,
    PceDeviceType_Enum observedType,
    bool justConnected
) {
  if (slot >= MAX_USB_OUT) {
    return observedType;
  }

  if (justConnected || dtype[slot] == PCE_DEVICE_NONE) {
    dtype[slot] = observedType;
    clearPendingDeviceType(slot);
    return dtype[slot];
  }

  if (observedType == dtype[slot]) {
    clearPendingDeviceType(slot);
    return dtype[slot];
  }

  const uint16_t requiredConfirmations =
    (dtype[slot] == PCE_DEVICE_PAD2 && observedType == PCE_DEVICE_PAD6)
      ? TYPE_UPGRADE_CONFIRM_POLLS
      : ((dtype[slot] == PCE_DEVICE_PAD6 && observedType == PCE_DEVICE_PAD2)
          ? TYPE_DOWNGRADE_CONFIRM_POLLS
          : TYPE_CHANGE_CONFIRM_POLLS);

  if (pendingDtype[slot] != observedType) {
    pendingDtype[slot] = observedType;
    pendingDtypeCount[slot] = 1;
  } else if (pendingDtypeCount[slot] < 0xFFFF) {
    ++pendingDtypeCount[slot];
  }

  if (pendingDtypeCount[slot] >= requiredConfirmations) {
    dtype[slot] = observedType;
    clearPendingDeviceType(slot);
  }

  return dtype[slot];
}

uint32_t __not_in_flash_func(RZInputPce::mapDigitalToButtons)(PceDeviceType_Enum type, uint16_t digital) const {
  uint32_t buttons = 0;
  buttons |= ((digital & PCE_PAD_UP) == 0) ? INPUT_PAD_U : 0;
  buttons |= ((digital & PCE_PAD_DOWN) == 0) ? INPUT_PAD_D : 0;
  buttons |= ((digital & PCE_PAD_LEFT) == 0) ? INPUT_PAD_L : 0;
  buttons |= ((digital & PCE_PAD_RIGHT) == 0) ? INPUT_PAD_R : 0;
  buttons |= ((digital & PCE_2) == 0) ? INPUT_A : 0;
  buttons |= ((digital & PCE_1) == 0) ? INPUT_B : 0;
  buttons |= ((digital & PCE_SELECT) == 0) ? INPUT_SELECT : 0;
  buttons |= ((digital & PCE_RUN) == 0) ? INPUT_START : 0;

  if (type == PCE_DEVICE_PAD6) {
    buttons |= ((digital & PCE_5) == 0) ? INPUT_X : 0;
    buttons |= ((digital & PCE_6) == 0) ? INPUT_Y : 0;
    buttons |= ((digital & PCE_4) == 0) ? INPUT_L1 : 0;
    buttons |= ((digital & PCE_3) == 0) ? INPUT_R1 : 0;
  }

  return buttons;
}

void __not_in_flash_func(RZInputPce::stageWebhidRawData)(PceDeviceType_Enum stableType,
                                                         uint16_t digital,
                                                         uint8_t port,
                                                         uint8_t sourceIndex) {
  pendingWebhidRaw[0] = (uint8_t)stableType;
  pendingWebhidRaw[1] = digital >> 8;
  pendingWebhidRaw[2] = digital & 0xFF;
  pendingWebhidRaw[3] = port;
  pendingWebhidRaw[4] = sourceIndex;
  pendingWebhidRawDirty = true;
}

void RZInputPce::flushPendingWebhidRawData() {
  if (!pendingWebhidRawDirty) {
    return;
  }
  uint8_t raw[16] = {0};
  memcpy(raw, pendingWebhidRaw, sizeof(pendingWebhidRaw));
  webhid_store_raw_data(raw, 16);
  pendingWebhidRawDirty = false;
}

bool __not_in_flash_func(RZInputPce::poll)() {
  beginPollCycle();

  uint8_t port_controller_count[input_ports] = {0};
  bool port_raw_present[input_ports] = {false};
  uint8_t total_reserved_slots = 0;

  for (uint8_t port = 0; port < input_ports; ++port) {
    if (port < MAX_USB_OUT) {
      pce[port]->update();
      port_controller_count[port] = pce[port]->getControllerCount();
      port_raw_present[port] = rawControllerPresent(port);
      if (!port_raw_present[port]) {
        port_controller_count[port] = 0;
      }
      uint8_t currentTap = pce[port]->getMultitapPorts();
      uint8_t observedCapacity = currentTap ? currentTap : 1;
      if (observedCapacity > portSlotCapacity[port]) {
        portSlotCapacity[port] = observedCapacity;
      }
    }
    total_reserved_slots += portSlotCapacity[port];
  }
  setInputPortCount(min(total_reserved_slots, (uint8_t)MAX_USB_OUT));

  uint8_t slotBase = 0;

  for (uint8_t port = 0; port < input_ports; ++port) {
    if (slotBase >= MAX_USB_OUT)
      break;

    uint8_t slotCapacity = portSlotCapacity[port];
    uint8_t reservedSlots = min(slotCapacity, (uint8_t)(MAX_USB_OUT - slotBase));
    uint8_t activeSlots = min(port_controller_count[port], reservedSlots);

    for (uint8_t c = 0; c < activeSlots; ++c) {
      uint8_t i = slotBase + c;

      const PceController& sc = pce[port]->getPceController(c);
      slotLastSeenMs[i] = millis();

      controller_state_t& frame = inputFrame(i);
      bool justConnected = !frame.connected;
      if (justConnected) {
        setInputFrameConnected(i, true);
        setUpdated(i);
      }

      const PceDeviceType_Enum observedType = sc.deviceType();
      const PceDeviceType_Enum previousType = dtype[i];
      const PceDeviceType_Enum stableType =
        stabilizeDeviceType(i, observedType, justConnected);
      input_pce_debug_update_slot(i, (uint8_t)stableType, (uint8_t)observedType,
                                  (uint8_t)pendingDtype[i], pendingDtypeCount[i],
                                  port, c);
      const bool typeChanged = (previousType != stableType);
      const bool typeObservationPending = (observedType != stableType) && !typeChanged;
      const bool stateChanged = sc.stateChanged() && !typeObservationPending;

      if (stateChanged || justConnected || typeChanged) {
        resetState(i);
        if (sc.deviceJustChanged() || justConnected || typeChanged) {
          setInputFrameConnected(i, stableType != PCE_DEVICE_NONE);
          switch (stableType) {
            case PCE_DEVICE_PAD2:
              setInputFrameTypeName(i, "2-Button");
              break;
            case PCE_DEVICE_PAD6:
              setInputFrameTypeName(i, "6-Button");
              break;
            default:
              clearInputFrameTypeName(i);
              break;
          }
        }

        switch (stableType) {
          case PCE_DEVICE_NONE:
          case PCE_DEVICE_NOTSUPPORTED:
            break;
          case PCE_DEVICE_PAD6:
          case PCE_DEVICE_PAD2:
            frame.digital_buttons = mapDigitalToButtons(stableType, sc.currentState.digital);
            setInputFrameConnected(i, true);
            break;
        }
        setUpdated(i);

        if (i == 0) {
          stageWebhidRawData(stableType, sc.currentState.digital, port, c);
        }
      }
    }

    for (uint8_t stale = activeSlots; stale < reservedSlots; ++stale) {
      uint8_t i = slotBase + stale;
      if (dtype[i] != PCE_DEVICE_NONE) {
        uint32_t now_ms = millis();
        if (slotLastSeenMs[i] != 0 && (now_ms - slotLastSeenMs[i]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
          continue;
        }
        dtype[i] = PCE_DEVICE_NONE;
        clearPendingDeviceType(i);
        input_pce_debug_clear_slot(i);
        slotLastSeenMs[i] = 0;
        setInputFrameConnected(i, false);
        clearInputFrameTypeName(i);
        setUpdated(i);
      }
    }
    slotBase += reservedSlots;
  }

  while (slotBase < MAX_USB_OUT) {
    if (dtype[slotBase] != PCE_DEVICE_NONE) {
      uint32_t now_ms = millis();
      if (slotLastSeenMs[slotBase] != 0 && (now_ms - slotLastSeenMs[slotBase]) < SLOT_DISCONNECT_DEBOUNCE_MS) {
        ++slotBase;
        continue;
      }
      dtype[slotBase] = PCE_DEVICE_NONE;
      clearPendingDeviceType(slotBase);
      input_pce_debug_clear_slot(slotBase);
      slotLastSeenMs[slotBase] = 0;
      setInputFrameConnected(slotBase, false);
      clearInputFrameTypeName(slotBase);
      setUpdated(slotBase);
    }
    ++slotBase;
  }

  return endPollCycle();
}

void RZInputPce::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  flushPendingWebhidRawData();
}
