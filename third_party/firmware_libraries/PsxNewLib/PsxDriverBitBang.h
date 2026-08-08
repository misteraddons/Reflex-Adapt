/*******************************************************************************
 * Bit-banged PSX driver for boards where the controller wiring is not reliable
 * through RP2040 hardware SPI pin muxing.
 ******************************************************************************/

#pragma once

#include "PsxDriver.h"

#include <Arduino.h>
#include <hardware/gpio.h>

class PsxDriverBitBang: public PsxDriver {
private:
  const uint8_t att;
  const uint8_t ack;
  const uint8_t cmd;
  const uint8_t dat;
  const uint8_t clk;
  const bool waitForAck;

  unsigned long byteFinishTime { 0 };
  bool ackSawLow { false };

protected:
  virtual byte shiftInOut(const byte out) override {
    byte rx = 0;
    ackSawLow = false;

    for (uint8_t i = 0; i < 8; ++i) {
      gpio_put(cmd, (out >> i) & 0x01);
      delayMicroseconds(2);

      gpio_put(clk, LOW);
      delayMicroseconds(2);

      gpio_put(clk, HIGH);
      delayMicroseconds(2);

      if (gpio_get(dat)) {
        rx |= (1u << i);
      }
    }

    gpio_put(cmd, HIGH);
    byteFinishTime = micros();
    return rx;
  }

public:
  PsxDriverBitBang(uint8_t SPI_NUMBER,
                   uint8_t PIN_ATT,
                   uint8_t PIN_ACK,
                   uint8_t PIN_CMD,
                   uint8_t PIN_DAT,
                   uint8_t PIN_CLK,
                   bool requireAck)
      : att{PIN_ATT},
        ack{PIN_ACK},
        cmd{PIN_CMD},
        dat{PIN_DAT},
        clk{PIN_CLK},
        waitForAck{requireAck} {
    (void)SPI_NUMBER;
  }

  virtual void attention() override {
    gpio_put(cmd, HIGH);
    gpio_put(clk, HIGH);
    gpio_put(att, LOW);
  }

  virtual void noAttention() override {
    gpio_put(cmd, HIGH);
    gpio_put(clk, HIGH);
    gpio_put(att, HIGH);
  }

  virtual boolean acknowledged() override {
    if (!waitForAck) {
      return micros() - byteFinishTime > INTER_CMD_BYTE_DELAY;
    }

    if (!ackSawLow && !gpio_get(ack)) {
      ackSawLow = true;
    }
    return ackSawLow && gpio_get(ack);
  }

  virtual boolean begin() override {
    gpio_init(att);
    gpio_init(ack);
    gpio_init(cmd);
    gpio_init(dat);
    gpio_init(clk);

    gpio_set_dir(att, GPIO_OUT);
    gpio_set_dir(cmd, GPIO_OUT);
    gpio_set_dir(clk, GPIO_OUT);
    gpio_set_dir(dat, GPIO_IN);
    gpio_set_dir(ack, GPIO_IN);

    gpio_pull_up(dat);
    gpio_pull_up(ack);

    gpio_put(att, HIGH);
    gpio_put(cmd, HIGH);
    gpio_put(clk, HIGH);

    return PsxDriver::begin();
  }
};
