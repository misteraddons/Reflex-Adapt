#include "oled_serial_runtime.h"

#include <Adafruit_TinyUSB.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr uint16_t kOledSerialHeaderCapacity = 64;
constexpr int kOledSerialCdcChunkMax = 64;

uint8_t oledFrame[OLED_SERIAL_FRAME_BYTES];
char oledSerialHeader[kOledSerialHeaderCapacity];
bool oledSerialEnabled = false;
bool oledFrameDirty = true;
uint16_t oledSerialRate = OLED_SERIAL_DEFAULT_RATE_HZ;
uint32_t oledSerialLastWriteMs = 0;
uint32_t oledSerialFrames = 0;
uint8_t oledSerialPendingPhase = 0;
uint16_t oledSerialPendingOffset = 0;
uint16_t oledSerialPendingHeaderLength = 0;

uint16_t clampRate(uint16_t rateHz) {
  if (rateHz < OLED_SERIAL_MIN_RATE_HZ) {
    return OLED_SERIAL_MIN_RATE_HZ;
  }
  if (rateHz > OLED_SERIAL_MAX_RATE_HZ) {
    return OLED_SERIAL_MAX_RATE_HZ;
  }
  return rateHz;
}

uint16_t intervalForRate(uint16_t rateHz) {
  const uint16_t clamped = clampRate(rateHz);
  const uint16_t interval = (uint16_t)(1000u / clamped);
  return interval == 0 ? 1 : interval;
}

bool oledSerialDueForFrame() {
  if (!oledSerialEnabled || !oledFrameDirty) {
    return false;
  }

  const uint32_t now = millis();
  const uint16_t intervalMs = intervalForRate(oledSerialRate);
  if (oledSerialLastWriteMs != 0 &&
      (uint32_t)(now - oledSerialLastWriteMs) < intervalMs) {
    return false;
  }
  oledSerialLastWriteMs = now;
  return true;
}

void beginPendingFrame() {
  oledSerialPendingHeaderLength = (uint16_t)snprintf(
    oledSerialHeader,
    sizeof(oledSerialHeader),
    "OLEDRAW SEQ=%lu W=%u H=%u LEN=%u\n",
    (unsigned long)oledSerialFrames,
    (unsigned)OLED_SERIAL_WIDTH,
    (unsigned)OLED_SERIAL_HEIGHT,
    (unsigned)OLED_SERIAL_FRAME_BYTES
  );
  if (oledSerialPendingHeaderLength >= sizeof(oledSerialHeader)) {
    oledSerialPendingHeaderLength = sizeof(oledSerialHeader) - 1u;
  }
  oledSerialPendingPhase = 1;
  oledSerialPendingOffset = 0;
  oledFrameDirty = false;
}

void resetPendingFrame() {
  oledSerialPendingPhase = 0;
  oledSerialPendingOffset = 0;
  oledSerialPendingHeaderLength = 0;
}

void advancePendingPhase() {
  oledSerialPendingOffset = 0;
  if (oledSerialPendingPhase == 1) {
    oledSerialPendingPhase = 2;
  } else if (oledSerialPendingPhase == 2) {
    oledSerialPendingPhase = 3;
  } else {
    resetPendingFrame();
    ++oledSerialFrames;
  }
}

void servicePendingFrameCdc(Adafruit_USBD_CDC& stream) {
  if (oledSerialPendingPhase == 0) {
    return;
  }

  const int available = stream.availableForWrite();
  if (available <= 0) {
    return;
  }

  const uint8_t* data = nullptr;
  uint16_t length = 0;
  uint8_t newline = '\n';
  switch (oledSerialPendingPhase) {
    case 1:
      data = reinterpret_cast<const uint8_t*>(oledSerialHeader);
      length = oledSerialPendingHeaderLength;
      break;
    case 2:
      data = oledFrame;
      length = OLED_SERIAL_FRAME_BYTES;
      break;
    case 3:
      data = &newline;
      length = 1;
      break;
    default:
      resetPendingFrame();
      return;
  }

  if (oledSerialPendingOffset >= length) {
    advancePendingPhase();
    return;
  }

  uint16_t remaining = (uint16_t)(length - oledSerialPendingOffset);
  uint16_t chunk = (remaining > (uint16_t)available) ? (uint16_t)available : remaining;
  if (chunk > kOledSerialCdcChunkMax) {
    chunk = kOledSerialCdcChunkMax;
  }

  const size_t written = stream.write(data + oledSerialPendingOffset, chunk);
  oledSerialPendingOffset += (uint16_t)written;
  if (written == 0) {
    return;
  }
  if (oledSerialPendingOffset >= length) {
    advancePendingPhase();
  }
}

}  // namespace

extern "C" void reflex_oled_mirror_ram_write(uint8_t col, uint8_t row, uint8_t value) {
  if (col >= OLED_SERIAL_WIDTH || row >= (OLED_SERIAL_HEIGHT / 8)) {
    return;
  }

  const uint16_t index = (uint16_t)row * OLED_SERIAL_WIDTH + col;
  if (oledFrame[index] != value) {
    oledFrame[index] = value;
    oledFrameDirty = true;
  }
}

void oledSerialSetEnabled(bool enabled) {
  oledSerialEnabled = enabled;
  oledSerialLastWriteMs = 0;
  oledFrameDirty = true;
  resetPendingFrame();
}

bool oledSerialIsEnabled() {
  return oledSerialEnabled;
}

void oledSerialSetRateHz(uint16_t rateHz) {
  oledSerialRate = clampRate(rateHz);
}

uint16_t oledSerialRateHz() {
  return oledSerialRate;
}

uint32_t oledSerialFrameCount() {
  return oledSerialFrames;
}

void oledSerialWriteStatus(Print& stream) {
  stream.print(F("OLED ENABLED="));
  stream.print(oledSerialEnabled ? 1 : 0);
  stream.print(F(" RATE_HZ="));
  stream.print((int)oledSerialRate);
  stream.print(F(" W="));
  stream.print((int)OLED_SERIAL_WIDTH);
  stream.print(F(" H="));
  stream.print((int)OLED_SERIAL_HEIGHT);
  stream.print(F(" LEN="));
  stream.print((int)OLED_SERIAL_FRAME_BYTES);
  stream.print(F(" FRAMES="));
  stream.println(oledSerialFrames);
}

void oledSerialWriteFrame(Print& stream) {
  stream.print(F("OLEDRAW SEQ="));
  stream.print(oledSerialFrames);
  stream.print(F(" W="));
  stream.print((int)OLED_SERIAL_WIDTH);
  stream.print(F(" H="));
  stream.print((int)OLED_SERIAL_HEIGHT);
  stream.print(F(" LEN="));
  stream.println((int)OLED_SERIAL_FRAME_BYTES);
  stream.write(oledFrame, OLED_SERIAL_FRAME_BYTES);
  stream.write('\n');
  ++oledSerialFrames;
  oledFrameDirty = false;
}

void oledSerialTask(Print& stream) {
  if (!oledSerialDueForFrame()) {
    return;
  }

  oledSerialWriteFrame(stream);
}

void oledSerialTask(Adafruit_USBD_CDC& stream) {
  if (!oledSerialEnabled) {
    resetPendingFrame();
    return;
  }

  if (oledSerialPendingPhase == 0 && oledSerialDueForFrame()) {
    beginPendingFrame();
  }
  servicePendingFrameCdc(stream);
}
