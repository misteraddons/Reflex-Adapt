#include "../../product_config.h"

#include "ps_auth_dongle_runtime.h"

#if defined(ENABLE_USB_AUTH_SIDECAR)

#include <Arduino.h>
#include <string.h>
#include <tusb.h>

#include "../../platform/usb_host_bridge.h"
#include "../out_report.h"
#include "../output_capabilities.h"

namespace {

enum DongleAuthState : uint8_t {
  AUTH_IDLE = 0,
  AUTH_SENDING_RESET = 1,
  AUTH_SENDING_NONCE = 2,
  AUTH_WAITING_FOR_SIG = 3,
  AUTH_RECEIVING_SIG = 4,
  AUTH_P5G_SEND_F0 = 16,
  AUTH_P5G_SEND_F0_WAIT = 17,
  AUTH_P5G_RECV_F1 = 18,
  AUTH_P5G_RECV_F1_WAIT = 19,
  AUTH_P5G_RECV_F2_DELAY = 20,
  AUTH_P5G_RECV_F2 = 21,
  AUTH_P5G_RECV_F2_WAIT = 22,
};

enum DongleProtocol : uint8_t {
  AUTH_PROTOCOL_NONE = 0,
  AUTH_PROTOCOL_PS4 = 1,
  AUTH_PROTOCOL_P5GENERAL = 2,
};

enum DongleLastError : uint8_t {
  AUTH_ERR_NONE = 0,
  AUTH_ERR_NOT_READY = 1,
  AUTH_ERR_BAD_LENGTH = 2,
  AUTH_ERR_QUEUE_BUSY = 3,
};

constexpr uint8_t kReportF0PayloadSize = 63;
constexpr uint8_t kReportF1PayloadSize = 63;
constexpr uint8_t kReportF2PayloadSize = 15;
constexpr uint8_t kReportF3PayloadSize = 7;
constexpr uint8_t kPsAuthNoncePages = 5;
constexpr uint8_t kPsAuthSignaturePages = 19;
constexpr uint8_t kPsAuthPageBytes = 56;
constexpr uint8_t kP5GeneralReportSize = 64;
constexpr uint8_t kP5GeneralReportSequenceOffset = 12;
constexpr uint8_t kP5GeneralReportHashOffset = 56;

#ifndef PS5_AUTH_REPORT_REPEAT_COUNT
#define PS5_AUTH_REPORT_REPEAT_COUNT 4u
#endif
constexpr uint32_t kP5GeneralBusyTimeoutUs = 250000u;

// Donor source of truth:
// Adapt-Dev retains the RetroZord donor ps_auth.cpp snapshot used for this layout.
constexpr uint8_t kOutput03Ps4[] = {
  0x21, 0x27, 0x04, 0x4d, 0x00, 0x2c, 0x56,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x0D, 0x0D, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

constexpr uint8_t kOutput03Ps5[] = {
  0x21, 0x28, 0x03, 0xC3, 0x00, 0x2C, 0x56,
  0x01, 0x00, 0xD0, 0x07, 0x00, 0x80, 0x04, 0x00,
  0x00, 0x80, 0x0D, 0x0D, 0x84, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

constexpr uint8_t kOutputF3[] = {0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00};

uint8_t authDev = 0;
uint8_t authInstance = 0;
uint16_t authVid = 0;
uint16_t authPid = 0;
uint16_t lastVid = 0;
uint16_t lastPid = 0;
uint16_t lastDescLen = 0;
uint8_t lastReportCount = 0;
bool lastDescriptorCandidate = false;
uint8_t authProtocol = AUTH_PROTOCOL_NONE;
bool authConnected = false;
bool authBusy = false;
uint32_t authBusySinceUs = 0;
bool signatureReady = false;
bool usePs5Identity = false;
uint8_t authState = AUTH_IDLE;
uint8_t nonceId = 0;
uint8_t noncePart = 0;
uint8_t signaturePart = 0;
union AuthScratchBuffers {
  struct {
    uint8_t nonce[300];
    uint8_t signature[1140];
    uint8_t setBuffer[64];
  } ps4;
  struct {
    uint8_t hashPending[64];
    uint8_t hashInFlight[64];
    uint8_t hashFinish[64];
    uint8_t authBuffer[64];
    uint8_t lastReport[64];
  } ps5;
};

AuthScratchBuffers authScratch = {};
uint8_t p5RepeatCount = 0;
uint8_t p5F1Count = 0;
uint32_t p5AuthF2ReadyAtUs = 0;
bool p5HashPending = false;
bool p5HashInFlight = false;
bool p5HashReady = false;
uint32_t p5ReportSequence = 0;
uint32_t p5LastDongleQueueUs = 0;
uint32_t p5LastHostReportUs = 0;
uint32_t p5LastActiveRefreshUs = 0;
uint32_t p5LastDongleInputUs = 0;
uint8_t lastError = AUTH_ERR_NONE;
uint32_t getReportCount = 0;
uint32_t setReportCount = 0;
uint32_t sentReportCount = 0;
uint32_t inputReportCount = 0;
uint32_t p5PrepareCount = 0;
uint32_t p5HostSubmitCount = 0;
uint32_t p5QueueBusyCount = 0;
uint32_t p5LastHostDeltaUs = 0;
uint32_t p5LastDongleDeltaUs = 0;
uint32_t p5BusyTimeoutCount = 0;
uint8_t lastHostGetId = 0;
uint8_t lastHostSetId = 0;
uint16_t lastHostGetLen = 0;
uint16_t lastHostSetLen = 0;
uint8_t lastP5F0Command = 0;
uint8_t lastP5F0Detail = 0;
uint8_t lastDongleQueueId = 0;
uint8_t lastDongleQueueType = 0;
uint16_t lastDongleQueueLen = 0;
uint8_t lastDongleGetDoneId = 0;
uint16_t lastDongleGetDoneLen = 0;
uint8_t lastDongleSetDoneId = 0;
uint16_t lastDongleSentLen = 0;
uint16_t lastDongleInputLen = 0;
uint8_t lastP5RawButtons0 = 0;
uint8_t lastP5RawButtons1 = 0;
uint8_t lastP5RawButtons2 = 0;
uint8_t lastP5SignedButtons0 = 0;
uint8_t lastP5SignedButtons1 = 0;
uint8_t lastP5SignedButtons2 = 0;
uint8_t lastP5F2Status0 = 0;
uint8_t lastP5F2Status1 = 0;
uint8_t lastP5F2Status2 = 0;
uint32_t p5HashMismatchCount = 0;

enum AuthTraceCode : uint8_t {
  TRACE_DEV_SEEN = 1,
  TRACE_DEV_ACCEPT = 2,
  TRACE_DEV_IGNORE = 3,
  TRACE_DEV_DISC = 4,
  TRACE_GET_REQ = 5,
  TRACE_SET_REQ = 6,
  TRACE_QUEUE = 7,
  TRACE_GET_DONE = 8,
  TRACE_SET_DONE = 9,
  TRACE_SENT_DONE = 10,
  TRACE_INPUT = 11,
  TRACE_ERROR = 12,
  TRACE_STATE = 13,
};

#ifdef ENABLE_USB_AUTH_SERIAL_TRACE
struct AuthTraceEvent {
  uint32_t ms;
  uint8_t code;
  uint8_t a;
  uint8_t b;
  uint16_t c;
  uint16_t d;
};

constexpr uint8_t kAuthTraceRingSize = 64;
volatile uint8_t authTraceHead = 0;
volatile uint8_t authTraceTail = 0;
AuthTraceEvent authTraceRing[kAuthTraceRingSize] = {};

const __FlashStringHelper* traceCodeName(uint8_t code) {
  switch (code) {
    case TRACE_DEV_SEEN: return F("DEV");
    case TRACE_DEV_ACCEPT: return F("ACCEPT");
    case TRACE_DEV_IGNORE: return F("IGNORE");
    case TRACE_DEV_DISC: return F("DISC");
    case TRACE_GET_REQ: return F("GET_REQ");
    case TRACE_SET_REQ: return F("SET_REQ");
    case TRACE_QUEUE: return F("QUEUE");
    case TRACE_GET_DONE: return F("GET_DONE");
    case TRACE_SET_DONE: return F("SET_DONE");
    case TRACE_SENT_DONE: return F("SENT_DONE");
    case TRACE_INPUT: return F("INPUT");
    case TRACE_ERROR: return F("ERROR");
    case TRACE_STATE: return F("STATE");
    default: return F("EV");
  }
}

void traceEvent(uint8_t code,
                uint8_t a = 0,
                uint8_t b = 0,
                uint16_t c = 0,
                uint16_t d = 0) {
  const uint8_t next = (uint8_t)((authTraceHead + 1) % kAuthTraceRingSize);
  if (next == authTraceTail) {
    authTraceTail = (uint8_t)((authTraceTail + 1) % kAuthTraceRingSize);
  }
  authTraceRing[authTraceHead] = {millis(), code, a, b, c, d};
  authTraceHead = next;
}

void traceState(uint8_t state) {
  traceEvent(TRACE_STATE, state, authBusy ? 1 : 0,
             (uint16_t)((noncePart << 8) | signaturePart),
             (uint16_t)((authProtocol << 8) | lastError));
}
#else
void traceEvent(uint8_t, uint8_t = 0, uint8_t = 0, uint16_t = 0, uint16_t = 0) {}
void traceState(uint8_t) {}
#endif

void setAuthBusy(bool value) {
  authBusy = value;
  authBusySinceUs = value ? micros() : 0;
}

void recoverStalePs5BusyIfNeeded() {
  if (!usePs5Identity || authProtocol != AUTH_PROTOCOL_P5GENERAL ||
      !authBusy || authBusySinceUs == 0) {
    return;
  }
  if ((uint32_t)(micros() - authBusySinceUs) <= kP5GeneralBusyTimeoutUs) {
    return;
  }
  setAuthBusy(false);
  p5HashInFlight = false;
  p5BusyTimeoutCount++;
  do_reset_out_report_queue();
}

uint8_t ps5RepeatCountDefault() {
  return (PS5_AUTH_REPORT_REPEAT_COUNT > 255u)
    ? 255u
    : (uint8_t)PS5_AUTH_REPORT_REPEAT_COUNT;
}

void queuePs5HashReport(const uint8_t* report) {
  memcpy(authScratch.ps5.hashPending, report, kP5GeneralReportSize);
  memset(authScratch.ps5.hashPending + kP5GeneralReportHashOffset, 0,
         kP5GeneralReportSize - kP5GeneralReportHashOffset);
  const uint32_t sequence = ++p5ReportSequence;
  memcpy(authScratch.ps5.hashPending + kP5GeneralReportSequenceOffset,
         &sequence,
         sizeof(sequence));
  p5HashPending = true;
}

void storePs5FeatureResponse(uint8_t reportId,
                             const uint8_t* report,
                             uint16_t len,
                             uint8_t payloadLen) {
  memset(authScratch.ps5.authBuffer, 0, sizeof(authScratch.ps5.authBuffer));
  authScratch.ps5.authBuffer[0] = reportId;
  if (report == nullptr || len == 0) {
    return;
  }
  if (report[0] == reportId) {
    memcpy(authScratch.ps5.authBuffer, report,
           min((uint16_t)sizeof(authScratch.ps5.authBuffer), len));
    return;
  }
  memcpy(authScratch.ps5.authBuffer + 1,
         report,
         min((uint16_t)payloadLen, len));
}

bool ps5SignedReportMatchesInFlight(const uint8_t* report) {
  if (!p5HashInFlight || report == nullptr) {
    return false;
  }
  return memcmp(report, authScratch.ps5.hashInFlight,
                kP5GeneralReportSequenceOffset) == 0 &&
         memcmp(report + kP5GeneralReportSequenceOffset + sizeof(uint32_t),
                authScratch.ps5.hashInFlight + kP5GeneralReportSequenceOffset + sizeof(uint32_t),
                kP5GeneralReportHashOffset - kP5GeneralReportSequenceOffset - sizeof(uint32_t)) == 0;
}

void resetAuthTransport() {
  setAuthBusy(false);
  authState = AUTH_IDLE;
  noncePart = 0;
  signaturePart = 0;
  signatureReady = false;
  p5HashPending = false;
  p5HashInFlight = false;
  p5HashReady = false;
  p5RepeatCount = 0;
  p5F1Count = 0;
  p5ReportSequence = 0;
  p5AuthF2ReadyAtUs = 0;
  p5HashMismatchCount = 0;
  p5LastDongleQueueUs = 0;
  p5LastHostReportUs = 0;
  p5LastActiveRefreshUs = 0;
  p5LastDongleInputUs = 0;
  p5PrepareCount = 0;
  p5HostSubmitCount = 0;
  p5QueueBusyCount = 0;
  p5LastHostDeltaUs = 0;
  p5LastDongleDeltaUs = 0;
  p5BusyTimeoutCount = 0;
  lastError = AUTH_ERR_NONE;
  memset(&authScratch, 0, sizeof(authScratch));
  do_reset_out_report_queue();
}

bool descriptorLooksLikeAuthDevice(const uint8_t* reportDesc, uint16_t descLen) {
  lastDescLen = descLen;
  lastReportCount = 0;
  lastDescriptorCandidate = false;
  if (reportDesc == nullptr || descLen == 0) {
    return false;
  }

  tuh_hid_report_info_t reportInfo[4];
  const uint8_t reportCount =
    tuh_hid_parse_report_descriptor(reportInfo, 4, reportDesc, descLen);
  lastReportCount = reportCount;
  for (uint8_t i = 0; i < reportCount; ++i) {
    if (reportInfo[i].usage_page == 0xFFF0 &&
        reportInfo[i].report_id == 0xF3) {
      lastDescriptorCandidate = true;
      return true;
    }
  }
  return false;
}

bool vidPidLooksLikeAuthDevice(uint16_t vid, uint16_t pid) {
  if (vid == 0x0079 && pid == 0x1893) {
    return true;  // MagicBoots for PS4
  }
  if (vid == 0x054C && pid == 0x09CC) {
    return true;  // DualShock 4 v2
  }
  if (vid == 0x0F0D && pid == 0x0084) {
    return true;  // Hori Fighting Commander PS4
  }
  return false;
}

bool vidPidLooksLikeP5General(uint16_t vid, uint16_t pid) {
  return vid == 0x2B81 && pid == 0x0101;
}

bool authSidecarShouldClaimDevices() {
  switch (canonicalizeOutputMode(outputMode)) {
    case OUTPUT_PS4:
    case OUTPUT_PS5:
      return true;
    default:
      return false;
  }
}

bool protocolProvidesMode(uint8_t protocol, outputMode_t mode) {
  mode = canonicalizeOutputMode(mode);
  if (mode == OUTPUT_PS5) {
    return protocol == AUTH_PROTOCOL_P5GENERAL;
  }
  if (mode == OUTPUT_PS4) {
    return protocol == AUTH_PROTOCOL_PS4;
  }
  return protocol != AUTH_PROTOCOL_NONE;
}

uint16_t copyReport(uint8_t* buffer,
                    uint16_t reqlen,
                    const uint8_t* source,
                    uint16_t sourceLen) {
  const uint16_t copyLen = (reqlen < sourceLen) ? reqlen : sourceLen;
  memcpy(buffer, source, copyLen);
  if (reqlen > copyLen) {
    memset(buffer + copyLen, 0, reqlen - copyLen);
  }
  return reqlen;
}

}  // namespace

void ps_auth_dongle_set_output_mode(outputMode_t mode) {
  usePs5Identity = (mode == OUTPUT_PS5);
}

void ps_auth_dongle_device_connected(uint8_t dev_addr,
                                     uint8_t instance,
                                     uint16_t vid,
                                     uint16_t pid,
                                     const uint8_t* report_desc,
                                     uint16_t desc_len) {
  traceEvent(TRACE_DEV_SEEN, dev_addr, instance, vid, pid);
  lastVid = vid;
  lastPid = pid;
  if (!authSidecarShouldClaimDevices()) {
    traceEvent(TRACE_DEV_IGNORE, dev_addr, instance, vid, pid);
    return;
  }
  const bool descriptorCandidate =
    descriptorLooksLikeAuthDevice(report_desc, desc_len);
  const bool vidPidCandidate = vidPidLooksLikeAuthDevice(vid, pid);
  const bool p5GeneralCandidate = vidPidLooksLikeP5General(vid, pid);

  if (authConnected) {
    traceEvent(TRACE_DEV_IGNORE, dev_addr, instance, vid, pid);
    return;
  }

  if (!descriptorCandidate && !vidPidCandidate && !p5GeneralCandidate) {
    traceEvent(TRACE_DEV_IGNORE, dev_addr, instance, desc_len, lastReportCount);
    return;
  }

  authDev = dev_addr;
  authInstance = instance;
  authVid = vid;
  authPid = pid;
  authProtocol = p5GeneralCandidate ? AUTH_PROTOCOL_P5GENERAL : AUTH_PROTOCOL_PS4;
  authConnected = true;
  traceEvent(TRACE_DEV_ACCEPT, dev_addr, instance, vid, pid);
  resetAuthTransport();
  if (authProtocol == AUTH_PROTOCOL_P5GENERAL) {
    tuh_hid_receive_report(dev_addr, instance);
  }
}

void ps_auth_dongle_device_disconnected(uint8_t dev_addr, uint8_t instance) {
  traceEvent(TRACE_DEV_DISC, dev_addr, instance, authVid, authPid);
  if (dev_addr != authDev || instance != authInstance) {
    return;
  }

  authDev = 0;
  authInstance = 0;
  authVid = 0;
  authPid = 0;
  authProtocol = AUTH_PROTOCOL_NONE;
  authConnected = false;
  resetAuthTransport();
}

bool ps_auth_dongle_is_auth_device(uint16_t vid, uint16_t pid) {
  if (!authSidecarShouldClaimDevices()) {
    return false;
  }
  return vidPidLooksLikeAuthDevice(vid, pid) || vidPidLooksLikeP5General(vid, pid);
}

bool ps_auth_dongle_is_auth_instance(uint8_t dev_addr, uint8_t instance) {
  return authConnected && authDev == dev_addr && authInstance == instance;
}

bool ps_auth_dongle_has_provider_for_mode(outputMode_t mode) {
  if (!authConnected) {
    return false;
  }
  return protocolProvidesMode(authProtocol, mode);
}

void ps_auth_dongle_task() {
  recoverStalePs5BusyIfNeeded();
  if (authBusy || !authConnected) {
    return;
  }

  if (usePs5Identity && authProtocol == AUTH_PROTOCOL_P5GENERAL) {
    if (p5HashPending && tuh_hid_send_ready(authDev, authInstance)) {
      lastDongleQueueId = 0;
      lastDongleQueueType = HID_REPORT_TYPE_OUTPUT;
      lastDongleQueueLen = kP5GeneralReportSize;
      traceEvent(TRACE_QUEUE, 0, HID_REPORT_TYPE_OUTPUT,
                 kP5GeneralReportSize, authState);
      memcpy(authScratch.ps5.hashInFlight, authScratch.ps5.hashPending,
             kP5GeneralReportSize);
      queue_set_output_report(authDev, authInstance, true, 0,
                              authScratch.ps5.hashPending, kP5GeneralReportSize);
      setAuthBusy(true);
      p5HashPending = false;
      p5HashInFlight = true;
      p5LastDongleQueueUs = micros();
    }

    switch ((DongleAuthState)authState) {
      case AUTH_P5G_SEND_F0:
        lastDongleQueueId = 0xF0;
        lastDongleQueueType = HID_REPORT_TYPE_FEATURE;
        lastDongleQueueLen = kReportF0PayloadSize;
        traceEvent(TRACE_QUEUE, 0xF0, HID_REPORT_TYPE_FEATURE,
                   kReportF0PayloadSize, authState);
        queue_set_feature_report(authDev, authInstance, false, 0xF0,
                                 authScratch.ps5.authBuffer + 1, kReportF0PayloadSize);
        setAuthBusy(true);
        authState = AUTH_P5G_SEND_F0_WAIT;
        traceState(authState);
        break;
      case AUTH_P5G_RECV_F1:
        if (p5F1Count != 0) {
          lastDongleQueueId = 0xF1;
          lastDongleQueueType = HID_REPORT_TYPE_FEATURE;
          lastDongleQueueLen = kReportF1PayloadSize + 1;
          traceEvent(TRACE_QUEUE, 0xF1, HID_REPORT_TYPE_FEATURE,
                     kReportF1PayloadSize + 1, authState);
          queue_get_feature_report(authDev, authInstance, false, 0xF1,
                                   kReportF1PayloadSize + 1);
          setAuthBusy(true);
          authState = AUTH_P5G_RECV_F1_WAIT;
          p5F1Count--;
          traceState(authState);
        } else {
          authState = AUTH_IDLE;
          traceState(authState);
        }
        break;
      case AUTH_P5G_RECV_F2_DELAY:
        if ((int32_t)(micros() - p5AuthF2ReadyAtUs) >= 0) {
          authState = AUTH_P5G_RECV_F2;
          traceState(authState);
        } else {
          // Keep the signed input-report pipeline moving during the donor
          // 500 ms F2 wait so gameplay can still hit the 1 ms poll cadence.
          break;
        }
        [[fallthrough]];
      case AUTH_P5G_RECV_F2:
        lastDongleQueueId = 0xF2;
        lastDongleQueueType = HID_REPORT_TYPE_FEATURE;
        lastDongleQueueLen = kReportF2PayloadSize + 1;
        traceEvent(TRACE_QUEUE, 0xF2, HID_REPORT_TYPE_FEATURE,
                   kReportF2PayloadSize + 1, authState);
        queue_get_feature_report(authDev, authInstance, false, 0xF2,
                                 kReportF2PayloadSize + 1);
        setAuthBusy(true);
        authState = AUTH_P5G_RECV_F2_WAIT;
        traceState(authState);
        break;
      default:
        break;
    }
    return;
  }

  switch (authState) {
    case AUTH_IDLE:
      break;
    case AUTH_SENDING_RESET:
      lastDongleQueueId = 0xF3;
      lastDongleQueueType = HID_REPORT_TYPE_FEATURE;
      lastDongleQueueLen = kReportF3PayloadSize + 1;
      traceEvent(TRACE_QUEUE, 0xF3, HID_REPORT_TYPE_FEATURE,
                 kReportF3PayloadSize + 1, authState);
      queue_get_feature_report(authDev, authInstance, false, 0xF3,
                               kReportF3PayloadSize + 1);
      setAuthBusy(true);
      break;
    case AUTH_SENDING_NONCE:
      authScratch.ps4.setBuffer[0] = nonceId;
      authScratch.ps4.setBuffer[1] = noncePart;
      authScratch.ps4.setBuffer[2] = 0;
      memcpy(authScratch.ps4.setBuffer + 3,
             authScratch.ps4.nonce + (noncePart * 60),
             60);
      lastDongleQueueId = 0xF0;
      lastDongleQueueType = HID_REPORT_TYPE_FEATURE;
      lastDongleQueueLen = kReportF0PayloadSize;
      traceEvent(TRACE_QUEUE, 0xF0, HID_REPORT_TYPE_FEATURE,
                 kReportF0PayloadSize, authState);
      queue_set_feature_report(authDev, authInstance, false, 0xF0,
                               authScratch.ps4.setBuffer, kReportF0PayloadSize);
      setAuthBusy(true);
      noncePart++;
      break;
    case AUTH_WAITING_FOR_SIG:
      lastDongleQueueId = 0xF2;
      lastDongleQueueType = HID_REPORT_TYPE_FEATURE;
      lastDongleQueueLen = kReportF2PayloadSize + 1;
      traceEvent(TRACE_QUEUE, 0xF2, HID_REPORT_TYPE_FEATURE,
                 kReportF2PayloadSize + 1, authState);
      queue_get_feature_report(authDev, authInstance, false, 0xF2,
                               kReportF2PayloadSize + 1);
      setAuthBusy(true);
      break;
    case AUTH_RECEIVING_SIG:
      lastDongleQueueId = 0xF1;
      lastDongleQueueType = HID_REPORT_TYPE_FEATURE;
      lastDongleQueueLen = kReportF1PayloadSize + 1;
      traceEvent(TRACE_QUEUE, 0xF1, HID_REPORT_TYPE_FEATURE,
                 kReportF1PayloadSize + 1, authState);
      queue_get_feature_report(authDev, authInstance, false, 0xF1,
                               kReportF1PayloadSize + 1);
      setAuthBusy(true);
      break;
  }
}

uint16_t ps_auth_dongle_handle_get_report(uint8_t report_id,
                                          uint8_t* buffer,
                                          uint16_t reqlen) {
  if (buffer == nullptr || reqlen == 0) {
    return 0;
  }

  getReportCount++;
  lastHostGetId = report_id;
  lastHostGetLen = reqlen;
  traceEvent(TRACE_GET_REQ, report_id, usePs5Identity ? 1 : 0,
             reqlen, authState);

  if (usePs5Identity) {
    switch (report_id) {
      case 0x03:
        // Donor P5General answers the definition report without resetting the
        // hash pipeline. Resetting here can discard the first hashed input
        // report after the dongle prepared it, leaving the PS5 at G1/S0.
        return copyReport(buffer, reqlen, kOutput03Ps5, sizeof(kOutput03Ps5));
      case 0xF1:
        if (reqlen < kReportF1PayloadSize) {
          lastError = AUTH_ERR_BAD_LENGTH;
          return 0;
        }
        memcpy(buffer, authScratch.ps5.authBuffer + 1, kReportF1PayloadSize);
        if (authState == AUTH_IDLE) {
          authState = AUTH_P5G_RECV_F1;
          traceState(authState);
        }
        return kReportF1PayloadSize;
      case 0xF2:
        if (reqlen < kReportF2PayloadSize) {
          lastError = AUTH_ERR_BAD_LENGTH;
          return 0;
        }
        memcpy(buffer, authScratch.ps5.authBuffer + 1, kReportF2PayloadSize);
        if (authState == AUTH_IDLE) {
          authState = AUTH_P5G_RECV_F1;
          traceState(authState);
        }
        return kReportF2PayloadSize;
      default:
        return 0;
    }
  }

  switch (report_id) {
    case 0x03:
      resetAuthTransport();
      return copyReport(buffer, reqlen,
                        usePs5Identity ? kOutput03Ps5 : kOutput03Ps4,
                        sizeof(kOutput03Ps4));
    case 0xF3:
      signatureReady = false;
      signaturePart = 0;
      return copyReport(buffer, reqlen, kOutputF3, sizeof(kOutputF3));
    case 0xF1:
      if (reqlen < kReportF1PayloadSize) {
        return 0;
      }
      memset(buffer, 0, reqlen);
      buffer[0] = nonceId;
      buffer[1] = signaturePart;
      buffer[2] = 0;
      memcpy(buffer + 3,
             authScratch.ps4.signature + (signaturePart * kPsAuthPageBytes),
             kPsAuthPageBytes);
      signaturePart++;
      if (signaturePart >= kPsAuthSignaturePages) {
        signaturePart = 0;
      }
      return reqlen;
    case 0xF2:
      if (reqlen < kReportF2PayloadSize) {
        return 0;
      }
      memset(buffer, 0, reqlen);
      buffer[0] = nonceId;
      buffer[1] = signatureReady ? 0 : 16;
      return reqlen;
    default:
      break;
  }

  return 0;
}

bool ps_auth_dongle_handle_set_report(uint8_t report_id,
                                      const uint8_t* buffer,
                                      uint16_t reqlen) {
  lastHostSetId = report_id;
  lastHostSetLen = reqlen;
  if (report_id != 0xF0) {
    traceEvent(TRACE_SET_REQ, report_id, usePs5Identity ? 1 : 0,
               reqlen, authState);
    return false;
  }
  if (buffer == nullptr || reqlen < kReportF0PayloadSize) {
    traceEvent(TRACE_ERROR, AUTH_ERR_BAD_LENGTH, report_id, reqlen, authState);
    return true;
  }

  setReportCount++;
  traceEvent(TRACE_SET_REQ, report_id, usePs5Identity ? 1 : 0,
             reqlen, authState);

  if (usePs5Identity) {
    if (buffer == nullptr || reqlen != kReportF0PayloadSize) {
      lastError = AUTH_ERR_BAD_LENGTH;
      return true;
    }
    lastP5F0Command = buffer[0];
    lastP5F0Detail = (reqlen > 2) ? buffer[2] : 0;
    if (authState == AUTH_IDLE) {
      authScratch.ps5.authBuffer[0] = report_id;
      memcpy(authScratch.ps5.authBuffer + 1, buffer, kReportF0PayloadSize);
      authState = AUTH_P5G_SEND_F0;
      traceState(authState);
    }
    return true;
  }

  nonceId = buffer[0];
  const uint8_t part = buffer[1];
  if (part >= kPsAuthNoncePages) {
    return true;
  }

  memcpy(authScratch.ps4.nonce + (part * 60), buffer + 3, 60);
  if (part == kPsAuthNoncePages - 1) {
    authState = AUTH_SENDING_RESET;
    noncePart = 0;
    signatureReady = false;
    traceState(authState);
  }
  return true;
}

void ps_auth_dongle_handle_get_report_response(uint8_t dev_addr,
                                               uint8_t instance,
                                               uint8_t report_id,
                                               const uint8_t* report,
                                               uint16_t len) {
  if (dev_addr != authDev || instance != authInstance || report == nullptr) {
    return;
  }

  setAuthBusy(false);
  lastDongleGetDoneId = report_id;
  lastDongleGetDoneLen = len;
  traceEvent(TRACE_GET_DONE, report_id, usePs5Identity ? 1 : 0,
             len, authState);
  if (usePs5Identity && authProtocol == AUTH_PROTOCOL_P5GENERAL) {
    switch (report_id) {
      case 0xF2:
        if (authState == AUTH_P5G_RECV_F2_WAIT && len > 0) {
          storePs5FeatureResponse(report_id, report, len, kReportF2PayloadSize);
          const uint8_t* payload = (report[0] == report_id && len > 1)
            ? report + 1
            : report;
          const uint16_t payloadLen = (report[0] == report_id && len > 1)
            ? (uint16_t)(len - 1)
            : len;
          lastP5F2Status0 = (payloadLen > 0) ? payload[0] : 0;
          lastP5F2Status1 = (payloadLen > 1) ? payload[1] : 0;
          lastP5F2Status2 = (payloadLen > 2) ? payload[2] : 0;
          authState = AUTH_IDLE;
          traceState(authState);
        }
        break;
      case 0xF1:
        if (authState == AUTH_P5G_RECV_F1_WAIT && len > 0) {
          storePs5FeatureResponse(report_id, report, len, kReportF1PayloadSize);
          authState = AUTH_IDLE;
          traceState(authState);
        }
        break;
      default:
        break;
    }
    return;
  }

  switch (report_id) {
    case 0xF3:
      authState = AUTH_SENDING_NONCE;
      traceState(authState);
      break;
    case 0xF2:
      if (len >= 3 && report[2] == 0) {
        signaturePart = 0;
        authState = AUTH_RECEIVING_SIG;
        traceState(authState);
      }
      break;
    case 0xF1:
      if (len >= 60 && signaturePart < kPsAuthSignaturePages) {
        memcpy(authScratch.ps4.signature + (signaturePart * kPsAuthPageBytes),
               report + 4,
               kPsAuthPageBytes);
        signaturePart++;
        if (signaturePart >= kPsAuthSignaturePages) {
          authState = AUTH_IDLE;
          signatureReady = true;
          signaturePart = 0;
          traceState(authState);
        }
      }
      break;
    default:
      break;
  }
}

void ps_auth_dongle_handle_set_report_complete(uint8_t dev_addr,
                                               uint8_t instance,
                                               uint8_t report_id) {
  if (dev_addr != authDev || instance != authInstance || report_id != 0xF0) {
    return;
  }

  setAuthBusy(false);
  lastDongleSetDoneId = report_id;
  traceEvent(TRACE_SET_DONE, report_id, usePs5Identity ? 1 : 0,
             0, authState);
  if (usePs5Identity && authProtocol == AUTH_PROTOCOL_P5GENERAL) {
    if (report_id == 0xF0 && authState == AUTH_P5G_SEND_F0_WAIT) {
      switch (authScratch.ps5.authBuffer[1]) {
        case 0x01:
          p5F1Count = 4;
          break;
        case 0x03:
          p5F1Count = 1;
          break;
        case 0x02:
        default:
          p5F1Count = 0;
          break;
      }
      if (((authScratch.ps5.authBuffer[1] == 0x01) &&
           (authScratch.ps5.authBuffer[3] == 3)) ||
          (authScratch.ps5.authBuffer[1] == 0x02) ||
          (authScratch.ps5.authBuffer[1] == 0x03)) {
        p5AuthF2ReadyAtUs = micros() + 500000u;
        authState = AUTH_P5G_RECV_F2_DELAY;
        traceState(authState);
      } else {
        authState = AUTH_IDLE;
        traceState(authState);
      }
    }
    return;
  }

  if (noncePart >= kPsAuthNoncePages) {
    authState = AUTH_WAITING_FOR_SIG;
    traceState(authState);
  }
}

void ps_auth_dongle_handle_sent_report_response(uint8_t dev_addr,
                                                uint8_t instance,
                                                const uint8_t* report,
                                                uint16_t len) {
  (void)report;
  (void)len;
  if (dev_addr != authDev || instance != authInstance) {
    return;
  }
  setAuthBusy(false);
  sentReportCount++;
  lastDongleSentLen = len;
  traceEvent(TRACE_SENT_DONE, 0, usePs5Identity ? 1 : 0, len, authState);
}

void ps_auth_dongle_handle_input_report_received(uint8_t dev_addr,
                                                 uint8_t instance,
                                                 const uint8_t* report,
                                                 uint16_t len) {
  if (dev_addr != authDev || instance != authInstance ||
      authProtocol != AUTH_PROTOCOL_P5GENERAL || report == nullptr) {
    return;
  }
  inputReportCount++;
  const uint32_t nowUs = micros();
  if (p5LastDongleInputUs != 0) {
    p5LastDongleDeltaUs = nowUs - p5LastDongleInputUs;
  }
  p5LastDongleInputUs = nowUs;
  lastDongleInputLen = len;
  traceEvent(TRACE_INPUT, 0, usePs5Identity ? 1 : 0, len, authState);
  if (!p5HashReady && len >= kP5GeneralReportSize) {
    memcpy(authScratch.ps5.hashFinish, report, kP5GeneralReportSize);
    p5HashReady = true;
    if (!ps5SignedReportMatchesInFlight(report)) {
      p5HashMismatchCount++;
    }
    p5HashInFlight = false;
    lastError = AUTH_ERR_NONE;
  }
  tuh_hid_receive_report(dev_addr, instance);
}

bool ps_auth_dongle_prepare_ps5_output_report(const uint8_t* report,
                                              uint16_t len,
                                              uint8_t* out,
                                              uint16_t* out_len,
                                              uint16_t out_capacity) {
  p5PrepareCount++;
  if (!usePs5Identity || authProtocol != AUTH_PROTOCOL_P5GENERAL ||
      !authConnected) {
    lastError = AUTH_ERR_NOT_READY;
    traceEvent(TRACE_ERROR, lastError, 0, len, authState);
    return false;
  }
  if (report == nullptr || out == nullptr || out_len == nullptr ||
      len != kP5GeneralReportSize || out_capacity < kP5GeneralReportSize) {
    lastError = AUTH_ERR_BAD_LENGTH;
    traceEvent(TRACE_ERROR, lastError, 0, len, authState);
    return false;
  }

  lastP5RawButtons0 = report[8];
  lastP5RawButtons1 = report[9];
  lastP5RawButtons2 = report[10];

  if (p5HashReady) {
    memcpy(out, authScratch.ps5.hashFinish, kP5GeneralReportSize);
    *out_len = kP5GeneralReportSize;
    lastP5SignedButtons0 = authScratch.ps5.hashFinish[8];
    lastP5SignedButtons1 = authScratch.ps5.hashFinish[9];
    lastP5SignedButtons2 = authScratch.ps5.hashFinish[10];
    lastError = AUTH_ERR_NONE;
    return true;
  }

  if (p5HashPending) {
    lastError = AUTH_ERR_QUEUE_BUSY;
    p5QueueBusyCount++;
    return false;
  }

  const bool changed =
    (memcmp(authScratch.ps5.lastReport, report, kP5GeneralReportSize) != 0);

  if (changed) {
    memcpy(authScratch.ps5.lastReport, report, kP5GeneralReportSize);
    queuePs5HashReport(report);
    p5RepeatCount = ps5RepeatCountDefault();
  } else if (p5RepeatCount != 0) {
    p5RepeatCount--;
    queuePs5HashReport(report);
  }

  lastError = p5HashPending ? AUTH_ERR_QUEUE_BUSY : AUTH_ERR_NONE;
  return false;
}

void ps_auth_dongle_complete_ps5_output_report() {
  if (usePs5Identity && authProtocol == AUTH_PROTOCOL_P5GENERAL) {
    p5HashReady = false;
    const uint32_t nowUs = micros();
    if (p5LastHostReportUs != 0) {
      p5LastHostDeltaUs = nowUs - p5LastHostReportUs;
    }
    p5LastHostReportUs = nowUs;
    p5HostSubmitCount++;
    lastError = AUTH_ERR_NONE;
  }
}

bool ps_auth_dongle_has_provider() {
  return authConnected && protocolProvidesMode(authProtocol, usePs5Identity ? OUTPUT_PS5 : OUTPUT_PS4);
}

PsAuthDongleStatus ps_auth_dongle_status() {
  const bool ready = (authProtocol == AUTH_PROTOCOL_P5GENERAL)
    ? authConnected
    : signatureReady;
  return {
    true,
    authConnected,
    ready,
    authBusy,
    authDev,
    authInstance,
    authVid,
    authPid,
    lastVid,
    lastPid,
    lastDescLen,
    lastReportCount,
    lastDescriptorCandidate,
    authProtocol,
    lastError,
    authState,
    noncePart,
    signaturePart,
    getReportCount,
    setReportCount,
    sentReportCount,
    inputReportCount,
    lastHostGetId,
    lastHostSetId,
    lastHostGetLen,
    lastHostSetLen,
    lastP5F0Command,
    lastP5F0Detail,
    lastDongleQueueId,
    lastDongleQueueType,
    lastDongleQueueLen,
    lastDongleGetDoneId,
    lastDongleGetDoneLen,
    lastDongleSetDoneId,
    lastDongleSentLen,
    lastDongleInputLen,
    lastP5RawButtons0,
    lastP5RawButtons1,
    lastP5RawButtons2,
    lastP5SignedButtons0,
    lastP5SignedButtons1,
    lastP5SignedButtons2,
    lastP5F2Status0,
    lastP5F2Status1,
    lastP5F2Status2,
    (uint8_t)((authConnected ? 0x01 : 0x00) |
              (p5HashPending ? 0x02 : 0x00) |
              (p5HashReady ? 0x04 : 0x00) |
              (authBusy ? 0x08 : 0x00)),
    p5HashMismatchCount,
    p5PrepareCount,
    p5HostSubmitCount,
    p5QueueBusyCount,
    p5LastHostDeltaUs,
    p5LastDongleDeltaUs,
  };
}

#ifdef ENABLE_USB_AUTH_SERIAL_TRACE
void ps_auth_dongle_trace_flush(Print& out) {
  while (authTraceTail != authTraceHead) {
    const AuthTraceEvent ev = authTraceRing[authTraceTail];
    authTraceTail = (uint8_t)((authTraceTail + 1) % kAuthTraceRingSize);

    out.print(F("AUTHTRACE T="));
    out.print(ev.ms);
    out.print(F(" EV="));
    out.print(traceCodeName(ev.code));
    out.print(F(" A="));
    out.print((int)ev.a);
    out.print(F(" B="));
    out.print((int)ev.b);
    out.print(F(" C=0x"));
    out.print(ev.c, HEX);
    out.print(F(" D=0x"));
    out.println(ev.d, HEX);
  }
}

void ps_auth_dongle_debug_ps5_simulate(Print& out) {
  ps_auth_dongle_set_output_mode(OUTPUT_PS5);

  uint8_t featureBuffer[64] = {};
  const uint16_t definitionLen =
    ps_auth_dongle_handle_get_report(0x03, featureBuffer, sizeof(kOutput03Ps5));

  uint8_t f0Payload[kReportF0PayloadSize] = {};
  f0Payload[0] = 0x03;
  const bool acceptedF0 =
    ps_auth_dongle_handle_set_report(0xF0, f0Payload, sizeof(f0Payload));

  static uint32_t simulatedSequence = 0;
  uint8_t sampleReport[kP5GeneralReportSize] = {};
  sampleReport[0] = 0x01;
  sampleReport[1] = 0x80;
  sampleReport[2] = 0x80;
  sampleReport[3] = 0x80;
  sampleReport[4] = 0x80;
  sampleReport[8] = 0x08;
  const uint32_t sequence = ++simulatedSequence;
  memcpy(sampleReport + 12, &sequence, sizeof(sequence));
  sampleReport[30] = 0x1a;
  sampleReport[31] = 0x00;
  sampleReport[32] = 0x80;
  sampleReport[36] = 0x80;

  uint8_t hashedReport[kP5GeneralReportSize] = {};
  uint16_t hashedLen = 0;
  const bool hashReady = ps_auth_dongle_prepare_ps5_output_report(
    sampleReport,
    sizeof(sampleReport),
    hashedReport,
    &hashedLen,
    sizeof(hashedReport));

  out.print(F("OK:AUTH_P5SIM DEF="));
  out.print(definitionLen);
  out.print(F(" F0="));
  out.print(acceptedF0 ? 1 : 0);
  out.print(F(" HASH_READY="));
  out.print(hashReady ? 1 : 0);
  out.print(F(" HASH_LEN="));
  out.print(hashedLen);
  out.print(F(" ST="));
  out.print((int)authState);
  out.print(F(" BUSY="));
  out.print(authBusy ? 1 : 0);
  out.print(F(" ERR="));
  out.println((int)lastError);
}
#endif

#else

void ps_auth_dongle_set_output_mode(outputMode_t mode) { (void)mode; }
void ps_auth_dongle_device_connected(uint8_t dev_addr,
                                     uint8_t instance,
                                     uint16_t vid,
                                     uint16_t pid,
                                     const uint8_t* report_desc,
                                     uint16_t desc_len) {
  (void)dev_addr; (void)instance; (void)vid; (void)pid;
  (void)report_desc; (void)desc_len;
}
void ps_auth_dongle_device_disconnected(uint8_t dev_addr, uint8_t instance) {
  (void)dev_addr; (void)instance;
}
bool ps_auth_dongle_is_auth_device(uint16_t vid, uint16_t pid) {
  (void)vid; (void)pid;
  return false;
}
bool ps_auth_dongle_is_auth_instance(uint8_t dev_addr, uint8_t instance) {
  (void)dev_addr; (void)instance;
  return false;
}
bool ps_auth_dongle_has_provider_for_mode(outputMode_t mode) {
  (void)mode;
  return false;
}
void ps_auth_dongle_task() {}
uint16_t ps_auth_dongle_handle_get_report(uint8_t report_id,
                                          uint8_t* buffer,
                                          uint16_t reqlen) {
  (void)report_id; (void)buffer; (void)reqlen;
  return 0;
}
bool ps_auth_dongle_handle_set_report(uint8_t report_id,
                                      const uint8_t* buffer,
                                      uint16_t reqlen) {
  (void)report_id; (void)buffer; (void)reqlen;
  return false;
}
void ps_auth_dongle_handle_get_report_response(uint8_t dev_addr,
                                               uint8_t instance,
                                               uint8_t report_id,
                                               const uint8_t* report,
                                               uint16_t len) {
  (void)dev_addr; (void)instance; (void)report_id; (void)report; (void)len;
}
void ps_auth_dongle_handle_set_report_complete(uint8_t dev_addr,
                                               uint8_t instance,
                                               uint8_t report_id) {
  (void)dev_addr; (void)instance; (void)report_id;
}
void ps_auth_dongle_handle_sent_report_response(uint8_t dev_addr,
                                                uint8_t instance,
                                                const uint8_t* report,
                                                uint16_t len) {
  (void)dev_addr; (void)instance; (void)report; (void)len;
}
void ps_auth_dongle_handle_input_report_received(uint8_t dev_addr,
                                                 uint8_t instance,
                                                 const uint8_t* report,
                                                 uint16_t len) {
  (void)dev_addr; (void)instance; (void)report; (void)len;
}
bool ps_auth_dongle_prepare_ps5_output_report(const uint8_t* report,
                                              uint16_t len,
                                              uint8_t* out,
                                              uint16_t* out_len,
                                              uint16_t out_capacity) {
  (void)report; (void)len; (void)out; (void)out_len; (void)out_capacity;
  return false;
}
void ps_auth_dongle_complete_ps5_output_report() {}
bool ps_auth_dongle_has_provider() { return false; }
PsAuthDongleStatus ps_auth_dongle_status() {
  PsAuthDongleStatus status = {};
  return status;
}

#endif
