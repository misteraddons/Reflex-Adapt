#include "konami_code.h"

#if !defined(ADAPT_DISABLE_KONAMI_CODE)

#include <Arduino.h>

#include "controller_frame_state.h"
#include "../input/shared/input_button_bits.h"
#include "../platform/buzzer.h"
#include "../platform/rgb_led.h"

namespace {

constexpr uint32_t kSequence[] = {
  INPUT_PAD_U, INPUT_PAD_U,
  INPUT_PAD_D, INPUT_PAD_D,
  INPUT_PAD_L, INPUT_PAD_R,
  INPUT_PAD_L, INPUT_PAD_R,
  INPUT_B, INPUT_A,
};
constexpr uint8_t kSequenceLength = sizeof(kSequence) / sizeof(kSequence[0]);
constexpr uint32_t kDirectionMask = INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R;
constexpr uint32_t kFaceMask = INPUT_A | INPUT_B;
constexpr uint32_t kRelevantMask = kDirectionMask | kFaceMask;

constexpr buzzer_note_t kJingleNotes[] = {
  { NOTE_C6, DUR_SIXTEENTH },
  { NOTE_D6, DUR_SIXTEENTH },
  { NOTE_E6, DUR_SIXTEENTH },
  { NOTE_F6, DUR_SIXTEENTH },
  { NOTE_G6, DUR_SIXTEENTH },
  { NOTE_F6, DUR_SIXTEENTH },
  { NOTE_E6, DUR_SIXTEENTH },
  { NOTE_D6, DUR_SIXTEENTH },
  { NOTE_C6, DUR_HALF },
};
constexpr buzzer_melody_t kJingle = { kJingleNotes, 9, 100 };

class KonamiDetector {
 public:
  bool update(uint32_t buttons) {
    const uint32_t relevant = buttons & kRelevantMask;
    const uint32_t newPress = relevant & ~lastInput_;
    lastInput_ = relevant;
    if (newPress == 0) {
      return false;
    }

    const uint32_t now = millis();
    if (sequenceIndex_ > 0 && now - lastPressTime_ > 2000) {
      reset();
    }

    bool match = false;
    const uint32_t expected = kSequence[sequenceIndex_];
    if ((expected & kDirectionMask) != 0) {
      match = (newPress & kDirectionMask) == expected;
    } else if (sequenceIndex_ == 8) {
      const uint32_t face = newPress & kFaceMask;
      if (face == INPUT_A || face == INPUT_B) {
        firstFaceButton_ = face;
        match = true;
      }
    } else {
      const uint32_t second = firstFaceButton_ == INPUT_A ? INPUT_B : INPUT_A;
      match = (newPress & kFaceMask) == second;
    }

    if (match) {
      lastPressTime_ = now;
      if (++sequenceIndex_ == kSequenceLength) {
        reset();
        return true;
      }
    } else {
      reset();
    }
    return false;
  }

 private:
  void reset() {
    sequenceIndex_ = 0;
    firstFaceButton_ = 0;
  }

  uint8_t sequenceIndex_ = 0;
  uint32_t lastInput_ = 0;
  uint32_t lastPressTime_ = 0;
  uint32_t firstFaceButton_ = 0;
};

KonamiDetector detector;

}  // namespace

void updateKonamiCodeObserver(bool polled) {
  if (!polled) {
    return;
  }

  uint32_t buttons = 0;
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (frame.connected) {
      buttons = frame.digital_buttons;
      break;
    }
  }

  if (detector.update(buttons)) {
    buzzer.play(kJingle);
    rgbLed.setMode(LED_MODE_RAINBOW);
  }
}

#endif
