#pragma once

inline void map_mdmini_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);

#ifdef ENABLE_INPUT_JAGUAR
  if (jaguarRotaryActiveOnPort(port)) {
    _mdmini.lx = 0x7f;
  } else
#endif
  if (frame.PAD_L) {
    _mdmini.lx = 0x00;
  } else if (frame.PAD_R) {
    _mdmini.lx = 0xff;
  } else {
    _mdmini.lx = 0x7f;
  }

  if (frame.PAD_U) {
    _mdmini.ly = 0x00;
  } else if (frame.PAD_D) {
    _mdmini.ly = 0xff;
  } else {
    _mdmini.ly = 0x7f;
  }

  if (isPositionButtonMapActive()) {
    _mdmini.a = frame.B;
    _mdmini.b = frame.A;
    _mdmini.x = frame.Y;
    _mdmini.y = frame.X;
  } else {
    _mdmini.a = frame.A;
    _mdmini.b = frame.B;
    _mdmini.x = frame.X;
    _mdmini.y = frame.Y;
  }
  _mdmini.c = frame.R1;
  _mdmini.z = frame.L1;
  _mdmini.l = frame.L2;
  _mdmini.r = frame.R2;
  _mdmini.mode = frame.SELECT;
  _mdmini.start = frame.START;
}
