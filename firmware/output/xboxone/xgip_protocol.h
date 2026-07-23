/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 *
 * Ported into Reflex Adapt from the GP2040-CE Xbox One XGIP implementation.
 * The protocol behavior follows Santroller/GIMX-derived public research as
 * carried by GP2040-CE; keep this transport boring and donor-aligned. Do not
 * copy Santroller GPL source here without an explicit license decision.
 */

#pragma once

#include <stdint.h>

constexpr uint8_t XGIP_MAX_CHUNK_SIZE = 0x3A;

enum XboxOneGipCommand : uint8_t {
  GIP_ACK_RESPONSE = 0x01,
  GIP_ANNOUNCE = 0x02,
  GIP_KEEPALIVE = 0x03,
  GIP_DEVICE_DESCRIPTOR = 0x04,
  GIP_POWER_MODE_DEVICE_CONFIG = 0x05,
  GIP_AUTH = 0x06,
  GIP_VIRTUAL_KEYCODE = 0x07,
  GIP_CMD_RUMBLE = 0x09,
  GIP_CMD_LED_ON = 0x0A,
  GIP_FINAL_AUTH = 0x1E,
  GIP_INPUT_REPORT = 0x20,
  GIP_HID_REPORT = 0x21,
};

struct __attribute__((packed)) GipHeader {
  uint8_t command;
  uint8_t client : 4;
  uint8_t needsAck : 1;
  uint8_t internal : 1;
  uint8_t chunkStart : 1;
  uint8_t chunked : 1;
  uint8_t sequence;
  uint8_t length;
};

class XGIPProtocol {
 public:
  XGIPProtocol();

  void reset();
  bool parse(const uint8_t* buffer, uint16_t len);
  bool validate() const;
  bool endOfChunk() const;
  void setAttributes(uint8_t cmd, uint8_t seq, uint8_t internal, uint8_t isChunked, uint8_t needsAck);
  bool setData(const uint8_t* buffer, uint16_t len);
  uint8_t* generatePacket();
  uint8_t* generateAckPacket();
  uint8_t getCommand() const;
  uint8_t getSequence() const;
  uint8_t getChunked() const;
  uint8_t getPacketAck() const;
  uint8_t getPacketLength() const;
  uint8_t* getData();
  const uint8_t* getData() const;
  uint16_t getDataLength() const;
  bool ackRequired() const;

 private:
  GipHeader header = {};
  uint16_t totalChunkLength = 0;
  uint16_t actualDataReceived = 0;
  uint16_t totalChunkReceived = 0;
  uint16_t totalChunkSent = 0;
  uint16_t totalDataSent = 0;
  uint16_t numberOfChunksSent = 0;
  bool chunkEnded = false;
  uint8_t packet[64] = {};
  uint16_t packetLength = 0;
  uint8_t data[1024] = {};
  uint16_t dataLength = 0;
  bool isValidPacket = false;
};
