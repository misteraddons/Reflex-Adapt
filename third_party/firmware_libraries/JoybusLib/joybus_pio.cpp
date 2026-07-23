#include <Arduino.h>
#include <hardware/dma.h>

#include "joybus_pio.hpp"
#include "joybus_long_write.pio.h"
#include "joybus_private.pio.h"
#include "../../../firmware/platform/latency_trace_gpio.h"

#define PAYLOAD_PACKET_MAX 3
#define PAYLOAD_DMA_WORD_MAX 16
#define OFFSET_NOT_LOADED 0xFFFFFFFF
#define PIO_INDEX(instance) ((instance.pio == pio0) ? 0 : 1)

uint joybus_pio_offsets[2] = {OFFSET_NOT_LOADED, OFFSET_NOT_LOADED};
uint joybus_long_write_offset = OFFSET_NOT_LOADED;

uint32_t joybus_pio_timeout_ms = 10;

JoybusPIOInstance joybus_pio_program_init(PIO _pio, uint _sm, uint _pin) {
  JoybusPIOInstance instance;
  instance.pio = _pio;
  instance.sm = _sm;
  instance.pin = _pin;
  instance.offset = joybus_pio_offsets[PIO_INDEX(instance)];
  if (instance.offset == OFFSET_NOT_LOADED) {
    instance.offset = pio_add_program(instance.pio, &joybus_pio_program);
    joybus_pio_offsets[PIO_INDEX(instance)] = instance.offset;
  }

  pio_sm_set_enabled(instance.pio, instance.sm, false);

  gpio_set_dir(instance.pin, GPIO_IN);
  gpio_disable_pulls(instance.pin);
  gpio_set_oeover(instance.pin, GPIO_OVERRIDE_HIGH);
  gpio_set_outover(instance.pin, GPIO_OVERRIDE_LOW);

  instance.config = joybus_pio_program_get_default_config(instance.offset);
  sm_config_set_in_pins(&instance.config, instance.pin);
  sm_config_set_out_pins(&instance.config, instance.pin, 1);
  sm_config_set_set_pins(&instance.config, instance.pin, 1);

  sm_config_set_out_shift(&instance.config, false, false, 32);
  sm_config_set_in_shift(&instance.config, false, true, 32);

  float frac = (clock_get_hz(clk_sys) / 1000000) / 16;
  sm_config_set_clkdiv(&instance.config, frac);

  pio_gpio_init(instance.pio, instance.pin);

  joybus_pio_reset(instance);

  return instance;
}

void joybus_pio_reset(JoybusPIOInstance instance) {
  pio_sm_set_enabled(instance.pio, instance.sm, false);
  pio_sm_init(instance.pio, instance.sm, instance.offset, &instance.config);
  pio_sm_set_consecutive_pindirs(instance.pio, instance.sm, instance.pin, 1,
                                 false);
  pio_sm_clear_fifos(instance.pio, instance.sm);
  pio_sm_restart(instance.pio, instance.sm);
  pio_sm_set_enabled(instance.pio, instance.sm, true);
}

void joybus_pio_unload(PIO pio) {
  uint8_t idx = (pio == pio0) ? 0 : 1;
  uint offset = joybus_pio_offsets[idx];
  if (offset != OFFSET_NOT_LOADED) {
    pio_remove_program(pio, &joybus_pio_program, offset);
    joybus_pio_offsets[idx] = OFFSET_NOT_LOADED;
  }
  if (pio == pio1 && joybus_long_write_offset != OFFSET_NOT_LOADED) {
    pio_remove_program(pio1, &joybus_long_write_program,
                       joybus_long_write_offset);
    joybus_long_write_offset = OFFSET_NOT_LOADED;
  }
}

static bool init_long_write_sm(JoybusPIOInstance instance,
                               JoybusPIOInstance *long_instance) {
  if (joybus_long_write_offset == OFFSET_NOT_LOADED) {
    if (!pio_can_add_program(pio1, &joybus_long_write_program)) {
      return false;
    }
    joybus_long_write_offset =
      pio_add_program(pio1, &joybus_long_write_program);
  }

  long_instance->pio = pio1;
  long_instance->sm = instance.sm;
  long_instance->pin = instance.pin;
  long_instance->offset = joybus_long_write_offset;
  long_instance->config =
    joybus_long_write_program_get_default_config(joybus_long_write_offset);
  sm_config_set_in_pins(&long_instance->config, instance.pin);
  sm_config_set_out_pins(&long_instance->config, instance.pin, 1);
  sm_config_set_set_pins(&long_instance->config, instance.pin, 1);
  sm_config_set_out_shift(&long_instance->config, false, false, 9);
  sm_config_set_in_shift(&long_instance->config, false, true, 8);
  constexpr int cycles_per_bit = 40;
  const float divider = clock_get_hz(clk_sys) /
                        (cycles_per_bit * 250000.0f);
  sm_config_set_clkdiv(&long_instance->config, divider);
  return true;
}

static int transmit_receive_long_write(JoybusPIOInstance instance,
                                       const uint8_t *payload,
                                       int payload_len,
                                       uint8_t *response,
                                       int response_len) {
  constexpr int kLongWriteBadRequest = -101;
  constexpr int kLongWriteNoProgramSpace = -102;
  constexpr int kLongWriteNoDmaChannel = -103;
  constexpr int kLongWriteDmaTimeout = -104;
  constexpr int kLongWriteResponseTimeout = -105;
  constexpr int kPayloadBytes = 35;
  if (payload_len != kPayloadBytes) {
    return kLongWriteBadRequest;
  }

  JoybusPIOInstance long_instance;
  if (!init_long_write_sm(instance, &long_instance)) {
    return kLongWriteNoProgramSpace;
  }

  uint32_t words[kPayloadBytes] = {0};
  for (int byte = 0; byte < payload_len; ++byte) {
    const uint8_t inverted_payload = (uint8_t)~payload[byte];
    const uint32_t continue_marker = byte + 1 < payload_len ? 1u : 0u;
    words[byte] = ((uint32_t)inverted_payload << 24) |
                  (continue_marker << 23);
  }

  pio_sm_set_enabled(instance.pio, instance.sm, false);
  pio_sm_set_enabled(pio1, long_instance.sm, false);
  gpio_set_dir(instance.pin, GPIO_IN);
  gpio_disable_pulls(instance.pin);
  gpio_set_oeover(instance.pin, GPIO_OVERRIDE_HIGH);
  gpio_set_outover(instance.pin, GPIO_OVERRIDE_LOW);
  pio_gpio_init(pio1, instance.pin);
  pio_sm_init(pio1, long_instance.sm,
              long_instance.offset + joybus_long_write_offset_write,
              &long_instance.config);
  pio_sm_set_consecutive_pindirs(pio1, long_instance.sm, instance.pin, 1,
                                 false);
  pio_sm_clear_fifos(pio1, long_instance.sm);
  pio_sm_restart(pio1, long_instance.sm);
  pio_sm_set_enabled(pio1, long_instance.sm, true);

  int result = kLongWriteNoDmaChannel;
  const int channel = dma_claim_unused_channel(false);
  if (channel >= 0) {
    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, pio_get_dreq(pio1, long_instance.sm, true));
    dma_channel_configure(channel, &config, &pio1->txf[long_instance.sm],
                          words, kPayloadBytes, true);

    const uint32_t start = millis();
    while (dma_channel_is_busy(channel) &&
           (millis() - start) <= joybus_pio_timeout_ms) {
      tight_loop_contents();
    }
    if (dma_channel_is_busy(channel)) {
      dma_channel_abort(channel);
      result = kLongWriteDmaTimeout;
    } else if (response_len == 0) {
      result = 0;
    } else {
      uint8_t *response_cur = response;
      int remaining = response_len;
      while (remaining > 0 &&
             (millis() - start) <= joybus_pio_timeout_ms) {
        if (pio_sm_is_rx_fifo_empty(pio1, long_instance.sm)) {
          tight_loop_contents();
          continue;
        }
        const uint32_t rx = pio1->rxf[long_instance.sm];
        *response_cur++ = (uint8_t)rx;
        remaining--;
      }
      if (remaining == 0) {
        result = response_cur - response;
      } else {
        result = kLongWriteResponseTimeout;
      }
    }
    dma_channel_unclaim(channel);
  }

  pio_sm_set_enabled(pio1, long_instance.sm, false);
  pio_gpio_init(instance.pio, instance.pin);
  joybus_pio_reset(instance);
  return result;
}

// The request packet format is as follows:
//
// 0x00: 0bPPRRRRRR (P = payload len bytes (2 bits unsigned), R = response len bytes (6 bits unsigned))
// 0x01: Payload byte 1 (or 0x00)
// 0x02: Payload byte 2 (or 0x00)
// 0x03: Payload byte 3 (or 0x00)
//
// If response len > 0, will send stop bit after provided payload
// This allows for a maximum payload of 3 bytes per packet and unlimited per transmittion
//
// Responses are hard limited to 63 bytes (which is an acceptable trade-off as the biggest packet
// ever observed as part of Joybus sent by either side is < 40 bytes)
//
// The response packet format is the response bytes in groups of 4 bytes, padded by zeros in the last transmission

static uint32_t encode_tx_word(const uint8_t *payload, uint8_t payload_len,
                               uint8_t response_len) {
  uint8_t data[4] = { payload[2], payload[1], payload[0], (uint8_t)((payload_len << 6) | response_len) };
  uint32_t word;
  memcpy(&word, data, sizeof(word));
  return word ^ 0x00FFFFFF;
}

static bool __not_in_flash_func(tx_data)(JoybusPIOInstance instance, uint8_t *payload,
                                         uint8_t payload_len, uint8_t response_len) {
  unsigned long start = millis();
  while (pio_sm_is_tx_fifo_full(instance.pio, instance.sm)) {
    if ((millis() - start) > joybus_pio_timeout_ms) {
      joybus_pio_reset(instance);
      return false;
    }
    tight_loop_contents();
  }
  instance.pio->txf[instance.sm] = encode_tx_word(payload, payload_len, response_len);
  return true;
}

// Long Joybus writes must not pause between three-byte PIO words. A CPU-fed
// FIFO can underrun when an interrupt lands during the roughly 1.1 ms N64
// accessory packet, causing the accessory to discard the command entirely.
// DMA keeps the PIO FIFO fed without disabling USB interrupts.
static int tx_payload_dma(JoybusPIOInstance instance, const uint8_t *payload,
                          int payload_len, int response_len) {
  const int word_count = (payload_len + PAYLOAD_PACKET_MAX - 1) / PAYLOAD_PACKET_MAX;
  if (word_count <= 1 || word_count > PAYLOAD_DMA_WORD_MAX) {
    return 0;
  }

  uint32_t words[PAYLOAD_DMA_WORD_MAX];
  int offset = 0;
  for (int word = 0; word < word_count; ++word) {
    const int remaining = payload_len - offset;
    const uint8_t chunk_len = remaining > PAYLOAD_PACKET_MAX
      ? PAYLOAD_PACKET_MAX
      : (uint8_t)remaining;
    uint8_t chunk[PAYLOAD_PACKET_MAX] = {0};
    memcpy(chunk, payload + offset, chunk_len);
    offset += chunk_len;
    words[word] = encode_tx_word(
      chunk,
      chunk_len,
      offset == payload_len ? (uint8_t)response_len : 0
    );
  }

  const int channel = dma_claim_unused_channel(false);
  if (channel < 0) {
    return 0;
  }

  dma_channel_config config = dma_channel_get_default_config(channel);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
  channel_config_set_read_increment(&config, true);
  channel_config_set_write_increment(&config, false);
  channel_config_set_dreq(&config, pio_get_dreq(instance.pio, instance.sm, true));
  dma_channel_configure(
    channel,
    &config,
    &instance.pio->txf[instance.sm],
    words,
    word_count,
    true
  );

  const unsigned long start = millis();
  while (dma_channel_is_busy(channel)) {
    if ((millis() - start) > joybus_pio_timeout_ms) {
      dma_channel_abort(channel);
      dma_channel_unclaim(channel);
      joybus_pio_reset(instance);
      return -1;
    }
    tight_loop_contents();
  }

  dma_channel_unclaim(channel);
  return 1;
}

int __not_in_flash_func(joybus_pio_transmit_receive)(JoybusPIOInstance instance, uint8_t payload[],
                                                     int payload_len, uint8_t response[],
                                                     int response_len) {
  LatencyPhaseTraceScope pioTrace(LATENCY_TRACE_PHASE_JOYBUS_PIO_TXRX);
  const int dma_result = tx_payload_dma(instance, payload, payload_len, response_len);
  if (dma_result < 0) {
    return -1;
  }

  if (dma_result == 0) {
    uint8_t *payload_cur = payload;
    while (payload_len > PAYLOAD_PACKET_MAX) {
      if (!tx_data(instance, payload_cur, PAYLOAD_PACKET_MAX, 0)) {
        return -1;
      }
      payload_cur += PAYLOAD_PACKET_MAX;
      payload_len -= PAYLOAD_PACKET_MAX;
    }

    uint8_t payload_remaining[PAYLOAD_PACKET_MAX] = {0};
    memcpy(payload_remaining, payload_cur, payload_len);
    if (!tx_data(instance, payload_remaining, payload_len, response_len)) {
      return -1;
    }
  }

  uint8_t *response_cur = response;
  unsigned long start;
  uint32_t rxfifo_data;
  io_ro_32 *rxfifo_shift;
  while (response_len > 0) {
    rxfifo_shift = (io_ro_32 *)&instance.pio->rxf[instance.sm];
    start = millis();
    while (pio_sm_is_rx_fifo_empty(instance.pio, instance.sm)) {
      //if ((millis() - start) > 10) { // todo this is a timeout?
      if ((millis() - start) > joybus_pio_timeout_ms) {
        joybus_pio_reset(instance);
        return -1;
      }
    }
    rxfifo_data = *rxfifo_shift;

    int i = 32;
    while ((i -= 8) >= 0 && response_len-- > 0) {
      *(response_cur++) = (rxfifo_data >> i) & 0xFF;
    }
  }

  return response_cur - response;
}

void joybus_pio_set_timeout_ms(uint32_t timeout) {
  joybus_pio_timeout_ms = timeout;
}

uint32_t joybus_pio_get_timeout_ms() {
  return joybus_pio_timeout_ms;
}
