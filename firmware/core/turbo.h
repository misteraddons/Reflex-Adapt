#pragma once

// Turbo Mode Module
// Provides per-button auto-fire functionality
// Button availability depends on input mode

#include <Arduino.h>
#include "../firmware_platform_config.h"

#define FRAME_MS  17

#ifndef TURBO_ENUMS_DEFINED
enum TurboRate : uint8_t {
  TURBO_OFF = 0,
  TURBO_SLOW,
  TURBO_MEDIUM,
  TURBO_FAST,
  TURBO_MAX,
  TURBO_ULTRA,
  TURBO_SLOW_LATCHED,
  TURBO_MEDIUM_LATCHED,
  TURBO_FAST_LATCHED,
  TURBO_MAX_LATCHED,
  TURBO_ULTRA_LATCHED,
  TURBO_RATE_LAST
};

enum TurboButton : uint8_t {
  TURBO_BTN_0 = 0,
  TURBO_BTN_1,
  TURBO_BTN_2,
  TURBO_BTN_3,
  TURBO_BTN_4,
  TURBO_BTN_5,
  TURBO_BTN_6,
  TURBO_BTN_7,
  TURBO_BTN_8,
  TURBO_BTN_9,
  TURBO_BTN_10,
  TURBO_BTN_11,
  TURBO_BTN_12,
  TURBO_BTN_13,
  TURBO_BTN_14,
  TURBO_BTN_15,
  TURBO_BTN_16,
  TURBO_BTN_COUNT
};
#define TURBO_ENUMS_DEFINED
#endif

inline const char* getTurboRateName(TurboRate rate) {
  switch (rate) {
    case TURBO_OFF:    return "Off";
    case TURBO_SLOW:   return "Slow";
    case TURBO_MEDIUM: return "Med";
    case TURBO_FAST:   return "Fast";
    case TURBO_MAX:    return "Max";
    case TURBO_ULTRA:  return "Ultra";
    case TURBO_SLOW_LATCHED:   return "Slow Latched";
    case TURBO_MEDIUM_LATCHED: return "Med Latched";
    case TURBO_FAST_LATCHED:   return "Fast Latched";
    case TURBO_MAX_LATCHED:    return "Max Latched";
    case TURBO_ULTRA_LATCHED:  return "Ultra Latched";
    default:           return "?";
  }
}

inline const char* getTurboRateFullName(TurboRate rate) {
  switch (rate) {
    case TURBO_OFF:    return "Off";
    case TURBO_SLOW:   return "12.5Hz";
    case TURBO_MEDIUM: return "15Hz";
    case TURBO_FAST:   return "20Hz";
    case TURBO_MAX:    return "30Hz";
    case TURBO_ULTRA:  return "60Hz";
    case TURBO_SLOW_LATCHED:   return "12.5Hz Latched";
    case TURBO_MEDIUM_LATCHED: return "15Hz Latched";
    case TURBO_FAST_LATCHED:   return "20Hz Latched";
    case TURBO_MAX_LATCHED:    return "30Hz Latched";
    case TURBO_ULTRA_LATCHED:  return "60Hz Latched";
    default:           return "?";
  }
}

inline constexpr TurboRate turbo_rate_menu_order[TURBO_RATE_LAST] = {
  TURBO_OFF,
  TURBO_SLOW,
  TURBO_SLOW_LATCHED,
  TURBO_MEDIUM,
  TURBO_MEDIUM_LATCHED,
  TURBO_FAST,
  TURBO_FAST_LATCHED,
  TURBO_MAX,
  TURBO_MAX_LATCHED,
  TURBO_ULTRA,
  TURBO_ULTRA_LATCHED
};

inline TurboRate getTurboRateForMenuIndex(uint8_t index) {
  return (index < TURBO_RATE_LAST) ? turbo_rate_menu_order[index] : TURBO_OFF;
}

inline uint8_t getTurboMenuIndexForRate(TurboRate rate) {
  for (uint8_t i = 0; i < TURBO_RATE_LAST; ++i) {
    if (turbo_rate_menu_order[i] == rate) {
      return i;
    }
  }
  return 0;
}

inline bool isTurboRateLatched(TurboRate rate) {
  return rate == TURBO_SLOW_LATCHED ||
         rate == TURBO_MEDIUM_LATCHED ||
         rate == TURBO_FAST_LATCHED ||
         rate == TURBO_MAX_LATCHED ||
         rate == TURBO_ULTRA_LATCHED;
}

inline TurboRate getTurboBaseRate(TurboRate rate) {
  switch (rate) {
    case TURBO_SLOW_LATCHED:   return TURBO_SLOW;
    case TURBO_MEDIUM_LATCHED: return TURBO_MEDIUM;
    case TURBO_FAST_LATCHED:   return TURBO_FAST;
    case TURBO_MAX_LATCHED:    return TURBO_MAX;
    case TURBO_ULTRA_LATCHED:  return TURBO_ULTRA;
    default:                   return rate;
  }
}

inline uint16_t getTurboIntervalMs(TurboRate rate) {
  switch (getTurboBaseRate(rate)) {
    case TURBO_SLOW:   return 5 * FRAME_MS;
    case TURBO_MEDIUM: return 4 * FRAME_MS;
    case TURBO_FAST:   return 3 * FRAME_MS;
    case TURBO_MAX:    return 2 * FRAME_MS;
    case TURBO_ULTRA:  return 1 * FRAME_MS;
    default:           return 0;
  }
}

struct TurboButtonConfig {
  uint8_t count;
  uint8_t indices[TURBO_BTN_COUNT];
  const char* names[TURBO_BTN_COUNT];
};

enum TurboInputMode : uint8_t {
  TURBO_MODE_GENERIC = 0,
  TURBO_MODE_NES,
  TURBO_MODE_SNES,
  TURBO_MODE_VBOY,
  TURBO_MODE_N64,
  TURBO_MODE_GAMECUBE,
  TURBO_MODE_PSX,
  TURBO_MODE_PSX_JOG,
  TURBO_MODE_PSX_NEGCON,
  TURBO_MODE_SATURN,
  TURBO_MODE_DREAMCAST,
  TURBO_MODE_PCE,
  TURBO_MODE_NEOGEO,
  TURBO_MODE_3DO,
  TURBO_MODE_WII,
  TURBO_MODE_JAGUAR,
  TURBO_MODE_MEGADRIVE,
  TURBO_MODE_SMS,
  TURBO_MODE_JPC,
  TURBO_MODE_DRIVING,
};

inline const TurboButtonConfig& getTurboButtonConfig(TurboInputMode mode) {
  static const TurboButtonConfig configs[] = {
    { 12, {0,1,2,3,4,5,6,7,10,11,9,8}, {"A","B","X","Y","L1","R1","L2","R2","L3","R3","Select","Start"} },
    { 4, {0,1,9,8,0,0,0,0,0,0}, {"A","B","Select","Start","","","","","",""} },
    { 8, {0,1,2,3,4,5,9,8,0,0}, {"A","B","X","Y","L","R","Select","Start","",""} },
    { 10, {2,6,3,7,0,1,4,5,8,9}, {"R-Up","R-Down","R-Left","R-Right","A","B","L","R","Start","Select"} },
    { 10, {0,1,6,4,5,2,7,3,9,8}, {"A","B","Z","L","R","C-Up","C-Down","C-Left","C-Right","Start"} },
    { 8, {0,1,2,3,6,7,9,8,0,0}, {"A","B","X","Y","L","R","Z","Start","",""} },
    { 12, {0,1,2,3,4,5,6,7,10,11,9,8}, {"X","O","Square","Triangle","L1","R1","L2","R2","L3","R3","Select","Start"} },
    { 10, {0,1,2,3,4,5,6,7,9,8}, {"X","O","Square","Triangle","L1","R1","L2","R2","Select","Start"} },
    { 6, {1,3,0,2,5,8,0,0,0,0}, {"A","B","I","II","R","Start","","","",""} },
    { 9, {0,1,5,2,3,4,6,7,8,0}, {"A","B","C","X","Y","Z","L","R","Start",""} },
    { 7, {0,1,2,3,6,7,8,0,0,0}, {"A","B","X","Y","L","R","Start","","",""} },
    { 8, {1,0,5,4,2,3,9,8,0,0}, {"I","II","III","IV","V","VI","Select","Run","",""} },
    { 6, {0,1,2,3,9,8,0,0,0,0}, {"A","B","C","D","Select","Start","","","",""} },
    { 7, {0,1,2,4,5,8,9,0,0,0}, {"A","B","C","L","R","Play","X","","",""} },
    { 10, {0,1,2,3,4,5,6,7,8,9}, {"A","B","X","Y","L","R","ZL","ZR","+","-"} },
    { 17, {0,1,2,10,12,11,6,13,7,4,3,5,14,15,16,9,8}, {"A","B","C","1","2","3","4","5","6","7","8","9","*","0","#","Option","Pause"} },
    { 8, {0,1,5,2,3,4,7,8,0,0}, {"A","B","C","X","Y","Z","Mode","Start","",""} },
    { 3, {0,1,8,0,0,0,0,0,0,0}, {"1","2","Start","","","","","","",""} },
    { 4, {0,1,9,8,0,0,0,0,0,0}, {"1","2","Select","Run","","","","","",""} },
    { 1, {0,0,0,0,0,0,0,0,0,0}, {"Fire","","","","","","","","",""} },
  };
  static_assert(sizeof(configs) / sizeof(configs[0]) == TURBO_MODE_DRIVING + 1,
                "TurboButtonConfig array must match TurboInputMode enum count");
  return configs[mode];
}

inline constexpr uint32_t turbo_button_masks[TURBO_BTN_COUNT] = {
  0x0001,
  0x0002,
  0x0004,
  0x0008,
  0x0010,
  0x0020,
  0x0040,
  0x0080,
  0x0400,
  0x0800,
  0x0100,
  0x0200,
  1UL << 15,
  1UL << 18,
  1UL << 16,
  1UL << 14,
  1UL << 17
};

class TurboController {
private:
  TurboRate button_rates[TURBO_BTN_COUNT];
  uint32_t last_toggle_time[TURBO_BTN_COUNT];
  bool turbo_state[TURBO_BTN_COUNT];
  bool any_enabled;
  TurboInputMode input_mode;
  uint32_t raw_held_buttons[MAX_USB_OUT];
  uint32_t latched_buttons[MAX_USB_OUT];
  uint32_t last_physical_buttons[MAX_USB_OUT];

  uint32_t getTurboButtonMask(TurboButton btn) const;
  void refreshEnabledState();
  void resetTracking();

public:
  TurboController();

  void setInputMode(TurboInputMode mode);
  TurboInputMode getInputMode();
  void resetRuntimeState();
  const TurboButtonConfig& getButtonConfig();
  void setButtonRate(TurboButton btn, TurboRate rate);
  TurboRate getButtonRate(TurboButton btn);
  bool hasAnyEnabled();
  void updateRawButtons(uint8_t device, uint32_t current_buttons);
  bool isHoldingTurboButton(uint8_t device);
  uint32_t getRawHeldButtons(uint8_t device);
  uint16_t getEnabledButtonsMask();
  void setAllRates(const uint8_t* rates);
  void getAllRates(uint8_t* rates);
  void update();
  uint32_t applyTurbo(uint8_t device, uint32_t current_buttons);
  bool getButtonTurboState(TurboButton btn);
};

extern TurboController turbo;
