/*
  Derived class
  Dynamic HID with:
  32 buttons
  Hat
*/

#ifndef JOY2_h
#define JOY2_h

#include "Joystick.h"

//typedef struct {
//  uint8_t id;
//  uint32_t buttons;
//  uint8_t hat;
//} GamepadReport2;

class Joy2_ : public Joystick_
{
  public:
    Joy2_(const char* serial, const uint8_t reportId, const uint8_t deviceType, const uint8_t totalControllers);
    void resetState();
    //void setButton(const uint8_t index, const bool value) {
    //    if (index < 8) {
    //      if (value)
    //        _GamepadReport.byte1 |= 1 << index;
    //      else
    //        _GamepadReport.byte1 &= ~(1 << index);
    //    } else {
    //      if (value)
    //        _GamepadReport.byte2 |= 1 << (index-8);
    //      else
    //        _GamepadReport.byte2 &= ~(1 << (index-8));
    //    }
    //}; //bytes 1 and 2
    void setButtons(const uint32_t value) {
      const uint16_t low = value;
      const uint16_t high = value >> 16;
      setByte1(lowByte(low)); setByte2(highByte(low));
      setByte3(lowByte(high)); setByte4(highByte(high));
    };
    void setHatSwitch(const uint8_t value) { setByte5(value); };
    //void setAnalog0(const uint8_t value) { setByte4(value); };
    //void setAnalog1(const uint8_t value) { setByte5(value); };
    //void setAnalog2(const uint8_t value) { setByte6(value); };
    //void setAnalog3(const uint8_t value) { setByte7(value); };
    //void setAnalog4(const uint8_t value) { setByte8(value); };
};

#endif // JOY2_h
