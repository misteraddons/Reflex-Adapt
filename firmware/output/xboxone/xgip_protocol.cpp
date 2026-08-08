/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 *
 * Ported into Reflex Adapt from the GP2040-CE Xbox One XGIP implementation.
 */

#include "../../product_config.h"

#if defined(ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT)

#include "xgip_protocol.h"

#include <string.h>

XGIPProtocol::XGIPProtocol() {
  reset();
}

void XGIPProtocol::reset() {
  memset(&header, 0, sizeof(header));
  totalChunkLength = 0;
  actualDataReceived = 0;
  totalChunkReceived = 0;
  totalChunkSent = 0;
  totalDataSent = 0;
  numberOfChunksSent = 0;
  chunkEnded = false;
  memset(packet, 0, sizeof(packet));
  packetLength = 0;
  memset(data, 0, sizeof(data));
  dataLength = 0;
  isValidPacket = false;
}

bool XGIPProtocol::parse(const uint8_t* buffer, uint16_t len) {
  if (buffer == nullptr || len < sizeof(GipHeader)) {
    reset();
    return false;
  }

  packetLength = len;
  const GipHeader* newPacket = reinterpret_cast<const GipHeader*>(buffer);
  if (newPacket->command == GIP_ACK_RESPONSE) {
    if (len != 13 || newPacket->internal != 0x01 || newPacket->length != 0x09) {
      reset();
      return false;
    }
    memcpy(&header, buffer, sizeof(GipHeader));
    isValidPacket = true;
    return true;
  }

  if (!newPacket->chunked) {
    reset();
    memcpy(&header, buffer, sizeof(GipHeader));
    if (header.length > 0 && len >= sizeof(GipHeader) + header.length) {
      memcpy(data, buffer + sizeof(GipHeader), header.length);
    }
    actualDataReceived = header.length;
    dataLength = actualDataReceived;
    isValidPacket = true;
    return false;
  }

  memcpy(&header, buffer, sizeof(GipHeader));
  if (header.length == 0) {
    const uint16_t endChunkSize =
      static_cast<uint16_t>(buffer[4]) |
      (static_cast<uint16_t>(buffer[5]) << 8);
    if (totalChunkLength != endChunkSize) {
      isValidPacket = false;
      return false;
    }
    chunkEnded = true;
    isValidPacket = true;
    return true;
  }

  if (header.chunkStart) {
    reset();
    memcpy(&header, buffer, sizeof(GipHeader));
    if (header.length > XGIP_MAX_CHUNK_SIZE && buffer[4] == 0x00) {
      totalChunkLength = buffer[5];
    } else {
      totalChunkLength = static_cast<uint16_t>(buffer[4]) |
                         (static_cast<uint16_t>(buffer[5]) << 8);
    }

    dataLength = totalChunkLength;
    if (totalChunkLength > 0x100) {
      dataLength = static_cast<uint16_t>(dataLength - 0x100);
      dataLength = static_cast<uint16_t>(dataLength - ((dataLength / 0x100) * 0x80));
    }
    totalChunkReceived = header.length;
  } else {
    totalChunkReceived = static_cast<uint16_t>(totalChunkReceived + header.length);
  }

  uint16_t copyLength = header.length;
  if (header.length > XGIP_MAX_CHUNK_SIZE) {
    copyLength ^= 0x80;
  }
  if (sizeof(GipHeader) + 2 + copyLength <= len &&
      actualDataReceived + copyLength <= sizeof(data)) {
    memcpy(&data[actualDataReceived], &buffer[6], copyLength);
    actualDataReceived = static_cast<uint16_t>(actualDataReceived + copyLength);
  }
  numberOfChunksSent++;
  isValidPacket = true;
  return false;
}

bool XGIPProtocol::validate() const {
  return isValidPacket;
}

bool XGIPProtocol::endOfChunk() const {
  return chunkEnded;
}

void XGIPProtocol::setAttributes(uint8_t cmd, uint8_t seq, uint8_t internal, uint8_t isChunked, uint8_t needsAck) {
  header.command = cmd;
  header.sequence = seq;
  header.internal = internal;
  header.chunked = isChunked;
  header.needsAck = needsAck;
  header.client = 0;
  header.chunkStart = 0;
}

bool XGIPProtocol::setData(const uint8_t* buffer, uint16_t len) {
  if (buffer == nullptr && len != 0) {
    return false;
  }
  if (len > sizeof(data)) {
    return false;
  }
  if (len != 0) {
    memcpy(data, buffer, len);
  }
  dataLength = len;
  return true;
}

uint8_t* XGIPProtocol::generatePacket() {
  if (!header.chunked) {
    header.length = static_cast<uint8_t>(dataLength);
    memcpy(packet, &header, sizeof(GipHeader));
    if (dataLength != 0) {
      memcpy(&packet[sizeof(GipHeader)], data, dataLength);
    }
    packetLength = static_cast<uint16_t>(sizeof(GipHeader) + dataLength);
    return packet;
  }

  if (numberOfChunksSent > 0 && totalDataSent == dataLength) {
    header.needsAck = 0;
    header.length = 0;
    memcpy(packet, &header, sizeof(GipHeader));
    packet[4] = totalChunkLength & 0xFF;
    packet[5] = (totalChunkLength >> 8) & 0xFF;
    packetLength = sizeof(GipHeader) + 2;
    chunkEnded = true;
    return packet;
  }

  if (numberOfChunksSent == 0) {
    if (dataLength < XGIP_MAX_CHUNK_SIZE) {
      totalChunkLength = dataLength;
      header.chunkStart = 0;
      header.chunked = 0;
    } else {
      header.chunkStart = 1;
      uint16_t remaining = dataLength;
      uint16_t encoded = 0;
      do {
        if (remaining < XGIP_MAX_CHUNK_SIZE) {
          if ((encoded / 0x100) != ((encoded + remaining) / 0x100)) {
            encoded = static_cast<uint16_t>(encoded + (remaining | 0x80));
          } else {
            encoded = static_cast<uint16_t>(encoded + remaining);
          }
          remaining = 0;
        } else {
          if ((encoded + XGIP_MAX_CHUNK_SIZE > 0x80) &&
              (encoded + XGIP_MAX_CHUNK_SIZE < 0x100)) {
            encoded = static_cast<uint16_t>(encoded + XGIP_MAX_CHUNK_SIZE + 0x100);
          } else if ((encoded / 0x100) != ((encoded + XGIP_MAX_CHUNK_SIZE) / 0x100)) {
            encoded = static_cast<uint16_t>(encoded + (XGIP_MAX_CHUNK_SIZE | 0x80));
          } else {
            encoded = static_cast<uint16_t>(encoded + XGIP_MAX_CHUNK_SIZE);
          }
          remaining = static_cast<uint16_t>(remaining - XGIP_MAX_CHUNK_SIZE);
        }
      } while (remaining != 0);
      totalChunkLength = encoded;
    }
  } else {
    header.chunkStart = 0;
  }

  header.needsAck = (numberOfChunksSent == 0 || ((numberOfChunksSent + 1) % 5) == 0) ? 1 : 0;

  uint16_t dataToSend = XGIP_MAX_CHUNK_SIZE;
  if ((dataLength - totalDataSent) < dataToSend) {
    dataToSend = static_cast<uint16_t>(dataLength - totalDataSent);
    header.needsAck = 1;
  }

  if (numberOfChunksSent > 0 && totalChunkSent < 0x100) {
    header.length = static_cast<uint8_t>(dataToSend | 0x80);
  } else if (numberOfChunksSent == 0 && dataLength > XGIP_MAX_CHUNK_SIZE && dataLength < 0x80) {
    header.length = static_cast<uint8_t>(dataToSend | 0x80);
  } else {
    header.length = static_cast<uint8_t>(dataToSend);
  }

  memcpy(packet, &header, sizeof(GipHeader));
  memcpy(&packet[6], &data[totalDataSent], dataToSend);
  packetLength = static_cast<uint16_t>(sizeof(GipHeader) + 2 + dataToSend);

  const uint16_t chunkValue = (numberOfChunksSent == 0) ? totalChunkLength : totalChunkSent;
  if (chunkValue < 0x100) {
    packet[4] = 0x00;
    packet[5] = static_cast<uint8_t>(chunkValue);
  } else {
    packet[4] = chunkValue & 0xFF;
    packet[5] = (chunkValue >> 8) & 0xFF;
  }

  if (totalChunkSent < 0x100 && (totalChunkSent + dataToSend) > 0x80) {
    totalChunkSent = static_cast<uint16_t>(totalChunkSent + dataToSend + 0x100);
  } else if (((totalChunkSent + dataToSend) / 0x100) > (totalChunkSent / 0x100)) {
    totalChunkSent = static_cast<uint16_t>(totalChunkSent + (dataToSend | 0x80));
  } else {
    totalChunkSent = static_cast<uint16_t>(totalChunkSent + dataToSend);
  }
  totalDataSent = static_cast<uint16_t>(totalDataSent + dataToSend);
  numberOfChunksSent++;
  return packet;
}

uint8_t* XGIPProtocol::generateAckPacket() {
  packet[0] = GIP_ACK_RESPONSE;
  packet[1] = 0x20;
  packet[2] = header.sequence;
  packet[3] = 0x09;
  packet[4] = 0x00;
  packet[5] = header.command;
  packet[6] = 0x20;
  packet[7] = actualDataReceived & 0xFF;
  packet[8] = (actualDataReceived >> 8) & 0xFF;
  packet[9] = 0x00;
  packet[10] = 0x00;
  if (header.chunked) {
    const uint16_t left = static_cast<uint16_t>(dataLength - actualDataReceived);
    packet[11] = left & 0xFF;
    packet[12] = (left >> 8) & 0xFF;
  } else {
    packet[11] = 0x00;
    packet[12] = 0x00;
  }
  packetLength = 13;
  return packet;
}

uint8_t XGIPProtocol::getCommand() const {
  return header.command;
}

uint8_t XGIPProtocol::getSequence() const {
  return header.sequence;
}

uint8_t XGIPProtocol::getChunked() const {
  return header.chunked;
}

uint8_t XGIPProtocol::getPacketAck() const {
  return header.needsAck;
}

uint8_t XGIPProtocol::getPacketLength() const {
  return static_cast<uint8_t>(packetLength);
}

uint8_t* XGIPProtocol::getData() {
  return data;
}

const uint8_t* XGIPProtocol::getData() const {
  return data;
}

uint16_t XGIPProtocol::getDataLength() const {
  return dataLength;
}

bool XGIPProtocol::ackRequired() const {
  return header.needsAck;
}

#endif
