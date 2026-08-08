#include "Input_Psx.h"

namespace {

int16_t clampJogconAxis(int16_t value) {
  if (value > 127) return 127;
  if (value < -128) return -128;
  return value;
}

int16_t centeredJogconPaddleAxis(uint8_t paddle) {
  int16_t axis = (int16_t)paddle - 0x80;
  switch (menu_wheel_sensitivity) {
    case 0: axis /= 2; break;
    case 2: axis *= 2; break;
    default: break;
  }
  return clampJogconAxis(axis);
}

}  // namespace

void RZInputPSX::update_jogcon_display_name() {
  const char* names[] = { "JogCon-S", "JogCon-P", "JogCon-W", "JogCon-Dig" };
  if (mode <= 3) {
    controller_state_t& frame = inputFrame(0);
    setInputFrameTypeName(frame, names[mode]);
  }
}

void RZInputPSX::init_jogcon() {
  if (psx[0]->begin(*psxControllerDriver[0])) {
    haveController[0] = true;
    armControllerStartupGrace(0);

    if (!psx[0]->enterConfigMode ()) {
    } else {
      psx[0]->enableAnalogSticks ();
      psx[0]->enableRumble ();
      psx[0]->exitConfigMode ();
    }
    psx[0]->read ();
  }
}

bool RZInputPSX::handleJogconData() {
  bool stateChanged = false;

  uint8_t jogPosition = 0;
  uint8_t jogRevolutions = 0;
  JogconDirection jogDirection = JOGCON_DIR_NONE;
  JogconCommand cmdResult = JOGCON_CMD_NONE;

  bool gotJogconData = psx[0]->getJogconData(jogPosition, jogRevolutions, jogDirection, cmdResult);
  JogconCommand nextCmd = JOGCON_CMD_NONE;
  if(jogDirection == JOGCON_DIR_MAX) {
    nextCmd = JOGCON_CMD_DROP_REVOLUTIONS;
    jogDirection = jogcon_lastDirection;
    if (jogDirection == JOGCON_DIR_CW) {
      jogPosition = 254;
      jogRevolutions = 255;
    } else {
      jogPosition = 254;
      jogRevolutions = 0;
    }
    jogcon_prevcnt = 0;
    jogcon_cleancnt = 0;
    jogcon_counter = (jogRevolutions << 8) | jogPosition;
  }

  if(jogDirection != JOGCON_DIR_NONE)
    jogcon_lastDirection = jogDirection;

  jogcon_newcnt = (jogRevolutions << 8) | jogPosition;
  jogcon_newbtn = psx[0]->getButtonWord();
  jogcon_newbtn = (jogcon_newbtn & ~3) | ((jogcon_newbtn&1)<<2);

  if(!gotJogconData) {
    if (!enableMouseMove) {
      if(psx[0]->buttonPressed(PSB_L2) && psx[0]->buttonPressed(PSB_R2))
        mode = 0;
      else if(psx[0]->buttonPressed(PSB_L1) && psx[0]->buttonPressed(PSB_R1))
        mode = 3;
      else if(psx[0]->buttonPressed(PSB_L2))
        mode = 1;
      else if(psx[0]->buttonPressed(PSB_R2))
        mode = 2;

      if(psx[0]->buttonPressed(PSB_TRIANGLE))
        force = 1;
      else if(psx[0]->buttonPressed(PSB_CIRCLE))
        force = 3;
      else if(psx[0]->buttonPressed(PSB_CROSS))
        force = 7;
      else if(psx[0]->buttonPressed(PSB_SQUARE))
        force = 15;

      if(psx[0]->buttonPressed(PSB_L1) ^ psx[0]->buttonPressed(PSB_R1)) {
        sp_div = psx[0]->buttonPressed(PSB_L1) ? 2 : 1;
        sp_max = SP_MAX/sp_div;
        sp_half = sp_max/2;
      }
    } else {
      mode = 0;

      if(psx[0]->buttonPressed(PSB_CIRCLE))
        mouse_axis = 0;
      else if(psx[0]->buttonPressed(PSB_TRIANGLE))
        mouse_axis = 1;
      else if(psx[0]->buttonPressed(PSB_SQUARE))
        mouse_axis = 2;
      else if(psx[0]->buttonPressed(PSB_CROSS))
        mouse_axis = 3;
    }

    if(psx[0]->buttonPressed(PSB_PAD_UP))
      sp_step = 1;
    else if(psx[0]->buttonPressed(PSB_PAD_RIGHT))
      sp_step = 2;
    else if(psx[0]->buttonPressed(PSB_PAD_DOWN))
      sp_step = 4;
    else if(psx[0]->buttonPressed(PSB_PAD_LEFT))
      sp_step = 8;

    delay(200);
    init_jogcon();

    gotJogconData = psx[0]->getJogconData(jogPosition, jogRevolutions, jogDirection, cmdResult);

    jogcon_prevcnt = 0;
    jogcon_cleancnt = 0;
    jogcon_counter = (jogRevolutions << 8) | jogPosition;
    jogcon_pdlpos = sp_half;
    jogcon_wheelCenterSet = false;
    jogcon_oldbtn = 0xFFFF;
    jogcon_oldpaddle = 0xFF;
    jogcon_oldspinner = INT8_MIN;
    update_jogcon_display_name();
  } else {
    if(jogDirection != JOGCON_DIR_NONE) {
      jogcon_cleancnt += jogcon_newcnt - jogcon_counter;
      if(mode == 0 || mode == 3) {
        ff = 0;
        jogcon_pdlpos += (int16_t)(jogcon_newcnt - jogcon_counter);
        if(jogcon_pdlpos<0) jogcon_pdlpos = 0;
        if(jogcon_pdlpos>sp_max) jogcon_pdlpos = sp_max;
      }
    }

    if(mode == 1 || mode == 2) {
      if(((int16_t)jogcon_newcnt) < -sp_half) {
        jogcon_pdlpos = 0;
        if(mode == 1) ff = 1;
      }
      else if(((int16_t)jogcon_newcnt) > sp_half) {
        jogcon_pdlpos = sp_max;
        if(mode == 1) ff = 1;
      }
      else {
        if(mode == 1) ff = 0;
        jogcon_pdlpos = (uint16_t)(jogcon_newcnt + sp_half);
      }
    }

    if(mode == 2) ff = 1;

    if (ff == 1)
      psx[0]->setJogconMotorMode(JOGCON_DIR_START, nextCmd, force);
    else
      psx[0]->setJogconMotorMode(JOGCON_DIR_NONE, nextCmd, 0);

    int16_t val = ((int16_t)(jogcon_cleancnt - jogcon_prevcnt))/sp_step;
    if(val>127) val = 127; else if(val<-127) val = -127;
    jogcon_prevcnt += val*sp_step;
    const int8_t spinner = val;

    const uint8_t paddle = ((jogcon_pdlpos*255)/sp_max);

    if (mode == 3) {
      if(spinner < 0)
        jogcon_btntimeout = -64;
      else if(spinner > 0)
        jogcon_btntimeout = 63;
      bitWrite(jogcon_newbtn, 0, jogcon_btntimeout < 0);
      bitWrite(jogcon_newbtn, 1, jogcon_btntimeout > 0);
    }

    if(jogcon_oldbtn != jogcon_newbtn || jogcon_oldpaddle != paddle || jogcon_oldspinner != spinner) {
      stateChanged = true;
      jogcon_oldbtn = jogcon_newbtn;
      jogcon_oldpaddle = paddle;
      jogcon_oldspinner = spinner;

      handleDpad();
      const uint8_t i = 0;
      controller_state_t& frame = inputFrame(i);
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
      frame.LX = 0;
      frame.LY = 0;
      frame.RX = 0;
      frame.RY = 0;
      frame.HAS_ANALOG_STICK_MAIN = (!enableMouseMove && mode != 3);
      frame.HAS_ANALOG_STICK_AUX = 0;
      frame.PAD_U = psx[0]->buttonPressed(PSB_PAD_UP);
      frame.PAD_D = psx[0]->buttonPressed(PSB_PAD_DOWN);
      frame.PAD_L = psx[0]->buttonPressed(PSB_PAD_LEFT);
      frame.PAD_R = psx[0]->buttonPressed(PSB_PAD_RIGHT);
      frame.A  = psx[0]->buttonPressed(PSB_CROSS);
      frame.B  = psx[0]->buttonPressed(PSB_CIRCLE);
      frame.X  = psx[0]->buttonPressed(PSB_SQUARE);
      frame.Y  = psx[0]->buttonPressed(PSB_TRIANGLE);
      frame.L1 = psx[0]->buttonPressed(PSB_L1);
      frame.R1 = psx[0]->buttonPressed(PSB_R1);
      frame.L2 = psx[0]->buttonPressed(PSB_L2);
      frame.R2 = psx[0]->buttonPressed(PSB_R2);
      frame.HAS_ANALOG_TRIGGERS = 0;
      frame.ANALOG_L2 = 0;
      frame.ANALOG_R2 = 0;
      frame.L3 = psx[0]->buttonPressed(PSB_L3);
      frame.R3 = psx[0]->buttonPressed(PSB_R3);
      frame.SELECT = psx[0]->buttonPressed(PSB_SELECT);
      frame.START = psx[0]->buttonPressed(PSB_START);

      if (mode == 3) {
        bool spinCCW = jogcon_btntimeout < 0;
        bool spinCW = jogcon_btntimeout > 0;
        switch (menu_jogcon_digital_map) {
          case 0: frame.L3    |= spinCCW; frame.R3    |= spinCW; break;
          case 1: frame.PAD_L |= spinCCW; frame.PAD_R |= spinCW; break;
          case 2: frame.L1    |= spinCCW; frame.R1    |= spinCW; break;
          case 3: frame.PAD_U |= spinCCW; frame.PAD_D |= spinCW; break;
        }
      }

      if(enableMouseMove) {
        frame.mouse_x       = mouse_axis == 0 ? spinner : 0;
        frame.mouse_y       = mouse_axis == 1 ? spinner : 0;
        frame.mouse_wheel_y = mouse_axis == 2 ? spinner : 0;
        frame.mouse_wheel_x = mouse_axis == 3 ? spinner : 0;
      } else if (mode != 3) {
        const int16_t axis = centeredJogconPaddleAxis(paddle);
        if (menu_jogcon_wheel_axis == 0)
          frame.LX = axis;
        else
          frame.LY = axis;
        frame.paddle = paddle;
        frame.spinner = spinner;
      }
    }
  }
  jogcon_counter = jogcon_newcnt;

  if (mode == 3) {
    if(jogcon_btntimeout < 0)
      jogcon_btntimeout ++;
    else if(jogcon_btntimeout > 0)
      jogcon_btntimeout --;
  }
  return stateChanged;
}

void RZInputPSX::jogconSetup() {
  force = menu_jogcon_force;
  update_jogcon_display_name();
  mouse_axis = 0;
  sp_step = 4;
  sp_div = 1;
  sp_max = SP_MAX/sp_div;
  sp_half = sp_max/2;

  setInputPortCount(1);
  enableMouseMove = haveController[0] && psx[0]->buttonPressed(PSB_SELECT);
}

void RZInputPSX::jogconSetup2() {
  output_try_enable_psx_jogcon_mode();

  if (enableMouseMove)
    delay(200);
}
