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

uint8_t configuredJogconMode(bool mouseMode) {
  return mouseMode ? 0 : (menu_jogcon_mode <= 3 ? menu_jogcon_mode : 0);
}

int16_t configuredJogconSpinnerStep() {
  static const uint8_t steps[] = { 16, 8, 4, 2, 1 };
  return steps[spinner_speed <= 4 ? spinner_speed : 2];
}

uint8_t configuredJogconForce() {
  return menu_jogcon_force == 1 || menu_jogcon_force == 3 ||
         menu_jogcon_force == 7 || menu_jogcon_force == 15
           ? menu_jogcon_force
           : 15;
}

}  // namespace

void RZInputPSX::update_jogcon_display_name() {
  const char* names[] = { "JogCon-S", "JogCon-P", "JogCon-W", "JogCon-Fake" };
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
  const uint8_t selectedMode = configuredJogconMode(enableMouseMove);
  const bool modeChanged = mode != selectedMode;
  if (modeChanged) {
    mode = selectedMode;
    update_jogcon_display_name();
    // Match the physical MODE-button path: reset the JogCon's internal zero
    // and re-arm its motor before applying Paddle stoppers or Wheel centering.
    init_jogcon();
  }
  force = configuredJogconForce();

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

  if (modeChanged) {
    jogcon_prevcnt = 0;
    jogcon_cleancnt = 0;
    jogcon_counter = jogcon_newcnt;
    jogcon_pdlpos = sp_half;
    jogcon_spinnerAnalog = 0;
    jogcon_wheelCenterSet = false;
    jogcon_oldbtn = 0xFFFF;
    jogcon_oldpaddle = 0xFF;
    jogcon_oldspinner = INT8_MIN;
  }

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
      menu_jogcon_mode = mode;

      if(psx[0]->buttonPressed(PSB_TRIANGLE))
        force = 1;
      else if(psx[0]->buttonPressed(PSB_CIRCLE))
        force = 3;
      else if(psx[0]->buttonPressed(PSB_CROSS))
        force = 7;
      else if(psx[0]->buttonPressed(PSB_SQUARE))
        force = 15;
      menu_jogcon_force = force;

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
      spinner_speed = 4;
    else if(psx[0]->buttonPressed(PSB_PAD_RIGHT))
      spinner_speed = 3;
    else if(psx[0]->buttonPressed(PSB_PAD_DOWN))
      spinner_speed = 2;
    else if(psx[0]->buttonPressed(PSB_PAD_LEFT))
      spinner_speed = 1;

    delay(200);
    init_jogcon();

    gotJogconData = psx[0]->getJogconData(jogPosition, jogRevolutions, jogDirection, cmdResult);

    jogcon_prevcnt = 0;
    jogcon_cleancnt = 0;
    jogcon_counter = (jogRevolutions << 8) | jogPosition;
    jogcon_pdlpos = sp_half;
    jogcon_spinnerAnalog = 0;
    jogcon_wheelCenterSet = false;
    jogcon_oldbtn = 0xFFFF;
    jogcon_oldpaddle = 0xFF;
    jogcon_oldspinner = INT8_MIN;
    update_jogcon_display_name();
  } else {
    sp_step = configuredJogconSpinnerStep();
    if(jogDirection != JOGCON_DIR_NONE) {
      const int16_t delta = (int16_t)(jogcon_newcnt - jogcon_counter);
      jogcon_cleancnt += delta;
      if(mode == 0) {
        const int32_t analogMin = -128L * sp_step;
        const int32_t analogMax = 127L * sp_step;
        jogcon_spinnerAnalog += delta;
        if(jogcon_spinnerAnalog < analogMin) jogcon_spinnerAnalog = analogMin;
        if(jogcon_spinnerAnalog > analogMax) jogcon_spinnerAnalog = analogMax;
      }
    }

    const int16_t signedPosition = (int16_t)jogcon_newcnt;
    JogconDirection motorDirection = JOGCON_DIR_NONE;
    if(mode == 1 || mode == 2) {
      if(signedPosition < -sp_half) {
        jogcon_pdlpos = 0;
      }
      else if(signedPosition > sp_half) {
        jogcon_pdlpos = sp_max;
      }
      else {
        jogcon_pdlpos = (uint16_t)(signedPosition + sp_half);
      }
    }

    if(mode == 1) {
      // Match the proven MiSTer JogCon paddle behavior: START acts as a
      // motor brake beyond either endpoint, rather than driving the wheel.
      if(signedPosition < -sp_half || signedPosition > sp_half)
        motorDirection = JOGCON_DIR_START;
    } else if(mode == 2) {
      // Wheel actively drives back toward the zero captured on mode entry,
      // then uses START to hold a small center zone without hunting.
      constexpr int16_t kWheelCenterDeadzone = 4;
      if(signedPosition < -kWheelCenterDeadzone)
        motorDirection = JOGCON_DIR_CW;
      else if(signedPosition > kWheelCenterDeadzone)
        motorDirection = JOGCON_DIR_CCW;
      else
        motorDirection = JOGCON_DIR_START;
    }

    ff = motorDirection != JOGCON_DIR_NONE;
    psx[0]->setJogconMotorMode(
      motorDirection, nextCmd, ff ? force : 0);

    int16_t val = ((int16_t)(jogcon_cleancnt - jogcon_prevcnt))/sp_step;
    if(val>127) val = 127; else if(val<-127) val = -127;
    jogcon_prevcnt += val*sp_step;
    const int8_t spinner = val;

    const int16_t spinnerAnalogAxis =
      clampJogconAxis((int16_t)(jogcon_spinnerAnalog / sp_step));
    const uint8_t paddle = mode == 0
                             ? (uint8_t)(spinnerAnalogAxis + 0x80)
                             : (uint8_t)((jogcon_pdlpos * 255) / sp_max);

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
      frame.paddle = 0x80;
      frame.spinner = 0;
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
          case 0: frame.PAD_L |= spinCCW; frame.PAD_R |= spinCW; break;
          case 1: frame.L3    |= spinCCW; frame.R3    |= spinCW; break;
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
        const int16_t axis = mode == 0
                               ? spinnerAnalogAxis
                               : centeredJogconPaddleAxis(paddle);
        if (menu_jogcon_wheel_axis == 0)
          frame.LX = axis;
        else
          frame.LY = axis;
        if (mode == 0)
          frame.spinner = spinner;
        else
          frame.paddle = paddle;
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
  force = configuredJogconForce();
  mouse_axis = 0;
  sp_step = configuredJogconSpinnerStep();
  sp_div = 1;
  sp_max = SP_MAX/sp_div;
  sp_half = sp_max/2;
  jogcon_pdlpos = sp_half;
  jogcon_spinnerAnalog = 0;

  setInputPortCount(1);
  enableMouseMove = haveController[0] && psx[0]->buttonPressed(PSB_SELECT);
  mode = configuredJogconMode(enableMouseMove);
  update_jogcon_display_name();
}

void RZInputPSX::jogconSetup2() {
  output_try_enable_psx_jogcon_mode();

  if (enableMouseMove)
    delay(200);
}
