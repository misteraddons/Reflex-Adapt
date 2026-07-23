#include "../product_config.h"

#include "menu_pin_debug.h"

#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"
#include "menu_ui_state.h"

#if defined(PIN_HDMI1_01) && defined(PIN_HDMI2_13)

namespace {

const uint8_t HDMI_PORT1_PINS[] = {
  HDMI_1_01, HDMI_1_02, HDMI_1_03, HDMI_1_04, HDMI_1_05, HDMI_1_06, HDMI_1_07,
  HDMI_1_08, HDMI_1_09, HDMI_1_10, HDMI_1_11, HDMI_1_12, HDMI_1_13
};

const uint8_t HDMI_PORT2_PINS[] = {
  HDMI_2_01, HDMI_2_02, HDMI_2_03, HDMI_2_04, HDMI_2_05, HDMI_2_06, HDMI_2_07,
  HDMI_2_08, HDMI_2_09, HDMI_2_10, HDMI_2_11, HDMI_2_12, HDMI_2_13
};

const uint8_t HDMI_PIN_COUNT = 13;

}  // namespace

void renderPinDebug(bool btnChangeJustPressed, bool btnNavigateJustPressed) {
  static uint16_t prevPinStates = 0xFFFF;
  static uint8_t displayPage = 0;

  if (!pinDebugInitialized) {
    pinDebugInitialized = true;
    prevPinStates = 0xFFFF;
    displayPage = 0;

    for (uint8_t i = 0; i < HDMI_PIN_COUNT; i++) {
      pinMode(HDMI_PORT1_PINS[i], INPUT_PULLUP);
      pinMode(HDMI_PORT2_PINS[i], INPUT_PULLUP);
    }

    display.clear();
    display.setFont(System5x7);
  }

  if (btnNavigateJustPressed) {
    displayPage = (displayPage + 1) % 3;
    prevPinStates = 0xFFFF;
    buzzer.playMenuNav();
    display.clear();
  }

  if (btnChangeJustPressed) {
    pinDebugActive = false;
    pinDebugInitialized = false;
    display.setFont(System5x7);
    buzzer.playMenuNav();
    return;
  }

  if (displayPage == 2) {
    display.setCursor(0, 0);
    display.print("HDMI Pin -> GPIO");
    display.setCursor(0, 1);
    display.print("P1: ");
    for (uint8_t i = 0; i < 7; i++) {
      if (HDMI_PORT1_PINS[i] < 10) display.print(' ');
      display.print(HDMI_PORT1_PINS[i]);
      display.print(' ');
    }
    display.setCursor(0, 2);
    display.print("    ");
    for (uint8_t i = 7; i < HDMI_PIN_COUNT; i++) {
      if (HDMI_PORT1_PINS[i] < 10) display.print(' ');
      display.print(HDMI_PORT1_PINS[i]);
      display.print(' ');
    }
    display.setCursor(0, 3);
    display.print("P2: ");
    for (uint8_t i = 0; i < 7; i++) {
      if (HDMI_PORT2_PINS[i] < 10) display.print(' ');
      display.print(HDMI_PORT2_PINS[i]);
      display.print(' ');
    }
    display.setCursor(0, 4);
    display.print("    ");
    for (uint8_t i = 7; i < HDMI_PIN_COUNT; i++) {
      if (HDMI_PORT2_PINS[i] < 10) display.print(' ');
      display.print(HDMI_PORT2_PINS[i]);
      display.print(' ');
    }
    display.setCursor(0, 5);
    display.print("Pins 1-7, 8-13");
    display.setCursor(0, 7);
    display.print("Nav:Page Mode:Exit");
    return;
  }

  uint8_t port = displayPage;
  const uint8_t* pins = (port == 0) ? HDMI_PORT1_PINS : HDMI_PORT2_PINS;

  uint16_t currentPinStates = 0;
  for (uint8_t i = 0; i < HDMI_PIN_COUNT; i++) {
    if (digitalRead(pins[i])) {
      currentPinStates |= (1 << i);
    }
  }

  if (currentPinStates != prevPinStates) {
    uint8_t lowCount = 0;
    for (uint8_t i = 0; i < HDMI_PIN_COUNT; i++) {
      if (!(currentPinStates & (1 << i))) lowCount++;
    }
    display.setCursor(0, 0);
    display.print("Port ");
    display.print(port + 1);
    display.print(" (");
    display.print(lowCount);
    display.print(" LOW)    ");

    display.setCursor(0, 1);
    display.print("01 02 03 04 05 06 07");

    display.setCursor(0, 2);
    for (uint8_t i = 0; i < 7; i++) {
      display.print((currentPinStates & (1 << i)) ? '1' : '0');
      display.print("  ");
    }

    display.setCursor(0, 3);
    display.print("08 09 10 11 12 13");

    display.setCursor(0, 4);
    for (uint8_t i = 7; i < HDMI_PIN_COUNT; i++) {
      display.print((currentPinStates & (1 << i)) ? '1' : '0');
      display.print("  ");
    }

    display.setCursor(0, 5);
    display.print("LOW:");
    bool first = true;
    for (uint8_t i = 0; i < HDMI_PIN_COUNT; i++) {
      if (!(currentPinStates & (1 << i))) {
        if (!first) display.print(',');
        first = false;
        if (i + 1 < 10) display.print('0');
        display.print(i + 1);
      }
    }
    if (first) display.print(" none");
    display.print("      ");

    display.setCursor(0, 7);
    display.print("Nav:Page Mode:Exit");

    prevPinStates = currentPinStates;
  }
}

#else

void renderPinDebug(bool btnChangeJustPressed, bool btnNavigateJustPressed) {
  (void)btnChangeJustPressed;
  (void)btnNavigateJustPressed;
  pinDebugActive = false;
  pinDebugInitialized = false;
}

#endif
