#include "buzzer.h"

#include "hardware/clocks.h"

Buzzer::Buzzer()
    : pin(BUZZER_PIN),
      slice_num(0),
      enabled(true),
      volume(10),
      eventMask(SND_ALL),
      current_melody(nullptr),
      pending_melody(nullptr),
      current_note_index(0),
      note_end_time(0),
      pending_start_time(0),
      playing(false) {}

void Buzzer::setFrequency(uint16_t freq) {
  if (freq == 0) {
    stopTone();
    return;
  }

  gpio_set_function(pin, GPIO_FUNC_PWM);

  // Calculate PWM values from the active sysclk so tones stay stable across
  // 120 MHz, 240 MHz, and other product clock profiles.
  const uint32_t clock = clock_get_hz(clk_sys);
  uint32_t divider = clock / (freq * 4096);
  if (divider < 1) divider = 1;
  if (divider > 255) divider = 255;

  uint32_t wrap = clock / (divider * freq) - 1;
  if (wrap > 65535) wrap = 65535;

  // Calculate duty cycle based on volume (0-100% -> 0-50% duty)
  // Lower duty cycle = quieter sound (less energy transferred to piezo)
  uint32_t duty = (wrap * volume) / 200;
  if (duty < 1 && volume > 0) duty = 1;

  pwm_set_clkdiv(slice_num, divider);
  pwm_set_wrap(slice_num, wrap);
  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(pin), duty);
  pwm_set_enabled(slice_num, true);
}

void Buzzer::stopTone() {
  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(pin), 0);
  pwm_set_enabled(slice_num, false);
  gpio_set_function(pin, GPIO_FUNC_SIO);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, LOW);
}

void Buzzer::begin() {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  slice_num = pwm_gpio_to_slice_num(pin);
  pwm_set_enabled(slice_num, false);
}

void Buzzer::setEnabled(bool en) {
  enabled = en;
  if (!en) {
    stop();
  }
}

bool Buzzer::isEnabled() {
  return enabled;
}

bool Buzzer::isPlaying() {
  return playing;
}

void Buzzer::setVolume(uint8_t vol) {
  if (vol > 100) vol = 100;
  volume = vol;
}

uint8_t Buzzer::getVolume() {
  return volume;
}

void Buzzer::setEventMask(uint16_t mask) {
  eventMask = mask;
}

uint16_t Buzzer::getEventMask() {
  return eventMask;
}

void Buzzer::enableEvent(uint16_t event) {
  eventMask |= event;
}

void Buzzer::disableEvent(uint16_t event) {
  eventMask &= ~event;
}

bool Buzzer::isEventEnabled(uint16_t event) {
  return (eventMask & event) != 0;
}

void Buzzer::tone(uint16_t frequency, uint16_t duration_ms) {
  if (!enabled) return;
  setFrequency(frequency);
  delay(duration_ms);
  stopTone();
}

void Buzzer::startMelody(const buzzer_melody_t& melody) {
  current_melody = &melody;
  current_note_index = 0;
  playing = true;

  if (melody.length > 0) {
    const buzzer_note_t& note = melody.notes[0];
    setFrequency(note.frequency);
    note_end_time = millis() + (note.duration * melody.tempo);
  }
}

void Buzzer::play(const buzzer_melody_t& melody) {
  if (!enabled) return;
  pending_melody = nullptr;
  startMelody(melody);
}

void Buzzer::playDelayed(const buzzer_melody_t& melody, uint16_t delay_ms) {
  if (!enabled) return;
  pending_melody = &melody;
  pending_start_time = millis() + delay_ms;
}

void Buzzer::update() {
  if (!playing && pending_melody && (int32_t)(millis() - pending_start_time) >= 0) {
    const buzzer_melody_t* melody = pending_melody;
    pending_melody = nullptr;
    startMelody(*melody);
  }

  if (!playing || !current_melody) return;

  if (millis() >= note_end_time) {
    current_note_index++;

    if (current_note_index >= current_melody->length) {
      stop();
      return;
    }

    const buzzer_note_t& note = current_melody->notes[current_note_index];
    setFrequency(note.frequency);
    note_end_time = millis() + (note.duration * current_melody->tempo);
  }
}

void Buzzer::stop() {
  stopTone();
  playing = false;
  current_melody = nullptr;
  pending_melody = nullptr;
}

void Buzzer::playBoot() {
  if (eventMask & SND_BOOT) play(MELODY_BOOT);
}

void Buzzer::playBootDelayed(uint16_t delay_ms) {
  if (eventMask & SND_BOOT) playDelayed(MELODY_BOOT, delay_ms);
}

void Buzzer::playConnect() {
  if (eventMask & SND_CONNECT) play(MELODY_CONNECT);
}

void Buzzer::playDisconnect() {
  if (eventMask & SND_DISCONNECT) play(MELODY_DISCONNECT);
}

void Buzzer::playMenuEnter() {
  if (eventMask & SND_MENU_ENTER) play(MELODY_MENU_ENTER);
}

void Buzzer::playMenuNav() {
  if (eventMask & SND_MENU_NAV) play(MELODY_MENU_NAV);
}

void Buzzer::playSave() {
  if (eventMask & SND_SAVE) play(MELODY_SAVE);
}

void Buzzer::playTurboOn() {
  if (eventMask & SND_TURBO) play(MELODY_TURBO_ON);
}

void Buzzer::playTurboOff() {
  if (eventMask & SND_TURBO) play(MELODY_TURBO_OFF);
}

void Buzzer::playModeChange() {
  if (eventMask & SND_MODE_CHANGE) play(MELODY_MODE_CHANGE);
}

void Buzzer::playError() {
  if (eventMask & SND_ERROR) play(MELODY_ERROR);
}

void Buzzer::playFactoryReset() {
  if (eventMask & SND_RESET) play(MELODY_FACTORY_RESET);
}

void Buzzer::playHotkey() {
  if (eventMask & SND_HOTKEY) play(MELODY_HOTKEY);
}

void Buzzer::playCoin() {
  if (eventMask & SND_COIN) play(MELODY_COIN);
}

Buzzer buzzer;
