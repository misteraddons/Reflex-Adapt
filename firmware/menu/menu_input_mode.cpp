#include "../core/button_remap.h"
#include "../core/device_runtime_state.h"
#include "../platform/display_runtime_state.h"
#include "menu_input.h"
#include "menu_input_mode.h"
#include "menu_mode_labels.h"
#include "menu_mode_state.h"
#include "../core/settings_store.h"

bool should_hide_input_mode(DeviceEnum mode) {
  switch (mode) {
    case RZORD_NONE:              // Invalid mode
      return true;
    #ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE:         // Hidden - auto-detected via L+R+U+D in PSX mode
      return true;
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG:           // Hidden - auto-detected within PSX mode
      return true;
    #endif
    default:
      return false;
  }
}

const char* getInputModeName(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_USB
      case RZORD_USB:       return "USB";
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
      case RZORD_ESP32_SPI: return "ESP32 BT";
    #endif
    #ifdef ENABLE_INPUT_JVS
      case RZORD_JVS:       return "JVS";
    #endif
    #ifdef ENABLE_INPUT_DUMMY
      case RZORD_DUMMY:     return "Dummy";
    #endif
    #ifdef ENABLE_INPUT_CUSTOM
      case RZORD_CUSTOM:    return "Custom";
    #endif
    #ifdef ENABLE_INPUT_SATURN
      case RZORD_SATURN:    return "Saturn";
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
      case RZORD_MEGADRIVE: return "Genesis";
    #endif
    #ifdef ENABLE_INPUT_N64
      case RZORD_N64:       return "N64";
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
      case RZORD_GAMECUBE:  return "GameCube";
    #endif
    #ifdef ENABLE_INPUT_GBA
      case RZORD_GBA:       return "GBA";
    #endif
    #ifdef ENABLE_INPUT_NES
      case RZORD_NES:       return "NES";
    #endif
    #ifdef ENABLE_INPUT_SNES
      case RZORD_SNES:      return "SNES";
    #endif
    #ifdef ENABLE_INPUT_VBOY
      case RZORD_VBOY:      return "Virtual Boy";
    #endif
    #ifdef ENABLE_INPUT_PCE
      case RZORD_PCE:       return "PCE/TG-16";
    #endif
    #ifdef ENABLE_INPUT_WII
      case RZORD_WII:       return "Wii";
    #endif
    #ifdef ENABLE_INPUT_PSX
      case RZORD_PSX:       return "PSX";
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
      case RZORD_PSX_JOG:   return "JogCon";
    #endif
    #ifdef ENABLE_INPUT_PSX_DANCE
      case RZORD_PSX_DANCE: return "PSX Dance";
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
      case RZORD_NEOGEO:    return "NeoGeo";
    #endif
    #ifdef ENABLE_INPUT_3DO
      case RZORD_3DO:       return "3DO";
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
      case RZORD_JAGUAR:    return "Jaguar";
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
      case RZORD_DREAMCAST: return "Dreamcast";
    #endif
    #ifdef ENABLE_INPUT_INTV
      case RZORD_INTV:      return "INTV";
    #endif
    #ifdef ENABLE_INPUT_PADDLE
      case RZORD_PADDLE:    return "Paddle";
    #endif
    #ifdef ENABLE_INPUT_DRIVING
      case RZORD_DRIVING:   return "Atari Driver";
    #endif
    #ifdef ENABLE_INPUT_GAMEPORT
      case RZORD_GAMEPORT:  return "Gameport";
    #endif
    #ifdef ENABLE_INPUT_MEMCARD
      case RZORD_MEMCARD:   return "MemCard";
    #endif
    #ifdef ENABLE_INPUT_SMS
      case RZORD_SMS:       return "Atari/C64/SMS";
    #endif
    #ifdef ENABLE_INPUT_JPC
      case RZORD_JPC:       return "MSX/FM/X68K";
    #endif
    #ifdef ENABLE_INPUT_AUTODETECT
      case RZORD_AUTODETECT: return "Auto";
    #endif
    default:                return "?";
  }
}

void printInputModeWithMenuButtons(DeviceEnum mode) {
  const char* modeName = getInputCompactName(mode);
  const char* confirmName = getMenuConfirmButtonName(mode);
  const char* backName = getMenuBackButtonName(mode);

  display.print(modeName);
  display.print(" ");
  display.print(confirmName);
  display.print(">");
  display.print(backName);
}

void handle_item_input_mode(menu_item_action action) {
  switch (action) {
    case item_action_display:
      printInputModeWithMenuButtons(menu_input);
      break;
    case item_action_change:
      do {
        menu_input = (DeviceEnum)(menu_input + 1);
        if (menu_input >= RZORD_LAST)
          menu_input = (DeviceEnum)1;
      } while (should_hide_input_mode(menu_input));
      break;
    case item_action_change_prev:
      do {
        if (menu_input <= 1)
          menu_input = (DeviceEnum)(RZORD_LAST - 1);
        else
          menu_input = (DeviceEnum)(menu_input - 1);
      } while (should_hide_input_mode(menu_input));
      break;
    case item_action_reset:
      menu_input = deviceMode;
      break;
    case item_action_apply:
      deviceMode = menu_input;
      break;
    case item_action_save:
      break;
  }
}
