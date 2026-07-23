#include "button_remap.h"

#include <string.h>

#include "eeprom_helper.h"
#include "settings_store.h"

namespace {

#define REMAP_SLOT_A       0
#define REMAP_SLOT_B       1
#define REMAP_SLOT_X       2
#define REMAP_SLOT_Y       3
#define REMAP_SLOT_L1      4
#define REMAP_SLOT_R1      5
#define REMAP_SLOT_L2      6
#define REMAP_SLOT_R2      7
#define REMAP_SLOT_L3      8
#define REMAP_SLOT_R3      9
#define REMAP_SLOT_START  10
#define REMAP_SLOT_SELECT 11
#define REMAP_SLOT_HOME   12
#define REMAP_SLOT_CAPTURE 13
#define REMAP_SLOT_EXTRA1 14
#define REMAP_SLOT_EXTRA4 15
#define REMAP_SLOT_EXTRA0 16
#define REMAP_SLOT_EXTRA2 17
#define REMAP_SLOT_EXTRA3 18

constexpr uint8_t kGenericRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_L3, REMAP_SLOT_R3, REMAP_SLOT_SELECT, REMAP_SLOT_START,
  REMAP_SLOT_HOME, REMAP_SLOT_CAPTURE
};

#ifdef ENABLE_INPUT_SMS
constexpr uint8_t kSmsRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_JPC
constexpr uint8_t kJpcRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_DRIVING
constexpr uint8_t kDrivingRemapSlots[] = {
  REMAP_SLOT_A
};
#endif

#ifdef ENABLE_INPUT_NES
constexpr uint8_t kNesRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_SNES
constexpr uint8_t kSnesRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_VBOY
constexpr uint8_t kVirtualBoyRemapSlots[] = {
  REMAP_SLOT_X, REMAP_SLOT_L2, REMAP_SLOT_Y, REMAP_SLOT_R2,
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_L1, REMAP_SLOT_R1,
  REMAP_SLOT_START, REMAP_SLOT_SELECT
};
#endif

#ifdef ENABLE_INPUT_PSX
constexpr uint8_t kPsxRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_L3, REMAP_SLOT_R3, REMAP_SLOT_SELECT, REMAP_SLOT_START
};

constexpr uint8_t kPsxJogRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_START, REMAP_SLOT_SELECT
};

constexpr uint8_t kPsxNegconRemapSlots[] = {
  REMAP_SLOT_B, REMAP_SLOT_Y, REMAP_SLOT_A, REMAP_SLOT_X,
  REMAP_SLOT_R1, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_SATURN
constexpr uint8_t kSaturnRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_R1, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_L2, REMAP_SLOT_R2, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_MEGADRIVE
constexpr uint8_t kMegadriveRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_R1, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R2, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_PCE
constexpr uint8_t kPceRemapSlots[] = {
  REMAP_SLOT_B, REMAP_SLOT_A, REMAP_SLOT_R1, REMAP_SLOT_L1,
  REMAP_SLOT_X, REMAP_SLOT_Y, REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_NEOGEO
constexpr uint8_t kNeoGeoRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_3DO
constexpr uint8_t k3doRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_L1,
  REMAP_SLOT_R1, REMAP_SLOT_START, REMAP_SLOT_SELECT
};
#endif

#ifdef ENABLE_INPUT_JAGUAR
constexpr uint8_t kJaguarRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_L3,
  REMAP_SLOT_EXTRA1, REMAP_SLOT_R3, REMAP_SLOT_L2, REMAP_SLOT_EXTRA4,
  REMAP_SLOT_R2, REMAP_SLOT_L1, REMAP_SLOT_Y, REMAP_SLOT_R1,
  REMAP_SLOT_EXTRA2, REMAP_SLOT_EXTRA0, REMAP_SLOT_EXTRA3,
  REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_N64
constexpr uint8_t kN64RemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_L2, REMAP_SLOT_L1, REMAP_SLOT_R1,
  REMAP_SLOT_X, REMAP_SLOT_R2, REMAP_SLOT_Y, REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_GAMECUBE
constexpr uint8_t kGameCubeRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L2, REMAP_SLOT_R2, REMAP_SLOT_SELECT, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_DREAMCAST
constexpr uint8_t kDreamcastRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L2, REMAP_SLOT_R2, REMAP_SLOT_START
};
#endif

#ifdef ENABLE_INPUT_WII
constexpr uint8_t kWiiRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_START, REMAP_SLOT_SELECT, REMAP_SLOT_HOME
};
#endif

#ifdef ENABLE_INPUT_JVS
constexpr uint8_t kJvsRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_L3, REMAP_SLOT_R3, REMAP_SLOT_SELECT, REMAP_SLOT_START,
  REMAP_SLOT_HOME
};
#endif

#ifdef ENABLE_INPUT_PSX
bool livePsxControllerIsNeGcon(DeviceEnum mode) {
  return mode == RZORD_PSX &&
         mode == deviceMode &&
         strcmp(controllerFrameConst(0).controller_type_name, "NeGcon") == 0;
}
#endif

const uint8_t* getRemapSlotTable(DeviceEnum mode, uint8_t* count) {
  *count = 0;
  switch (mode) {
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      *count = sizeof(kSmsRemapSlots) / sizeof(kSmsRemapSlots[0]);
      return kSmsRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      *count = sizeof(kJpcRemapSlots) / sizeof(kJpcRemapSlots[0]);
      return kJpcRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      *count = sizeof(kDrivingRemapSlots) / sizeof(kDrivingRemapSlots[0]);
      return kDrivingRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      *count = sizeof(kNesRemapSlots) / sizeof(kNesRemapSlots[0]);
      return kNesRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      *count = sizeof(kSnesRemapSlots) / sizeof(kSnesRemapSlots[0]);
      return kSnesRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      *count = sizeof(kVirtualBoyRemapSlots) / sizeof(kVirtualBoyRemapSlots[0]);
      return kVirtualBoyRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (livePsxControllerIsNeGcon(mode)) {
        *count = sizeof(kPsxNegconRemapSlots) / sizeof(kPsxNegconRemapSlots[0]);
        return kPsxNegconRemapSlots;
      }
      *count = sizeof(kPsxRemapSlots) / sizeof(kPsxRemapSlots[0]);
      return kPsxRemapSlots;
    case RZORD_PSX_JOG:
      *count = sizeof(kPsxJogRemapSlots) / sizeof(kPsxJogRemapSlots[0]);
      return kPsxJogRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      *count = sizeof(kSaturnRemapSlots) / sizeof(kSaturnRemapSlots[0]);
      return kSaturnRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      *count = sizeof(kMegadriveRemapSlots) / sizeof(kMegadriveRemapSlots[0]);
      return kMegadriveRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      *count = sizeof(kPceRemapSlots) / sizeof(kPceRemapSlots[0]);
      return kPceRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      *count = sizeof(kNeoGeoRemapSlots) / sizeof(kNeoGeoRemapSlots[0]);
      return kNeoGeoRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      *count = sizeof(k3doRemapSlots) / sizeof(k3doRemapSlots[0]);
      return k3doRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      *count = sizeof(kJaguarRemapSlots) / sizeof(kJaguarRemapSlots[0]);
      return kJaguarRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      *count = sizeof(kN64RemapSlots) / sizeof(kN64RemapSlots[0]);
      return kN64RemapSlots;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      *count = sizeof(kGameCubeRemapSlots) / sizeof(kGameCubeRemapSlots[0]);
      return kGameCubeRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      *count = sizeof(kDreamcastRemapSlots) / sizeof(kDreamcastRemapSlots[0]);
      return kDreamcastRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      *count = sizeof(kWiiRemapSlots) / sizeof(kWiiRemapSlots[0]);
      return kWiiRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      *count = sizeof(kJvsRemapSlots) / sizeof(kJvsRemapSlots[0]);
      return kJvsRemapSlots;
    #endif
    default:
      break;
  }
  return nullptr;
}

DeviceEnum getModeForRemapBase(uint16_t remapBase) {
  for (uint8_t modeValue = 0; modeValue < MAX_INPUT_MODES; ++modeValue) {
    DeviceEnum mode = (DeviceEnum)modeValue;
    if (isConcreteModeForPerSettings(mode) && getPerModeRemapAddress(mode) == remapBase) {
      return mode;
    }
  }
  return RZORD_NONE;
}

}  // namespace

const char* const remap_button_names[REMAP_MAX_BUTTONS] = {
  "A", "B", "X", "Y", "L1", "R1", "L2", "R2",
  "L3", "R3", "Sta", "Sel", "Home", "Cap", "Ext1", "Ext4",
  "Ext0", "Ext2", "Ext3"
};

bool psxHasL3R3() {
  #ifdef ENABLE_INPUT_PSX
  if (deviceMode != RZORD_PSX) return false;
  if (MAX_USB_OUT == 0) return false;
  const char* type = controllerFrameConst(0).controller_type_name;
  if (strcmp(type, "Digital") == 0 ||
      strcmp(type, "FlightStick") == 0 ||
      strcmp(type, "NeGcon") == 0 ||
      strncmp(type, "JogCon", 6) == 0 ||
      strcmp(type, "GunCon") == 0 ||
      strcmp(type, "Mouse") == 0 ||
      strcmp(type, "IR Remote") == 0) {
    return false;
  }
  return true;
  #else
  return false;
  #endif
}

uint32_t getInputButtonMask(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L2 | BTN_BIT_R2 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (livePsxControllerIsNeGcon(mode)) {
        return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
               BTN_BIT_R1 | BTN_BIT_START;
      }
      if (psxHasL3R3()) {
        return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
               BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
               BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START;
      }
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START;
    case RZORD_PSX_JOG:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START |
             BTN_BIT_EXTRA0 | BTN_BIT_EXTRA1 | BTN_BIT_EXTRA2 |
             BTN_BIT_EXTRA3 | BTN_BIT_EXTRA4;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START | BTN_BIT_HOME;
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START | BTN_BIT_HOME;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L2 | BTN_BIT_R2 | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      return BTN_BIT_A;
    #endif
    #ifdef ENABLE_INPUT_ATARI
    case RZORD_ATARI:
      return BTN_BIT_A;
    #endif
    #ifdef ENABLE_INPUT_INTELLIVISION
    case RZORD_INTELLIVISION:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    default:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
  }
}

uint8_t getRemapButtonCount(DeviceEnum mode) {
  uint8_t table_count = 0;
  if (getRemapSlotTable(mode, &table_count) != nullptr) {
    return table_count;
  }
  return sizeof(kGenericRemapSlots) / sizeof(kGenericRemapSlots[0]);
}

uint8_t getRemapButtonSlot(DeviceEnum mode, uint8_t display_index) {
  uint8_t table_count = 0;
  const uint8_t* table = getRemapSlotTable(mode, &table_count);
  if (table != nullptr) {
    return (display_index < table_count) ? table[display_index] : 0xFF;
  }
  return (display_index < getRemapButtonCount(mode)) ? kGenericRemapSlots[display_index] : 0xFF;
}

uint8_t getRemapButtonDisplayIndex(DeviceEnum mode, uint8_t button_slot) {
  uint8_t table_count = 0;
  const uint8_t* table = getRemapSlotTable(mode, &table_count);
  if (table != nullptr) {
    for (uint8_t i = 0; i < table_count; ++i) {
      if (table[i] == button_slot) {
        return i;
      }
    }
    return 0xFF;
  }
  for (uint8_t i = 0; i < getRemapButtonCount(mode); ++i) {
    if (kGenericRemapSlots[i] == button_slot) {
      return i;
    }
  }
  return 0xFF;
}

const char* getRemapButtonName(DeviceEnum mode, uint8_t index) {
  switch (mode) {
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      switch (index) {
        case 0: return "1";
        case 1: return "2";
        case 2: return "Start";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      switch (index) {
        case 0: return "1";
        case 1: return "2";
        case 2: return "Sel";
        case 3: return "Run";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      switch (index) {
        case 0: return "Fire";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "Sel";
        case 3: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "Sel";
        case 7: return "Sta";
      }
      break;
    #endif
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (livePsxControllerIsNeGcon(mode)) {
        switch (index) {
          case 0: return "A";
          case 1: return "B";
          case 2: return "I";
          case 3: return "II";
          case 4: return "R";
          case 5: return "Sta";
        }
        break;
      }
      switch (index) {
        case 0: return "X";
        case 1: return "O";
        case 2: return "Sq";
        case 3: return "Tr";
        case 4: return "L1";
        case 5: return "R1";
        case 6: return "L2";
        case 7: return "R2";
        case 8: return "L3";
        case 9: return "R3";
        case 10: return "Sel";
        case 11: return "Sta";
      }
      break;
    case RZORD_PSX_JOG:
      switch (index) {
        case 0: return "X";
        case 1: return "O";
        case 2: return "Sq";
        case 3: return "Tr";
        case 4: return "L1";
        case 5: return "R1";
        case 6: return "L2";
        case 7: return "R2";
        case 8: return "Sta";
        case 9: return "Sel";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "X";
        case 4: return "Y";
        case 5: return "Z";
        case 6: return "L";
        case 7: return "R";
        case 8: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      switch (index) {
        case 0: return "R-Up";
        case 1: return "R-Down";
        case 2: return "R-Left";
        case 3: return "R-Right";
        case 4: return "A";
        case 5: return "B";
        case 6: return "L";
        case 7: return "R";
        case 8: return "Sta";
        case 9: return "Sel";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "X";
        case 4: return "Y";
        case 5: return "Z";
        case 6: return "Mode";
        case 7: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      switch (index) {
        case 0: return "I";
        case 1: return "II";
        case 2: return "III";
        case 3: return "IV";
        case 4: return "V";
        case 5: return "VI";
        case 6: return "Sel";
        case 7: return "Run";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "D";
        case 4: return "Sel";
        case 5: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "L";
        case 4: return "R";
        case 5: return "P";
        case 6: return "X";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "1";
        case 4: return "2";
        case 5: return "3";
        case 6: return "4";
        case 7: return "5";
        case 8: return "6";
        case 9: return "7";
        case 10: return "8";
        case 11: return "9";
        case 12: return "*";
        case 13: return "0";
        case 14: return "#";
        case 15: return "Opt";
        case 16: return "Pau";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "Z";
        case 3: return "L";
        case 4: return "R";
        case 5: return "C-U";
        case 6: return "C-D";
        case 7: return "C-L";
        case 8: return "C-R";
        case 9: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "Z";
        case 7: return "Sta";
        case 8: return "D-U";
        case 9: return "D-D";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "ZL";
        case 7: return "ZR";
        case 8: return "+";
        case 9: return "-";
        case 10: return "Hom";
      }
      break;
    #endif
    default:
      break;
  }
  if (index < REMAP_MAX_BUTTONS) {
    return remap_button_names[index];
  }
  return "?";
}

const char* getRemapButtonNameForSlot(DeviceEnum mode, uint8_t button_slot) {
  const uint8_t display_index = getRemapButtonDisplayIndex(mode, button_slot);
  if (display_index != 0xFF) {
    return getRemapButtonName(mode, display_index);
  }
  if (button_slot < REMAP_MAX_BUTTONS) {
    return remap_button_names[button_slot];
  }
  return "?";
}

ButtonRemapMenu remapMenu;

uint8_t active_remaps[REMAP_MAX_BUTTONS] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18
};

bool hasActiveRemap() {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    if (active_remaps[i] != i) return true;
  }
  return false;
}

void loadRemapsFromEEPROM() {
  const DeviceEnum remapMode = getModeForRemapBase(g_eeprom_remap_index);
  if (!isConcreteModeForPerSettings(remapMode)) {
    clearRemapsToDefault();
    return;
  }

  PerModeSettingsRecord settings{};
  readPerModeSettings(remapMode, settings);
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    uint8_t val = settings.remaps[i];
    active_remaps[i] = (val < REMAP_MAX_BUTTONS) ? val : i;
  }
}

void writeRemapsToEEPROM() {
  const DeviceEnum remapMode = getModeForRemapBase(g_eeprom_remap_index);
  if (!isConcreteModeForPerSettings(remapMode)) {
    return;
  }

  PerModeSettingsRecord settings{};
  readPerModeSettings(remapMode, settings);
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    settings.remaps[i] = (active_remaps[i] < REMAP_MAX_BUTTONS) ? active_remaps[i] : i;
  }
  sanitizePerModeSettings(remapMode, settings);
  writePerModeSettings(remapMode, settings);
}

void saveRemapsToEEPROM() {
  writeRemapsToEEPROM();
}

void clearRemapsToDefault() {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    active_remaps[i] = i;
  }
}
