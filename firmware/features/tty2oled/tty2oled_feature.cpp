#include "../../product_config.h"

#ifdef ENABLE_TTY2OLED_SERIAL

#include "tty2oled_feature.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/controller_settings_state.h"
#include "../../menu/menu_bridge.h"
#include "../../menu/menu_ui_state.h"
#include "../../platform/display_runtime_state.h"

namespace {

constexpr uint8_t kMaxTextRows = 8;
constexpr uint8_t kTextColumns = 21;
constexpr uint8_t kCoreNameCapacity = 32;
constexpr uint32_t kRawPayloadQuietMs = 80;
constexpr uint32_t kRawPayloadInitialMs = 500;
constexpr uint16_t kRawPayloadPixBytes = 1024;   // i2c2OLED-compatible 128x64 SSD1306 page bytes.
constexpr uint16_t kRawPayloadXbmBytes = 2048;   // tty2oled 256x64 1-bpp art.
constexpr uint16_t kRawPayloadGscBytes = 8192;   // tty2oled 256x64 4-bpp grayscale art.
constexpr uint8_t kRawCommandProbeCapacity = 64;

enum Tty2OledArtworkFormat : uint8_t {
  TTY2OLED_ART_NONE = 0,
  TTY2OLED_ART_XBM,
  TTY2OLED_ART_GSC,
  TTY2OLED_ART_PIX,
};

bool tty2oledActive = false;
bool tty2oledVisible = false;
bool tty2oledNeedsRender = true;
bool tty2oledBlank = false;
bool tty2oledRawDrainActive = false;
uint32_t tty2oledRawDrainUntilMs = 0;
uint32_t tty2oledCommandCount = 0;
uint16_t tty2oledRawPayloadLength = 0;
uint16_t tty2oledRawPayloadExpectedLength = 0;
uint8_t tty2oledRawCommandProbeLength = 0;
bool tty2oledRawBinaryFrame = false;
Tty2OledArtworkFormat tty2oledArtworkFormat = TTY2OLED_ART_NONE;
uint8_t tty2oledGscThreshold = 0;
char tty2oledCoreName[kCoreNameCapacity] = "MiSTer";
char tty2oledRows[kMaxTextRows][kTextColumns + 1] = {};
uint8_t tty2oledRawPayload[kRawPayloadGscBytes] = {};
char tty2oledRawCommandProbe[kRawCommandProbeCapacity] = {};

char toUpperAscii(char value) {
  return (value >= 'a' && value <= 'z') ? (char)(value - ('a' - 'A')) : value;
}

bool commandStartsWith(const char* command, const char* prefix, const char** tail = nullptr) {
  size_t index = 0;
  while (prefix[index] != '\0') {
    if (toUpperAscii(command[index]) != prefix[index]) {
      return false;
    }
    ++index;
  }
  if (tail) {
    *tail = command + index;
  }
  return true;
}

bool commandEquals(const char* command, const char* expected) {
  const char* tail = nullptr;
  return commandStartsWith(command, expected, &tail) && *tail == '\0';
}

bool isLineEnding(uint8_t value) {
  return value == '\r' || value == '\n';
}

bool isPrintableAscii(uint8_t value) {
  return value >= 0x20 && value <= 0x7E;
}

bool isLikelyCoreNameChar(char value) {
  return (value >= 'A' && value <= 'Z') ||
         (value >= 'a' && value <= 'z') ||
         (value >= '0' && value <= '9') ||
         value == '_' || value == '-' || value == ' ' ||
         value == '.' || value == '+' || value == '(' || value == ')';
}

bool isLikelyPlainCoreName(const char* command) {
  if (!command || *command == '\0') {
    return false;
  }
  if (commandStartsWith(command, "CMD") ||
      commandStartsWith(command, "SET") ||
      commandStartsWith(command, "PING") ||
      commandStartsWith(command, "STATE") ||
      commandStartsWith(command, "STATUS") ||
      commandStartsWith(command, "HELP") ||
      commandStartsWith(command, "BOOT") ||
      commandStartsWith(command, "BOOTLOADER") ||
      commandStartsWith(command, "REBOOT") ||
      commandStartsWith(command, "RESET") ||
      commandStartsWith(command, "GPIO") ||
      commandStartsWith(command, "DHELP") ||
      commandStartsWith(command, "HOTKEY") ||
      commandStartsWith(command, "CHORD") ||
      commandStartsWith(command, "LAT") ||
      commandStartsWith(command, "RUMBLE") ||
      commandStartsWith(command, "CARD") ||
      commandStartsWith(command, "AUTH") ||
      commandStartsWith(command, "JVS") ||
      commandStartsWith(command, "AUTO") ||
      commandStartsWith(command, "ADSCAN") ||
      commandStartsWith(command, "ADAPTSTATE") ||
      commandStartsWith(command, "OLED") ||
      commandStartsWith(command, "OLEDMIRROR") ||
      commandStartsWith(command, "OVERLAY") ||
      commandStartsWith(command, "OUT") ||
      commandStartsWith(command, "WIN") ||
      commandStartsWith(command, "USBHOST")) {
    return false;
  }

  bool hasNameChar = false;
  size_t length = 0;
  while (command[length] != '\0') {
    if (!isLikelyCoreNameChar(command[length])) {
      return false;
    }
    if ((command[length] >= 'A' && command[length] <= 'Z') ||
        (command[length] >= 'a' && command[length] <= 'z') ||
        (command[length] >= '0' && command[length] <= '9')) {
      hasNameChar = true;
    }
    ++length;
    if (length >= kCoreNameCapacity) {
      return false;
    }
  }
  return hasNameChar;
}

void resetRawCommandProbe() {
  tty2oledRawCommandProbeLength = 0;
  tty2oledRawCommandProbe[0] = '\0';
}

void clearTextRows() {
  for (uint8_t row = 0; row < kMaxTextRows; ++row) {
    tty2oledRows[row][0] = '\0';
  }
}

void clearArtwork() {
  tty2oledArtworkFormat = TTY2OLED_ART_NONE;
  tty2oledRawPayloadLength = 0;
  tty2oledRawPayloadExpectedLength = 0;
  tty2oledRawBinaryFrame = false;
  tty2oledGscThreshold = 0;
  resetRawCommandProbe();
}

uint8_t calculateGscThreshold() {
  uint16_t histogram[31] = {};
  for (uint16_t i = 0; i < kRawPayloadGscBytes; ++i) {
    const uint8_t source = tty2oledRawPayload[i];
    ++histogram[(source >> 4) + (source & 0x0F)];
  }

  uint32_t totalWeighted = 0;
  for (uint8_t i = 0; i < 31; ++i) {
    totalWeighted += (uint32_t)i * histogram[i];
  }

  uint32_t backgroundWeighted = 0;
  uint16_t backgroundCount = 0;
  uint8_t bestThreshold = 0;
  uint64_t bestVariance = 0;
  for (uint8_t threshold = 0; threshold < 31; ++threshold) {
    backgroundCount += histogram[threshold];
    backgroundWeighted += (uint32_t)threshold * histogram[threshold];
    if (backgroundCount == 0 || backgroundCount >= kRawPayloadGscBytes) {
      continue;
    }

    const uint16_t foregroundCount = kRawPayloadGscBytes - backgroundCount;
    const int64_t meanDeltaScaled =
      (int64_t)backgroundWeighted * foregroundCount -
      (int64_t)(totalWeighted - backgroundWeighted) * backgroundCount;
    const uint64_t delta = (uint64_t)((meanDeltaScaled < 0) ? -meanDeltaScaled : meanDeltaScaled);
    const uint64_t variance =
      (delta * delta) / ((uint32_t)backgroundCount * foregroundCount);
    if (variance > bestVariance) {
      bestVariance = variance;
      bestThreshold = threshold;
    }
  }
  return bestThreshold;
}

void copyCleanText(char* destination, size_t capacity, const char* source) {
  if (capacity == 0) {
    return;
  }
  size_t out = 0;
  while (*source != '\0' && out + 1 < capacity) {
    const char ch = *source++;
    if (ch == '\r' || ch == '\n') {
      break;
    }
    destination[out++] = (ch == '_') ? ' ' : ch;
  }
  destination[out] = '\0';
}

const char* nextCsvField(const char* text, char* destination, size_t capacity) {
  if (destination && capacity > 0) {
    destination[0] = '\0';
  }

  size_t out = 0;
  while (*text != '\0' && *text != ',') {
    if (destination && capacity > 0 && out + 1 < capacity) {
      destination[out++] = *text;
    }
    ++text;
  }
  if (destination && capacity > 0) {
    destination[out] = '\0';
  }
  return (*text == ',') ? text + 1 : text;
}

void renderTty2OledImmediateIfIdle();
void finishRawPayloadDrainIfQuiet();

void markDisplayDirty(bool active = true) {
  tty2oledActive = active;
  tty2oledVisible = active;
  tty2oledNeedsRender = true;
  ++tty2oledCommandCount;
  forceMainDisplayRefresh();
  renderTty2OledImmediateIfIdle();
}

void markDisplayActiveForPendingArtwork() {
  tty2oledActive = true;
  tty2oledVisible = true;
  tty2oledNeedsRender = false;
  ++tty2oledCommandCount;
  forceMainDisplayRefresh();
}

void beginRawPayloadDrain(uint16_t expectedLength = 0, bool exactBinaryFrame = false) {
  // CMDCOR/CMDAPD keep the legacy tty2oled receiver behavior: 2048 bytes is
  // XBM and 8192 bytes is GSC. CMDPIX is Adapt's i2c2OLED-compatible fast path:
  // exactly 1024 SSD1306 page bytes for the built-in 128x64 display.
  tty2oledRawDrainActive = true;
  tty2oledRawPayloadLength = 0;
  tty2oledRawPayloadExpectedLength = expectedLength;
  tty2oledRawBinaryFrame = exactBinaryFrame;
  resetRawCommandProbe();
  tty2oledRawDrainUntilMs = millis() + kRawPayloadInitialMs;
}

void handleCoreChange(const char* command) {
  const char* payload = command + strlen("CMDCOR");
  if (*payload == ',') {
    ++payload;
  }

  char field[kCoreNameCapacity] = {};
  nextCsvField(payload, field, sizeof(field));
  if (field[0] != '\0') {
    copyCleanText(tty2oledCoreName, sizeof(tty2oledCoreName), field);
  }
  clearTextRows();
  clearArtwork();
  tty2oledBlank = false;
  markDisplayDirty();
  beginRawPayloadDrain();
}

void handlePixFrameCommand(const char* command) {
  const char* payload = command + strlen("CMDPIX");
  if (*payload == ',') {
    ++payload;
  }

  char field[kCoreNameCapacity] = {};
  nextCsvField(payload, field, sizeof(field));
  if (field[0] != '\0') {
    copyCleanText(tty2oledCoreName, sizeof(tty2oledCoreName), field);
  }
  clearTextRows();
  clearArtwork();
  tty2oledBlank = false;
  markDisplayActiveForPendingArtwork();
  beginRawPayloadDrain(kRawPayloadPixBytes, true);
}

void handleTextCommand(const char* command) {
  const char* payload = command + strlen("CMDTXT");
  if (*payload == ',') {
    ++payload;
  }

  char field[24] = {};
  for (uint8_t i = 0; i < 5; ++i) {
    payload = nextCsvField(payload, field, sizeof(field));
  }

  long y = strtol(field, nullptr, 10);
  uint8_t row = (y <= 0) ? 0 : (uint8_t)(y / 8);
  if (row >= kMaxTextRows) {
    row = kMaxTextRows - 1;
  }
  copyCleanText(tty2oledRows[row], sizeof(tty2oledRows[row]), payload);
  clearArtwork();
  tty2oledBlank = false;
  markDisplayDirty();
}

void handlePlainCoreName(const char* command) {
  copyCleanText(tty2oledCoreName, sizeof(tty2oledCoreName), command);
  clearTextRows();
  clearArtwork();
  tty2oledBlank = false;
  markDisplayDirty();
}

#ifdef USE_I2C_DISPLAY
void clearArtworkRightEdge() {
  // Raw tty2oled/i2c2OLED frames bypass the text renderer, so force the final
  // visible column to a known state after each page frame.
  for (uint8_t page = 0; page < 8; ++page) {
    display.setCursor(127, page);
    display.ssd1306WriteRam(0);
  }
}

void renderArtworkPix() {
  for (uint8_t page = 0; page < 8; ++page) {
    display.setCursor(0, page);
    for (uint8_t x = 0; x < 128; ++x) {
      display.ssd1306WriteRam(tty2oledRawPayload[(uint16_t)page * 128 + x]);
    }
  }
  clearArtworkRightEdge();
}

void renderArtworkGsc() {
  for (uint8_t page = 0; page < 8; ++page) {
    display.setCursor(0, page);
    for (uint8_t x = 0; x < 128; ++x) {
      uint8_t columnByte = 0;
      for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint8_t y = (uint8_t)(page * 8 + bit);
        const uint8_t source = tty2oledRawPayload[(uint16_t)y * 128 + x];
        const uint8_t left = source >> 4;
        const uint8_t right = source & 0x0F;
        const uint8_t intensity = (uint8_t)(left + right);
        if (intensity > tty2oledGscThreshold) {
          columnByte |= (uint8_t)(1u << bit);
        }
      }
      display.ssd1306WriteRam(columnByte);
    }
  }
  clearArtworkRightEdge();
}

void renderArtworkXbm() {
  for (uint8_t page = 0; page < 8; ++page) {
    display.setCursor(0, page);
    for (uint8_t x = 0; x < 128; ++x) {
      uint8_t columnByte = 0;
      const uint8_t sourcePairBit = (uint8_t)((x & 0x03) * 2);
      for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint8_t y = (uint8_t)(page * 8 + bit);
        const uint8_t source = tty2oledRawPayload[(uint16_t)y * 32 + (x >> 2)];
        if (source & (uint8_t)(0x03u << sourcePairBit)) {
          columnByte |= (uint8_t)(1u << bit);
        }
      }
      display.ssd1306WriteRam(columnByte);
    }
  }
  clearArtworkRightEdge();
}

void renderCenteredTextRow(const char* text, uint8_t row, bool doubleSize) {
  display.set1X();
  if (doubleSize) {
    display.set2X();
  }
  const uint8_t charWidth = doubleSize ? 12 : 6;
  const uint8_t maxCols = doubleSize ? 10 : 21;
  const uint8_t length = (uint8_t)min((size_t)maxCols, strlen(text));
  const uint8_t col = (uint8_t)((128 - length * charWidth) / 2 / 6);
  display.setCursor(col, row);
  for (uint8_t i = 0; i < length; ++i) {
    display.print(text[i]);
  }
  display.set1X();
}

bool anyTextRows() {
  for (uint8_t row = 0; row < kMaxTextRows; ++row) {
    if (tty2oledRows[row][0] != '\0') {
      return true;
    }
  }
  return false;
}

void renderTty2Oled() {
  if (!tty2oledBlank && tty2oledArtworkFormat == TTY2OLED_ART_PIX) {
    renderArtworkPix();
    return;
  }
  if (!tty2oledBlank && tty2oledArtworkFormat == TTY2OLED_ART_GSC) {
    renderArtworkGsc();
    return;
  }
  if (!tty2oledBlank && tty2oledArtworkFormat == TTY2OLED_ART_XBM) {
    renderArtworkXbm();
    return;
  }

  display.clear();
  display.setFont(System5x7);
  display.set1X();
  if (tty2oledBlank) {
    return;
  }

  if (anyTextRows()) {
    for (uint8_t row = 0; row < kMaxTextRows; ++row) {
      if (tty2oledRows[row][0] == '\0') {
        continue;
      }
      display.setCursor(0, row);
      display.print(tty2oledRows[row]);
    }
    return;
  }

  renderCenteredTextRow("MiSTer", 0, false);
  renderCenteredTextRow(tty2oledCoreName, 2, true);
  display.set1X();
  renderCenteredTextRow("tty2oled serial", 7, false);
}

void renderTty2OledImmediateIfIdle() {
  if (!tty2oledActive || isMenuOpen || isQuickConfigOpen) {
    return;
  }

  // Core-change notifications arrive outside the normal OLED refresh cadence.
  // Render immediately so tty2oled does not wait for a later menu/home redraw.
  beginDisplayWire();
  display.begin(&Adafruit128x64, I2C_ADDRESS);
  restoreOledPanelOrientation();
  u8g2.setDrawColor(1);
  display.invertDisplay(false);
  display.setInvertMode(false);
  display.setContrast(display_contrast);
  renderTty2Oled();
  Wire.end();
  tty2oledNeedsRender = false;
  mainDisplayInitialized = true;
}
#else
void renderTty2OledImmediateIfIdle() {}
#endif

void appendRawPayloadByte(uint8_t value) {
  if (tty2oledRawPayloadLength < kRawPayloadGscBytes) {
    tty2oledRawPayload[tty2oledRawPayloadLength++] = value;
  }
}

void flushRawCommandProbeToPayload() {
  for (uint8_t i = 0; i < tty2oledRawCommandProbeLength; ++i) {
    appendRawPayloadByte((uint8_t)tty2oledRawCommandProbe[i]);
  }
  resetRawCommandProbe();
}

void finishRawPayloadDrain(bool flushTextProbe = true) {
  if (!tty2oledRawDrainActive) {
    return;
  }

  tty2oledRawDrainActive = false;
  if (flushTextProbe) {
    flushRawCommandProbeToPayload();
  } else {
    resetRawCommandProbe();
  }

  if (tty2oledRawPayloadExpectedLength == kRawPayloadPixBytes &&
      tty2oledRawPayloadLength == kRawPayloadPixBytes) {
    tty2oledArtworkFormat = TTY2OLED_ART_PIX;
  } else if (tty2oledRawPayloadExpectedLength == 0 &&
             tty2oledRawPayloadLength == kRawPayloadGscBytes) {
    tty2oledArtworkFormat = TTY2OLED_ART_GSC;
    tty2oledGscThreshold = calculateGscThreshold();
  } else if (tty2oledRawPayloadExpectedLength == 0 &&
             tty2oledRawPayloadLength == kRawPayloadXbmBytes) {
    tty2oledArtworkFormat = TTY2OLED_ART_XBM;
  } else {
    tty2oledArtworkFormat = TTY2OLED_ART_NONE;
    tty2oledGscThreshold = 0;
  }
  tty2oledRawPayloadExpectedLength = 0;
  tty2oledRawBinaryFrame = false;

  if (tty2oledArtworkFormat != TTY2OLED_ART_NONE) {
    tty2oledNeedsRender = true;
    forceMainDisplayRefresh();
  }
}

void finishRawPayloadDrainIfQuiet() {
  if (!tty2oledRawDrainActive) {
    return;
  }
  if ((int32_t)(millis() - tty2oledRawDrainUntilMs) >= 0) {
    finishRawPayloadDrain();
  }
}

bool handleTty2OledCommand(const char* command, Print& out) {
  if (commandEquals(command, "QWERTZ")) {
    clearTextRows();
    clearArtwork();
    tty2oledBlank = false;
    markDisplayDirty(false);
    out.print(F("ttyack;"));
    return true;
  }
  if (commandEquals(command, "CMDHWINF")) {
    out.print(F("HWADAPT2;AdaptV2;ttyack;"));
    return true;
  }
  if (commandEquals(command, "CMDCLS") || commandEquals(command, "CMDCLSWU")) {
    clearTextRows();
    clearArtwork();
    tty2oledBlank = true;
    markDisplayDirty(commandEquals(command, "CMDCLS"));
    out.print(F("ttyack;"));
    return true;
  }
  if (commandStartsWith(command, "CMDCLST") ||
      commandEquals(command, "CMDDOFF")) {
    clearTextRows();
    clearArtwork();
    tty2oledBlank = true;
    markDisplayDirty();
    out.print(F("ttyack;"));
    return true;
  }
  if (commandEquals(command, "CMDDON") ||
      commandEquals(command, "CMDDUPD") ||
      commandStartsWith(command, "CMDSPIC") ||
      commandStartsWith(command, "CMDSSCP")) {
    tty2oledBlank = false;
    markDisplayDirty();
    out.print(F("ttyack;"));
    return true;
  }
  if (commandEquals(command, "CMDSORG")) {
    copyCleanText(tty2oledCoreName, sizeof(tty2oledCoreName), "MiSTer");
    clearTextRows();
    clearArtwork();
    tty2oledBlank = false;
    markDisplayDirty();
    out.print(F("ttyack;"));
    return true;
  }
  if (commandEquals(command, "CMDSNAM")) {
    clearTextRows();
    clearArtwork();
    tty2oledBlank = false;
    markDisplayDirty();
    out.print(F("ttyack;"));
    return true;
  }
  if (commandStartsWith(command, "CMDCOR")) {
    handleCoreChange(command);
    out.print(F("ttyack;"));
    return true;
  }
  if (commandStartsWith(command, "CMDPIX")) {
    handlePixFrameCommand(command);
    out.print(F("ttyack;"));
    return true;
  }
  if (commandStartsWith(command, "CMDAPD")) {
    clearTextRows();
    clearArtwork();
    tty2oledBlank = false;
    markDisplayDirty();
    beginRawPayloadDrain();
    out.print(F("ttyack;"));
    return true;
  }
  if (commandStartsWith(command, "CMDTXT")) {
    handleTextCommand(command);
    out.print(F("ttyack;"));
    return true;
  }
  if (commandStartsWith(command, "CMDCON") ||
      commandStartsWith(command, "CMDROT") ||
      commandStartsWith(command, "CMDSAVER") ||
      commandStartsWith(command, "CMDSWSAVER") ||
      commandStartsWith(command, "CMDSETTIME") ||
      commandStartsWith(command, "CMDSTTYACK")) {
    ++tty2oledCommandCount;
    out.print(F("ttyack;"));
    return true;
  }
  if (tty2oledCommandCount != 0 && isLikelyPlainCoreName(command)) {
    handlePlainCoreName(command);
    out.print(F("ttyack;"));
    return true;
  }
  return false;
}

void appendTty2OledState(Print& out) {
  out.print(F(" TTY2OLED="));
  out.print(tty2oledActive ? 1 : 0);
  out.print(F(" TTY2OLED_CMDS="));
  out.print(tty2oledCommandCount);
  out.print(F(" TTY2OLED_CORE="));
  out.print(tty2oledCoreName);
  out.print(F(" TTY2OLED_ART="));
  out.print((int)tty2oledArtworkFormat);
  out.print(F(" TTY2OLED_RAW="));
  out.print(tty2oledRawPayloadLength);
  out.print(F(" TTY2OLED_RAW_EXPECT="));
  out.print(tty2oledRawPayloadExpectedLength);
  out.print(F(" TTY2OLED_GSC_TH="));
  out.print((int)tty2oledGscThreshold);
}

}  // namespace

bool tty2oledSerialDrainByte(uint8_t value, Print& out) {
  if (!tty2oledRawDrainActive) {
    return false;
  }
  if ((int32_t)(millis() - tty2oledRawDrainUntilMs) > 0) {
    finishRawPayloadDrain();
    return false;
  }

  if (tty2oledRawBinaryFrame) {
    appendRawPayloadByte(value);
    if (tty2oledRawPayloadExpectedLength != 0 &&
        tty2oledRawPayloadLength >= tty2oledRawPayloadExpectedLength) {
      finishRawPayloadDrain();
    } else {
      tty2oledRawDrainUntilMs = millis() + kRawPayloadQuietMs;
    }
    return true;
  }

  if (tty2oledRawPayloadLength == 0 || tty2oledRawCommandProbeLength != 0) {
    if (tty2oledRawCommandProbeLength != 0 && isLineEnding(value)) {
      tty2oledRawCommandProbe[tty2oledRawCommandProbeLength] = '\0';
      char command[kRawCommandProbeCapacity] = {};
      strncpy(command, tty2oledRawCommandProbe, sizeof(command) - 1);
      finishRawPayloadDrain(false);
      (void)handleTty2OledCommand(command, out);
      return true;
    }

    if (tty2oledRawCommandProbeLength == 0 && isLineEnding(value)) {
      return true;
    }

    if (isPrintableAscii(value)) {
      if (tty2oledRawCommandProbeLength < kRawCommandProbeCapacity - 1) {
        tty2oledRawCommandProbe[tty2oledRawCommandProbeLength++] = (char)value;
        tty2oledRawCommandProbe[tty2oledRawCommandProbeLength] = '\0';
        tty2oledRawDrainUntilMs = millis() + kRawPayloadQuietMs;
        return true;
      }

      flushRawCommandProbeToPayload();
    } else if (tty2oledRawCommandProbeLength != 0) {
      // A real binary GSC/XBM payload can begin with printable bytes. Only
      // treat a printable prefix as text if it reaches a line ending first.
      flushRawCommandProbeToPayload();
    }
  }

  appendRawPayloadByte(value);
  if ((tty2oledRawPayloadExpectedLength != 0 &&
       tty2oledRawPayloadLength >= tty2oledRawPayloadExpectedLength) ||
      tty2oledRawPayloadLength >= kRawPayloadGscBytes) {
    finishRawPayloadDrain();
  } else {
    tty2oledRawDrainUntilMs = millis() + kRawPayloadQuietMs;
  }
  return true;
}

const FeatureModule kTty2OledFeatureModule = {
  "tty2oled",
  nullptr,
  nullptr,
  handleTty2OledCommand,
  [](Print& out) { out.print(F(",QWERTZ,CMDCOR,CMDPIX,CMDAPD,CMDTXT,CMDCLS,CMDSNAM,CMDHWINF")); },
  appendTty2OledState,
#ifdef USE_I2C_DISPLAY
  []() -> bool {
    finishRawPayloadDrainIfQuiet();
    if (!tty2oledActive) {
      if (tty2oledVisible) {
        tty2oledVisible = false;
        forceMainDisplayRefresh();
      }
      return false;
    }
    if (tty2oledNeedsRender || !mainDisplayInitialized) {
      renderTty2Oled();
      tty2oledNeedsRender = false;
      mainDisplayInitialized = true;
    }
    return true;
  },
#else
  nullptr,
#endif
};

#endif
