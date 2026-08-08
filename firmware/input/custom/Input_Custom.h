#pragma once

#include "../base/RZInputModule.h"

//use -1 for non used buttons
typedef struct __attribute((packed, aligned(1))) {
  int8_t up;
  int8_t down;
  int8_t left;
  int8_t right;
  int8_t a;
  int8_t b;
  int8_t x;
  int8_t y;
  int8_t l1;
  int8_t r1;
  int8_t l2;
  int8_t r2;
  int8_t l3;
  int8_t r3;
  int8_t select;
  int8_t start;
  int8_t home;
  //int8_t capture;
} custom_buttons_t;

//use -1 for non used axis
typedef struct __attribute((packed, aligned(1))) {
  int8_t stick_lx;
  int8_t stick_ly;
  int8_t stick_rx;
  int8_t stick_ry;
} custom_sticks_t;


typedef struct __attribute((packed, aligned(1))) {
  union {
    custom_buttons_t buttons;
    uint8_t btnArray[sizeof(custom_buttons_t)];
  };
  union {
    custom_sticks_t sticks;
    uint8_t sticksArray[sizeof(custom_sticks_t)];
  };
  uint8_t debounceMs;
} input_custom_config_t;

extern const input_custom_config_t input_custom_config[1];


class RZInputCustom : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 1;
    ButtonDebouncer* debouncer[input_ports] = { nullptr };

    uint16_t tryReadAnalog(uint8_t pin);

  public:
    RZInputCustom();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
};
