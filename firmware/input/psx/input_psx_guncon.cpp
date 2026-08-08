#include "Input_Psx.h"

#include "../../core/settings_store.h"
#include "../../menu/menu_runtime_state.h"

uint16_t RZInputPSX::convertRange(const uint16_t gcMin, const uint16_t gcMax, const uint16_t value) {
  return map(value, gcMin, gcMax, 0, maxMouseValue);
}

void RZInputPSX::moveToCoords(uint16_t x, uint16_t y) {
#ifdef ENABLE_PSX_GUNCON_MOUSE
  if (enableMouseMove) {
      AbsMouse->setXAxis(x);
      AbsMouse->setYAxis(y);
  }
#endif

  if (enableJoystick) {
    controller_state_t& frame = inputFrame(0);
    frame.LX = x;
    frame.LY = y;
  }
}

void RZInputPSX::releaseAllButtons() {
#ifdef ENABLE_PSX_GUNCON_MOUSE
  if (enableMouseMove) {
      AbsMouse->setButtons(0);
      AbsMouse->sendState();
  }
#endif

  if (enableJoystick) {
    inputFrame(0).digital_buttons = 0;
  }
}

void RZInputPSX::readGuncon() {
  uint16_t x, y;
#if GUNCON_FORCE_MODE != 3
  uint16_t convertedX, convertedY;
#endif
  const GunconStatus gcStatus = psx[0]->getGunconCoordinates(x, y);

  if (gcStatus == GUNCON_OK) {
    noLightCount = 0;
#if GUNCON_FORCE_MODE == 3
    {
#else
    if (x >= minPossibleX && x <= maxPossibleX && y >= minPossibleY && y <= maxPossibleY) {
#endif
      lastX = x;
      lastY = y;

#if GUNCON_FORCE_MODE != 3
      if (x < minX)
        minX = x;
      else if (x > maxX)
        maxX = x;

      if (y < minY)
        minY = y;
      else if (y > maxY)
        maxY = y;
#endif

#if GUNCON_FORCE_MODE == 3
      if (enableJoystick) {
        moveToCoords(x + offsetX, y + offsetY);
      }
#else
      if (enableMouseMove || enableJoystick) {
        convertedX = convertRange(minX + offsetX, maxX + offsetX, x);
        convertedY = convertRange(minY + offsetY, maxY + offsetY, y);
        moveToCoords(convertedX, convertedY);
      }
#endif
    }
  }
  else if (gcStatus == GUNCON_NO_LIGHT) {
    if (lastX != 0 && lastY != 0) {
#if GUNCON_FORCE_MODE == 3
      moveToCoords(lastX + offsetX, lastY + offsetY);
#else
      convertedX = convertRange(minX + offsetX, maxX + offsetX, lastX);
      convertedY = convertRange(minY + offsetY, maxY + offsetY, lastY);
      moveToCoords(convertedX, convertedY);
#endif

      noLightCount++;

      if (noLightCount > maxNoLightCount) {
        noLightCount = 0;
        lastX = 0;
        lastY = 0;

#ifdef ENABLE_PSX_GUNCON_MOUSE
        if (enableMouseMove) {
            AbsMouse->setXAxis(0);
            AbsMouse->setYAxis(maxMouseValue);
        }
#endif

        if (enableJoystick) {
#if GUNCON_FORCE_MODE == 3
          controller_state_t& frame = inputFrame(0);
          frame.LX = 0;
          frame.LY = 0;
#else
          controller_state_t& frame = inputFrame(0);
          if (joyOffScreenEdge) {
            frame.LX = 0;
            frame.LY = maxMouseValue;
          } else {
            frame.LX = maxMouseValue/2;
            frame.LY = maxMouseValue/2;
          }
#endif
        }
      }
    }
    else if (psx[0]->buttonPressed(PSB_CIRCLE) && psx[0]->buttonPressed(PSB_START) && psx[0]->buttonPressed(PSB_CROSS)) {
      enableReport = false;
      releaseAllButtons();
      delay(1000);
    }
  }
}

void RZInputPSX::analogDeadZone(byte& value) {
  const int8_t delta = value - ANALOG_IDLE_VALUE;
  if (abs(delta) < ANALOG_DEAD_ZONE)
    value = ANALOG_IDLE_VALUE;
}

void RZInputPSX::readDualShock() {
  uint16_t x, y;
  byte analogX = ANALOG_IDLE_VALUE;
  byte analogY = ANALOG_IDLE_VALUE;
  if (psx[0]->getLeftAnalog(analogX, analogY)) {
    analogDeadZone(analogX);
    analogDeadZone(analogY);
  }
  x = convertRange(ANALOG_MIN_VALUE, ANALOG_MAX_VALUE, analogX);
  y = convertRange(ANALOG_MIN_VALUE, ANALOG_MAX_VALUE, analogY);
  moveToCoords(x, y);

  /*
  if (enableJoystick) {
    analogX = ANALOG_IDLE_VALUE;
    analogY = ANALOG_IDLE_VALUE;
    if (psx.getRightAnalog(analogX, analogY)) {
    }
    usbStick[0]->setRxAxis(analogX);
    usbStick[0]->setRyAxis(analogY);
  }
  */
}

void RZInputPSX::handleButtons() {
  uint8_t buttonData = 0;
  bitWrite(buttonData, 0, psx[0]->buttonPressed(PSB_CIRCLE));
  bitWrite(buttonData, 1, psx[0]->buttonPressed(PSB_START));
  bitWrite(buttonData, 2, psx[0]->buttonPressed(PSB_CROSS));

#ifdef ENABLE_PSX_GUNCON_MOUSE
  AbsMouse->setButtons(buttonData);
#endif
  inputFrame(0).digital_buttons = buttonData;
}

void RZInputPSX::runCalibration() {
  if (calibrationStep == 1) {
    if (psx[0]->buttonJustPressed(PSB_START)){
      offsetX = normalizeGunconAlignmentOffset((int16_t)offsetX - 1);
    } else if (psx[0]->buttonJustPressed(PSB_CROSS)){
      offsetX = normalizeGunconAlignmentOffset((int16_t)offsetX + 1);
    }

  } else if (calibrationStep == 2) {
    if (psx[0]->buttonJustPressed(PSB_START)) {
      offsetY = normalizeGunconAlignmentOffset((int16_t)offsetY - 1);
    } else if (psx[0]->buttonJustPressed(PSB_CROSS)) {
      offsetY = normalizeGunconAlignmentOffset((int16_t)offsetY + 1);
    }
  } else if (calibrationStep == 3) {
    menu_guncon_offset_x = offsetX;
    menu_guncon_offset_y = offsetY;
    saveGunconOffsets(offsetX, offsetY);
    calibrationStep = 0;
    return;
  }

  if (psx[0]->buttonJustPressed(PSB_CIRCLE)) {
    calibrationStep++;
  }
  uint16_t x, y;
  if (psx[0]->getGunconCoordinates(x, y) == GUNCON_OK) {
#if GUNCON_FORCE_MODE == 3
    moveToCoords(x + offsetX, y + offsetY);
#else
    moveToCoords(convertRange(minX + offsetX, maxX + offsetX, x), convertRange(minY + offsetY, maxY + offsetY, y));
#endif
  }

#ifdef ENABLE_PSX_GUNCON_MOUSE
  if (enableMouseMove) {
      AbsMouse->sendState();
  } else
#endif
  if (enableJoystick) {
  }
}

void RZInputPSX::loopGuncon() {
  if (calibrationStep != 0) {
    runCalibration();
    return;
  }

  if (!enableReport) {
    if (!enableMouseMove && !enableJoystick) {
#if defined(GUNCON_FORCE_MODE) && GUNCON_FORCE_MODE >= 0 && GUNCON_FORCE_MODE < 4
      enableReport = true;
#if GUNCON_FORCE_MODE == 0
      enableMouseMove = true;
#elif GUNCON_FORCE_MODE == 1
      enableJoystick = true;
#elif GUNCON_FORCE_MODE == 2
      enableJoystick = true;
      joyOffScreenEdge = true;
#elif GUNCON_FORCE_MODE == 3
      enableJoystick = true;
      enableReport = true;
#endif
#else
      if (psx[0]->buttonJustPressed(PSB_CIRCLE)) {
        enableReport = true;
        enableMouseMove = true;
        return;
      } else if (psx[0]->buttonJustPressed(PSB_START)) {
        enableReport = true;
        enableJoystick = true;
        return;
      } else if (psx[0]->buttonJustPressed(PSB_CROSS)) {
        enableReport = true;
        enableJoystick = true;
        joyOffScreenEdge = true;
        return;
      }
#endif
    } else if (psx[0]->buttonJustPressed(PSB_CIRCLE) || psx[0]->buttonJustPressed(PSB_START)) {
      enableReport = true;
      return;
    } else if (psx[0]->buttonJustPressed(PSB_CROSS)) {
      enableReport = true;
      calibrationStep = 1;
      delay(300);
      return;
    }
  }

  if (enableReport) {
    handleButtons();
    PsxControllerProtocol proto = psx[0]->getProtocol();
    switch (proto) {
    case PSPROTO_GUNCON:
      readGuncon();
      break;
    case PSPROTO_DUALSHOCK:
    case PSPROTO_DUALSHOCK2:
      break;
    default:
      return;
    }
  }

  if (enableReport) {
#ifdef ENABLE_PSX_GUNCON_MOUSE
    if (enableMouseMove) {
        AbsMouse->sendState();
    } else
#endif
    if (enableJoystick) {
    }
  }
}

void RZInputPSX::gunconSetup() {
  setInputPortCount(1);
  inputFrame(0).HAS_ANALOG_STICK_MAIN = 1;

  offsetX = menu_guncon_offset_x;
  offsetY = menu_guncon_offset_y;
}

void RZInputPSX::gunconSetup2() {
  output_try_enable_psx_guncon_mode();
}
