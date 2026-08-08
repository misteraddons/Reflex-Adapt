#pragma once

// Buzzer/Audio Feedback Module
// Uses PWM to generate tones on a piezo buzzer

#include "../product_config.h"
#include <Arduino.h>
#include "hardware/pwm.h"

// Configure buzzer pin from the active product before falling back to the
// legacy Reflex Adapt 2 shared GPIO8 footprint.
#ifndef BUZZER_PIN
  #ifdef PIN_BUZZER
    #define BUZZER_PIN PIN_BUZZER
  #elif defined(PIN_BUZZER_LED)
    #define BUZZER_PIN PIN_BUZZER_LED
  #else
    #define BUZZER_PIN 8
  #endif
#endif

// Note frequencies (Hz) - standard musical notes
#define NOTE_REST 0
#define NOTE_C4   262
#define NOTE_CS4  277
#define NOTE_D4   294
#define NOTE_DS4  311
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_FS4  370
#define NOTE_G4   392
#define NOTE_GS4  415
#define NOTE_A4   440
#define NOTE_AS4  466
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_CS5  554
#define NOTE_D5   587
#define NOTE_DS5  622
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_FS5  740
#define NOTE_G5   784
#define NOTE_GS5  831
#define NOTE_A5   880
#define NOTE_AS5  932
#define NOTE_B5   988
#define NOTE_C6   1047
#define NOTE_D6   1175
#define NOTE_E6   1319
#define NOTE_F6   1397
#define NOTE_G6   1568
#define NOTE_A6   1760

// Note duration multipliers (based on tempo)
#define DUR_WHOLE    16
#define DUR_HALF     8
#define DUR_QUARTER  4
#define DUR_EIGHTH   2
#define DUR_SIXTEENTH 1

// A note in a melody
typedef struct {
  uint16_t frequency;  // Hz (0 = rest)
  uint8_t duration;    // Duration units
} buzzer_note_t;

// Melody definition
typedef struct {
  const buzzer_note_t* notes;
  uint8_t length;
  uint16_t tempo;  // ms per duration unit
} buzzer_melody_t;

// ============= JINGLES =============

// Boot complete - cheerful ascending arpeggio
const buzzer_note_t jingle_boot[] = {
  { NOTE_C5, DUR_EIGHTH },
  { NOTE_E5, DUR_EIGHTH },
  { NOTE_G5, DUR_EIGHTH },
  { NOTE_C6, DUR_QUARTER },
};
const buzzer_melody_t MELODY_BOOT = { jingle_boot, 4, 50 };

// Controller connected - short ascending chime
const buzzer_note_t jingle_connect[] = {
  { NOTE_G5, DUR_EIGHTH },
  { NOTE_C6, DUR_EIGHTH },
};
const buzzer_melody_t MELODY_CONNECT = { jingle_connect, 2, 55 };

// Controller disconnected - falling two-tone
const buzzer_note_t jingle_disconnect[] = {
  { NOTE_C6, DUR_EIGHTH },
  { NOTE_G5, DUR_QUARTER },
};
const buzzer_melody_t MELODY_DISCONNECT = { jingle_disconnect, 2, 60 };

// Menu enter - short click
const buzzer_note_t jingle_menu_enter[] = {
  { NOTE_E6, DUR_SIXTEENTH },
};
const buzzer_melody_t MELODY_MENU_ENTER = { jingle_menu_enter, 1, 30 };

// Menu navigate - tiny tick
const buzzer_note_t jingle_menu_nav[] = {
  { NOTE_A5, DUR_SIXTEENTH },
};
const buzzer_melody_t MELODY_MENU_NAV = { jingle_menu_nav, 1, 20 };

// Settings saved - cheerful ascending confirmation
const buzzer_note_t jingle_save[] = {
  { NOTE_C6, DUR_SIXTEENTH },
  { NOTE_E6, DUR_SIXTEENTH },
  { NOTE_G6, DUR_EIGHTH },
};
const buzzer_melody_t MELODY_SAVE = { jingle_save, 3, 35 };

// Turbo enabled - high chirp
const buzzer_note_t jingle_turbo_on[] = {
  { NOTE_E6, DUR_EIGHTH },
  { NOTE_G6, DUR_EIGHTH },
};
const buzzer_melody_t MELODY_TURBO_ON = { jingle_turbo_on, 2, 40 };

// Turbo disabled - low chirp
const buzzer_note_t jingle_turbo_off[] = {
  { NOTE_G5, DUR_EIGHTH },
  { NOTE_E5, DUR_EIGHTH },
};
const buzzer_melody_t MELODY_TURBO_OFF = { jingle_turbo_off, 2, 40 };

// Mode changed - notification blip
const buzzer_note_t jingle_mode_change[] = {
  { NOTE_C6, DUR_SIXTEENTH },
  { NOTE_REST, DUR_SIXTEENTH },
  { NOTE_E6, DUR_SIXTEENTH },
};
const buzzer_melody_t MELODY_MODE_CHANGE = { jingle_mode_change, 3, 30 };

// Error/invalid - buzz
const buzzer_note_t jingle_error[] = {
  { NOTE_CS4, DUR_EIGHTH },
  { NOTE_REST, DUR_SIXTEENTH },
  { NOTE_CS4, DUR_EIGHTH },
  { NOTE_REST, DUR_SIXTEENTH },
  { NOTE_CS4, DUR_EIGHTH },
};
const buzzer_melody_t MELODY_ERROR = { jingle_error, 5, 50 };

// Factory reset - descending dramatic
const buzzer_note_t jingle_factory_reset[] = {
  { NOTE_C6, DUR_EIGHTH },
  { NOTE_G5, DUR_EIGHTH },
  { NOTE_E5, DUR_EIGHTH },
  { NOTE_C5, DUR_EIGHTH },
  { NOTE_G4, DUR_QUARTER },
  { NOTE_REST, DUR_EIGHTH },
  { NOTE_C4, DUR_HALF },
};
const buzzer_melody_t MELODY_FACTORY_RESET = { jingle_factory_reset, 7, 80 };

// Hotkey activated - quick confirmation
const buzzer_note_t jingle_hotkey[] = {
  { NOTE_A5, DUR_SIXTEENTH },
  { NOTE_E6, DUR_EIGHTH },
};
const buzzer_melody_t MELODY_HOTKEY = { jingle_hotkey, 2, 40 };

// Coin insert (arcade style) - classic quarter drop
const buzzer_note_t jingle_coin[] = {
  { NOTE_E6, DUR_SIXTEENTH },
  { NOTE_REST, DUR_SIXTEENTH },
  { NOTE_G6, DUR_EIGHTH },
};
const buzzer_melody_t MELODY_COIN = { jingle_coin, 3, 40 };

// ============= SOUND EVENT FLAGS =============
// Bitmask for enabling/disabling individual sound events
#define SND_BOOT        (1 << 0)
#define SND_CONNECT     (1 << 1)
#define SND_DISCONNECT  (1 << 2)
#define SND_MENU_ENTER  (1 << 3)
#define SND_MENU_NAV    (1 << 4)
#define SND_SAVE        (1 << 5)
#define SND_TURBO       (1 << 6)   // Covers both on and off
#define SND_MODE_CHANGE (1 << 7)
#define SND_ERROR       (1 << 8)
#define SND_RESET       (1 << 9)   // Factory reset
#define SND_HOTKEY      (1 << 10)
#define SND_COIN        (1 << 11)

// Default: all sounds enabled
#define SND_ALL         0x0FFF

// ============= BUZZER CLASS =============

class Buzzer {
private:
  uint8_t pin;
  uint slice_num;
  bool enabled;
  uint8_t volume;  // 0-100%, affects duty cycle
  uint16_t eventMask;  // Bitmask of enabled sound events

  // Non-blocking playback state
  const buzzer_melody_t* current_melody;
  const buzzer_melody_t* pending_melody;
  uint8_t current_note_index;
  uint32_t note_end_time;
  uint32_t pending_start_time;
  bool playing;

  void setFrequency(uint16_t freq);
  void stopTone();
  void startMelody(const buzzer_melody_t& melody);
  void playDelayed(const buzzer_melody_t& melody, uint16_t delay_ms);

public:
  Buzzer();

  void begin();
  void setEnabled(bool en);
  bool isEnabled();
  bool isPlaying();

  // Volume control (0-100, default 25 for quieter sound)
  void setVolume(uint8_t vol);
  uint8_t getVolume();

  // Event mask control - enable/disable individual sound events
  void setEventMask(uint16_t mask);
  uint16_t getEventMask();
  void enableEvent(uint16_t event);
  void disableEvent(uint16_t event);
  bool isEventEnabled(uint16_t event);

  // Play a single tone (blocking)
  void tone(uint16_t frequency, uint16_t duration_ms);

  // Start playing a melody (non-blocking)
  // NOTE: melody must have static lifetime (e.g., const global) - pointer is stored internally
  void play(const buzzer_melody_t& melody);

  // Call this in loop() to update non-blocking playback
  void update();
  void stop();

  // Convenience methods for common events (check event mask before playing)
  void playBoot();
  void playBootDelayed(uint16_t delay_ms);
  void playConnect();
  void playDisconnect();
  void playMenuEnter();
  void playMenuNav();
  void playSave();
  void playTurboOn();
  void playTurboOff();
  void playModeChange();
  void playError();
  void playFactoryReset();
  void playHotkey();
  void playCoin();
};

// Global buzzer instance
extern Buzzer buzzer;
