#include "../../product_config.h"

#include "xbone_auth_passthrough.h"

#if defined(ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT) && defined(ENABLE_USB_AUTH_SIDECAR)

#include <Arduino.h>
#include <string.h>
#include <tusb.h>

#include "../../input/usb_host/usb_xinput_host.h"
#include "../xboxone/xgip_protocol.h"

namespace {

enum XboneAuthState : uint8_t {
  XBONE_AUTH_IDLE = 0,
  XBONE_AUTH_SEND_CONSOLE_TO_DONGLE = 1,
  XBONE_AUTH_WAIT_CONSOLE_TO_DONGLE = 2,
};

struct QueuedReport {
  uint8_t data[64];
  uint16_t len;
};

constexpr uint8_t kReportQueueSize = 8;
constexpr uint32_t kReportQueueIntervalMs = 15;
constexpr uint8_t kXboxOneHostType = 2;

constexpr uint8_t kPowerOn[] = {
  0x06, 0x62, 0x45, 0xb8, 0x77, 0x26, 0x2c, 0x55,
  0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f
};
constexpr uint8_t kPowerOnSingle[] = {0x00};
constexpr uint8_t kLedOn[] = {0x00, 0x01, 0x14};
constexpr uint8_t kRumbleOn[] = {0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xeb};

XboxOneAuthSidecarStatus status = {};
XboxOneAuthPacket consolePacket = {};
XboxOneAuthPacket donglePacket = {};
bool consolePacketValid = false;
bool donglePacketValid = false;
XGIPProtocol incoming;
XGIPProtocol outgoing;
QueuedReport reportQueue[kReportQueueSize] = {};
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint32_t lastQueueSendMs = 0;

bool queueEmpty() {
  return queueHead == queueTail;
}

bool queueFull() {
  return static_cast<uint8_t>((queueHead + 1) % kReportQueueSize) == queueTail;
}

bool queueHostReport(const uint8_t* report, uint16_t len) {
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
  queueHead = static_cast<uint8_t>((queueHead + 1) % kReportQueueSize);
  status.queue_count++;
  return true;
}

void clearQueue() {
  queueHead = queueTail = 0;
  memset(reportQueue, 0, sizeof(reportQueue));
}

void setPacket(XboxOneAuthPacket& packet, uint8_t command, uint8_t sequence, const uint8_t* data, uint16_t len) {
  packet.command = command;
  packet.sequence = sequence;
  packet.length = (len > sizeof(packet.data)) ? sizeof(packet.data) : len;
  if (packet.length != 0 && data != nullptr) {
    memcpy(packet.data, data, packet.length);
  }
}

void processReportQueue() {
  if (!status.mounted || queueEmpty()) {
    return;
  }

  const uint32_t now = millis();
  if (lastQueueSendMs != 0 && (now - lastQueueSendMs) < kReportQueueIntervalMs) {
    return;
  }

  QueuedReport& report = reportQueue[queueTail];
  if (usb_xinput_host_send_report(status.dev_addr, status.instance, report.data, report.len)) {
    queueTail = static_cast<uint8_t>((queueTail + 1) % kReportQueueSize);
    lastQueueSendMs = now;
  }
}

void queueSimpleXgip(uint8_t command, uint8_t sequence, uint8_t internal, const uint8_t* data = nullptr, uint16_t len = 0) {
  outgoing.reset();
  outgoing.setAttributes(command, sequence, internal, 0, 0);
  outgoing.setData(data, len);
  queueHostReport(outgoing.generatePacket(), outgoing.getPacketLength());
}

void markDongleReadyIfDescriptorComplete() {
  if (!incoming.endOfChunk() || status.dongle_ready) {
    return;
  }

  queueSimpleXgip(GIP_POWER_MODE_DEVICE_CONFIG, 2, 1, kPowerOn, sizeof(kPowerOn));
  queueSimpleXgip(GIP_POWER_MODE_DEVICE_CONFIG, 3, 1, kPowerOnSingle, sizeof(kPowerOnSingle));
  queueSimpleXgip(GIP_CMD_LED_ON, 1, 0, kLedOn, sizeof(kLedOn));
  queueSimpleXgip(GIP_CMD_RUMBLE, 1, 0, kRumbleOn, sizeof(kRumbleOn));
  status.dongle_ready = true;
}

void processConsolePacket() {
  if (status.state == XBONE_AUTH_SEND_CONSOLE_TO_DONGLE && consolePacketValid) {
    const bool isChunked = consolePacket.length > XGIP_MAX_CHUNK_SIZE;
    const bool needsAck = consolePacket.length > 2;
    outgoing.reset();
    outgoing.setAttributes(consolePacket.command, consolePacket.sequence, 1, isChunked, needsAck);
    outgoing.setData(consolePacket.data, consolePacket.length);
    consolePacketValid = false;
    status.state = XBONE_AUTH_WAIT_CONSOLE_TO_DONGLE;
  }

  if (status.state == XBONE_AUTH_WAIT_CONSOLE_TO_DONGLE) {
    if (!queueHostReport(outgoing.generatePacket(), outgoing.getPacketLength())) {
      return;
    }
    if (!outgoing.getChunked() || outgoing.endOfChunk()) {
      status.state = XBONE_AUTH_IDLE;
    }
  }
}

}  // namespace

void xbone_auth_passthrough_reset() {
  const uint32_t mountCount = status.mount_count;
  memset(&status, 0, sizeof(status));
  status.supported = true;
  status.mount_count = mountCount;
  memset(&consolePacket, 0, sizeof(consolePacket));
  memset(&donglePacket, 0, sizeof(donglePacket));
  consolePacketValid = false;
  donglePacketValid = false;
  incoming.reset();
  outgoing.reset();
  clearQueue();
  lastQueueSendMs = 0;
}

void xbone_auth_passthrough_task() {
  processConsolePacket();
  processReportQueue();
}

void xbone_auth_passthrough_mount(uint8_t dev_addr, uint8_t instance, uint8_t controller_type, uint8_t subtype) {
  (void)subtype;
  if (controller_type != kXboxOneHostType || status.mounted) {
    return;
  }
  xbone_auth_passthrough_reset();
  status.mounted = true;
  status.dev_addr = dev_addr;
  status.instance = instance;
  status.mount_count++;
  uint16_t vid = 0;
  uint16_t pid = 0;
  if (tuh_vid_pid_get(dev_addr, &vid, &pid)) {
    status.vid = vid;
    status.pid = pid;
  }
}

void xbone_auth_passthrough_umount(uint8_t dev_addr, uint8_t instance) {
  if (!status.mounted || status.dev_addr != dev_addr || status.instance != instance) {
    return;
  }
  xbone_auth_passthrough_reset();
}

void xbone_auth_passthrough_report_received(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len) {
  if (!status.mounted || status.dev_addr != dev_addr || status.instance != instance ||
      report == nullptr || len == 0) {
    return;
  }

  status.input_count++;
  status.last_length = len;
  incoming.parse(report, len);
  if (!incoming.validate()) {
    status.invalid_count++;
    incoming.reset();
    return;
  }

  status.last_command = incoming.getCommand();
  status.last_sequence = incoming.getSequence();

  if (incoming.ackRequired()) {
    queueHostReport(incoming.generateAckPacket(), incoming.getPacketLength());
  }

  switch (incoming.getCommand()) {
    case GIP_ANNOUNCE:
      queueSimpleXgip(GIP_DEVICE_DESCRIPTOR, 1, 1);
      break;
    case GIP_DEVICE_DESCRIPTOR:
      markDongleReadyIfDescriptorComplete();
      break;
    case GIP_AUTH:
    case GIP_FINAL_AUTH:
      if (!incoming.getChunked() || incoming.endOfChunk()) {
        setPacket(donglePacket,
                  incoming.getCommand(),
                  incoming.getSequence(),
                  incoming.getData(),
                  incoming.getDataLength());
        donglePacketValid = true;
        status.dongle_to_console_count++;
        incoming.reset();
      }
      break;
    default:
      break;
  }
}

void xbone_auth_passthrough_report_sent(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len) {
  (void)report;
  (void)len;
  if (!status.mounted || status.dev_addr != dev_addr || status.instance != instance) {
    return;
  }
  status.sent_count++;
}

bool xbone_auth_passthrough_ready() {
  return status.mounted && status.dongle_ready;
}

bool xbone_auth_passthrough_authenticated() {
  return status.auth_completed;
}

void xbone_auth_passthrough_mark_authenticated() {
  status.auth_completed = true;
}

bool xbone_auth_passthrough_submit_console_packet(uint8_t command, uint8_t sequence, const uint8_t* data, uint16_t len) {
  if (!status.mounted || len > sizeof(consolePacket.data)) {
    return false;
  }
  if (consolePacketValid || status.state != XBONE_AUTH_IDLE) {
    return false;
  }
  setPacket(consolePacket, command, sequence, data, len);
  consolePacketValid = true;
  status.console_to_dongle_count++;
  status.state = XBONE_AUTH_SEND_CONSOLE_TO_DONGLE;
  return true;
}

bool xbone_auth_passthrough_take_dongle_packet(XboxOneAuthPacket* packet) {
  if (packet == nullptr || !donglePacketValid) {
    return false;
  }
  *packet = donglePacket;
  donglePacketValid = false;
  return true;
}

XboxOneAuthSidecarStatus xbone_auth_passthrough_status() {
  return status;
}

#else

void xbone_auth_passthrough_reset() {}
void xbone_auth_passthrough_task() {}
void xbone_auth_passthrough_mount(uint8_t dev_addr, uint8_t instance, uint8_t controller_type, uint8_t subtype) {
  (void)dev_addr; (void)instance; (void)controller_type; (void)subtype;
}
void xbone_auth_passthrough_umount(uint8_t dev_addr, uint8_t instance) {
  (void)dev_addr; (void)instance;
}
void xbone_auth_passthrough_report_received(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len) {
  (void)dev_addr; (void)instance; (void)report; (void)len;
}
void xbone_auth_passthrough_report_sent(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len) {
  (void)dev_addr; (void)instance; (void)report; (void)len;
}
bool xbone_auth_passthrough_ready() { return false; }
bool xbone_auth_passthrough_authenticated() { return false; }
void xbone_auth_passthrough_mark_authenticated() {}
bool xbone_auth_passthrough_submit_console_packet(uint8_t command, uint8_t sequence, const uint8_t* data, uint16_t len) {
  (void)command; (void)sequence; (void)data; (void)len;
  return false;
}
bool xbone_auth_passthrough_take_dongle_packet(XboxOneAuthPacket* packet) {
  (void)packet;
  return false;
}
XboxOneAuthSidecarStatus xbone_auth_passthrough_status() {
  XboxOneAuthSidecarStatus status = {};
#if defined(ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT)
  status.supported = true;
#endif
  return status;
}

#endif
