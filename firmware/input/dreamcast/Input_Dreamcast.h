#pragma once

// Dreamcast Input Module
// Uses MapleLib for Maple Bus communication
//
// Hardware connection:
// - HDMI_x_08: SDCKA (pin 1 on Dreamcast port)
// - HDMI_x_09: SDCKB (pin 5 on Dreamcast port)
// - GND: Pin 3
// - 5V: Pin 4 (or 3.3V for controller power)
//
// Note: 47 ohm series resistors recommended on SDCKA/SDCKB for protection
// Dreamcast signals are 3.3V TTL compatible

#include "../base/RZInputModule.h"
#include "input_dreamcast_debug_runtime_state.h"
#include <MapleLib/MapleLib.h>

#ifndef DREAMCAST_WEBHID_DEBUG
#define DREAMCAST_WEBHID_DEBUG 1
#endif

#ifndef DREAMCAST_SERIAL_DEBUG
#define DREAMCAST_SERIAL_DEBUG 0
#endif

#ifndef DREAMCAST_SERIAL_DEBUG_INTERVAL_MS
#define DREAMCAST_SERIAL_DEBUG_INTERVAL_MS 250
#endif

typedef struct {
  int8_t pinA;  // SDCKA - must be consecutive with pinB
  int8_t pinB;  // SDCKB (automatically pinA + 1 in MapleLib)
} input_dreamcast_config_t;

struct DreamcastVmuSerialStats {
  uint32_t read_count = 0;
  uint32_t success_count = 0;
  uint32_t timeout_count = 0;
  uint32_t timeout_attempt_count = 0;
  uint32_t retry_count = 0;
  uint32_t recovery_count = 0;
  uint32_t recovery_success_count = 0;
  uint32_t write_count = 0;
  uint32_t write_success_count = 0;
  uint32_t last_read_us = 0;
  uint32_t max_read_us = 0;
  uint16_t last_block = 0;
  uint8_t last_port = 0;
  uint8_t last_slot = 0;
  VMUBlockResult last_result = VMUBlockResult::SUCCESS;
};

const DreamcastVmuSerialStats& dreamcastVmuSerialStats();

// Pin configuration for each port
// Using HDMI pins 8 and 9 on both connectors (3.3V with pull-ups)
// MapleLib is PIO-only and requires pinB = pinA + 1.
//
// Current V2.1 PCB mapping:
//   HDMI_1_08 = GPIO 5,  HDMI_1_09 = GPIO 6
//   HDMI_2_08 = GPIO 16, HDMI_2_09 = GPIO 17
const input_dreamcast_config_t input_dreamcast_config[] = {
  { .pinA = HDMI_1_08, .pinB = HDMI_1_09 },
  { .pinA = HDMI_2_08, .pinB = HDMI_2_09 },
};

class RZInputDreamcast : public RZInputModule {
private:
  static const uint8_t input_ports = sizeof(input_dreamcast_config) / sizeof(input_dreamcast_config_t);
  MaplePort* maple[input_ports];
  bool wasConnected[MAX_USB_OUT] = {false};
  bool initSuccess[input_ports] = {false};
  uint32_t lastConnectedMs[MAX_USB_OUT] = {0};
  uint32_t vmuPollQuietUntil[MAX_USB_OUT] = {0};
  bool autoVmuScanDone[MAX_USB_OUT] = {false};
  uint32_t autoVmuScanDueAt[MAX_USB_OUT] = {0};
  bool pendingWebHidDebugFrame = false;
  uint32_t pendingSerialDebugNow = 0;
  static constexpr uint16_t HOTSWAP_RECENT_CONNECTION_GRACE_MS = 300;
#if DREAMCAST_SERIAL_DEBUG
  uint32_t lastSerialDebugTime = 0;
#endif

  void quietVmuPolling(uint8_t port, uint16_t ms = 90);
  bool isVmuPollingQuiet(uint8_t port, uint32_t now) const;
  void scheduleAutoVmuScan(uint8_t port, uint32_t now);
  void serviceAutoVmuScan(uint8_t port, uint32_t now);
  bool recoverVmuPort(uint8_t port);
  void storeWebHidDebugFrame();
  void printSerialDebug(uint32_t now);
  const char* getDebugStatus(uint8_t port);
  const char* getDeviceLabel(uint32_t func);
  const char* getControllerDeviceLabel(const MapleDeviceInfo& info, uint32_t accessoryMask);
  bool getPinState(uint8_t port, bool pinB);

public:
  RZInputDreamcast();
  ~RZInputDreamcast() override;

  const char* getUsbId() override;
  void configureBcdDeviceVersion() override;
  const char* getDescription() override;
  void setup() override;
  void setup2() override;
  bool poll() override;
  void afterOutputFrameSent(bool polled, bool updated) override;
  bool hasPhysicalConnectionForHotSwap() const override;

  bool refreshVmuInfo(uint8_t port, uint8_t slot, VMUInfo* info);
  bool getVmuInfo(uint8_t port, uint8_t slot, VMUInfo* info) const;
  VMUBlockResult readVmuBlock(uint8_t port, uint8_t slot, uint16_t block, uint8_t* buffer);
  VMUBlockResult writeVmuBlock(uint8_t port, uint8_t slot, uint16_t block, const uint8_t* buffer);
  void printDiagnostics(Print& out);
};
