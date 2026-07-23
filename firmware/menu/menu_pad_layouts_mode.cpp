#include "../product_config.h"

#include "menu_pad_layouts_internal.h"

#include <cstring>

#include "../core/controller_frame_state.h"
#ifdef ENABLE_INPUT_JVS
#include "../input/jvs/jvs_host_runtime.h"
#endif

using namespace menu_pad_layouts_internal;

bool isDrivingFallbackActive() {
  #ifdef ENABLE_INPUT_DRIVING
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (std::strncmp(frame.controller_type_name, "Driving", sizeof(frame.controller_type_name)) == 0) {
      return true;
    }
  }
  #endif
  return false;
}

bool getSharedControllerTypePadLayout(const char* typeName, const PadButton** layout, uint8_t* layoutCount) {
  #ifdef ENABLE_INPUT_USB
  if (std::strcmp(typeName, "Switch") == 0) {
    *layout = padLayoutUsbSwitch;
    *layoutCount = PAD_LAYOUT_USB_SWITCH_COUNT;
    return true;
  }
  if (std::strcmp(typeName, "DualShock3") == 0) {
    *layout = padLayoutPS3;
    *layoutCount = PAD_LAYOUT_PS3_COUNT;
    return true;
  }
  if (std::strcmp(typeName, "DualShock4") == 0 ||
      std::strcmp(typeName, "DualSense") == 0) {
    *layout = padLayoutPSX;
    *layoutCount = PAD_LAYOUT_PSX_COUNT;
    return true;
  }
  if (std::strcmp(typeName, "XInput") == 0 ||
      std::strcmp(typeName, "8BitDo") == 0 ||
      std::strcmp(typeName, "USB Pad") == 0 ||
      std::strcmp(typeName, "USB") == 0) {
    *layout = padLayoutUsbModern;
    *layoutCount = PAD_LAYOUT_USB_MODERN_COUNT;
    return true;
  }
  if (std::strcmp(typeName, "USB Dig") == 0) {
    *layout = padLayoutSnes;
    *layoutCount = PAD_LAYOUT_SNES_COUNT;
    return true;
  }
  #endif

  if (std::strcmp(typeName, "Dance Pad") == 0) {
    *layout = padLayoutDancePad;
    *layoutCount = PAD_LAYOUT_DANCEPAD_COUNT;
    return true;
  }

  #ifdef ENABLE_INPUT_PSX
  if (std::strcmp(typeName, "Pop'n") == 0 || std::strcmp(typeName, "PopN") == 0) {
    *layout = padLayoutPopn;
    *layoutCount = PAD_LAYOUT_POPN_COUNT;
    return true;
  }
  if (std::strcmp(typeName, "GuitarFreaks") == 0) {
    *layout = padLayoutGuitarFreaks;
    *layoutCount = PAD_LAYOUT_GUITARFREAKS_COUNT;
    return true;
  }
  #endif

  return false;
}

void getPadLayoutForMode(DeviceEnum mode, const PadButton** layout, uint8_t* count, const char** name) {
  switch (mode) {
#ifdef ENABLE_INPUT_USB
    case RZORD_USB:
      *layout = padLayoutUsbModern;
      *count = PAD_LAYOUT_USB_MODERN_COUNT;
      *name = "USB";
      break;
#endif
#ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      *layout = padLayoutNES;
      *count = PAD_LAYOUT_NES_COUNT;
      *name = "NES";
      break;
#endif
#ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      *layout = padLayoutWii;
      *count = PAD_LAYOUT_WII_COUNT;
      *name = "Wii Classic";
      break;
#endif
#ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      *layout = padLayoutSnes;
      *count = PAD_LAYOUT_SNES_COUNT;
      *name = "SNES";
      break;
#endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      *layout = padLayoutVBoy;
      *count = PAD_LAYOUT_VBOY_COUNT;
      *name = "Virtual Boy";
      break;
    #endif
#ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      *layout = padLayoutN64;
      *count = PAD_LAYOUT_N64_COUNT;
      *name = "N64";
      break;
#endif
#ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      *layout = padLayoutGC;
      *count = PAD_LAYOUT_GC_COUNT;
      *name = "GameCube";
      break;
#endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      *layout = padLayoutDreamcast;
      *count = PAD_LAYOUT_DREAMCAST_COUNT;
      *name = "Dreamcast";
      break;
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      switch (getJvsPhysicalButtonsPerPlayer(getJvsBoardInfo())) {
        case 0:
        case 8:
        default:
          *layout = padLayoutJvs8;
          *count = PAD_LAYOUT_JVS8_COUNT;
          break;
        case 7:
          *layout = padLayoutJvs7;
          *count = PAD_LAYOUT_JVS7_COUNT;
          break;
        case 6:
          *layout = padLayoutJvs6;
          *count = PAD_LAYOUT_JVS6_COUNT;
          break;
      }
      *name = "JVS Panel";
      break;
    #endif
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      *layout = padLayoutPSX;
      *count = PAD_LAYOUT_PSX_COUNT;
      *name = "PSX";
      break;
#endif
    #ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE:
      *layout = padLayoutDancePad;
      *count = PAD_LAYOUT_DANCEPAD_COUNT;
      *name = "Dance Pad";
      break;
    #endif
#ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      *layout = padLayoutSaturn;
      *count = PAD_LAYOUT_SATURN_COUNT;
      *name = "Saturn";
      break;
#endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      *layout = padLayoutGenesis6;
      *count = PAD_LAYOUT_GENESIS6_COUNT;
      *name = "Genesis";
      break;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      *layout = padLayoutPCE;
      *count = PAD_LAYOUT_PCE_COUNT;
      *name = "PC Engine";
      break;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      *layout = padLayoutNeoGeo;
      *count = PAD_LAYOUT_NEOGEO_COUNT;
      *name = "Neo Geo";
      break;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      *layout = padLayout3DO;
      *count = PAD_LAYOUT_3DO_COUNT;
      *name = "3DO";
      break;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      *layout = padLayoutJaguar;
      *count = PAD_LAYOUT_JAGUAR_COUNT;
      *name = "Jaguar";
      break;
    #endif
    #ifdef ENABLE_INPUT_INTV
    case RZORD_INTV:
      *layout = padLayoutIntv;
      *count = PAD_LAYOUT_INTV_COUNT;
      *name = "Intellivision";
      break;
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      *layout = padLayoutSMS;
      *count = PAD_LAYOUT_SMS_COUNT;
      *name = "Atari/C64/SMS";
      break;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      *layout = padLayoutJPC;
      *count = PAD_LAYOUT_JPC_COUNT;
      *name = "JPN PC";
      break;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      *layout = padLayoutDriving;
      *count = PAD_LAYOUT_DRIVING_COUNT;
      *name = "Atari Driving";
      break;
    #endif
    default:
      *layout = padLayoutGeneric;
      *count = PAD_LAYOUT_GENERIC_COUNT;
      *name = "Pad Test";
      break;
  }
}
