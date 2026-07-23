#pragma once

// Shared output-side mapping helpers. This stays header-only so the runtime
// mapping headers can reuse it without widening the public output surface.

int16_t __not_in_flash_func(convertAnalogPrecision)(int16_t inputValue, analog_stick_precision from, analog_stick_precision to) {
  int16_t inputMin = 0;
  int16_t inputMax = 0;
  int16_t outputMin = 0;
  int16_t outputMax = 0;
  switch (from) {
    case ANALOG_STICK_PRECISION_8:  inputMin = INT8_MIN;  inputMax = INT8_MAX;  break;
    case ANALOG_STICK_PRECISION_12: inputMin = -2048;     inputMax = 2047;      break;
    case ANALOG_STICK_PRECISION_16: inputMin = INT16_MIN; inputMax = INT16_MAX; break;
  }
  switch (to) {
    case ANALOG_STICK_PRECISION_8:  outputMin = -127;      outputMax = INT8_MAX;  break;
    case ANALOG_STICK_PRECISION_12: outputMin = -2048;     outputMax = 2047;      break;
    case ANALOG_STICK_PRECISION_16: outputMin = INT16_MIN; outputMax = INT16_MAX; break;
  }

  inputValue = constrain(inputValue, inputMin, inputMax);

  if (inputValue == 0 || from == to) {
    return constrain(inputValue, outputMin, outputMax);
  }

  int32_t result = map(inputValue, inputMin, inputMax, outputMin, outputMax);
  return constrain(result, outputMin, outputMax);
}

static uint8_t __not_in_flash_func(directionsToHat)(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  bool up = frame.PAD_U;
  bool down = frame.PAD_D;
  bool left = frame.PAD_L;
  bool right = frame.PAD_R;

  #ifdef ENABLE_INPUT_JAGUAR
  if (jaguarRotaryActiveOnPort(port)) {
    left = false;
    right = false;
  }
  #endif

  if (up && down) {
    up = false;
    down = false;
  }
  if (left && right) {
    left = false;
    right = false;
  }

  if (up) {
    if (right) return 1;
    if (left) return 7;
    return 0;
  }

  if (down) {
    if (right) return 3;
    if (left) return 5;
    return 4;
  }

  if (left) return 6;
  if (right) return 2;
  return 8;
}
