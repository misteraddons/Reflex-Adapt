// MapleLib - Dreamcast Maple Bus Library for RP2040
// PIO-based implementation for reliable 2Mbps communication
//
// Based on sega-dreamcast/dreamcast-controller-usb-pico (MIT License)
// Adapted for Arduino/TinyUSB environment

#include "MapleLib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

#include "../../../firmware/platform/latency_trace_gpio.h"

// Include PIO programs
#include "maple_out.pio.h"
#include "maple_in.pio.h"

// Timing constants
#define MAPLE_RESPONSE_TIMEOUT_US 2000  // 2ms timeout for response
#define MAPLE_VMU_WRITE_RESPONSE_TIMEOUT_US 100000  // VMU flash writes can ACK slowly.
#define MAPLE_VMU_WRITE_PHASE_DELAY_MS 24
#define MAPLE_WRITE_TIMEOUT_US    5000  // 5ms timeout for write completion
#define MAPLE_RX_GUARD_US         0     // Start RX immediately at TX end
#define MAPLE_TX_END_TIMEOUT_US   100   // Wait for TX end-sequence IRQ (pins released)
#define MAPLE_INTER_WORD_TIMEOUT_US 300 // Stop RX after this idle gap once data started
#ifndef MAPLE_ENABLE_POSTTX_GPIO_DIAGNOSTICS
#define MAPLE_ENABLE_POSTTX_GPIO_DIAGNOSTICS 0
#endif

// PIO configuration - will be assigned dynamically
static PIO pio_out = nullptr;
static PIO pio_in = nullptr;

static bool pio_programs_loaded = false;
static uint maple_out_offset = 0;
static uint maple_in_offset = 0;

// Debug: track initialization failure reason
// 0 = success, 1 = out program fail, 2 = in program fail, 3 = sm_out fail, 4 = sm_in fail, 5 = dma_write fail, 6 = dma_read fail
volatile uint8_t maple_init_fail_reason = 0;

// Debug: which PIO blocks we ended up using
volatile uint8_t maple_pio_out_block = 0xFF;
volatile uint8_t maple_pio_in_block = 0xFF;
// Debug: available instruction slots on each PIO (before loading)
volatile uint8_t maple_pio0_free = 0;
volatile uint8_t maple_pio1_free = 0;
// Debug: communication counters
volatile uint16_t maple_write_ok = 0;
volatile uint16_t maple_write_fail = 0;
volatile uint16_t maple_read_ok = 0;
volatile uint16_t maple_read_timeout = 0;
volatile uint16_t maple_bad_frame = 0;
volatile uint16_t maple_update_calls = 0;
volatile uint16_t maple_bus_busy = 0;
volatile uint16_t maple_dma_words = 0;  // Last DMA word count
volatile uint16_t maple_gpio_activity = 0;  // Debug: detected GPIO activity after TX
volatile uint16_t maple_gpio_activity_a = 0;
volatile uint16_t maple_gpio_activity_b = 0;
volatile uint16_t maple_rx_irq_hits = 0;
volatile uint16_t maple_posttx_edges_a = 0;
volatile uint16_t maple_posttx_edges_b = 0;
volatile uint32_t maple_sys_hz = 0;
volatile uint16_t maple_tx_low_seen_a = 0;
volatile uint16_t maple_tx_low_seen_b = 0;
volatile uint16_t maple_wait_low_a = 0;
volatile uint16_t maple_wait_low_b = 0;
volatile uint32_t maple_last_rx_word0_raw = 0;
volatile uint32_t maple_last_rx_word0_swapped = 0;
volatile uint8_t maple_last_rx_len = 0;
volatile uint8_t maple_last_condition_layout = 0;
volatile uint8_t maple_last_probe_recipient = 0x20;
volatile uint8_t maple_last_devinfo_sender = 0;
volatile uint8_t maple_last_devinfo_recipient = 0;
volatile uint8_t maple_last_devinfo_len = 0;
volatile uint32_t maple_last_devinfo_func_native = 0;
volatile uint32_t maple_last_devinfo_func_swapped = 0;
volatile uint8_t maple_last_vmu_addr = 0;
volatile uint8_t maple_last_vmu_phase = 0;
volatile uint16_t maple_last_vmu_block = 0;
volatile uint8_t maple_last_vmu_cmd = 0;
volatile uint8_t maple_last_vmu_len = 0;
volatile uint32_t maple_last_vmu_word0 = 0;
volatile uint32_t maple_last_vmu_word1 = 0;
volatile uint32_t maple_last_vmu_word2 = 0;
volatile uint8_t maple_last_vmu_write_count = 0;
volatile uint16_t maple_last_vmu_phase_bytes = 0;
volatile uint8_t maple_last_vmu_stage = 0;

// Instance tracking for IRQ callbacks
static MaplePort* maple_instances[4] = {nullptr, nullptr, nullptr, nullptr};
static uint8_t maple_active_instances = 0;

// Helper: count contiguous free instruction slots in a PIO
static uint8_t countPioFreeSlots(PIO pio) {
    // Try progressively smaller programs to find max available
    for (uint8_t size = 32; size > 0; size--) {
        pio_program_t test = { .instructions = nullptr, .length = size, .origin = -1 };
        if (pio_can_add_program(pio, &test)) {
            return size;
        }
    }
    return 0;
}

static void releaseMaplePins(uint pinA, uint pinB) {
    gpio_set_function(pinA, GPIO_FUNC_SIO);
    gpio_set_function(pinB, GPIO_FUNC_SIO);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);
}

static void unloadPioProgramsIfUnused() {
    if (!pio_programs_loaded || maple_active_instances != 0) return;

    if (pio_out != nullptr) {
        pio_remove_program(pio_out, &maple_out_program, maple_out_offset);
    }
    if (pio_in != nullptr) {
        pio_remove_program(pio_in, &maple_in_program, maple_in_offset);
    }

    pio_programs_loaded = false;
    pio_out = nullptr;
    pio_in = nullptr;
    maple_out_offset = 0;
    maple_in_offset = 0;
    maple_pio_out_block = 0xFF;
    maple_pio_in_block = 0xFF;
}

static uint32_t mapleInfoWord(const uint32_t* response, uint8_t wordIndex, bool use_swapped) {
    const uint32_t word = response[wordIndex];
    return use_swapped ? __builtin_bswap32(word) : word;
}

static uint8_t mapleInfoByte(const uint32_t* response, uint8_t len, bool use_swapped, uint16_t byteOffset) {
    const uint8_t wordIndex = 5 + (byteOffset / 4);
    if (wordIndex >= len) return 0;
    const uint32_t word = mapleInfoWord(response, wordIndex, use_swapped);
    return (word >> ((3 - (byteOffset % 4)) * 8)) & 0xFF;
}

static uint16_t mapleInfoBe16(const uint32_t* response, uint8_t len, bool use_swapped, uint16_t byteOffset) {
    return ((uint16_t)mapleInfoByte(response, len, use_swapped, byteOffset) << 8) |
           (uint16_t)mapleInfoByte(response, len, use_swapped, byteOffset + 1);
}

static void copyMapleTextField(char* dest,
                               size_t destSize,
                               const uint32_t* response,
                               uint8_t len,
                               bool use_swapped,
                               uint16_t byteOffset,
                               uint16_t fieldLen) {
    if (dest == nullptr || destSize == 0) return;
    dest[0] = '\0';

    size_t out = 0;
    for (uint16_t i = 0; i < fieldLen && out + 1 < destSize; ++i) {
        const uint8_t raw = mapleInfoByte(response, len, use_swapped, byteOffset + i);
        if (raw == 0) break;
        dest[out++] = (raw >= 0x20 && raw <= 0x7E) ? (char)raw : ' ';
    }

    while (out > 0 && dest[out - 1] == ' ') {
        --out;
    }
    dest[out] = '\0';
}

// Load PIO programs (only once)
// Tries to find available PIO blocks for both programs
static bool loadPioPrograms() {
    if (pio_programs_loaded) return true;

    // Record available space before loading anything
    maple_pio0_free = countPioFreeSlots(pio0);
    maple_pio1_free = countPioFreeSlots(pio1);

    // Try to find a PIO block for the output program (31 instructions)
    // Try pio0 first, then pio1
    pio_out = nullptr;
    if (pio_can_add_program(pio0, &maple_out_program)) {
        pio_out = pio0;
        maple_pio_out_block = 0;
    } else if (pio_can_add_program(pio1, &maple_out_program)) {
        pio_out = pio1;
        maple_pio_out_block = 1;
    } else {
        maple_init_fail_reason = 1;  // out program fail - no PIO has space
        return false;
    }
    maple_out_offset = pio_add_program(pio_out, &maple_out_program);

    // Try to find a PIO block for the input program (29 instructions)
    // Can be same or different PIO as output
    pio_in = nullptr;
    if (pio_can_add_program(pio0, &maple_in_program)) {
        pio_in = pio0;
        maple_pio_in_block = 0;
    } else if (pio_can_add_program(pio1, &maple_in_program)) {
        pio_in = pio1;
        maple_pio_in_block = 1;
    } else {
        // Cleanup output program
        pio_remove_program(pio_out, &maple_out_program, maple_out_offset);
        maple_init_fail_reason = 2;  // in program fail - no PIO has space
        return false;
    }
    maple_in_offset = pio_add_program(pio_in, &maple_in_program);

    pio_programs_loaded = true;
    return true;
}

// CRC calculation
uint8_t MaplePort::calculateCrc(const uint32_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (data[i] >> 0) & 0xFF;
        crc ^= (data[i] >> 8) & 0xFF;
        crc ^= (data[i] >> 16) & 0xFF;
        crc ^= (data[i] >> 24) & 0xFF;
    }
    return crc;
}

bool MaplePort::begin(uint _pinA) {
    pinA = _pinA;
    pinB = _pinA + 1;
    initialized = false;  // Will be set true on success
    maple_sys_hz = clock_get_hz(clk_sys);

    // Load PIO programs if not already loaded
    if (!loadPioPrograms()) {
        // PIO not available - fail reason already set in loadPioPrograms()
        return false;
    }

    // Claim state machines
    int sm_o = pio_claim_unused_sm(pio_out, false);
    int sm_i = pio_claim_unused_sm(pio_in, false);

    if (sm_o < 0 || sm_i < 0) {
        if (sm_o >= 0) pio_sm_unclaim(pio_out, sm_o);
        if (sm_i >= 0) pio_sm_unclaim(pio_in, sm_i);
        maple_init_fail_reason = (sm_o < 0) ? 3 : 4;  // sm_out or sm_in fail
        return false;
    }

    sm_out = sm_o;
    sm_in = sm_i;

    // Claim DMA channels
    dma_write_channel = dma_claim_unused_channel(false);
    dma_read_channel = dma_claim_unused_channel(false);

    if (dma_write_channel < 0 || dma_read_channel < 0) {
        if (dma_write_channel >= 0) dma_channel_unclaim(dma_write_channel);
        if (dma_read_channel >= 0) dma_channel_unclaim(dma_read_channel);
        pio_sm_unclaim(pio_out, sm_out);
        pio_sm_unclaim(pio_in, sm_in);
        maple_init_fail_reason = (dma_write_channel < 0) ? 5 : 6;  // dma_write or dma_read fail
        return false;
    }

    // Initialize GPIO pins
    gpio_init(pinA);
    gpio_init(pinB);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    // Initialize PIO state machines
    maple_out_program_init(pio_out, sm_out, maple_out_offset, pinA);
    maple_in_program_init(pio_in, sm_in, maple_in_offset, pinA);

    // Configure write DMA (memory -> PIO TX FIFO)
    dma_channel_config c_write = dma_channel_get_default_config(dma_write_channel);
    channel_config_set_transfer_data_size(&c_write, DMA_SIZE_32);
    channel_config_set_read_increment(&c_write, true);
    channel_config_set_write_increment(&c_write, false);
    channel_config_set_dreq(&c_write, pio_get_dreq(pio_out, sm_out, true));
    // No DMA bswap - we manually swap data words (but not bit count which PIO consumes)
    channel_config_set_bswap(&c_write, false);
    dma_channel_configure(dma_write_channel, &c_write,
                          &pio_out->txf[sm_out],  // Write to PIO TX FIFO
                          write_buffer,           // Read from write buffer
                          0,                      // Transfer count set later
                          false);                 // Don't start yet

    // Configure read DMA (PIO RX FIFO -> memory)
    dma_channel_config c_read = dma_channel_get_default_config(dma_read_channel);
    channel_config_set_transfer_data_size(&c_read, DMA_SIZE_32);
    channel_config_set_read_increment(&c_read, false);
    channel_config_set_write_increment(&c_read, true);
    channel_config_set_dreq(&c_read, pio_get_dreq(pio_in, sm_in, false));
    // No DMA bswap - we handle byte order in parsing
    channel_config_set_bswap(&c_read, false);
    dma_channel_configure(dma_read_channel, &c_read,
                          read_buffer,            // Write to read buffer
                          &pio_in->rxf[sm_in],    // Read from PIO RX FIFO
                          MAPLE_MAX_PACKET_WORDS, // Max words to receive
                          false);                 // Don't start yet

    // Store instance for potential IRQ callback
    for (int i = 0; i < 4; i++) {
        if (maple_instances[i] == nullptr) {
            maple_instances[i] = this;
            break;
        }
    }

    connected = false;
    status = MapleBusStatus::IDLE;
    initialized = true;  // Mark as successfully initialized

    // IMPORTANT: Release pins back to GPIO control after PIO init
    // The PIO state machines will reclaim them when enabled for TX/RX
    gpio_set_function(pinA, GPIO_FUNC_SIO);
    gpio_set_function(pinB, GPIO_FUNC_SIO);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    // DEBUG: Quick GPIO toggle test to verify pins can be driven
    // This creates a brief pulse on startup that should be visible on scope
    gpio_set_dir(pinA, GPIO_OUT);
    gpio_set_dir(pinB, GPIO_OUT);
    for (int i = 0; i < 10; i++) {
        gpio_put(pinA, 0);
        gpio_put(pinB, 0);
        delayMicroseconds(50);
        gpio_put(pinA, 1);
        gpio_put(pinB, 1);
        delayMicroseconds(50);
    }
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    // Wait for bus to stabilize (short delay, don't block USB)
    delay(10);

    maple_active_instances++;

    return true;
}

void MaplePort::end() {
    if (!initialized) return;

    if (dma_write_channel >= 0) {
        dma_channel_abort(dma_write_channel);
        dma_channel_unclaim(dma_write_channel);
        dma_write_channel = -1;
    }
    if (dma_read_channel >= 0) {
        dma_channel_abort(dma_read_channel);
        dma_channel_unclaim(dma_read_channel);
        dma_read_channel = -1;
    }

    pio_sm_set_enabled(pio_out, sm_out, false);
    pio_sm_set_enabled(pio_in, sm_in, false);
    pio_sm_clear_fifos(pio_out, sm_out);
    pio_sm_clear_fifos(pio_in, sm_in);
    pio_interrupt_clear(pio_out, sm_out);
    pio_interrupt_clear(pio_in, sm_in);
    pio_sm_unclaim(pio_out, sm_out);
    pio_sm_unclaim(pio_in, sm_in);

    // Release lines to pull-up idle state.
    gpio_set_function(pinA, GPIO_FUNC_SIO);
    gpio_set_function(pinB, GPIO_FUNC_SIO);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    for (int i = 0; i < 4; i++) {
        if (maple_instances[i] == this) {
            maple_instances[i] = nullptr;
        }
    }

    if (maple_active_instances > 0) {
        maple_active_instances--;
    }
    unloadPioProgramsIfUnused();

    connected = false;
    initialized = false;
    status = MapleBusStatus::IDLE;
    consecutive_fail_count = 0;
    last_response_cmd = MAPLE_CMD_NO_RESPONSE;
}

void MaplePort::resetPio() {
    // Disable state machines
    pio_sm_set_enabled(pio_out, sm_out, false);
    pio_sm_set_enabled(pio_in, sm_in, false);

    // Clear FIFOs
    pio_sm_clear_fifos(pio_out, sm_out);
    pio_sm_clear_fifos(pio_in, sm_in);

    // Return pins to regular GPIO control and set as inputs with pull-ups
    gpio_set_function(pinA, GPIO_FUNC_SIO);
    gpio_set_function(pinB, GPIO_FUNC_SIO);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    // Restart state machines at beginning
    pio_sm_restart(pio_out, sm_out);
    pio_sm_restart(pio_in, sm_in);
    pio_sm_exec(pio_out, sm_out, pio_encode_jmp(maple_out_offset));
    pio_sm_exec(pio_in, sm_in, pio_encode_jmp(maple_in_offset));
}

bool MaplePort::writePacket(uint8_t cmd, uint8_t recipient, const uint32_t* data, uint8_t len) {
    // Build packet in write buffer
    // Format: [bit_count] [frame_word] [data...] [crc_byte]
    //
    // Maple Bus wire protocol: bytes are sent LSB first within each 32-bit word
    // PIO uses left shift (MSB first), so we byte-swap transmitted words
    // Reference: mc.pp.se/dc/maplewire.html - "the byte sent first is actually the last of the four bytes"

    // Frame word in logical order: [cmd][recipient][sender][length]
    // But on wire, length byte is sent first, then sender, recipient, command
    uint32_t frame = ((uint32_t)cmd << 24) | ((uint32_t)recipient << 16) | (0x00 << 8) | len;

    // Calculate total bits: frame(32) + data(len*32) + crc(8)
    uint32_t total_bits = 32 + (len * 32) + 8;

    // Build buffer
    write_buffer[0] = total_bits;  // Bit count for PIO (NOT transmitted, no swap)
    write_buffer[1] = __builtin_bswap32(frame);  // Frame word byte-swapped for wire

    // Copy data with byte swap for wire format
    for (uint8_t i = 0; i < len; i++) {
        write_buffer[2 + i] = __builtin_bswap32(data[i]);
    }

    // Calculate CRC (XOR of all bytes in logical order)
    uint8_t crc = 0;
    crc ^= cmd;
    crc ^= recipient;
    crc ^= 0x00;  // sender
    crc ^= len;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (data[i] >> 0) & 0xFF;
        crc ^= (data[i] >> 8) & 0xFF;
        crc ^= (data[i] >> 16) & 0xFF;
        crc ^= (data[i] >> 24) & 0xFF;
    }

    // CRC is only 8 bits - PIO sends 8 bits then consumes remaining 24
    // No bswap needed - CRC must be in MSB position (bits 31-24) for PIO left shift
    write_buffer[2 + len] = (uint32_t)crc << 24;

    // Total words to send: 1(bit_count) + 1(frame) + len(data) + 1(crc)
    uint32_t words_to_send = 3 + len;

    releaseMaplePins(pinA, pinB);

    // Check bus is idle (both lines high)
    if (!gpio_get(pinA) || !gpio_get(pinB)) {
        maple_bus_busy++;
        return false;  // Bus busy
    }

    // Reset and configure output PIO
    pio_sm_set_enabled(pio_out, sm_out, false);
    pio_sm_clear_fifos(pio_out, sm_out);
    pio_sm_restart(pio_out, sm_out);

    // Reconfigure pins for PIO output (may have been set to input for receiving)
    pio_gpio_init(pio_out, pinA);
    pio_gpio_init(pio_out, pinA + 1);
    pio_sm_set_consecutive_pindirs(pio_out, sm_out, pinA, 2, true);

    pio_sm_exec(pio_out, sm_out, pio_encode_jmp(maple_out_offset));

    // Configure and start DMA
    dma_channel_set_read_addr(dma_write_channel, write_buffer, false);
    dma_channel_set_trans_count(dma_write_channel, words_to_send, false);
    dma_channel_start(dma_write_channel);

    // Enable output state machine
    pio_sm_set_enabled(pio_out, sm_out, true);

    // Wait for transmission to complete (with timeout)
    uint32_t start = micros();
    while (dma_channel_is_busy(dma_write_channel)) {
        if (micros() - start > MAPLE_WRITE_TIMEOUT_US) {
            dma_channel_abort(dma_write_channel);
            pio_sm_set_enabled(pio_out, sm_out, false);
            maple_write_fail++;  // DMA timeout
            return false;
        }
    }

    // Wait for PIO to finish (IRQ or TX FIFO empty)
    while (!pio_sm_is_tx_fifo_empty(pio_out, sm_out)) {
        if (micros() - start > MAPLE_WRITE_TIMEOUT_US) {
            pio_sm_set_enabled(pio_out, sm_out, false);
            maple_bus_busy++;  // Reuse: FIFO timeout
            return false;
        }
    }

    // Wait for TX end-sequence IRQ so we know set pindirs,0 has executed.
    uint32_t end_wait_start = micros();
    while (!pio_interrupt_get(pio_out, sm_out)) {
        if (micros() - end_wait_start > MAPLE_TX_END_TIMEOUT_US) {
            pio_sm_set_enabled(pio_out, sm_out, false);
            releaseMaplePins(pinA, pinB);
            maple_bus_busy++;  // TX did not reach end marker
            return false;
        }
    }
    pio_interrupt_clear(pio_out, sm_out);

    // Disable output state machine
    pio_sm_set_enabled(pio_out, sm_out, false);

    // Ensure pins are back to input mode for receiving response
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    maple_write_ok++;
    return true;
}

bool MaplePort::readPacket(uint32_t* data, uint8_t* len, uint32_t timeout_us) {
    // Input PIO is already running (started by writeAndRead).
    // Wait for first data up to timeout_us, then drain with inter-word timeout.
    uint32_t start_wait = micros();
    uint32_t last_progress = start_wait;
    uint32_t last_count = MAPLE_MAX_PACKET_WORDS;
    uint32_t remaining = MAPLE_MAX_PACKET_WORDS;
    bool data_started = false;
    bool irq_seen = false;
    uint32_t irq_seen_time = 0;
    bool saw_low_a = false;
    bool saw_low_b = false;

    while (true) {
        bool curA = gpio_get(pinA);
        bool curB = gpio_get(pinB);

        if (!curA) saw_low_a = true;
        if (!curB) saw_low_b = true;

        uint32_t now_us = micros();
        if (!data_started && (now_us - start_wait > timeout_us)) {
            if (saw_low_a) maple_wait_low_a++;
            if (saw_low_b) maple_wait_low_b++;
            // Timeout - release pins before returning
            dma_channel_abort(dma_read_channel);
            pio_sm_set_enabled(pio_in, sm_in, false);
            gpio_set_function(pinA, GPIO_FUNC_SIO);
            gpio_set_function(pinB, GPIO_FUNC_SIO);
            gpio_set_dir(pinA, GPIO_IN);
            gpio_set_dir(pinB, GPIO_IN);
            gpio_pull_up(pinA);
            gpio_pull_up(pinB);
            maple_dma_words = 0;
            maple_read_timeout++;
            *len = 0;
            return false;
        }

        // Check if we've received any data
        remaining = dma_channel_hw_addr(dma_read_channel)->transfer_count;

        if (remaining < last_count) {
            data_started = true;
            last_count = remaining;
            last_progress = now_us;
        }

        // PIO end-detect can fire before all words have drained from DMA.
        // Treat IRQ as a hint and keep draining briefly.
        if (!irq_seen && pio_interrupt_get(pio_in, sm_in)) {
            maple_rx_irq_hits++;
            irq_seen = true;
            irq_seen_time = now_us;
        }

        // Check if DMA is complete
        if (remaining == 0) {
            break;
        }

        if (data_started) {
            if (now_us - last_progress > MAPLE_INTER_WORD_TIMEOUT_US) {
                break;
            }
            if (irq_seen && (now_us - irq_seen_time > 120)) {
                break;
            }
        }

    }

    // Snapshot DMA progress BEFORE abort. Reading transfer_count after abort is unreliable.
    uint32_t remaining_before_abort = remaining;

    if (saw_low_a) maple_wait_low_a++;
    if (saw_low_b) maple_wait_low_b++;

    // Stop DMA and PIO
    dma_channel_abort(dma_read_channel);
    pio_sm_set_enabled(pio_in, sm_in, false);
    pio_interrupt_clear(pio_in, sm_in);

    // Ensure pins are released to input mode with pull-ups
    gpio_set_function(pinA, GPIO_FUNC_SIO);
    gpio_set_function(pinB, GPIO_FUNC_SIO);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    // Calculate how many words we received
    uint32_t words_received = MAPLE_MAX_PACKET_WORDS - remaining_before_abort;
    maple_dma_words = words_received;

    if (words_received < 1) {
        maple_read_timeout++;
        *len = 0;
        return false;
    }

    maple_last_rx_word0_raw = read_buffer[0];

    // Copy to output buffer with byte swap to convert from wire format to logical format
    for (uint32_t i = 0; i < words_received && i < MAPLE_MAX_PACKET_WORDS; i++) {
        data[i] = __builtin_bswap32(read_buffer[i]);
    }

    // Parse frame in logical format: [cmd(31-24), recipient(23-16), sender(15-8), length(7-0)]
    uint32_t frame = data[0];
    maple_last_rx_word0_swapped = frame;
    uint8_t pkt_len = frame & 0xFF;
    maple_last_rx_len = pkt_len;

    // Sanity checks: reject obviously invalid frame headers.
    if (pkt_len > (MAPLE_MAX_PACKET_WORDS - 1) || words_received < (uint32_t)(pkt_len + 1)) {
        maple_bad_frame++;
        maple_read_timeout++;
        *len = 0;
        return false;
    }

    maple_read_ok++;
    *len = pkt_len + 1;  // +1 for frame word

    return true;
}

// Combined write and read - starts input PIO BEFORE transmitting to catch immediate response
bool MaplePort::writeAndRead(uint8_t cmd, uint8_t recipient, const uint32_t* txData, uint8_t txLen,
                              uint32_t* rxData, uint8_t* rxLen, uint32_t timeout_us) {
    // Build packet in write buffer (same as writePacket)
    uint32_t frame = ((uint32_t)cmd << 24) | ((uint32_t)recipient << 16) | (0x00 << 8) | txLen;
    uint32_t total_bits = 32 + (txLen * 32) + 8;

    write_buffer[0] = total_bits;
    write_buffer[1] = __builtin_bswap32(frame);

    for (uint8_t i = 0; i < txLen; i++) {
        write_buffer[2 + i] = __builtin_bswap32(txData[i]);
    }

    uint8_t crc = 0;
    crc ^= cmd;
    crc ^= recipient;
    crc ^= 0x00;
    crc ^= txLen;
    for (uint8_t i = 0; i < txLen; i++) {
        crc ^= (txData[i] >> 0) & 0xFF;
        crc ^= (txData[i] >> 8) & 0xFF;
        crc ^= (txData[i] >> 16) & 0xFF;
        crc ^= (txData[i] >> 24) & 0xFF;
    }
    write_buffer[2 + txLen] = (uint32_t)crc << 24;

    uint32_t words_to_send = 3 + txLen;

    auto prepare_input_capture = [&]() {
        pio_gpio_init(pio_in, pinA);
        pio_gpio_init(pio_in, pinA + 1);
        pio_sm_set_consecutive_pindirs(pio_in, sm_in, pinA, 2, false);
        gpio_pull_up(pinA);
        gpio_pull_up(pinB);

        pio_sm_set_enabled(pio_in, sm_in, false);
        pio_sm_clear_fifos(pio_in, sm_in);
        pio_sm_restart(pio_in, sm_in);
        pio_sm_exec(pio_in, sm_in, pio_encode_jmp(maple_in_offset));
        memset(read_buffer, 0, sizeof(read_buffer));
        dma_channel_set_write_addr(dma_read_channel, read_buffer, false);
        dma_channel_set_trans_count(dma_read_channel, MAPLE_MAX_PACKET_WORDS, false);
    };

    auto start_input_capture = [&]() {
        dma_channel_start(dma_read_channel);
        pio_sm_set_enabled(pio_in, sm_in, true);
    };

    {
        LatencyPhaseTraceScope mapleTxTrace(LATENCY_TRACE_PHASE_MAPLE_TX);
        releaseMaplePins(pinA, pinB);

        // Check bus is idle
        if (!gpio_get(pinA) || !gpio_get(pinB)) {
            maple_bus_busy++;
            return false;
        }

        // Pre-arm input capture before transmit.
        prepare_input_capture();

        // Configure and start output PIO
        pio_sm_set_enabled(pio_out, sm_out, false);
        pio_sm_clear_fifos(pio_out, sm_out);
        pio_sm_restart(pio_out, sm_out);
        pio_gpio_init(pio_out, pinA);
        pio_gpio_init(pio_out, pinA + 1);
        pio_sm_set_consecutive_pindirs(pio_out, sm_out, pinA, 2, true);
        pio_sm_exec(pio_out, sm_out, pio_encode_jmp(maple_out_offset));

        dma_channel_set_read_addr(dma_write_channel, write_buffer, false);
        dma_channel_set_trans_count(dma_write_channel, words_to_send, false);
        dma_channel_start(dma_write_channel);
        pio_sm_set_enabled(pio_out, sm_out, true);

        // Wait for transmission to complete
        uint32_t start = micros();
        bool txSawA = false;
        bool txSawB = false;

        auto sample_tx_levels = [&]() {
            if (!gpio_get(pinA)) txSawA = true;
            if (!gpio_get(pinB)) txSawB = true;
        };

        while (dma_channel_is_busy(dma_write_channel)) {
            sample_tx_levels();
            if (micros() - start > MAPLE_WRITE_TIMEOUT_US) {
                dma_channel_abort(dma_write_channel);
                pio_sm_set_enabled(pio_out, sm_out, false);
                // Release pins before returning
                gpio_set_function(pinA, GPIO_FUNC_SIO);
                gpio_set_function(pinB, GPIO_FUNC_SIO);
                gpio_set_dir(pinA, GPIO_IN);
                gpio_set_dir(pinB, GPIO_IN);
                gpio_pull_up(pinA);
                gpio_pull_up(pinB);
                maple_write_fail++;
                return false;
            }
        }
        // Wait for output PIO FIFO to drain
        while (!pio_sm_is_tx_fifo_empty(pio_out, sm_out)) {
            sample_tx_levels();
            if (micros() - start > MAPLE_WRITE_TIMEOUT_US) {
                pio_sm_set_enabled(pio_out, sm_out, false);
                // Release pins before returning
                gpio_set_function(pinA, GPIO_FUNC_SIO);
                gpio_set_function(pinB, GPIO_FUNC_SIO);
                gpio_set_dir(pinA, GPIO_IN);
                gpio_set_dir(pinB, GPIO_IN);
                gpio_pull_up(pinA);
                gpio_pull_up(pinB);
                maple_bus_busy++;
                return false;
            }
        }

        // Wait for TX end-sequence IRQ so we know pins have been released by
        // "set pindirs, 0" before handing ownership to RX.
        uint32_t end_wait_start = micros();
        while (!pio_interrupt_get(pio_out, sm_out)) {
            sample_tx_levels();
            if (micros() - end_wait_start > MAPLE_TX_END_TIMEOUT_US) {
                pio_sm_set_enabled(pio_out, sm_out, false);
                releaseMaplePins(pinA, pinB);
                maple_bus_busy++;
                return false;
            }
        }
        if (txSawA) maple_tx_low_seen_a++;
        if (txSawB) maple_tx_low_seen_b++;
        pio_interrupt_clear(pio_out, sm_out);

        pio_sm_set_enabled(pio_out, sm_out, false);

        if (MAPLE_RX_GUARD_US > 0) {
            delayMicroseconds(MAPLE_RX_GUARD_US);
        }

        // Hand GPIO mux back to RX PIO as soon as TX is done.
        pio_gpio_init(pio_in, pinA);
        pio_gpio_init(pio_in, pinA + 1);
        pio_sm_set_consecutive_pindirs(pio_in, sm_in, pinA, 2, false);
        gpio_pull_up(pinA);
        gpio_pull_up(pinB);

        // Start RX after TX has fully completed so we don't lock onto host TX.
        start_input_capture();
    }

    // Optional low-level Maple bus diagnostic. Keep disabled for normal input
    // polling because it adds measurable time before the response read.
#if MAPLE_ENABLE_POSTTX_GPIO_DIAGNOSTICS
    {
        LatencyPhaseTraceScope maplePostTxGpioTrace(LATENCY_TRACE_PHASE_MAPLE_POSTTX_GPIO);
        bool sawA = false;
        bool sawB = false;
        uint16_t edgesA = 0;
        uint16_t edgesB = 0;
        bool prevA = gpio_get(pinA);
        bool prevB = gpio_get(pinB);
        for (int i = 0; i < 500; i++) {
            bool curA = gpio_get(pinA);
            bool curB = gpio_get(pinB);
            if (!curA) sawA = true;
            if (!curB) sawB = true;
            if (curA != prevA) {
                edgesA++;
                prevA = curA;
            }
            if (curB != prevB) {
                edgesB++;
                prevB = curB;
            }
            if (sawA && sawB) break;
        }
        if (sawA) maple_gpio_activity_a++;
        if (sawB) maple_gpio_activity_b++;
        if (sawA || sawB) maple_gpio_activity++;
        maple_posttx_edges_a = edgesA;
        maple_posttx_edges_b = edgesB;
    }
#endif

    maple_write_ok++;

    // Now wait for response. If we detect a short/garbage frame, immediately
    // re-arm RX and keep listening within the same timeout window.
    uint32_t deadline = micros() + timeout_us;
    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
        uint32_t now_us = micros();
        if ((int32_t)(deadline - now_us) <= 0) {
            break;
        }

        uint32_t remaining_us = deadline - now_us;
        uint16_t bad_before = maple_bad_frame;
        bool read_ok = false;
        {
            LatencyPhaseTraceScope mapleReadTrace(LATENCY_TRACE_PHASE_MAPLE_READ);
            read_ok = readPacket(rxData, rxLen, remaining_us);
        }
        if (read_ok) {
            return true;
        }

        bool bad_frame_seen = (maple_bad_frame != bad_before);
        bool short_or_empty = (maple_dma_words <= 1);
        if (attempt >= 2 || (!bad_frame_seen && !short_or_empty)) {
            break;
        }

        prepare_input_capture();
        start_input_capture();
    }

    return false;
}

void MaplePort::update() {
    // Safety check - don't do anything if not initialized
    if (!initialized) return;

    uint32_t now = micros();

    // Poll connected pads promptly, but probe empty ports much more gently.
    // Empty-port Maple probes are real bus transactions and can disturb a live
    // Dreamcast controller/VMU when both ports share timing-sensitive PIO.
    const uint32_t poll_interval_us = connected ? 500 : 500000;
    if ((uint32_t)(now - last_poll_time) < poll_interval_us) return;
    last_poll_time = now;

    maple_update_calls++;  // Debug: count successful update calls
    status = MapleBusStatus::IDLE;

    constexpr uint8_t kConnectedFailureDisconnectCount = 60;
    auto register_failure = [this]() {
        if (consecutive_fail_count < 0xFF) {
            consecutive_fail_count++;
        }

        if (consecutive_fail_count > kConnectedFailureDisconnectCount) {
            connected = false;
            consecutive_fail_count = 0;
            status = MapleBusStatus::ERROR;
            last_seen_func = 0;

            // Clear stale state when we consider the controller disconnected.
            controller.buttons = 0xFFFF;
            controller.rtrigger = 0;
            controller.ltrigger = 0;
            controller.joyx = 128;
            controller.joyy = 128;
            controller.joyx2 = 128;
            controller.joyy2 = 128;
        }
    };

    // If not connected, try to detect device.
    // Probe one address per update to avoid saturating the bus while disconnected.
    if (!connected) {
        static const uint8_t probe_addrs[] = { 0x20, 0x40, 0x80 };
        uint8_t probe_count = sizeof(probe_addrs);
        uint8_t addr = probe_addrs[probe_addr_index % probe_count];
        probe_addr_index = (probe_addr_index + 1) % probe_count;
        maple_last_probe_recipient = addr;

        uint32_t response[64];
        uint8_t len = 0;
        uint32_t func = MAPLE_FUNC_CONTROLLER;
        if (!writeAndRead(MAPLE_CMD_REQUEST_DEVICE_INFO, addr, &func, 1,
                          response, &len, MAPLE_RESPONSE_TIMEOUT_US)) {
            last_response_cmd = MAPLE_CMD_NO_RESPONSE;
            status = MapleBusStatus::TIMEOUT;
        } else {
            uint8_t cmd = (response[0] >> 24) & 0xFF;
            last_response_cmd = cmd;
            if (cmd == MAPLE_CMD_DEVICE_INFO_RESPONSE && len >= 7) {
                maple_last_devinfo_recipient = (response[0] >> 16) & 0xFF;
                uint8_t response_sender = (response[0] >> 8) & 0xFF;
                maple_last_devinfo_sender = response_sender;
                maple_last_devinfo_len = response[0] & 0xFF;
                // Parse device info. Some builds/paths can still produce payload words
                // with opposite byte order; normalize so controller detection is robust.
                uint32_t func_native = response[1];
                uint32_t func_swapped = __builtin_bswap32(func_native);
                maple_last_devinfo_func_native = func_native;
                maple_last_devinfo_func_swapped = func_swapped;
                bool native_is_controller = (func_native & MAPLE_FUNC_CONTROLLER) != 0;
                bool swapped_is_controller = (func_swapped & MAPLE_FUNC_CONTROLLER) != 0;
                bool use_swapped = !native_is_controller && swapped_is_controller;

                device_info.func = use_swapped ? func_swapped : func_native;
                last_seen_func = device_info.func;
                device_info.func_data[0] = use_swapped ? __builtin_bswap32(response[2]) : response[2];
                device_info.func_data[1] = use_swapped ? __builtin_bswap32(response[3]) : response[3];
                device_info.func_data[2] = use_swapped ? __builtin_bswap32(response[4]) : response[4];
                device_info.area_code = mapleInfoByte(response, len, use_swapped, 0);
                device_info.connector_direction = mapleInfoByte(response, len, use_swapped, 1);
                copyMapleTextField(device_info.product_name, sizeof(device_info.product_name),
                                   response, len, use_swapped, 2, 30);
                copyMapleTextField(device_info.license, sizeof(device_info.license),
                                   response, len, use_swapped, 32, 60);
                device_info.standby_power = mapleInfoBe16(response, len, use_swapped, 92);
                device_info.max_power = mapleInfoBe16(response, len, use_swapped, 94);

                if (device_info.func & MAPLE_FUNC_CONTROLLER) {
                    connected = true;
                    uint8_t candidate_addr = response_sender;
                    if (candidate_addr != 0x20 && candidate_addr != 0x40 && candidate_addr != 0x80) {
                        candidate_addr = addr;
                    }
                    device_addr = candidate_addr;
                    consecutive_fail_count = 0;
                    status = MapleBusStatus::READ_COMPLETE;
                    probe_addr_index = 0;
                    reverse_condition_words = false;
                    condition_layout_locked = false;
                    condition_layout_score_normal = 0;
                    condition_layout_score_reversed = 0;
                    condition_layout_samples = 0;
                    maple_last_condition_layout = 0;
                    last_vmu_probe_time = 0;
                    vmu_probe_slot = 0;
                    pending_accessory_rescan = true;
                } else {
                    status = MapleBusStatus::ERROR;
                }
            } else {
                status = MapleBusStatus::ERROR;
            }
        }
    }

    // If connected, poll controller
    if (connected) {
        // Send get condition command
        uint32_t func = MAPLE_FUNC_CONTROLLER;
        uint32_t response[64];
        uint8_t len = 0;

        // Use combined write+read to avoid timing gap
        if (writeAndRead(MAPLE_CMD_GET_CONDITION, device_addr, &func, 1,
                         response, &len, MAPLE_RESPONSE_TIMEOUT_US)) {
            // Data is in native format (byte-swapped by readPacket)
            uint8_t cmd = (response[0] >> 24) & 0xFF;
            last_response_cmd = cmd;
            if (cmd == MAPLE_CMD_DATA_TRANSFER && len >= 3) {
                uint8_t sender = (response[0] >> 8) & 0xFF;
                if (sender == 0x20 || sender == 0x40 || sender == 0x80) {
                    device_addr = sender;
                }

                uint32_t cond_func_native = response[1];
                uint32_t cond_func_swapped = __builtin_bswap32(cond_func_native);
                const uint32_t accessory_mask = (MAPLE_FUNC_MEMORY | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK |
                                                 MAPLE_FUNC_PURUPURU | MAPLE_FUNC_MOUSE | MAPLE_FUNC_KEYBOARD |
                                                 MAPLE_FUNC_MICROPHONE | MAPLE_FUNC_AR_GUN | MAPLE_FUNC_LIGHT_GUN);
                bool has_controller_func =
                    ((cond_func_native & MAPLE_FUNC_CONTROLLER) != 0) ||
                    ((cond_func_swapped & MAPLE_FUNC_CONTROLLER) != 0);
                if (!has_controller_func) {
                    // VMU/peripheral chatter can appear while a pad is connected.
                    // Ignore non-controller frames so they don't break controller lock
                    // or suppress periodic accessory probing.
                    uint32_t accessory_func = (cond_func_native & accessory_mask) ? cond_func_native : cond_func_swapped;
                    uint8_t sender = (response[0] >> 8) & 0xFF;
                    uint8_t slot = 0xFF;
                    if (sender & 0x01) slot = 0;
                    else if (sender & 0x02) slot = 1;
                    if (slot < 2 && (accessory_func & accessory_mask)) {
                        vmu_info[slot].func = accessory_func;
                        vmu_info[slot].present = (accessory_func & (MAPLE_FUNC_MEMORY | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK)) != 0;
                    }
                    pending_accessory_rescan = true;
                    status = MapleBusStatus::READ_COMPLETE;
                } else {
                    last_seen_func = (cond_func_native & MAPLE_FUNC_CONTROLLER) ? cond_func_native : cond_func_swapped;

                    // Parse controller condition (Dreamcast format):
                    // response[1] = function code
                    // response[2] = buttons + triggers
                    // response[3] = analog sticks
                    //
                    // Some hardware paths can deliver payload words with reversed byte order
                    // while still producing a valid frame header. Score both layouts and
                    // lock the better one per connection.
                    uint32_t data1 = response[2];
                    uint32_t data2 = (len >= 4) ? response[3] : 0x80808080;

                    uint8_t d1b0 = (data1 >> 24) & 0xFF;
                    uint8_t d1b1 = (data1 >> 16) & 0xFF;
                    uint8_t d1b2 = (data1 >> 8) & 0xFF;
                    uint8_t d1b3 = data1 & 0xFF;

                    // Layout A (normal): [btn_lo][btn_hi][r][l]
                    uint16_t buttons_a = (uint16_t)d1b0 | ((uint16_t)d1b1 << 8);
                    uint8_t r_a = d1b2;
                    uint8_t l_a = d1b3;

                    // Layout B (reversed): [l][r][btn_hi][btn_lo]
                    uint16_t buttons_b = (uint16_t)d1b3 | ((uint16_t)d1b2 << 8);
                    uint8_t r_b = d1b1;
                    uint8_t l_b = d1b0;

                    auto score_layout = [](uint16_t buttons, uint8_t rtrig, uint8_t ltrig) -> uint8_t {
                        uint8_t ones = __builtin_popcount((uint32_t)(buttons & 0x0FFF));
                        uint8_t score = (uint8_t)(ones * 2);
                        if ((buttons & 0xF000) == 0xF000) score += 2;
                        if (ltrig < 224) score += 2;
                        if (rtrig < 224) score += 2;
                        if (ltrig < 32) score += 1;
                        if (rtrig < 32) score += 1;
                        return score;
                    };

                    uint8_t score_a = score_layout(buttons_a, r_a, l_a);
                    uint8_t score_b = score_layout(buttons_b, r_b, l_b);

                    if (!condition_layout_locked) {
                        condition_layout_score_normal += score_a;
                        condition_layout_score_reversed += score_b;
                        if (condition_layout_samples < 64) condition_layout_samples++;

                        int16_t diff = condition_layout_score_normal - condition_layout_score_reversed;
                        if (condition_layout_samples >= 4 && (diff >= 12 || diff <= -12)) {
                            reverse_condition_words = (diff < 0);
                            condition_layout_locked = true;
                        } else if (score_b > (uint8_t)(score_a + 8)) {
                            reverse_condition_words = true;
                        } else if (score_a > (uint8_t)(score_b + 8)) {
                            reverse_condition_words = false;
                        }
                    }

                    maple_last_condition_layout = reverse_condition_words ? 1 : 0;

                    uint8_t d2b0 = (data2 >> 24) & 0xFF;
                    uint8_t d2b1 = (data2 >> 16) & 0xFF;
                    uint8_t d2b2 = (data2 >> 8) & 0xFF;
                    uint8_t d2b3 = data2 & 0xFF;

                    if (!reverse_condition_words) {
                        controller.buttons = buttons_a;
                        controller.rtrigger = r_a;
                        controller.ltrigger = l_a;
                        controller.joyx = d2b0;
                        controller.joyy = d2b1;
                        controller.joyx2 = d2b2;
                        controller.joyy2 = d2b3;
                    } else {
                        controller.buttons = buttons_b;
                        controller.rtrigger = r_b;
                        controller.ltrigger = l_b;
                        controller.joyx = d2b3;
                        controller.joyy = d2b2;
                        controller.joyx2 = d2b1;
                        controller.joyy2 = d2b0;
                    }

                    status = MapleBusStatus::READ_COMPLETE;
                    consecutive_fail_count = 0;
                }
            } else if (cmd == MAPLE_CMD_NO_RESPONSE || cmd == MAPLE_CMD_ERROR) {
                status = MapleBusStatus::ERROR;
                register_failure();
            } else {
                status = MapleBusStatus::ERROR;
                register_failure();
            }
        } else {
            // No response - controller may be disconnected
            last_response_cmd = MAPLE_CMD_NO_RESPONSE;
            status = MapleBusStatus::TIMEOUT;
            register_failure();
        }

        // Controller condition frames can already advertise accessory functions.
        // Avoid active background VMU probes here: those extra Maple transactions
        // can steal timing from normal controller polling and make the pad appear
        // disconnected. Serial VMU tools perform explicit scans when needed.
        constexpr uint32_t kAccessoryProbeIntervalMs = 2000;
        if (connected && (pending_accessory_rescan || (now - last_vmu_probe_time) >= kAccessoryProbeIntervalMs)) {
            pending_accessory_rescan = false;
            vmu_probe_slot = 0;
            last_vmu_probe_time = now;
        }
    }
}

namespace {
uint32_t memoryFunctionDefinition(uint32_t func, const uint32_t* func_data) {
    if ((func & MAPLE_FUNC_MEMORY) == 0 || func_data == nullptr) {
        return 0;
    }

    uint8_t index = 0;
    for (int8_t bit = 31; bit >= 0; --bit) {
        const uint32_t mask = (uint32_t)1 << bit;
        if ((func & mask) == 0) {
            continue;
        }
        if (mask == MAPLE_FUNC_MEMORY) {
            return (index < 3) ? func_data[index] : 0;
        }
        ++index;
        if (index >= 3) {
            break;
        }
    }
    return 0;
}

void applyMemoryFunctionDefinition(VMUInfo& info, uint32_t definition) {
    if (definition == 0) {
        return;
    }

    const uint8_t partitions = ((definition >> 24) & 0xFF) + 1;
    const uint16_t block_size = (uint16_t)(((definition >> 16) & 0xFF) + 1) << 5;
    const uint8_t write_count = (definition >> 12) & 0x0F;
    const uint8_t read_count = (definition >> 8) & 0x0F;

    if (partitions != 0) {
        info.partitions = partitions;
    }
    if (block_size != 0) {
        info.blockSize = block_size;
    }
    if (write_count != 0) {
        info.writeAccess = write_count;
    }
    if (read_count != 0) {
        info.readAccess = read_count;
    }
}
}

// VMU functions
bool MaplePort::queryVMU(uint8_t slot) {
    if (!connected || slot > 1) return false;

    uint8_t root = (device_addr == 0x20 || device_addr == 0x40 || device_addr == 0x80) ? device_addr : 0x20;
    uint8_t slot_bit = (slot & 1) ? 0x02 : 0x01;
    // Some devices/adapters respond to root|slot, others to slot-only.
    uint8_t addrs[2] = { (uint8_t)(root | slot_bit), slot_bit };

    const uint32_t accessory_mask = (MAPLE_FUNC_MEMORY | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK |
                                     MAPLE_FUNC_PURUPURU | MAPLE_FUNC_MOUSE | MAPLE_FUNC_KEYBOARD |
                                     MAPLE_FUNC_MICROPHONE | MAPLE_FUNC_AR_GUN | MAPLE_FUNC_LIGHT_GUN);
    const uint32_t known_mask = accessory_mask | MAPLE_FUNC_CONTROLLER;

    // Step 1: DEVICE_INFO probe on both addressing styles.
    // Try both 0-word and 1-word payload styles for compatibility.
    const uint32_t req_func = MAPLE_FUNC_MEMORY;
    for (uint8_t i = 0; i < 2; i++) {
        maple_last_probe_recipient = addrs[i];
        for (uint8_t pass = 0; pass < 2; pass++) {
            uint32_t response[64];
            uint8_t len = 0;
            const uint32_t* tx = (pass == 0) ? nullptr : &req_func;
            uint8_t tx_len = (pass == 0) ? 0 : 1;
            if (!writeAndRead(MAPLE_CMD_REQUEST_DEVICE_INFO, addrs[i], tx, tx_len,
                              response, &len, MAPLE_RESPONSE_TIMEOUT_US)) {
                continue;
            }

            uint8_t cmd = (response[0] >> 24) & 0xFF;
            if (cmd != MAPLE_CMD_DEVICE_INFO_RESPONSE || len < 7) continue;

            uint32_t func_native = response[1];
            uint32_t func_swapped = __builtin_bswap32(func_native);
            bool native_known = (func_native & known_mask) != 0;
            bool swapped_known = (func_swapped & known_mask) != 0;

            uint32_t func = native_known && !swapped_known ? func_native :
                            swapped_known && !native_known ? func_swapped :
                            func_native;

            if (func & accessory_mask) {
                uint32_t func_data[3] = { response[2], response[3], response[4] };
                if (swapped_known && !native_known) {
                    func_data[0] = __builtin_bswap32(func_data[0]);
                    func_data[1] = __builtin_bswap32(func_data[1]);
                    func_data[2] = __builtin_bswap32(func_data[2]);
                }
                vmu_info[slot].func = func;
                vmu_info[slot].present = (func & (MAPLE_FUNC_MEMORY | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK)) != 0;
                applyMemoryFunctionDefinition(vmu_info[slot], memoryFunctionDefinition(func, func_data));
                return true;
            }
        }
    }

    // Step 2: fallback memory probe. Some setups return controller-only info at slot addresses,
    // but still answer GET_MEMORY_INFO when a VMU is actually present.
    for (uint8_t i = 0; i < 2; i++) {
        uint32_t req[2] = { MAPLE_FUNC_MEMORY, 0 };
        uint32_t response[64];
        uint8_t len = 0;
        if (!writeAndRead(MAPLE_CMD_GET_MEMORY_INFO, addrs[i], req, 2,
                          response, &len, MAPLE_RESPONSE_TIMEOUT_US)) {
            continue;
        }

        uint8_t cmd = (response[0] >> 24) & 0xFF;
        if (cmd == MAPLE_CMD_DATA_TRANSFER && len >= 3) {
            uint32_t info_native = response[2];
            uint32_t info_swapped = __builtin_bswap32(info_native);
            uint16_t blocks_native = (info_native >> 16) & 0xFF;
            uint16_t blocks_swapped = (info_swapped >> 16) & 0xFF;
            uint16_t blocks = blocks_native ? blocks_native : blocks_swapped;
            if (blocks == 0) blocks = 256;

            vmu_info[slot].present = true;
            vmu_info[slot].func = MAPLE_FUNC_MEMORY;
            vmu_info[slot].totalBlocks = blocks;
            if (vmu_info[slot].blockSize == 0) vmu_info[slot].blockSize = 512;
            if (vmu_info[slot].readAccess == 0) vmu_info[slot].readAccess = 1;
            if (vmu_info[slot].writeAccess == 0) vmu_info[slot].writeAccess = 4;
            return true;
        }
    }

    if (vmu_info[slot].present) {
        if (vmu_info[slot].totalBlocks == 0) vmu_info[slot].totalBlocks = 256;
        if (vmu_info[slot].blockSize == 0) vmu_info[slot].blockSize = 512;
        if (vmu_info[slot].readAccess == 0) vmu_info[slot].readAccess = 1;
        if (vmu_info[slot].writeAccess == 0) vmu_info[slot].writeAccess = 4;
    }
    return vmu_info[slot].present;
}

bool MaplePort::getVMUMemoryInfo(uint8_t slot) {
    if (slot > 1) return false;

    uint8_t root = (device_addr == 0x20 || device_addr == 0x40 || device_addr == 0x80) ? device_addr : 0x20;
    uint8_t slot_bit = (slot & 1) ? 0x02 : 0x01;
    uint8_t addrs[2] = { (uint8_t)(root | slot_bit), slot_bit };

    for (uint8_t i = 0; i < 2; i++) {
        uint32_t cmd_data[2] = { MAPLE_FUNC_MEMORY, 0 };
        uint32_t response[64];
        uint8_t len = 0;
        if (!writeAndRead(MAPLE_CMD_GET_MEMORY_INFO, addrs[i], cmd_data, 2,
                          response, &len, MAPLE_RESPONSE_TIMEOUT_US)) {
            continue;
        }

        uint8_t cmd = (response[0] >> 24) & 0xFF;
        if (cmd == MAPLE_CMD_DATA_TRANSFER && len >= 3) {
            uint32_t info_native = response[2];
            uint32_t info_swapped = __builtin_bswap32(info_native);
            uint16_t blocks_native = (info_native >> 16) & 0xFF;
            uint16_t blocks_swapped = (info_swapped >> 16) & 0xFF;
            uint16_t blocks = blocks_native ? blocks_native : blocks_swapped;
            if (blocks == 0) blocks = 256;

            vmu_info[slot].present = true;
            vmu_info[slot].func |= MAPLE_FUNC_MEMORY;
            vmu_info[slot].totalBlocks = blocks;
            if (vmu_info[slot].blockSize == 0) vmu_info[slot].blockSize = 512;
            if (vmu_info[slot].readAccess == 0) vmu_info[slot].readAccess = 1;
            if (vmu_info[slot].writeAccess == 0) vmu_info[slot].writeAccess = 4;
            return true;
        }
    }

    if (vmu_info[slot].present) {
        if (vmu_info[slot].totalBlocks == 0) vmu_info[slot].totalBlocks = 256;
        if (vmu_info[slot].blockSize == 0) vmu_info[slot].blockSize = 512;
        if (vmu_info[slot].readAccess == 0) vmu_info[slot].readAccess = 1;
        if (vmu_info[slot].writeAccess == 0) vmu_info[slot].writeAccess = 4;
    }
    return vmu_info[slot].present;
}

VMUBlockResult MaplePort::readVMUBlock(uint8_t slot, uint16_t block, uint8_t* buffer) {
    if (!vmu_info[slot].present) return VMUBlockResult::NOT_PRESENT;
    if (block >= 256) return VMUBlockResult::INVALID_BLOCK;

    // Serial VMU streaming reads many blocks back-to-back. Keep the passive
    // accessory probe from racing the active read and clearing cached VMU state
    // on one transient timeout.
    last_vmu_probe_time = millis();

    uint8_t root = (device_addr == 0x20 || device_addr == 0x40 || device_addr == 0x80) ? device_addr : 0x20;
    uint8_t slot_bit = (slot & 1) ? 0x02 : 0x01;
    uint8_t addrs[2] = { (uint8_t)(root | slot_bit), slot_bit };

    VMUBlockResult last_error = VMUBlockResult::READ_ERROR;
    for (uint8_t ai = 0; ai < 2; ai++) {
        uint8_t addr = addrs[ai];
        bool ok = true;

        // Standard VMUs return a whole 512-byte block in one read access:
        // payload = func, location, 128 data words.
        for (uint8_t phase = 0; phase < 1; phase++) {
            uint32_t cmd_data[2];
            cmd_data[0] = MAPLE_FUNC_MEMORY;
            cmd_data[1] = ((uint32_t)phase << 16) | block;  // partition 0 + phase + block
            maple_last_vmu_addr = addr;
            maple_last_vmu_phase = phase;
            maple_last_vmu_block = block;
            maple_last_vmu_cmd = 0;
            maple_last_vmu_len = 0;
            maple_last_vmu_word0 = 0;
            maple_last_vmu_word1 = 0;
            maple_last_vmu_word2 = 0;

            uint32_t response[MAPLE_MAX_PACKET_WORDS];
            uint8_t len = 0;
            if (!writeAndRead(MAPLE_CMD_BLOCK_READ, addr, cmd_data, 2,
                              response, &len, MAPLE_RESPONSE_TIMEOUT_US * 6)) {
                last_error = VMUBlockResult::TIMEOUT;
                ok = false;
                break;
            }

            uint8_t cmd = (response[0] >> 24) & 0xFF;  // cmd is at bits 31-24
            maple_last_vmu_cmd = cmd;
            maple_last_vmu_len = len;
            maple_last_vmu_word0 = response[0];
            maple_last_vmu_word1 = (len > 1) ? response[1] : 0;
            maple_last_vmu_word2 = (len > 2) ? response[2] : 0;
            if (cmd != MAPLE_CMD_DATA_TRANSFER || len < 131) {
                last_error = VMUBlockResult::READ_ERROR;
                ok = false;
                break;
            }

            // response[1] is func and response[2] is the echoed location. Copy
            // the 128 payload words with byte order restored to raw VMU bytes.
            for (uint16_t i = 0; i < 128; ++i) {
                uint32_t word = __builtin_bswap32(response[3 + i]);
                memcpy(&buffer[i * 4], &word, sizeof(word));
            }
        }

        if (ok) {
            last_vmu_probe_time = millis();
            return VMUBlockResult::SUCCESS;
        }
    }

    last_vmu_probe_time = millis();
    return last_error;
}

VMUBlockResult MaplePort::writeVMUBlock(uint8_t slot, uint16_t block, const uint8_t* buffer) {
    if (!vmu_info[slot].present) return VMUBlockResult::NOT_PRESENT;
    if (block >= 256) return VMUBlockResult::INVALID_BLOCK;

    uint8_t write_count = vmu_info[slot].writeAccess ? vmu_info[slot].writeAccess : 4;
    if (write_count == 0 || write_count > 8) {
        write_count = 4;
    }
    uint16_t block_size = vmu_info[slot].blockSize ? vmu_info[slot].blockSize : 512;
    if (block_size == 512 && (write_count == 0 || (block_size / write_count) != 128)) {
        // Stock VMUs advertise 512-byte flash blocks written as four 128-byte
        // phases. Some controller paths expose ambiguous function_data; never
        // let that collapse a VMU write into one invalid 512-byte phase.
        write_count = 4;
    }
    maple_last_vmu_write_count = write_count;
    if (block_size != 512 || (block_size % write_count) != 0) {
        maple_last_vmu_stage = 3;
        maple_last_vmu_phase_bytes = 0;
        return VMUBlockResult::WRITE_ERROR;
    }
    const uint16_t phase_bytes = block_size / write_count;
    if (phase_bytes == 0 || phase_bytes > 512 || (phase_bytes % 4) != 0) {
        maple_last_vmu_stage = 3;
        maple_last_vmu_phase_bytes = phase_bytes;
        return VMUBlockResult::WRITE_ERROR;
    }
    maple_last_vmu_phase_bytes = phase_bytes;
    const uint8_t phase_words = phase_bytes / 4;

    uint8_t root = (device_addr == 0x20 || device_addr == 0x40 || device_addr == 0x80) ? device_addr : 0x20;
    uint8_t slot_bit = (slot & 1) ? 0x02 : 0x01;
    uint8_t addrs[2] = { (uint8_t)(root | slot_bit), slot_bit };

    VMUBlockResult last_error = VMUBlockResult::WRITE_ERROR;
    for (uint8_t ai = 0; ai < 2; ai++) {
        uint8_t addr = addrs[ai];
        bool ok = true;

        // Write in the VMU-advertised number of phases. Stock VMUs use four
        // 128-byte phases, but honoring the function definition keeps this
        // safe for compatible memory devices too.
        for (uint8_t phase = 0; phase < write_count; phase++) {
            uint32_t cmd_data[130];
            cmd_data[0] = MAPLE_FUNC_MEMORY;
            cmd_data[1] = ((uint32_t)phase << 16) | block;
            maple_last_vmu_stage = 1;
            maple_last_vmu_addr = addr;
            maple_last_vmu_phase = phase;
            maple_last_vmu_block = block;
            maple_last_vmu_cmd = MAPLE_CMD_BLOCK_WRITE;
            maple_last_vmu_len = 0;
            maple_last_vmu_word0 = 0;
            maple_last_vmu_word1 = 0;
            maple_last_vmu_word2 = 0;

            for (uint8_t word_index = 0; word_index < phase_words; ++word_index) {
                uint32_t word = 0;
                memcpy(&word, &buffer[phase * phase_bytes + word_index * sizeof(word)], sizeof(word));
                cmd_data[2 + word_index] = __builtin_bswap32(word);
            }

            uint32_t response[MAPLE_MAX_PACKET_WORDS];
            uint8_t len = 0;
            if (!writeAndRead(MAPLE_CMD_BLOCK_WRITE, addr, cmd_data, phase_words + 2,
                              response, &len, MAPLE_VMU_WRITE_RESPONSE_TIMEOUT_US)) {
                last_error = VMUBlockResult::TIMEOUT;
                maple_last_vmu_stage = 4;
                ok = false;
                break;
            }

            uint8_t cmd = (response[0] >> 24) & 0xFF;  // cmd is at bits 31-24
            maple_last_vmu_cmd = cmd;
            maple_last_vmu_len = len;
            maple_last_vmu_word0 = response[0];
            maple_last_vmu_word1 = (len > 1) ? response[1] : 0;
            maple_last_vmu_word2 = (len > 2) ? response[2] : 0;
            if (cmd != MAPLE_CMD_ACK) {
                last_error = VMUBlockResult::WRITE_ERROR;
                maple_last_vmu_stage = 5;
                ok = false;
                break;
            }

            delay(MAPLE_VMU_WRITE_PHASE_DELAY_MS);
        }

        if (ok) {
            // Per Maple storage FT1, the final write phase only stages the
            // block. A Get Last Error request with the next phase commits the
            // staged block from VMU RAM into flash.
            uint32_t commit_data[2];
            commit_data[0] = MAPLE_FUNC_MEMORY;
            commit_data[1] = ((uint32_t)write_count << 16) | block;
            maple_last_vmu_stage = 2;
            maple_last_vmu_addr = addr;
            maple_last_vmu_phase = write_count;
            maple_last_vmu_block = block;
            maple_last_vmu_cmd = MAPLE_CMD_GET_LAST_ERROR;
            maple_last_vmu_len = 0;
            maple_last_vmu_word0 = 0;
            maple_last_vmu_word1 = 0;
            maple_last_vmu_word2 = 0;

            uint32_t response[MAPLE_MAX_PACKET_WORDS];
            uint8_t len = 0;
            if (!writeAndRead(MAPLE_CMD_GET_LAST_ERROR, addr, commit_data, 2,
                              response, &len, MAPLE_VMU_WRITE_RESPONSE_TIMEOUT_US)) {
                last_error = VMUBlockResult::TIMEOUT;
                maple_last_vmu_stage = 4;
                ok = false;
            } else {
                uint8_t cmd = (response[0] >> 24) & 0xFF;
                maple_last_vmu_cmd = cmd;
                maple_last_vmu_len = len;
                maple_last_vmu_word0 = response[0];
                maple_last_vmu_word1 = (len > 1) ? response[1] : 0;
                maple_last_vmu_word2 = (len > 2) ? response[2] : 0;
                if (cmd != MAPLE_CMD_ACK) {
                    last_error = VMUBlockResult::WRITE_ERROR;
                    maple_last_vmu_stage = 5;
                    ok = false;
                }
            }
        }

        if (ok) return VMUBlockResult::SUCCESS;
    }

    return last_error;
}

uint32_t MaplePort::readVMUAll(uint8_t slot, uint8_t* buffer, uint32_t bufferSize) {
    if (!vmu_info[slot].present) return 0;

    uint32_t bytesRead = 0;
    uint16_t totalBlocks = (bufferSize < 128 * 1024) ? bufferSize / 512 : 256;

    for (uint16_t block = 0; block < totalBlocks; block++) {
        if (readVMUBlock(slot, block, &buffer[block * 512]) != VMUBlockResult::SUCCESS) {
            break;
        }
        bytesRead += 512;
    }

    return bytesRead;
}

uint32_t MaplePort::writeVMUAll(uint8_t slot, const uint8_t* buffer, uint32_t dataSize) {
    if (!vmu_info[slot].present) return 0;

    uint32_t bytesWritten = 0;
    uint16_t totalBlocks = dataSize / 512;
    if (totalBlocks > 256) totalBlocks = 256;

    for (uint16_t block = 0; block < totalBlocks; block++) {
        if (writeVMUBlock(slot, block, &buffer[block * 512]) != VMUBlockResult::SUCCESS) {
            break;
        }
        bytesWritten += 512;
    }

    return bytesWritten;
}

// ============= Debug Functions =============

bool MaplePort::testGPIO() {
    // Temporarily disable PIO if initialized
    bool wasInit = initialized;
    if (wasInit) {
        pio_sm_set_enabled(pio_out, sm_out, false);
        pio_sm_set_enabled(pio_in, sm_in, false);
    }

    // Save current pin configuration
    bool success = true;

    // Test pin A - set as output, toggle, read back
    gpio_set_dir(pinA, GPIO_OUT);
    gpio_put(pinA, 1);
    delayMicroseconds(10);
    // Can't easily read back our own output on RP2040 without special config
    // So just test that we can drive the pin low and it goes low (with pull-up)

    gpio_set_dir(pinA, GPIO_IN);
    gpio_pull_up(pinA);
    delayMicroseconds(10);
    if (!gpio_get(pinA)) {
        success = false;  // Pin A stuck low
    }

    // Test pin B
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinB);
    delayMicroseconds(10);
    if (!gpio_get(pinB)) {
        success = false;  // Pin B stuck low
    }

    // Restore pin configuration
    gpio_set_dir(pinA, GPIO_IN);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinA);
    gpio_pull_up(pinB);

    // Re-enable PIO if it was initialized
    if (wasInit) {
        // Don't re-enable - let next poll restart it
    }

    return success;
}

uint8_t MaplePort::debugProbe() {
    if (!initialized) return 0;

    // Try sending a device info request and see if we get any response
    uint32_t func = MAPLE_FUNC_CONTROLLER;
    if (!writePacket(MAPLE_CMD_REQUEST_DEVICE_INFO, 0x20, &func, 1)) {
        return 0;  // Write failed
    }

    uint32_t response[64];
    uint8_t len = 0;

    if (readPacket(response, &len, MAPLE_RESPONSE_TIMEOUT_US)) {
        return len;  // Return number of words received
    }

    return 0;  // No response
}

bool MaplePort::probeDeviceInfo(uint8_t recipient, uint32_t* funcNative, uint32_t* funcSwapped,
                                uint8_t* sender, uint8_t* responseLen) {
    if (!initialized) return false;

    uint32_t response[64];
    uint8_t len = 0;
    uint32_t func = MAPLE_FUNC_CONTROLLER;
    if (!writeAndRead(MAPLE_CMD_REQUEST_DEVICE_INFO, recipient, &func, 1,
                      response, &len, MAPLE_RESPONSE_TIMEOUT_US)) {
        last_response_cmd = MAPLE_CMD_NO_RESPONSE;
        status = MapleBusStatus::TIMEOUT;
        return false;
    }

    uint8_t cmd = (response[0] >> 24) & 0xFF;
    last_response_cmd = cmd;
    if (cmd != MAPLE_CMD_DEVICE_INFO_RESPONSE || len < 7) {
        status = MapleBusStatus::ERROR;
        return false;
    }

    uint8_t response_sender = (response[0] >> 8) & 0xFF;
    uint32_t native = response[1];
    uint32_t swapped = __builtin_bswap32(native);
    maple_last_devinfo_recipient = (response[0] >> 16) & 0xFF;
    maple_last_devinfo_sender = response_sender;
    maple_last_devinfo_len = response[0] & 0xFF;
    maple_last_devinfo_func_native = native;
    maple_last_devinfo_func_swapped = swapped;
    status = MapleBusStatus::READ_COMPLETE;

    if (funcNative) *funcNative = native;
    if (funcSwapped) *funcSwapped = swapped;
    if (sender) *sender = response_sender;
    if (responseLen) *responseLen = len;
    return true;
}
