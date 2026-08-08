/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Portions Copyright (c) 2024 OpenStickCommunity
 *
 * Xbox One/XGIP output for Reflex Adapt. This intentionally preserves the
 * donor XGIP sequencing used by GP2040-CE instead of re-inventing auth.
 */

#include "../../product_config.h"

#include "out_xboxone.h"

#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT)

#include <Arduino.h>
#include <string.h>

#include "../../core/button_map_mode.h"
#include "../../core/controller_frame_state.h"
#include "../../core/controller_settings_state.h"
#include "../../core/device_runtime_state.h"
#include "../auth/xbone_auth_passthrough.h"
#include "../output_capabilities.h"
#include "../output_runtime_state.h"
#include "xgip_protocol.h"

namespace {

constexpr uint8_t kEndpointSize = 64;
constexpr uint32_t kAnnounceDelayMs = 500;
constexpr uint32_t kKeepaliveMs = 15000;
constexpr uint32_t kReportQueueIntervalMs = 35;
constexpr uint32_t kAckTimeoutMs = 2000;

enum XboxOneDriverState : uint8_t {
  XBONE_READY_ANNOUNCE = 0,
  XBONE_WAIT_DESCRIPTOR_REQUEST = 1,
  XBONE_SEND_DESCRIPTOR = 2,
  XBONE_SETUP_AUTH = 3,
  XBONE_AUTH_DONE = 4,
  XBONE_NOT_READY = 5,
};

struct __attribute__((packed)) XboxOneGamepadReport {
  GipHeader header;
  uint8_t sync : 1;
  uint8_t guide : 1;
  uint8_t start : 1;
  uint8_t back : 1;
  uint8_t a : 1;
  uint8_t b : 1;
  uint8_t x : 1;
  uint8_t y : 1;
  uint8_t dpadUp : 1;
  uint8_t dpadDown : 1;
  uint8_t dpadLeft : 1;
  uint8_t dpadRight : 1;
  uint8_t leftShoulder : 1;
  uint8_t rightShoulder : 1;
  uint8_t leftThumbClick : 1;
  uint8_t rightThumbClick : 1;
  uint16_t leftTrigger;
  uint16_t rightTrigger;
  int16_t leftStickX;
  int16_t leftStickY;
  int16_t rightStickX;
  int16_t rightStickY;
  uint8_t reserved[18];
};

struct QueuedReport {
  uint8_t data[kEndpointSize];
  uint16_t len;
};

struct __attribute__((packed)) OsCompatibleIdDescriptorSingle {
  uint32_t totalLength;
  uint16_t version;
  uint16_t index;
  uint8_t totalSections;
  uint8_t reserved[7];
  uint8_t firstInterfaceNumber;
  uint8_t reserved2;
  uint8_t compatibleId[8];
  uint8_t subCompatibleId[8];
  uint8_t reserved3[6];
};

constexpr uint8_t kDeviceDescriptor[] = {
  0x12, 0x01, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0x40,
  0x6F, 0x0E, 0xA4, 0x02, 0x01, 0x01, 0x01, 0x02,
  0x03, 0x01,
};

constexpr uint8_t kConfigDescriptor[] = {
  0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xA0, 0xFA,
  0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x47, 0xD0, 0x00,
  0x07, 0x05, 0x81, 0x03, 0x40, 0x00, 0x01,
  0x07, 0x05, 0x02, 0x03, 0x40, 0x00, 0x01,
};

constexpr uint8_t kAnnouncePacket[] = {
  0x00, 0x2a, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
  0xdf, 0x33, 0x14, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x17, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x01, 0x00, 0x01, 0x00
};

constexpr uint8_t kXboxOneDescriptor[] = {
  0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCA, 0x00,
  0x8B, 0x00, 0x16, 0x00, 0x1F, 0x00, 0x20, 0x00,
  0x27, 0x00, 0x2D, 0x00, 0x4A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
  0x06, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x05,
  0x01, 0x04, 0x05, 0x06, 0x0A, 0x01, 0x1A, 0x00,
  0x57, 0x69, 0x6E, 0x64, 0x6F, 0x77, 0x73, 0x2E,
  0x58, 0x62, 0x6F, 0x78, 0x2E, 0x49, 0x6E, 0x70,
  0x75, 0x74, 0x2E, 0x47, 0x61, 0x6D, 0x65, 0x70,
  0x61, 0x64, 0x04, 0x56, 0xFF, 0x76, 0x97, 0xFD,
  0x9B, 0x81, 0x45, 0xAD, 0x45, 0xB6, 0x45, 0xBB,
  0xA5, 0x26, 0xD6, 0x2C, 0x40, 0x2E, 0x08, 0xDF,
  0x07, 0xE1, 0x45, 0xA5, 0xAB, 0xA3, 0x12, 0x7A,
  0xF1, 0x97, 0xB5, 0xE7, 0x1F, 0xF3, 0xB8, 0x86,
  0x73, 0xE9, 0x40, 0xA9, 0xF8, 0x2F, 0x21, 0x26,
  0x3A, 0xCF, 0xB7, 0xFE, 0xD2, 0xDD, 0xEC, 0x87,
  0xD3, 0x94, 0x42, 0xBD, 0x96, 0x1A, 0x71, 0x2E,
  0x3D, 0xC7, 0x7D, 0x02, 0x17, 0x00, 0x20, 0x20,
  0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x17, 0x00, 0x09, 0x3C, 0x00,
  0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00
};

constexpr uint8_t kAuthReady[] = {0x01, 0x00};
constexpr uint8_t kGuideOn[] = {0x01, 0x5b};
constexpr uint8_t kGuideOff[] = {0x00, 0x5b};
constexpr uint8_t kIdlePayload[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
  0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

constexpr OsCompatibleIdDescriptorSingle kDevCompatId = {
  sizeof(OsCompatibleIdDescriptorSingle),
  0x0100,
  0x0004,
  1,
  {0},
  0,
  0x01,
  {'X','G','I','P','1','0',0,0},
  {0},
  {0}
};

Adafruit_USBD_XboxOne* xboxOneDevice = nullptr;
XboxOneDriverState driverState = XBONE_NOT_READY;
XboxOneOutputStatus status = {};
XGIPProtocol incoming;
XGIPProtocol outgoing;
QueuedReport reportQueue[8] = {};
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint32_t lastQueueSendMs = 0;
uint32_t announceStartMs = 0;
uint32_t keepaliveMs = 0;
uint32_t ackWaitStartMs = 0;
bool waitingAck = false;
uint8_t keepaliveSequence = 1;
uint8_t inputSequence = 1;
uint8_t virtualKeySequence = 1;
bool guidePressed = false;
uint8_t lastReport[sizeof(XboxOneGamepadReport)] = {};
uint8_t epIn = 0;
uint8_t epOut = 0;
CFG_TUSB_MEM_ALIGN uint8_t epOutBuffer[kEndpointSize] = {};
uint8_t itfNum = 0;

bool queueEmpty() {
  return queueHead == queueTail;
}

bool queueFull() {
  return static_cast<uint8_t>((queueHead + 1) % 8) == queueTail;
}

bool sendXboxOneUsb(const uint8_t* report, uint16_t len) {
  if (report == nullptr || len == 0 || epIn == 0 || !tud_ready() ||
      usbd_edpt_busy(0, epIn)) {
    return false;
  }
  if (!usbd_edpt_claim(0, epIn)) {
    return false;
  }
  const bool ok = usbd_edpt_xfer(0, epIn, const_cast<uint8_t*>(report), len);
  usbd_edpt_release(0, epIn);
  if (ok) {
    status.out_count++;
  }
  return ok;
}

bool queueXboxOneReport(const uint8_t* report, uint16_t len) {
  if (report == nullptr || len == 0 || len > sizeof(reportQueue[0].data)) {
    status.queue_drop_count++;
    return false;
  }
  if (queueFull()) {
    status.queue_drop_count++;
    return false;
  }
  memcpy(reportQueue[queueHead].data, report, len);
  reportQueue[queueHead].len = len;
  queueHead = static_cast<uint8_t>((queueHead + 1) % 8);
  status.queue_count++;
  return true;
}

void clearQueue() {
  queueHead = queueTail = 0;
  memset(reportQueue, 0, sizeof(reportQueue));
}

void processReportQueue(uint32_t now) {
  if (queueEmpty()) {
    return;
  }
  if (lastQueueSendMs != 0 && (now - lastQueueSendMs) < kReportQueueIntervalMs) {
    return;
  }
  if (sendXboxOneUsb(reportQueue[queueTail].data, reportQueue[queueTail].len)) {
    memcpy(lastReport, reportQueue[queueTail].data,
           min((size_t)reportQueue[queueTail].len, sizeof(lastReport)));
    queueTail = static_cast<uint8_t>((queueTail + 1) % 8);
    lastQueueSendMs = now;
  }
}

void setAckWait(uint32_t now) {
  waitingAck = true;
  ackWaitStartMs = now;
}

void initGamepadHeader(XboxOneGamepadReport& report, uint8_t sequence) {
  memset(&report, 0, sizeof(report));
  report.header.command = GIP_INPUT_REPORT;
  report.header.sequence = sequence;
  report.header.length = sizeof(report) - sizeof(GipHeader);
}

int16_t convertAxis16(int16_t value, analog_stick_precision precision) {
  switch (precision) {
    case ANALOG_STICK_PRECISION_8: {
      const int16_t clamped = constrain(value, (int16_t)-128, (int16_t)127);
      if (clamped <= -128) return INT16_MIN;
      if (clamped >= 127) return INT16_MAX;
      return static_cast<int16_t>(clamped * 256);
    }
    case ANALOG_STICK_PRECISION_12:
      return static_cast<int16_t>(constrain(value, (int16_t)-2048, (int16_t)2047) * 16);
    case ANALOG_STICK_PRECISION_16:
    default:
      return value;
  }
}

void mapXboxOneReport(uint8_t port, XboxOneGamepadReport& report) {
  initGamepadHeader(report, inputSequence);

  const controller_state_t& frame = controllerFrameConst(port);
  const bool positionMapActive =
    buttonMapModeAppliesToInputMode(deviceMode) && button_map_mode == 1;
  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    applyNintendoPositionButtonMap(frame.digital_buttons, positionMapActive),
    frame);
  report.a = (faceButtons & INPUT_A) != 0;
  report.b = (faceButtons & INPUT_B) != 0;
  report.x = (faceButtons & INPUT_X) != 0;
  report.y = (faceButtons & INPUT_Y) != 0;
  report.leftShoulder = frame.L1;
  report.rightShoulder = frame.R1;
  report.leftThumbClick = frame.L3;
  report.rightThumbClick = frame.R3;
  report.start = frame.START;
  report.back = output_n64_c_backing_select(frame);
  report.dpadUp = frame.PAD_U;
  report.dpadDown = frame.PAD_D;
  report.dpadLeft = frame.PAD_L;
  report.dpadRight = frame.PAD_R;
  report.leftTrigger = frame.HAS_ANALOG_TRIGGERS ? ((uint16_t)frame.ANALOG_L2 << 2) : (frame.L2 ? 0x03FF : 0);
  report.rightTrigger = frame.HAS_ANALOG_TRIGGERS ? ((uint16_t)frame.ANALOG_R2 << 2) : (output_n64_c_backing_r2(frame) ? 0x03FF : 0);
  report.leftStickX = convertAxis16(frame.LX, frame.sticks_precision_bits);
  report.leftStickY = static_cast<int16_t>(~convertAxis16(frame.LY, frame.sticks_precision_bits));
  report.rightStickX = convertAxis16(frame.RX, frame.sticks_precision_bits);
  report.rightStickY = static_cast<int16_t>(~convertAxis16(frame.RY, frame.sticks_precision_bits));
}

bool sendVirtualKeycode(bool pressed) {
  uint8_t payload[2] = {};
  memcpy(payload, pressed ? kGuideOn : kGuideOff, sizeof(payload));
  outgoing.reset();
  uint8_t sequence = ++virtualKeySequence;
  if (sequence == 0) {
    sequence = virtualKeySequence = 1;
  }
  outgoing.setAttributes(GIP_VIRTUAL_KEYCODE, sequence, 1, 0, 0);
  outgoing.setData(payload, sizeof(payload));
  return sendXboxOneUsb(outgoing.generatePacket(), outgoing.getPacketLength());
}

void sendKeepalive(uint32_t now) {
  static const uint8_t keepalivePayload[] = {0x80, 0x00, 0x00, 0x00};
  if ((now - keepaliveMs) <= kKeepaliveMs) {
    return;
  }
  outgoing.reset();
  outgoing.setAttributes(GIP_KEEPALIVE, keepaliveSequence, 1, 0, 0);
  outgoing.setData(keepalivePayload, sizeof(keepalivePayload));
  if (sendXboxOneUsb(outgoing.generatePacket(), outgoing.getPacketLength())) {
    keepaliveMs = now;
    keepaliveSequence++;
    if (keepaliveSequence == 0) {
      keepaliveSequence = 1;
    }
  }
}

void updateDriverState(uint32_t now) {
  processReportQueue(now);
  if (waitingAck) {
    if ((now - ackWaitStartMs) < kAckTimeoutMs) {
      return;
    }
    waitingAck = false;
  }

  switch (driverState) {
    case XBONE_READY_ANNOUNCE:
      if ((now - announceStartMs) > kAnnounceDelayMs) {
        uint8_t announce[sizeof(kAnnouncePacket)] = {};
        memcpy(announce, kAnnouncePacket, sizeof(announce));
        announce[3] = now & 0xFF;
        announce[4] = (now >> 8) & 0xFF;
        announce[5] = (now >> 16) & 0xFF;
        outgoing.reset();
        outgoing.setAttributes(GIP_ANNOUNCE, 1, 1, 0, 0);
        outgoing.setData(announce, sizeof(announce));
        queueXboxOneReport(outgoing.generatePacket(), outgoing.getPacketLength());
        driverState = XBONE_WAIT_DESCRIPTOR_REQUEST;
      }
      break;
    case XBONE_SEND_DESCRIPTOR:
      if (queueXboxOneReport(outgoing.generatePacket(), outgoing.getPacketLength())) {
        if (outgoing.endOfChunk()) {
          driverState = XBONE_SETUP_AUTH;
        }
        if (outgoing.getPacketAck()) {
          setAckWait(now);
        }
      }
      break;
    case XBONE_SETUP_AUTH: {
      XboxOneAuthPacket packet = {};
      if (xbone_auth_passthrough_take_dongle_packet(&packet)) {
        const bool isChunked = packet.length > XGIP_MAX_CHUNK_SIZE;
        outgoing.reset();
        outgoing.setAttributes(packet.command, packet.sequence, 1, isChunked, 1);
        outgoing.setData(packet.data, packet.length);
        status.auth_dongle_count++;
        if (queueXboxOneReport(outgoing.generatePacket(), outgoing.getPacketLength()) &&
            outgoing.getPacketAck()) {
          setAckWait(now);
        }
      }
      break;
    }
    default:
      break;
  }
}

void resetRuntimeState() {
  driverState = XBONE_NOT_READY;
  memset(&status, 0, sizeof(status));
  status.supported = true;
  incoming.reset();
  outgoing.reset();
  clearQueue();
  lastQueueSendMs = 0;
  announceStartMs = millis();
  keepaliveMs = millis();
  ackWaitStartMs = 0;
  waitingAck = false;
  keepaliveSequence = 1;
  inputSequence = 1;
  virtualKeySequence = 1;
  guidePressed = false;
  memset(lastReport, 0, sizeof(lastReport));
  epIn = 0;
  epOut = 0;
  itfNum = 0;
}

}  // namespace

Adafruit_USBD_XboxOne::Adafruit_USBD_XboxOne(uint8_t interval_ms)
  : _interval_ms(interval_ms) {}

bool Adafruit_USBD_XboxOne::begin(void) {
  if (!TinyUSBDevice.addInterface(*this)) {
    return false;
  }
  TinyUSBDevice.setVersion(0x0200);
  TinyUSBDevice.setDeviceVersion(0x0101);
  TinyUSBDevice.setMaxPacketSize0(64);
  xboxOneDevice = this;
  return true;
}

bool Adafruit_USBD_XboxOne::ready(void) {
  return epIn != 0 && tud_ready() && !usbd_edpt_busy(0, epIn);
}

bool Adafruit_USBD_XboxOne::sendReport(uint8_t port) {
  return xboxone_send_report_for_port(port);
}

uint16_t Adafruit_USBD_XboxOne::getInterfaceDescriptor(uint8_t itfnum, uint8_t* buf, uint16_t bufsize) {
  (void)itfnum;
  const uint16_t headerSize = 9;
  const uint16_t len = sizeof(kConfigDescriptor) - headerSize;
  if (buf == nullptr) {
    return len;
  }
  if (bufsize < len) {
    return 0;
  }

  memcpy(buf, kConfigDescriptor + headerSize, len);
  itfNum = TinyUSBDevice.allocInterface(1);
  const uint8_t allocIn = TinyUSBDevice.allocEndpoint(TUSB_DIR_IN);
  const uint8_t allocOut = TinyUSBDevice.allocEndpoint(TUSB_DIR_OUT);
  epIn = allocIn ? allocIn : 0x81;
  epOut = allocOut ? allocOut : 0x02;
  _endpoint_in = epIn;
  _endpoint_out = epOut;

  buf[2] = itfNum;
  buf[11] = epIn;
  buf[18] = epOut;
  return len;
}

bool xboxone_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t* request) {
  if (stage != CONTROL_STAGE_SETUP) {
    return true;
  }

  if (request->bmRequestType == 0xC0 &&
      request->bRequest == 0x20 &&
      request->wIndex == 0x0004) {
    const uint16_t len = min((uint16_t)sizeof(kDevCompatId), request->wLength);
    return tud_control_xfer(rhport, request, (void*)&kDevCompatId, len);
  }

  if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
    return tud_control_status(rhport, request);
  }
  return false;
}

bool xboxone_send_report_for_port(uint8_t port) {
  const uint32_t now = millis();
  updateDriverState(now);

  if (!xbone_auth_passthrough_authenticated()) {
    XboxOneGamepadReport idle = {};
    initGamepadHeader(idle, inputSequence);
    memcpy(reinterpret_cast<uint8_t*>(&idle) + sizeof(GipHeader),
           kIdlePayload,
           sizeof(kIdlePayload));
    sendXboxOneUsb(reinterpret_cast<const uint8_t*>(&idle), sizeof(idle));
    return true;
  }

  sendKeepalive(now);

  const bool home = controllerFrameConst(port).HOME;
  if (home != guidePressed && sendVirtualKeycode(home)) {
    guidePressed = home;
    return true;
  }

  XboxOneGamepadReport report = {};
  mapXboxOneReport(port, report);
  if (memcmp(lastReport + sizeof(GipHeader),
             reinterpret_cast<const uint8_t*>(&report) + sizeof(GipHeader),
             sizeof(report) - sizeof(GipHeader)) == 0) {
    return false;
  }

  report.header.sequence = inputSequence + 1;
  if (report.header.sequence == 0) {
    report.header.sequence = 1;
  }

  if (sendXboxOneUsb(reinterpret_cast<const uint8_t*>(&report), sizeof(report))) {
    inputSequence = report.header.sequence;
    memcpy(lastReport, &report, sizeof(report));
    return true;
  }
  return false;
}

XboxOneOutputStatus xboxone_output_status() {
  status.mounted = epIn != 0;
  status.ready = epIn != 0 && tud_ready();
  status.auth_ready = xbone_auth_passthrough_authenticated();
  status.state = driverState;
  status.ep_in = epIn;
  status.ep_out = epOut;
  return status;
}

extern "C" {

static void xboxone_init(void) {
  resetRuntimeState();
}

static void xboxone_reset(uint8_t rhport) {
  (void)rhport;
  resetRuntimeState();
}

static uint16_t xboxone_open(uint8_t rhport, tusb_desc_interface_t const* itf_desc, uint16_t max_len) {
  if (!itf_desc ||
      itf_desc->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC ||
      itf_desc->bInterfaceSubClass != 0x47 ||
      itf_desc->bInterfaceProtocol != 0xD0) {
    return 0;
  }

  const uint16_t drvLen = sizeof(tusb_desc_interface_t) +
                          (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
  TU_VERIFY(max_len >= drvLen, 0);

  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(itf_desc);
  ptr = tu_desc_next(ptr);
  uint8_t opened = 0;
  while (opened < itf_desc->bNumEndpoints) {
    const auto* ep = reinterpret_cast<const tusb_desc_endpoint_t*>(ptr);
    if (tu_desc_type(ep) == TUSB_DESC_ENDPOINT) {
      TU_ASSERT(usbd_edpt_open(rhport, ep), 0);
      if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN) {
        epIn = ep->bEndpointAddress;
      } else {
        epOut = ep->bEndpointAddress;
      }
      opened++;
    }
    ptr = tu_desc_next(ptr);
  }

  itfNum = itf_desc->bInterfaceNumber;
  announceStartMs = millis();
  keepaliveMs = millis();
  driverState = XBONE_READY_ANNOUNCE;
  incoming.reset();
  outgoing.reset();
  if (epOut != 0) {
    usbd_edpt_xfer(rhport, epOut, epOutBuffer, sizeof(epOutBuffer));
  }
  return drvLen;
}

static bool xboxone_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
  if (result != XFER_RESULT_SUCCESS) {
    if (ep_addr == epOut) {
      usbd_edpt_xfer(rhport, epOut, epOutBuffer, sizeof(epOutBuffer));
    }
    return true;
  }

  if (ep_addr == epIn) {
    return true;
  }

  if (ep_addr != epOut) {
    return false;
  }

  status.in_count++;
  status.last_len = static_cast<uint16_t>(xferred_bytes);
  incoming.parse(epOutBuffer, static_cast<uint16_t>(xferred_bytes));
  if (incoming.validate()) {
    status.last_command = incoming.getCommand();
    status.last_sequence = incoming.getSequence();
    if (incoming.ackRequired()) {
      queueXboxOneReport(incoming.generateAckPacket(), incoming.getPacketLength());
    }

    const uint8_t command = incoming.getCommand();
    if (command == GIP_ACK_RESPONSE) {
      waitingAck = false;
    } else if (command == GIP_DEVICE_DESCRIPTOR) {
      outgoing.reset();
      outgoing.setAttributes(GIP_DEVICE_DESCRIPTOR, incoming.getSequence(), 1, 1, 0);
      outgoing.setData(kXboxOneDescriptor, sizeof(kXboxOneDescriptor));
      driverState = XBONE_SEND_DESCRIPTOR;
    } else if (command == GIP_CMD_LED_ON) {
      if (driverState == XBONE_WAIT_DESCRIPTOR_REQUEST) {
        outgoing.reset();
        outgoing.setAttributes(GIP_DEVICE_DESCRIPTOR, incoming.getSequence(), 1, 1, 0);
        outgoing.setData(kXboxOneDescriptor, sizeof(kXboxOneDescriptor));
        driverState = XBONE_SEND_DESCRIPTOR;
      }
    } else if (command == GIP_AUTH || command == GIP_FINAL_AUTH) {
      if (incoming.getDataLength() == sizeof(kAuthReady) &&
          memcmp(incoming.getData(), kAuthReady, sizeof(kAuthReady)) == 0) {
        xbone_auth_passthrough_mark_authenticated();
        driverState = XBONE_AUTH_DONE;
      }
      if (!incoming.getChunked() || incoming.endOfChunk()) {
        if (xbone_auth_passthrough_submit_console_packet(command,
                                                         incoming.getSequence(),
                                                         incoming.getData(),
                                                         incoming.getDataLength())) {
          status.auth_console_count++;
        }
        incoming.reset();
      }
    }
  }

  usbd_edpt_xfer(rhport, epOut, epOutBuffer, sizeof(epOutBuffer));
  return true;
}

static const usbd_class_driver_t xboxone_driver = {
#if CFG_TUSB_DEBUG >= 2
  .name = "XBONE",
#endif
  .init = xboxone_init,
  .reset = xboxone_reset,
  .open = xboxone_open,
  .control_xfer_cb = xboxone_vendor_control_xfer_cb,
  .xfer_cb = xboxone_xfer_cb,
  .sof = nullptr,
};

const usbd_class_driver_t* xboxone_get_driver() {
  return &xboxone_driver;
}

}  // extern "C"

#else

Adafruit_USBD_XboxOne::Adafruit_USBD_XboxOne(uint8_t interval_ms) : _interval_ms(interval_ms) {}
bool Adafruit_USBD_XboxOne::begin(void) { return false; }
bool Adafruit_USBD_XboxOne::ready(void) { return false; }
bool Adafruit_USBD_XboxOne::sendReport(uint8_t port) { (void)port; return false; }
uint16_t Adafruit_USBD_XboxOne::getInterfaceDescriptor(uint8_t itfnum, uint8_t* buf, uint16_t bufsize) {
  (void)itfnum; (void)buf; (void)bufsize; return 0;
}
const usbd_class_driver_t* xboxone_get_driver() { return nullptr; }
bool xboxone_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t* request) {
  (void)rhport; (void)stage; (void)request; return false;
}
bool xboxone_send_report_for_port(uint8_t port) { (void)port; return false; }
XboxOneOutputStatus xboxone_output_status() {
  XboxOneOutputStatus status = {};
  return status;
}

#endif
