// MapleLib - Dreamcast Maple Bus Library for RP2040
// Based on protocol documentation from mc.pp.se/dc/maplebus.html
// and the dreamcast-controller-usb-pico project
//
// The Maple Bus uses two wires (SDCKA and SDCKB) that alternate
// as clock and data. Pin B must be pinA + 1.
//
// Hardware notes:
// - Dreamcast uses 5V power, 3.3V TTL signals
// - 47 ohm series resistors recommended for input protection
// - Original Dreamcast uses 36 ohm resistors

#pragma once

#include <Arduino.h>
#include "hardware/pio.h"
#include "hardware/dma.h"

// Maple Bus commands
#define MAPLE_MAX_PACKET_WORDS          256
#define MAPLE_CMD_REQUEST_DEVICE_INFO   0x01
#define MAPLE_CMD_DEVICE_INFO_RESPONSE  0x05
#define MAPLE_CMD_GET_CONDITION         0x09
#define MAPLE_CMD_DATA_TRANSFER         0x08
#define MAPLE_CMD_GET_MEMORY_INFO       0x0A
#define MAPLE_CMD_BLOCK_READ            0x0B
#define MAPLE_CMD_BLOCK_WRITE           0x0C
#define MAPLE_CMD_GET_LAST_ERROR        0x0D
#define MAPLE_CMD_SET_CONDITION         0x0E
#define MAPLE_CMD_ACK                   0x07
#define MAPLE_CMD_ERROR                 0xFC
#define MAPLE_CMD_NO_RESPONSE           0xFF

// Maple device function codes
#define MAPLE_FUNC_CONTROLLER   0x00000001
#define MAPLE_FUNC_MEMORY       0x00000002
#define MAPLE_FUNC_LCD          0x00000004
#define MAPLE_FUNC_CLOCK        0x00000008
#define MAPLE_FUNC_MICROPHONE   0x00000010
#define MAPLE_FUNC_AR_GUN       0x00000020
#define MAPLE_FUNC_KEYBOARD     0x00000040
#define MAPLE_FUNC_LIGHT_GUN    0x00000080
#define MAPLE_FUNC_PURUPURU     0x00000100  // Vibration
#define MAPLE_FUNC_MOUSE        0x00000200

// Dreamcast controller button bits (active LOW - 0 = pressed)
// Standard controller buttons are in the low 16 bits
#define DC_BTN_C      (1 << 0)
#define DC_BTN_B      (1 << 1)
#define DC_BTN_A      (1 << 2)
#define DC_BTN_START  (1 << 3)
#define DC_BTN_UP     (1 << 4)
#define DC_BTN_DOWN   (1 << 5)
#define DC_BTN_LEFT   (1 << 6)
#define DC_BTN_RIGHT  (1 << 7)
#define DC_BTN_Z      (1 << 8)
#define DC_BTN_Y      (1 << 9)
#define DC_BTN_X      (1 << 10)
#define DC_BTN_D      (1 << 11)
// Bits 12-15: Second D-pad (UP2, DOWN2, LEFT2, RIGHT2)

// Dreamcast controller condition data
struct DreamcastControllerCondition {
  uint16_t buttons;     // Button state (0 = pressed, inverted logic)
  uint8_t rtrigger;     // Right trigger (0 = released, 255 = full)
  uint8_t ltrigger;     // Left trigger
  uint8_t joyx;         // Joystick X (0 = left, 128 = center, 255 = right)
  uint8_t joyy;         // Joystick Y (0 = up, 128 = center, 255 = down)
  uint8_t joyx2;        // Second joystick X (if present)
  uint8_t joyy2;        // Second joystick Y (if present)
};

// VMU/Memory card info
struct VMUInfo {
  bool present;           // VMU detected in slot
  uint32_t func;          // Function codes (should have MAPLE_FUNC_MEMORY)
  uint16_t totalBlocks;   // Total blocks (typically 256 for standard VMU)
  uint16_t blockSize;     // Bytes per block (512 for VMU)
  uint8_t partitions;     // Number of partitions
  uint8_t readAccess;     // Read access per phase
  uint8_t writeAccess;    // Write access per phase
};

// VMU block read result
enum class VMUBlockResult {
  SUCCESS,
  NOT_PRESENT,
  READ_ERROR,
  WRITE_ERROR,
  TIMEOUT,
  INVALID_BLOCK
};

// Device info structure
struct MapleDeviceInfo {
  uint32_t func;        // Function codes
  uint32_t func_data[3]; // Function-specific data
  uint8_t area_code;    // Region
  uint8_t connector_direction;
  char product_name[31];
  char license[61];
  uint16_t standby_power;
  uint16_t max_power;
};

// Maple packet header
struct MaplePacketHeader {
  uint8_t command;
  uint8_t recipient;    // Destination address
  uint8_t sender;       // Source address
  uint8_t length;       // Data length in 32-bit words
};

// Maple Bus status
enum class MapleBusStatus {
  IDLE,
  WRITE_IN_PROGRESS,
  READ_IN_PROGRESS,
  READ_COMPLETE,
  WRITE_COMPLETE,
  TIMEOUT,
  ERROR
};

// Debug: track initialization failure reason (defined in MapleLib.cpp)
// 0 = success, 1 = out program fail, 2 = in program fail, 3 = sm_out fail, 4 = sm_in fail, 5 = dma_write fail, 6 = dma_read fail
extern volatile uint8_t maple_init_fail_reason;
// Debug: which PIO blocks we ended up using (0xFF = not assigned)
extern volatile uint8_t maple_pio_out_block;
extern volatile uint8_t maple_pio_in_block;
// Debug: available instruction slots on each PIO (before loading)
extern volatile uint8_t maple_pio0_free;
extern volatile uint8_t maple_pio1_free;
// Debug: communication counters
extern volatile uint16_t maple_write_ok;
extern volatile uint16_t maple_write_fail;
extern volatile uint16_t maple_read_ok;
extern volatile uint16_t maple_read_timeout;
extern volatile uint16_t maple_bad_frame;
extern volatile uint16_t maple_update_calls;
extern volatile uint16_t maple_bus_busy;
extern volatile uint16_t maple_dma_words;
extern volatile uint16_t maple_gpio_activity;  // Debug: detected any GPIO activity after TX
extern volatile uint16_t maple_gpio_activity_a;  // Debug: pin A observed low after TX
extern volatile uint16_t maple_gpio_activity_b;  // Debug: pin B observed low after TX
extern volatile uint16_t maple_rx_irq_hits;      // Debug: RX end IRQ observed
extern volatile uint16_t maple_posttx_edges_a;   // Debug: post-TX edge count on A
extern volatile uint16_t maple_posttx_edges_b;   // Debug: post-TX edge count on B
extern volatile uint32_t maple_sys_hz;           // Debug: system clock at Maple init
extern volatile uint16_t maple_tx_low_seen_a;    // Debug: TX active-low observed on A
extern volatile uint16_t maple_tx_low_seen_b;    // Debug: TX active-low observed on B
extern volatile uint16_t maple_wait_low_a;       // Debug: any low on A during RX wait
extern volatile uint16_t maple_wait_low_b;       // Debug: any low on B during RX wait
extern volatile uint32_t maple_last_rx_word0_raw;      // First RX word before byte-swap
extern volatile uint32_t maple_last_rx_word0_swapped;  // First RX word after byte-swap
extern volatile uint8_t maple_last_rx_len;             // Parsed frame length byte
extern volatile uint8_t maple_last_condition_layout;   // 0=normal byte order, 1=reversed byte order
extern volatile uint8_t maple_last_probe_recipient;    // Last recipient address probed
extern volatile uint8_t maple_last_devinfo_sender;     // Sender seen in last DEVICE_INFO response
extern volatile uint8_t maple_last_devinfo_recipient;  // Recipient seen in last DEVICE_INFO response
extern volatile uint8_t maple_last_devinfo_len;        // Word length from last DEVICE_INFO frame
extern volatile uint32_t maple_last_devinfo_func_native;   // response[1] raw
extern volatile uint32_t maple_last_devinfo_func_swapped;  // bswap(response[1])
extern volatile uint8_t maple_last_vmu_addr;           // Last VMU address used for block access
extern volatile uint8_t maple_last_vmu_phase;          // Last VMU phase used for block access
extern volatile uint16_t maple_last_vmu_block;         // Last VMU block used for block access
extern volatile uint8_t maple_last_vmu_cmd;            // Last VMU response command
extern volatile uint8_t maple_last_vmu_len;            // Last VMU response length including frame word
extern volatile uint32_t maple_last_vmu_word0;         // Last VMU response header
extern volatile uint32_t maple_last_vmu_word1;         // Last VMU response word 1
extern volatile uint32_t maple_last_vmu_word2;         // Last VMU response word 2
extern volatile uint8_t maple_last_vmu_write_count;    // Advertised/effective write phase count
extern volatile uint16_t maple_last_vmu_phase_bytes;   // Effective bytes per write phase
extern volatile uint8_t maple_last_vmu_stage;          // 1=phase, 2=commit, 3=geometry, 4=timeout, 5=response error

// Maple Bus controller class
class MaplePort {
private:
  PIO pio;
  uint sm_out;
  uint sm_in;
  uint pinA;
  uint pinB;
  int dma_write_channel;
  int dma_read_channel;

  uint32_t write_buffer[MAPLE_MAX_PACKET_WORDS];  // TX buffer
  uint32_t read_buffer[MAPLE_MAX_PACKET_WORDS];   // RX buffer

  MapleBusStatus status;
  bool connected;
  bool initialized;  // Track if begin() succeeded
  uint32_t last_poll_time;
  uint8_t consecutive_fail_count;
  uint8_t last_response_cmd;
  uint8_t device_addr;  // Active Maple recipient address (default 0x20)
  uint8_t probe_addr_index;  // Round-robin probe index while disconnected
  bool reverse_condition_words;  // Auto-detected per connection for robust payload decoding
  bool condition_layout_locked;
  int16_t condition_layout_score_normal;
  int16_t condition_layout_score_reversed;
  uint8_t condition_layout_samples;
  uint32_t last_seen_func;
  uint32_t last_vmu_probe_time;
  uint8_t vmu_probe_slot;
  bool pending_accessory_rescan;

  // Device info
  MapleDeviceInfo device_info;
  DreamcastControllerCondition controller;

  // VMU info for each slot (2 slots per controller)
  VMUInfo vmu_info[2];

  // Internal methods
  bool initPio();
  void resetPio();
  bool writePacket(uint8_t cmd, uint8_t recipient, const uint32_t* data, uint8_t len);
  bool readPacket(uint32_t* data, uint8_t* len, uint32_t timeout_us);
  bool writeAndRead(uint8_t cmd, uint8_t recipient, const uint32_t* txData, uint8_t txLen,
                    uint32_t* rxData, uint8_t* rxLen, uint32_t timeout_us);
  uint8_t calculateCrc(const uint32_t* data, uint8_t len);

public:
  MaplePort() : pio(nullptr), sm_out(0), sm_in(0), pinA(0), pinB(0),
                dma_write_channel(-1), dma_read_channel(-1),
                status(MapleBusStatus::IDLE), connected(false), initialized(false), last_poll_time(0),
                consecutive_fail_count(0), last_response_cmd(MAPLE_CMD_NO_RESPONSE), device_addr(0x20),
                probe_addr_index(0), reverse_condition_words(false), condition_layout_locked(false),
                condition_layout_score_normal(0), condition_layout_score_reversed(0), condition_layout_samples(0),
                last_seen_func(0), last_vmu_probe_time(0), vmu_probe_slot(0), pending_accessory_rescan(false) {
    memset(&device_info, 0, sizeof(device_info));
    memset(&controller, 0, sizeof(controller));
    memset(&vmu_info, 0, sizeof(vmu_info));
    controller.buttons = 0xFFFF;  // All buttons released (active low)
    controller.joyx = 128;
    controller.joyy = 128;
    controller.joyx2 = 128;
    controller.joyy2 = 128;
  }
  ~MaplePort() { end(); }

  // Check if initialization succeeded
  bool isInitialized() const { return initialized; }

  // Initialize the Maple Bus on the given pins
  // pinA = SDCKA, pinB = SDCKB (must be pinA + 1)
  bool begin(uint pinA);
  void end();

  // Poll the bus for devices
  void update();

  // Check if a controller is connected
  bool isConnected() const { return connected; }

  // Get the last poll status
  MapleBusStatus getStatus() const { return status; }

  // Get controller state
  const DreamcastControllerCondition& getController() const { return controller; }

  // Get device info
  const MapleDeviceInfo& getDeviceInfo() const { return device_info; }
  uint32_t getLastSeenFunction() const { return last_seen_func; }
  uint32_t getAccessoryFunctionMask() const { return vmu_info[0].func | vmu_info[1].func; }

  // Debug helpers
  uint8_t getConsecutiveFailCount() const { return consecutive_fail_count; }
  uint8_t getLastResponseCmd() const { return last_response_cmd; }
  uint8_t getDeviceAddress() const { return device_addr; }

  // Check if device has a specific function
  bool hasFunction(uint32_t func) const { return (device_info.func & func) != 0; }

  // Get button state (returns true if pressed)
  bool buttonPressed(uint16_t button) const {
    return (controller.buttons & button) == 0;  // Active low
  }

  // Get analog values
  uint8_t getLeftTrigger() const { return controller.ltrigger; }
  uint8_t getRightTrigger() const { return controller.rtrigger; }
  uint8_t getJoystickX() const { return controller.joyx; }
  uint8_t getJoystickY() const { return controller.joyy; }

  // Convert joystick from 0-255 to signed (-128 to 127)
  int8_t getJoystickXSigned() const { return (int8_t)(controller.joyx - 128); }
  int8_t getJoystickYSigned() const { return (int8_t)(controller.joyy - 128); }

  // ============= Debug Functions =============

  // Get pin states (true = high)
  bool getPinA() const { return gpio_get(pinA); }
  bool getPinB() const { return gpio_get(pinB); }
  uint8_t getPinANum() const { return pinA; }
  uint8_t getPinBNum() const { return pinB; }

  // Test GPIO by manually toggling (call before begin() or after disabling PIO)
  // Returns true if both pins can be toggled and read back correctly
  bool testGPIO();

  // Debug: Manually send a start sequence and check response
  // Returns number of 32-bit words received, or 0 if no response
  uint8_t debugProbe();
  bool probeDeviceInfo(uint8_t recipient, uint32_t* funcNative, uint32_t* funcSwapped,
                       uint8_t* sender, uint8_t* responseLen);

  // ============= VMU/Memory Card Functions =============

  // Check if VMU is present in a slot (0 = slot A, 1 = slot B)
  bool isVMUPresent(uint8_t slot) const {
    if (slot > 1) return false;
    return vmu_info[slot].present;
  }

  // Get VMU info for a slot
  const VMUInfo& getVMUInfo(uint8_t slot) const {
    return vmu_info[slot < 2 ? slot : 0];
  }

  void clearVMUInfo(uint8_t slot) {
    if (slot < 2) {
      memset(&vmu_info[slot], 0, sizeof(VMUInfo));
    }
  }

  // Query VMU device info (call after controller is connected)
  bool queryVMU(uint8_t slot);

  // Get VMU memory info (block count, size, etc.)
  bool getVMUMemoryInfo(uint8_t slot);

  // Read a block from VMU (512 bytes)
  // block: 0-255 for standard VMU
  // buffer: must be at least 512 bytes
  VMUBlockResult readVMUBlock(uint8_t slot, uint16_t block, uint8_t* buffer);

  // Write a block to VMU (512 bytes)
  // block: 0-255 for standard VMU
  // buffer: 512 bytes of data to write
  VMUBlockResult writeVMUBlock(uint8_t slot, uint16_t block, const uint8_t* buffer);

  // Read entire VMU to buffer (128KB for standard VMU)
  // Returns number of bytes read, or 0 on error
  uint32_t readVMUAll(uint8_t slot, uint8_t* buffer, uint32_t bufferSize);

  // Write entire VMU from buffer
  // Returns number of bytes written, or 0 on error
  uint32_t writeVMUAll(uint8_t slot, const uint8_t* buffer, uint32_t dataSize);
};
