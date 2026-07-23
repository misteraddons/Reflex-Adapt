#include "classic_dual_merge_config.h"

#include <string.h>

#include "button_chord_remap.h"
#include "controller_frame_state.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "eeprom_helper.h"
#include "settings_store.h"
#include "../input/shared/input_button_bits.h"
#include "../menu/menu_runtime_state.h"

#ifdef ENABLE_BLUETOOTH_PAIRING
#include "bluetooth_bond_storage.h"
#endif

namespace {

constexpr uint16_t kPersistedBlockMagicClassicDualMergeLegacy = 0x324D;  // M2
constexpr uint16_t kPersistedBlockMagicClassicDualMerge = 0x334D;  // M3
constexpr uint32_t classicDualMergeExtraMask(uint8_t bit) {
  return 1UL << (14 + bit);
}

constexpr uint32_t kClassicDualMergeExtra0Mask = classicDualMergeExtraMask(0);
constexpr uint32_t kClassicDualMergeExtra1Mask = classicDualMergeExtraMask(1);
constexpr uint32_t kClassicDualMergeExtra2Mask = classicDualMergeExtraMask(2);
constexpr uint32_t kClassicDualMergeExtra3Mask = classicDualMergeExtraMask(3);
constexpr uint32_t kClassicDualMergeExtra4Mask = classicDualMergeExtraMask(4);
constexpr uint32_t kClassicDualMergeExtra5Mask = classicDualMergeExtraMask(5);
constexpr uint32_t kClassicDualMergeExtra6Mask = classicDualMergeExtraMask(6);
constexpr uint32_t kClassicDualMergeExtra7Mask = classicDualMergeExtraMask(7);
constexpr uint32_t kClassicDualMergeExtra8Mask = classicDualMergeExtraMask(8);
constexpr uint32_t kClassicDualMergeExtra9Mask = classicDualMergeExtraMask(9);
constexpr uint32_t kClassicDualMergeNttExtraMask =
  kClassicDualMergeExtra0Mask | kClassicDualMergeExtra1Mask |
  kClassicDualMergeExtra2Mask | kClassicDualMergeExtra3Mask |
  kClassicDualMergeExtra4Mask | kClassicDualMergeExtra5Mask |
  kClassicDualMergeExtra6Mask | kClassicDualMergeExtra7Mask |
  kClassicDualMergeExtra8Mask | kClassicDualMergeExtra9Mask;
constexpr uint32_t kClassicDualMergeJaguar2Mask = kClassicDualMergeExtra1Mask;
constexpr uint32_t kClassicDualMergeJaguar5Mask = kClassicDualMergeExtra4Mask;
constexpr uint32_t kClassicDualMergeJaguarStarMask = kClassicDualMergeExtra2Mask;
constexpr uint32_t kClassicDualMergeJaguar0Mask = kClassicDualMergeExtra0Mask;
constexpr uint32_t kClassicDualMergeJaguarHashMask = kClassicDualMergeExtra3Mask;

struct ClassicDualMergeLegacyRecord {
  uint32_t valid_modes;
  uint32_t p2_masks[MAX_INPUT_MODES];
};

struct ClassicDualMergeRecord {
  uint32_t valid_modes;
  uint32_t enabled_modes;
  uint32_t p2_masks[MAX_INPUT_MODES];
};

constexpr uint16_t kClassicDualMergeLegacyRecordSize =
  PERSISTED_BLOCK_HEADER_SIZE + sizeof(ClassicDualMergeLegacyRecord);
constexpr uint16_t kClassicDualMergeRecordSize =
  PERSISTED_BLOCK_HEADER_SIZE + sizeof(ClassicDualMergeRecord);

struct ClassicDualMergeButtonDef {
  uint32_t mask;
  const char* genericName;
};

constexpr ClassicDualMergeButtonDef kMergeButtons[CLASSIC_DUAL_MERGE_BUTTON_COUNT] = {
  { INPUT_PAD_U, "DPad Up" },
  { INPUT_PAD_D, "DPad Down" },
  { INPUT_PAD_L, "DPad Left" },
  { INPUT_PAD_R, "DPad Right" },
  { INPUT_A, "A" },
  { INPUT_B, "B" },
  { INPUT_X, "X" },
  { INPUT_Y, "Y" },
  { INPUT_L1, "L1" },
  { INPUT_R1, "R1" },
  { INPUT_L2, "L2" },
  { INPUT_R2, "R2" },
  { INPUT_L3, "L3" },
  { INPUT_R3, "R3" },
  { kClassicDualMergeExtra0Mask, "0" },
  { kClassicDualMergeExtra1Mask, "1" },
  { kClassicDualMergeExtra2Mask, "2" },
  { kClassicDualMergeExtra3Mask, "3" },
  { kClassicDualMergeExtra4Mask, "4" },
  { kClassicDualMergeExtra5Mask, "5" },
  { kClassicDualMergeExtra6Mask, "6" },
  { kClassicDualMergeExtra7Mask, "7" },
  { kClassicDualMergeExtra8Mask, "8" },
  { kClassicDualMergeExtra9Mask, "9" },
  { INPUT_START, "Start" },
  { INPUT_SELECT, "Select" },
  { INPUT_HOME, "Home" },
  { INPUT_CAPTURE, "Capture" },
  { 0, nullptr },
};

constexpr ClassicDualMergeButtonDef kJaguarMergeButtons[CLASSIC_DUAL_MERGE_BUTTON_COUNT] = {
  { INPUT_PAD_U, "DPad Up" },
  { INPUT_PAD_D, "DPad Down" },
  { INPUT_PAD_L, "DPad Left" },
  { INPUT_PAD_R, "DPad Right" },
  { INPUT_A, "A" },
  { INPUT_B, "B" },
  { INPUT_X, "C" },
  { INPUT_L3, "1" },
  { kClassicDualMergeJaguar2Mask, "2" },
  { INPUT_R3, "3" },
  { INPUT_L2, "4" },
  { kClassicDualMergeJaguar5Mask, "5" },
  { INPUT_R2, "6" },
  { INPUT_L1, "7" },
  { INPUT_Y, "8" },
  { INPUT_R1, "9" },
  { kClassicDualMergeJaguarStarMask, "*" },
  { kClassicDualMergeJaguar0Mask, "0" },
  { kClassicDualMergeJaguarHashMask, "#" },
  { INPUT_START, "Pause" },
  { INPUT_SELECT, "Option" },
  { 0, nullptr },
};

const ClassicDualMergeButtonDef* mergeButtonsForMode(DeviceEnum mode) {
#ifdef ENABLE_INPUT_JAGUAR
  if (mode == RZORD_JAGUAR) {
    return kJaguarMergeButtons;
  }
#else
  (void)mode;
#endif
  return kMergeButtons;
}

uint16_t classicDualMergeRecordBase() {
#ifdef ENABLE_BLUETOOTH_PAIRING
  return bluetoothBondStorageRequiredEnd();
#else
  return buttonChordStorageRequiredEnd();
#endif
}

bool validMode(DeviceEnum mode) {
  return mode > RZORD_NONE && mode < RZORD_LAST && (uint8_t)mode < MAX_INPUT_MODES;
}

bool livePrimaryControllerIsNeGcon(DeviceEnum mode) {
#ifdef ENABLE_INPUT_PSX
  return mode == RZORD_PSX &&
         mode == deviceMode &&
         strcmp(controllerFrameConst(0).controller_type_name, "NeGcon") == 0;
#else
  (void)mode;
  return false;
#endif
}

bool controllerTypeIsJogCon(const char* controllerTypeName) {
  return controllerTypeName != nullptr &&
         strncmp(controllerTypeName, "JogCon", 6) == 0;
}

bool livePrimaryControllerIsJogCon(DeviceEnum mode) {
#ifdef ENABLE_INPUT_PSX
  return mode == RZORD_PSX &&
         mode == deviceMode &&
         controllerTypeIsJogCon(controllerFrameConst(0).controller_type_name);
#else
  (void)mode;
  return false;
#endif
}

uint32_t modeBit(DeviceEnum mode) {
  if (!validMode(mode) || (uint8_t)mode >= 32) {
    return 0;
  }
  return 1UL << (uint8_t)mode;
}

bool legacyGlobalClassicDualMergeEnabled() {
  GlobalSettingsRecord globalSettings{};
  if (!readGlobalSettings(globalSettings)) {
    return false;
  }
  return (globalSettings.home_screen_debug & CLASSIC_DUAL_MERGE_MASK) != 0;
}

uint32_t baseAllowedMaskForMode(DeviceEnum mode) {
  uint32_t mask = INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R |
                  INPUT_A | INPUT_B | INPUT_START;
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_L3 | INPUT_R3 | INPUT_SELECT | INPUT_HOME |
             kClassicDualMergeNttExtraMask;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_L3 | INPUT_R3 | INPUT_SELECT | INPUT_HOME |
             kClassicDualMergeNttExtraMask;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return mask | INPUT_X | INPUT_Y | INPUT_L2 | INPUT_R2 | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_SELECT | INPUT_HOME;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (livePrimaryControllerIsJogCon(mode)) {
        return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
               INPUT_R2 | INPUT_SELECT;
      }
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_L3 | INPUT_R3 | INPUT_SELECT;
    case RZORD_PSX_JOG:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 | INPUT_R2;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_R2;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return mask | INPUT_X | INPUT_Y | INPUT_L2 | INPUT_R2;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      return mask | INPUT_X | INPUT_Y | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      return mask | INPUT_X | INPUT_L1 | INPUT_R1 | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return mask | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_L3 | INPUT_R3 | INPUT_SELECT |
             kClassicDualMergeJaguar0Mask | kClassicDualMergeJaguar2Mask |
             kClassicDualMergeJaguarStarMask | kClassicDualMergeJaguarHashMask |
             kClassicDualMergeJaguar5Mask;
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      return mask;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      return mask | INPUT_SELECT;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      return INPUT_A;
    #endif
    default:
      return mask;
  }
}

uint32_t sourceSpecificAllowedMaskForMode(DeviceEnum mode, const char* controllerTypeName) {
  const uint32_t base = INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R |
                        INPUT_A | INPUT_B | INPUT_START;
  if (controllerTypeName == nullptr || controllerTypeName[0] == '\0') {
    return baseAllowedMaskForMode(mode);
  }

  #ifdef ENABLE_INPUT_SNES
  if (mode == RZORD_SNES || mode == RZORD_NES || mode == RZORD_VBOY) {
    if (strcmp(controllerTypeName, "NTT Data") == 0) {
      return base | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_L3 | INPUT_R3 | INPUT_SELECT | INPUT_HOME |
             kClassicDualMergeNttExtraMask;
    }
    if (strcmp(controllerTypeName, "VB Pad") == 0) {
      return base | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_SELECT;
    }
    if (strcmp(controllerTypeName, "Power Pad") == 0) {
      return base | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
             INPUT_R2 | INPUT_L3 | INPUT_R3 | INPUT_SELECT;
    }
    if (strcmp(controllerTypeName, "Zapper") == 0) {
      return INPUT_A | INPUT_B;
    }
    if (strcmp(controllerTypeName, "NES Pad") == 0 ||
        strcmp(controllerTypeName, "Four Score") == 0) {
      return base | INPUT_SELECT;
    }
    if (strcmp(controllerTypeName, "SNES Pad") == 0 ||
        strcmp(controllerTypeName, "Multitap") == 0) {
      return base | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_SELECT;
    }
  }
  #endif

  #ifdef ENABLE_INPUT_PCE
  if (mode == RZORD_PCE) {
    if (strcmp(controllerTypeName, "2-Button") == 0) {
      return base | INPUT_SELECT;
    }
    if (strcmp(controllerTypeName, "6-Button") == 0) {
      return baseAllowedMaskForMode(mode);
    }
  }
  #endif

  #ifdef ENABLE_INPUT_PSX
  if (mode == RZORD_PSX && controllerTypeIsJogCon(controllerTypeName)) {
    return base | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1 | INPUT_L2 |
           INPUT_R2 | INPUT_SELECT;
  }
  #endif

  #ifdef ENABLE_INPUT_MEGADRIVE
  if (mode == RZORD_MEGADRIVE) {
    if (strcmp(controllerTypeName, "Mega3") == 0) {
      return base | INPUT_R1;
    }
    if (strcmp(controllerTypeName, "Mega6") == 0) {
      return baseAllowedMaskForMode(mode);
    }
  }
  #endif

  return baseAllowedMaskForMode(mode);
}

ClassicDualMergeRecord readRecord() {
  ClassicDualMergeRecord record{};
  uint8_t slot = 0;
  uint16_t generation = 0;
  if (!readLatestPersistedRecordAB(
        classicDualMergeRecordBase(),
        kClassicDualMergeRecordSize,
        kPersistedBlockMagicClassicDualMerge,
        slot,
        generation,
        record)) {
    ClassicDualMergeLegacyRecord legacy{};
    if (readLatestPersistedRecordAB(
          classicDualMergeRecordBase(),
          kClassicDualMergeLegacyRecordSize,
          kPersistedBlockMagicClassicDualMergeLegacy,
          slot,
          generation,
          legacy)) {
      record.valid_modes = legacy.valid_modes;
      memcpy(record.p2_masks, legacy.p2_masks, sizeof(record.p2_masks));
      if (legacyGlobalClassicDualMergeEnabled()) {
        record.enabled_modes |= modeBit(deviceMode);
      }
      writePersistedRecordAB(
        classicDualMergeRecordBase(),
        kClassicDualMergeRecordSize,
        kPersistedBlockMagicClassicDualMerge,
        record
      );
    } else {
      memset(&record, 0, sizeof(record));
    }
  }
  return record;
}

void writeRecord(const ClassicDualMergeRecord& record) {
  writePersistedRecordAB(
    classicDualMergeRecordBase(),
    kClassicDualMergeRecordSize,
    kPersistedBlockMagicClassicDualMerge,
    record
  );
}

const char* nativeNameForMask(DeviceEnum mode, uint32_t mask) {
  switch (mode) {
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      if (mask == INPUT_A) return "1";
      if (mask == INPUT_B) return "2";
      break;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      if (mask == INPUT_A) return "1";
      if (mask == INPUT_B) return "2";
      if (mask == INPUT_START) return "Run";
      break;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      if (mask == INPUT_A) return "Fire";
      break;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      if (mask == INPUT_L1) return "L";
      if (mask == INPUT_R1) return "R";
      if (mask == INPUT_L2) return "*";
      if (mask == INPUT_R2) return "C";
      if (mask == INPUT_L3) return "#";
      if (mask == INPUT_R3) return ".";
      if (mask == INPUT_HOME) return "=";
      break;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      if (mask == INPUT_L2) return "Z";
      if (mask == INPUT_X) return "C-Up";
      if (mask == INPUT_L3) return "C-Down";
      if (mask == INPUT_Y) return "C-Left";
      if (mask == INPUT_R3) return "C-Right";
      if (mask == INPUT_L1) return "L";
      if (mask == INPUT_R1) return "R";
      break;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      if (mask == INPUT_L2) return "L";
      if (mask == INPUT_R2) return "R";
      if (mask == INPUT_SELECT) return "Z";
      break;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      if (mask == INPUT_SELECT) return "-";
      if (mask == INPUT_START) return "+";
      if (mask == INPUT_L1) return "L";
      if (mask == INPUT_R1) return "R";
      if (mask == INPUT_L2) return "ZL";
      if (mask == INPUT_R2) return "ZR";
      if (mask == INPUT_HOME) return "Home";
      break;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    case RZORD_PSX_JOG:
      if (mask == INPUT_A) return "Cross";
      if (mask == INPUT_B) return "Circle";
      if (mask == INPUT_X) return "Square";
      if (mask == INPUT_Y) return "Triangle";
      break;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      if (mask == INPUT_L1) return "Z";
      if (mask == INPUT_R1) return "C";
      if (mask == INPUT_L2) return "L";
      if (mask == INPUT_R2) return "R";
      break;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      if (mask == INPUT_L1) return "Z";
      if (mask == INPUT_R1) return "C";
      if (mask == INPUT_R2) return "Mode";
      break;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      if (mask == INPUT_A) return "I";
      if (mask == INPUT_B) return "II";
      if (mask == INPUT_R1) return "III";
      if (mask == INPUT_L1) return "IV";
      if (mask == INPUT_X) return "V";
      if (mask == INPUT_Y) return "VI";
      if (mask == INPUT_START) return "Run";
      break;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      if (mask == INPUT_X) return "RPad U";
      if (mask == INPUT_Y) return "RPad L";
      if (mask == INPUT_L2) return "RPad D";
      if (mask == INPUT_R2) return "RPad R";
      if (mask == INPUT_L1) return "L";
      if (mask == INPUT_R1) return "R";
      break;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      if (mask == INPUT_X) return "C";
      if (mask == INPUT_Y) return "D";
      break;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      if (mask == INPUT_X) return "C";
      if (mask == INPUT_L1) return "L";
      if (mask == INPUT_R1) return "R";
      if (mask == INPUT_START) return "P";
      if (mask == INPUT_SELECT) return "X";
      break;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      if (mask == INPUT_X) return "C";
      if (mask == INPUT_Y) return "8";
      if (mask == INPUT_L1) return "7";
      if (mask == INPUT_R1) return "9";
      if (mask == INPUT_L2) return "4";
      if (mask == INPUT_R2) return "6";
      if (mask == INPUT_L3) return "1";
      if (mask == INPUT_R3) return "3";
      if (mask == kClassicDualMergeJaguar0Mask) return "0";
      if (mask == kClassicDualMergeJaguar2Mask) return "2";
      if (mask == kClassicDualMergeJaguarStarMask) return "*";
      if (mask == kClassicDualMergeJaguarHashMask) return "#";
      if (mask == kClassicDualMergeJaguar5Mask) return "5";
      if (mask == INPUT_START) return "Pause";
      if (mask == INPUT_SELECT) return "Option";
      break;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      if (mask == INPUT_L2) return "L";
      if (mask == INPUT_R2) return "R";
      break;
    #endif
    default:
      break;
  }
  return nullptr;
}

}  // namespace

uint32_t classic_dual_merge_p2_mask = 0;

uint32_t defaultClassicDualMergeP2Mask(DeviceEnum mode) {
  if (livePrimaryControllerIsNeGcon(mode)) {
    return 0;
  }
  uint32_t mask = INPUT_A | INPUT_B | INPUT_X | INPUT_Y |
                  INPUT_L1 | INPUT_R1 | INPUT_L2 | INPUT_R2;
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      mask = INPUT_A | INPUT_B;
      break;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      mask = INPUT_A | INPUT_B | INPUT_X | INPUT_Y | INPUT_L1 | INPUT_R1;
      break;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      mask |= INPUT_SELECT;
      break;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      mask |= INPUT_SELECT;
      break;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (!livePrimaryControllerIsJogCon(mode)) {
        mask |= INPUT_L3 | INPUT_R3;
      }
      break;
    case RZORD_PSX_JOG:
      break;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      mask |= INPUT_L3 | INPUT_R3 |
              kClassicDualMergeJaguar0Mask | kClassicDualMergeJaguar2Mask |
              kClassicDualMergeJaguarStarMask | kClassicDualMergeJaguarHashMask |
              kClassicDualMergeJaguar5Mask;
      break;
    #endif
    default:
      break;
  }
  return sanitizeClassicDualMergeP2Mask(mode, mask);
}

uint32_t sanitizeClassicDualMergeP2Mask(DeviceEnum mode, uint32_t mask) {
  if (livePrimaryControllerIsNeGcon(mode)) {
    return 0;
  }
  return mask & baseAllowedMaskForMode(mode);
}

uint32_t getClassicDualMergeDisplayMask(DeviceEnum mode) {
  if (livePrimaryControllerIsNeGcon(mode)) {
    return 0;
  }
  uint32_t allowed = baseAllowedMaskForMode(mode);
  if (mode == deviceMode && max_devices >= 2 && MAX_USB_OUT >= 2) {
    const controller_state_t& source = controllerFrameConst(1);
    if (source.connected) {
      allowed = sourceSpecificAllowedMaskForMode(mode, source.controller_type_name);
    }
  }
  return allowed & baseAllowedMaskForMode(mode);
}

bool classicDualMergeEnabledForMode(DeviceEnum mode) {
  const uint32_t bit = modeBit(mode);
  if (bit == 0) {
    return false;
  }
  ClassicDualMergeRecord record = readRecord();
  return (record.enabled_modes & bit) != 0;
}

uint32_t storedClassicDualMergeP2Mask(DeviceEnum mode) {
  if (!validMode(mode)) {
    return 0;
  }
  ClassicDualMergeRecord record = readRecord();
  const uint32_t bit = modeBit(mode);
  const uint8_t index = (uint8_t)mode;
  if (bit == 0 || (record.valid_modes & bit) == 0) {
    return defaultClassicDualMergeP2Mask(mode);
  }
  return sanitizeClassicDualMergeP2Mask(mode, record.p2_masks[index]);
}

void loadClassicDualMergeConfigForMode(DeviceEnum mode) {
  classic_dual_merge_p2_mask = storedClassicDualMergeP2Mask(mode);
  menu_classic_dual_merge = classicDualMergeEnabledForMode(mode) ? 1 : 0;
  classic_dual_merge_enabled = menu_classic_dual_merge;
}

void saveClassicDualMergeEnabledForMode(DeviceEnum mode, bool enabled) {
  const uint32_t bit = modeBit(mode);
  if (bit == 0) {
    return;
  }
  ClassicDualMergeRecord record = readRecord();
  if (enabled) {
    record.enabled_modes |= bit;
  } else {
    record.enabled_modes &= ~bit;
  }
  writeRecord(record);
  if (mode == deviceMode) {
    menu_classic_dual_merge = enabled ? 1 : 0;
    classic_dual_merge_enabled = menu_classic_dual_merge;
  }
}

void saveClassicDualMergeP2MaskForMode(DeviceEnum mode, uint32_t mask) {
  const uint32_t bit = modeBit(mode);
  if (bit == 0) {
    return;
  }
  ClassicDualMergeRecord record = readRecord();
  const uint8_t index = (uint8_t)mode;
  record.valid_modes |= bit;
  record.p2_masks[index] = sanitizeClassicDualMergeP2Mask(mode, mask);
  writeRecord(record);
  if (mode == deviceMode) {
    classic_dual_merge_p2_mask = record.p2_masks[index];
  }
}

void clearClassicDualMergeConfig() {
  ClassicDualMergeRecord empty{};
  writeRecord(empty);
  classic_dual_merge_p2_mask = defaultClassicDualMergeP2Mask(deviceMode);
  menu_classic_dual_merge = 0;
  classic_dual_merge_enabled = 0;
}

uint16_t classicDualMergeStorageRequiredEnd() {
  return classicDualMergeRecordBase() +
         (PERSISTED_AB_SLOT_COUNT * kClassicDualMergeRecordSize);
}

uint8_t getClassicDualMergeButtonCount(DeviceEnum mode) {
  const uint32_t allowed = getClassicDualMergeDisplayMask(mode);
  const ClassicDualMergeButtonDef* buttons = mergeButtonsForMode(mode);
  uint8_t count = 0;
  for (uint8_t i = 0; i < CLASSIC_DUAL_MERGE_BUTTON_COUNT; ++i) {
    if (buttons[i].mask != 0 && (allowed & buttons[i].mask) != 0) {
      ++count;
    }
  }
  return count;
}

uint32_t getClassicDualMergeButtonMask(DeviceEnum mode, uint8_t displayIndex) {
  const uint32_t allowed = getClassicDualMergeDisplayMask(mode);
  const ClassicDualMergeButtonDef* buttons = mergeButtonsForMode(mode);
  uint8_t visible = 0;
  for (uint8_t i = 0; i < CLASSIC_DUAL_MERGE_BUTTON_COUNT; ++i) {
    const uint32_t mask = buttons[i].mask;
    if (mask == 0 || (allowed & mask) == 0) {
      continue;
    }
    if (visible == displayIndex) {
      return mask;
    }
    ++visible;
  }
  return 0;
}

const char* getClassicDualMergeButtonName(DeviceEnum mode, uint8_t displayIndex) {
  const uint32_t mask = getClassicDualMergeButtonMask(mode, displayIndex);
  if (mask == 0) {
    return "?";
  }
  if (const char* native = nativeNameForMask(mode, mask)) {
    return native;
  }
  const ClassicDualMergeButtonDef* buttons = mergeButtonsForMode(mode);
  for (uint8_t i = 0; i < CLASSIC_DUAL_MERGE_BUTTON_COUNT; ++i) {
    if (buttons[i].mask == mask) {
      return buttons[i].genericName;
    }
  }
  return "?";
}
