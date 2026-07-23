#include "controller_graphics.h"

uint32_t buildGfxButtonMask(uint8_t player) {
  const controller_state_t& r = controllerFrameConst(player);
  uint32_t mask = 0;

  if (r.A)      mask |= GFX_BTN_A;
  if (r.B)      mask |= GFX_BTN_B;
  if (r.X)      mask |= GFX_BTN_X;
  if (r.Y)      mask |= GFX_BTN_Y;
  if (r.L1)     mask |= GFX_BTN_L1;
  if (r.R1)     mask |= GFX_BTN_R1;
  if (r.L2)     mask |= GFX_BTN_L2;
  if (r.R2)     mask |= GFX_BTN_R2;
  if (r.SELECT) mask |= GFX_BTN_SELECT;
  if (r.START)  mask |= GFX_BTN_START;
  if (r.L3)     mask |= GFX_BTN_L3;
  if (r.R3)     mask |= GFX_BTN_R3;
  if (r.HOME)   mask |= GFX_BTN_HOME;
  if (r.PAD_U)  mask |= GFX_BTN_UP;
  if (r.PAD_D)  mask |= GFX_BTN_DOWN;
  if (r.PAD_L)  mask |= GFX_BTN_LEFT;
  if (r.PAD_R)  mask |= GFX_BTN_RIGHT;

  return mask;
}

ControllerGfxType getInputGfxType(DeviceEnum mode) {
  switch (mode) {
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX: return GFX_CTRL_PSX;
#endif
#ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG: return GFX_CTRL_PSX;
#endif
#ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE: return GFX_CTRL_PSX;
#endif
#ifdef ENABLE_INPUT_SNES
    case RZORD_SNES: return GFX_CTRL_SNES;
#endif
#ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY: return GFX_CTRL_SNES;
#endif
#ifdef ENABLE_INPUT_NES
    case RZORD_NES: return GFX_CTRL_NES;
#endif
#ifdef ENABLE_INPUT_N64
    case RZORD_N64: return GFX_CTRL_N64;
#endif
#ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE: return GFX_CTRL_GC;
#endif
#ifdef ENABLE_INPUT_GBA
    case RZORD_GBA: return GFX_CTRL_GC;
#endif
#ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN: return GFX_CTRL_SATURN;
#endif
#ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE: return GFX_CTRL_SATURN;
#endif
#ifdef ENABLE_INPUT_PCE
    case RZORD_PCE: return GFX_CTRL_PCE;
#endif
#ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO: return GFX_CTRL_NEOGEO;
#endif
#ifdef ENABLE_INPUT_3DO
    case RZORD_3DO: return GFX_CTRL_3DO;
#endif
#ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR: return GFX_CTRL_JAGUAR;
#endif
#ifdef ENABLE_INPUT_WII
    case RZORD_WII: return GFX_CTRL_WII;
#endif
#ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST: return GFX_CTRL_DC;
#endif
    default: return GFX_CTRL_GENERIC;
  }
}

ControllerGfxType getOutputGfxType(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XID:
      return GFX_CTRL_XBOX;
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
      return GFX_CTRL_SWITCH;
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
    case OUTPUT_PANTHERLORD:
      return GFX_CTRL_PSX;
    case OUTPUT_GCWIIU:
      return GFX_CTRL_GC;
    case OUTPUT_MDMINI:
      return GFX_CTRL_SATURN;
    default:
      return GFX_CTRL_GENERIC;
  }
}

void drawController(ControllerGfxType type, int16_t x, int16_t y, uint32_t state,
                    int8_t lx, int8_t ly, int8_t rx, int8_t ry, bool l3, bool r3) {
  switch (type) {
    case GFX_CTRL_PSX:
      drawControllerPSX(x, y, state, lx, ly, rx, ry, l3, r3);
      break;
    case GFX_CTRL_SNES:
      drawControllerSNES(x, y, state);
      break;
    case GFX_CTRL_NES:
      drawControllerNES(x, y, state);
      break;
    case GFX_CTRL_N64:
      drawControllerN64(x, y, state, lx, ly);
      break;
    case GFX_CTRL_GC:
      drawControllerGC(x, y, state, lx, ly, rx, ry);
      break;
    case GFX_CTRL_SATURN:
      drawControllerSaturn(x, y, state);
      break;
    case GFX_CTRL_PCE:
      drawControllerPCE(x, y, state);
      break;
    case GFX_CTRL_NEOGEO:
      drawControllerNeoGeo(x, y, state);
      break;
    case GFX_CTRL_3DO:
      drawController3DO(x, y, state);
      break;
    case GFX_CTRL_JAGUAR:
      drawControllerJaguar(x, y, state);
      break;
    case GFX_CTRL_WII:
      drawControllerWiiClassic(x, y, state, lx, ly, rx, ry);
      break;
    case GFX_CTRL_DC:
      drawControllerDreamcast(x, y, state, lx, ly);
      break;
    case GFX_CTRL_XBOX:
      drawControllerXbox(x, y, state, lx, ly, rx, ry);
      break;
    case GFX_CTRL_SWITCH:
      drawControllerSwitchPro(x, y, state, lx, ly, rx, ry);
      break;
    default:
      drawControllerGenericHID(x, y, state, lx, ly, rx, ry);
      break;
  }
}
