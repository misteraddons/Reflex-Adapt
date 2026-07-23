#include "../product_config.h"

#include "menu_home_mode_line.h"

#include <cstdio>
#include <cstring>

#include <Arduino.h>
#include <tusb.h>

#include "../core/device_runtime_state.h"
#include "../input/mixed/input_mixed_runtime_state.h"
#include "../output/auth/auth_status.h"
#include "../output/output_runtime_state.h"
#include "../platform/display_runtime_state.h"
#include "menu_mode_labels.h"

namespace {

constexpr uint8_t kAuthKeyGlyph = 130;
constexpr uint8_t kAuthKeyCol = 122;
constexpr uint8_t kHomeModeLineRow = 2;
constexpr uint8_t kTextCharPixels = 6;
constexpr uint8_t kFullModeLinePixels = 128;
constexpr uint8_t kAuthIconModeLinePixels = 116;
constexpr uint32_t kPs4NoAuthGraceMs = 8UL * 60UL * 1000UL;

bool shouldShowAuthKeyIcon(bool showScanning) {
  if (showScanning) {
    return false;
  }
  if (outputMode == OUTPUT_XINPUT2P &&
      configuredOutputMode != OUTPUT_XINPUT &&
      autoDetectState != AUTO_STATE_XBOX_360) {
    return false;
  }
  if (configuredOutputMode == OUTPUT_AUTO &&
      autoDetectState == AUTO_STATE_PLAYSTATION) {
    return false;
  }
  return authOutputModeShouldShowKey(outputMode);
}

void drawAuthKeyIcon(bool showKey) {
#ifndef USE_I2C_DISPLAY
  (void)showKey;
  return;
#else
  if (!showKey) {
    return;
  }

  display.setFont(ReflexPad5x7);
  display.setCursor(kAuthKeyCol, kHomeModeLineRow);
  display.print((char)kAuthKeyGlyph);
  display.setFont(System5x7);
#endif
}

void formatAutoFallbackStatus(char* buffer, size_t bufferSize, bool compact) {
#if defined(AUTODETECT_SHOW_VERBOSE_OLED) && \
    (defined(AUTODETECT_VERBOSE_USB_HW_OLED) || defined(AUTODETECT_VERBOSE_OLED))
  const uint8_t descDevice = (output_autodetect_probe_counters >> 12) & 0x0F;
  const uint8_t descConfig = (output_autodetect_probe_counters >> 8) & 0x0F;
  const uint8_t interfaces = (output_autodetect_probe_counters >> 4) & 0x0F;
  const uint8_t requests = output_autodetect_probe_counters & 0x0F;
  if (compact) {
    snprintf(buffer, bufferSize, "Fb%02X%02X %X%X%X%X",
             output_autodetect_last_flags,
             output_autodetect_aux_flags,
             descDevice,
             descConfig,
             interfaces,
             requests);
    return;
  }
  snprintf(buffer, bufferSize, "Fb %02X%02X D%XC%XI%XR%X",
           output_autodetect_last_flags,
           output_autodetect_aux_flags,
           descDevice,
           descConfig,
           interfaces,
           requests);
#else
  snprintf(buffer, bufferSize, "MiSTer DIn");
#endif
}

bool formatAuthAwareOutputName(outputMode_t mode, char* buffer, size_t bufferSize, bool compact) {
  mode = canonicalizeOutputMode(mode);
  if (mode == OUTPUT_PS5) {
    if (authOutputModeHasProvider(OUTPUT_PS5)) {
      snprintf(buffer, bufferSize, compact ? "PS5 Auth" : "PS5 (Auth)");
    } else {
      snprintf(buffer, bufferSize, compact ? "PS5 Lock" : "PS5 Locked");
    }
    return true;
  }

  if (mode != OUTPUT_PS4) {
    if (mode == OUTPUT_XBOXONE) {
      if (authOutputModeHasProvider(OUTPUT_XBOXONE)) {
        snprintf(buffer, bufferSize, compact ? "XB1 Auth" : "Xbox One (Auth)");
      } else {
        snprintf(buffer, bufferSize, compact ? "XB1 Lock" : "Xbox One Locked");
      }
      return true;
    }
    return false;
  }

  if (authOutputModeHasProvider(OUTPUT_PS4)) {
    snprintf(buffer, bufferSize, compact ? "PS4 Auth" : "PS4 (Auth)");
    return true;
  }

  const uint32_t elapsedMs = millis();
  const uint32_t remainingMs = (elapsedMs >= kPs4NoAuthGraceMs)
    ? 0
    : (kPs4NoAuthGraceMs - elapsedMs);
  const uint16_t remainingSeconds = (uint16_t)((remainingMs + 999UL) / 1000UL);
  snprintf(buffer, bufferSize, compact ? "PS4 NA %u:%02u" : "PS4 NoAuth %u:%02u",
           remainingSeconds / 60,
           remainingSeconds % 60);
  return true;
}

bool formatWindowsAutoOutputName(outputMode_t mode, char* buffer, size_t bufferSize, bool compact) {
  switch (canonicalizeOutputMode(mode)) {
    case OUTPUT_HID:
    case OUTPUT_MISTER:
      snprintf(buffer, bufferSize, compact ? "Win DIn" : "Windows DIn");
      return true;
    case OUTPUT_XINPUT2P:
      snprintf(buffer, bufferSize, compact ? "Win XIn" : "Windows (XInput)");
      return true;
    case OUTPUT_KEYBOARD:
      snprintf(buffer, bufferSize, compact ? "Win Kbd" : "Windows (Keyboard)");
      return true;
    default:
      return false;
  }
}

bool formatXbox360OutputName(char* buffer, size_t bufferSize, bool compact) {
  snprintf(buffer, bufferSize, compact ? "XB360" : "Xbox 360");
  return true;
}

bool formatPlayStationResolvingName(char* buffer, size_t bufferSize, bool compact) {
  snprintf(buffer, bufferSize, compact ? "Detect PS" : "Detect PlayStation");
  return true;
}

void copyLabelChars(char* dest, size_t destSize, const char* src, uint8_t maxChars) {
  if (destSize == 0) {
    return;
  }
  size_t i = 0;
  for (; i + 1 < destSize && i < maxChars && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  dest[i] = '\0';
}

}  // namespace

const char* getOutputDisplayName() {
  static char auth_status[24];
  if (configuredOutputMode == OUTPUT_AUTO) {
    static char auto_status[24];
    if (autoDetectState == AUTO_STATE_FALLBACK_HID) {
      formatAutoFallbackStatus(auto_status, sizeof(auto_status), false);
      return auto_status;
    }
    if (autoDetectState != AUTO_STATE_IDLE) {
      if (autoDetectState == AUTO_STATE_PLAYSTATION) {
        formatPlayStationResolvingName(auth_status, sizeof(auth_status), false);
        return auth_status;
      }
      if (autoDetectState == AUTO_STATE_WINDOWS &&
          formatWindowsAutoOutputName(outputMode, auth_status, sizeof(auth_status), false)) {
        return auth_status;
      }
      if (autoDetectState == AUTO_STATE_XBOX_360) {
        formatXbox360OutputName(auth_status, sizeof(auth_status), false);
        return auth_status;
      }
      if (formatAuthAwareOutputName(outputMode, auth_status, sizeof(auth_status), false)) {
        return auth_status;
      }
      return getOutputShortName(outputMode);
    }
    snprintf(auto_status, sizeof(auto_status), "Detecting host...");
    return auto_status;
  }
  if (configuredOutputMode == OUTPUT_XINPUT) {
    formatXbox360OutputName(auth_status, sizeof(auth_status), false);
    return auth_status;
  }
  if (formatAuthAwareOutputName(outputMode, auth_status, sizeof(auth_status), false)) {
    return auth_status;
  }
  return getOutputShortName(outputMode);
}

const char* getOutputDisplayCompactName() {
  static char auth_status[16];
  if (configuredOutputMode == OUTPUT_AUTO) {
    static char auto_status[16];
    if (autoDetectState == AUTO_STATE_FALLBACK_HID) {
      formatAutoFallbackStatus(auto_status, sizeof(auto_status), true);
      return auto_status;
    }
    if (autoDetectState != AUTO_STATE_IDLE) {
      if (autoDetectState == AUTO_STATE_PLAYSTATION) {
        formatPlayStationResolvingName(auth_status, sizeof(auth_status), true);
        return auth_status;
      }
      if (autoDetectState == AUTO_STATE_WINDOWS &&
          formatWindowsAutoOutputName(outputMode, auth_status, sizeof(auth_status), true)) {
        return auth_status;
      }
      if (autoDetectState == AUTO_STATE_XBOX_360) {
        formatXbox360OutputName(auth_status, sizeof(auth_status), true);
        return auth_status;
      }
      if (formatAuthAwareOutputName(outputMode, auth_status, sizeof(auth_status), true)) {
        return auth_status;
      }
      return getOutputCompactName(outputMode);
    }
    snprintf(auto_status, sizeof(auto_status), "Detecting");
    return auto_status;
  }
  if (configuredOutputMode == OUTPUT_XINPUT) {
    formatXbox360OutputName(auth_status, sizeof(auth_status), true);
    return auth_status;
  }
  if (formatAuthAwareOutputName(outputMode, auth_status, sizeof(auth_status), true)) {
    return auth_status;
  }
  return getOutputCompactName(outputMode);
}

void renderHomeModeLine(bool showScanning, bool force) {
#ifndef USE_I2C_DISPLAY
  (void)showScanning;
  (void)force;
  return;
#else
  const bool showAuthKey = shouldShowAuthKeyIcon(showScanning);
  const uint8_t maxModeLineChars = showAuthKey ? 19 : 21;
  const uint8_t availablePixels = showAuthKey ? kAuthIconModeLinePixels : kFullModeLinePixels;
  const bool showMixedInput = !showScanning && inputMixedModeActive();
  const char* inputName = showScanning ? "Auto" : (showMixedInput ? "Mixed" : getInputShortName(deviceMode));
  const char* compactInputName = showScanning ? "Auto" : (showMixedInput ? "Mixed" : getInputCompactName(deviceMode));
  const char* outputName = getOutputDisplayName();
  const char* compactOutputName = getOutputDisplayCompactName();

  const char* displayInputName = inputName;
  const char* displayOutputName = outputName;
  if (strlen(displayInputName) + 1 + strlen(displayOutputName) > maxModeLineChars) {
    displayOutputName = compactOutputName;
  }
  if (strlen(displayInputName) + 1 + strlen(displayOutputName) > maxModeLineChars) {
    displayInputName = compactInputName;
  }

  static char lastModeLine[24] = {0};
  char modeLine[24];
  const uint8_t maxAvailableChars = availablePixels / kTextCharPixels;
  char combinedModeLine[48];
  snprintf(combinedModeLine, sizeof(combinedModeLine), "%s>%s", displayInputName, displayOutputName);
  copyLabelChars(modeLine, sizeof(modeLine), combinedModeLine, maxAvailableChars);
  bool splitModeLine = false;

  static bool lastModeLineShowAuthKey = false;
  if (!force && lastModeLineShowAuthKey == showAuthKey &&
      strcmp(lastModeLine, modeLine) == 0) {
    return;
  }

  strncpy(lastModeLine, modeLine, sizeof(lastModeLine) - 1);
  lastModeLine[sizeof(lastModeLine) - 1] = '\0';
  lastModeLineShowAuthKey = showAuthKey;

  display.clear(0, 127, kHomeModeLineRow, kHomeModeLineRow);
  display.setRow(kHomeModeLineRow);
  if (splitModeLine) {
    display.setCol(0);
    display.print(modeLine);
  } else {
    uint8_t totalPixels = strlen(modeLine) * kTextCharPixels;
    uint8_t startCol = (totalPixels < availablePixels) ? ((availablePixels - totalPixels) / 2) : 0;
    display.setCol(startCol);
    display.print(modeLine);
  }
  drawAuthKeyIcon(showAuthKey);
#endif
}
