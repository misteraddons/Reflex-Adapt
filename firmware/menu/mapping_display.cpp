#include "mapping_display.h"

#include <Arduino.h>

#include "../core/button_remap.h"
#include "controller_graphics.h"
#include "../output/output_runtime_state.h"

namespace {

constexpr int16_t MAPPING_LEFT_X = 0;
constexpr int16_t MAPPING_RIGHT_X = 64;
constexpr int16_t MAPPING_CTRL_Y = 10;

const char* getMappingInputName(DeviceEnum mode) {
  switch (mode) {
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX: return "PSX";
#endif
#ifdef ENABLE_INPUT_SNES
    case RZORD_SNES: return "SNES";
#endif
#ifdef ENABLE_INPUT_NES
    case RZORD_NES: return "NES";
#endif
#ifdef ENABLE_INPUT_N64
    case RZORD_N64: return "N64";
#endif
#ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE: return "GC";
#endif
#ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN: return "SAT";
#endif
#ifdef ENABLE_INPUT_WII
    case RZORD_WII: return "WII";
#endif
#ifdef ENABLE_INPUT_PCE
    case RZORD_PCE: return "PCE";
#endif
#ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO: return "NGO";
#endif
#ifdef ENABLE_INPUT_3DO
    case RZORD_3DO: return "3DO";
#endif
#ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR: return "JAG";
#endif
#ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST: return "DC";
#endif
#ifdef ENABLE_INPUT_SMS
    case RZORD_SMS: return "SMS";
#endif
#ifdef ENABLE_INPUT_INTV
    case RZORD_INTV: return "INTV";
#endif
    default: return "IN";
  }
}

const char* getMappingOutputName(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID: return "HID";
    case OUTPUT_MISTER: return "MiST";
    case OUTPUT_MISTER_JOGCON: return "MJOG";
    case OUTPUT_RESERVED_JOGCON: return "MJOG";
    case OUTPUT_MISTER_NEGCON: return "MNEG";
    case OUTPUT_MISTER_GUNCON: return "MGUN";
    case OUTPUT_RESERVED_MOUSE: return "MiST";
    case OUTPUT_XINPUT: return "XINP";
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P: return "XIN";
    case OUTPUT_XID: return "XID";
    case OUTPUT_SWITCH: return "POKE";
    case OUTPUT_SWITCHPRO: return "SWPR";
    case OUTPUT_PS3: return "PS3";
    case OUTPUT_PS4: return "PS4";
    case OUTPUT_PS5: return "PS5";
    case OUTPUT_PANTHERLORD: return "PLRD";
    case OUTPUT_GCWIIU: return "GC";
    case OUTPUT_MDMINI: return "MDMI";
    case OUTPUT_KEYBOARD: return "KBD";
    default: return "OUT";
  }
}

class MappingDisplay {
private:
  bool active;
  bool needsFullRedraw;
  uint32_t prevInputState;
  uint32_t prevOutputState;
  int8_t prevLX;
  int8_t prevLY;
  int8_t prevRX;
  int8_t prevRY;

  void drawInputController(uint32_t state) {
    ControllerGfxType type = getInputGfxType(deviceMode);
    int16_t x = MAPPING_LEFT_X + 6;
    int16_t y = MAPPING_CTRL_Y;

    const controller_state_t& frame = controllerFrameConst(0);
    int8_t lx = frame.LX;
    int8_t ly = frame.LY;
    int8_t rx = frame.RX;
    int8_t ry = frame.RY;
    bool l3 = frame.L3;
    bool r3 = frame.R3;

    drawController(type, x, y, state, lx, ly, rx, ry, l3, r3);
  }

  void drawOutputController(uint32_t state) {
    ControllerGfxType type = getOutputGfxType(outputMode);
    int16_t x = MAPPING_RIGHT_X + 6;
    int16_t y = MAPPING_CTRL_Y;

    const controller_state_t& frame = controllerFrameConst(0);
    int8_t lx = frame.LX;
    int8_t ly = frame.LY;
    int8_t rx = frame.RX;
    int8_t ry = frame.RY;
    bool l3 = state & GFX_BTN_L3;
    bool r3 = state & GFX_BTN_R3;

    drawController(type, x, y, state, lx, ly, rx, ry, l3, r3);
  }

  uint32_t applyRemapToMask(uint32_t inputState) {
    uint32_t outputState = 0;

    outputState |= (inputState & (GFX_BTN_UP | GFX_BTN_DOWN | GFX_BTN_LEFT | GFX_BTN_RIGHT));

    uint32_t buttonMasks[] = {
      GFX_BTN_A, GFX_BTN_B, GFX_BTN_X, GFX_BTN_Y,
      GFX_BTN_L1, GFX_BTN_R1, GFX_BTN_L2, GFX_BTN_R2,
      GFX_BTN_L3, GFX_BTN_R3, GFX_BTN_START, GFX_BTN_SELECT, GFX_BTN_HOME
    };

    for (uint8_t i = 0; i < 13; i++) {
      if (inputState & buttonMasks[i]) {
        uint8_t target = active_remaps[i];
        if (target < 13) {
          outputState |= buttonMasks[target];
        }
      }
    }

    return outputState;
  }

public:
  MappingDisplay()
    : active(false),
      needsFullRedraw(true),
      prevInputState(0),
      prevOutputState(0),
      prevLX(0),
      prevLY(0),
      prevRX(0),
      prevRY(0) {}

  bool isActive() {
    return active;
  }

  void activate() {
    active = true;
    needsFullRedraw = true;
    prevInputState = 0;
    prevOutputState = 0;
  }

  void deactivate() {
    active = false;
  }

  bool render() {
    if (!active) {
      return false;
    }

    uint32_t inputState = buildGfxButtonMask(0);
    uint32_t outputState = applyRemapToMask(inputState);

    const controller_state_t& frame = controllerFrameConst(0);
    int8_t lx = frame.LX;
    int8_t ly = frame.LY;
    int8_t rx = frame.RX;
    int8_t ry = frame.RY;

    bool stateChanged = (inputState != prevInputState) ||
                        (outputState != prevOutputState) ||
                        (lx != prevLX) || (ly != prevLY) ||
                        (rx != prevRX) || (ry != prevRY);

    if (!needsFullRedraw && !stateChanged) {
      return false;
    }

    u8g2.clearBuffer();
    u8g2.drawVLine(63, 0, 64);
    u8g2.setFont(u8g2_font_5x7_tr);

    u8g2.drawStr(MAPPING_LEFT_X + 2, 7, "IN:");
    u8g2.drawStr(MAPPING_LEFT_X + 17, 7, getMappingInputName(deviceMode));

    u8g2.drawStr(MAPPING_RIGHT_X + 2, 7, "OUT:");
    u8g2.drawStr(MAPPING_RIGHT_X + 22, 7, getMappingOutputName(outputMode));

    drawInputController(inputState);
    drawOutputController(outputState);

    bool hasRemaps = false;
    for (uint8_t i = 0; i < 13; i++) {
      if (active_remaps[i] != i) {
        hasRemaps = true;
        break;
      }
    }

    if (hasRemaps) {
      u8g2.drawStr(60, 63, "R");
    }

    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(0, 63, "Mode:exit");

    u8g2.sendBuffer();

    prevInputState = inputState;
    prevOutputState = outputState;
    prevLX = lx;
    prevLY = ly;
    prevRX = rx;
    prevRY = ry;
    needsFullRedraw = false;

    return true;
  }
};

MappingDisplay mappingDisplay;

} // namespace

bool isMappingDisplayActive() {
  return mappingDisplay.isActive();
}

void activateMappingDisplay() {
  mappingDisplay.activate();
}

void deactivateMappingDisplay() {
  mappingDisplay.deactivate();
}

bool renderMappingDisplay() {
  return mappingDisplay.render();
}
