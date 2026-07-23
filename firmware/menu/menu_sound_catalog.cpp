#include "menu_sound_catalog.h"

#include "../platform/buzzer.h"

const SoundEventItem sound_event_items[] = {
  { SND_BOOT,        "Boot" },
  { SND_CONNECT,     "Connect" },
  { SND_DISCONNECT,  "Disconnect" },
  { SND_MENU_ENTER,  "Menu Enter" },
  { SND_MENU_NAV,    "Menu Nav" },
  { SND_SAVE,        "Save" },
  { SND_TURBO,       "Turbo" },
  { SND_MODE_CHANGE, "Mode Change" },
  { SND_ERROR,       "Error" },
  { SND_RESET,       "Factory Reset" },
  { SND_HOTKEY,      "Hotkey" },
};

const uint8_t SOUND_EVENT_COUNT = sizeof(sound_event_items) / sizeof(SoundEventItem);
