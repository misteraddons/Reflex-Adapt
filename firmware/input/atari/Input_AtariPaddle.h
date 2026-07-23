#pragma once

#include "../base/RZInputModule.h"

typedef struct {
  uint8_t pot_a;
  uint8_t pot_b;
  uint8_t button_a;
  uint8_t button_b;
} input_paddle_config_t;

extern const input_paddle_config_t input_paddle_config;

class RZInputPaddle : public RZInputModule {
  private:
    static constexpr uint8_t input_ports = 2;
    uint32_t samples_a[2] = {0};
    uint32_t samples_b[2] = {0};
    uint8_t sample_index = 0;
    uint32_t cal_min_a = 100000;
    uint32_t cal_max_a = 100;
    uint32_t cal_min_b = 100000;
    uint32_t cal_max_b = 100;

    uint32_t readPaddleRC(uint8_t pin);
    uint32_t getSmoothedValue(uint32_t* samples);
    int16_t timeToAxis(uint32_t time, uint32_t& cal_min, uint32_t& cal_max);

  public:
    uint32_t debug_time_a = 0;
    uint32_t debug_time_b = 0;
    uint32_t debug_smoothed_a = 0;
    uint32_t debug_smoothed_b = 0;
    bool debug_btn_a = false;
    bool debug_btn_b = false;

    RZInputPaddle();

    const char* getUsbId() override;
    void configureBcdDeviceVersion() override;
    const char* getDescription() override;
    void setup() override;
    void setup2() override;
    bool poll() override;
    uint32_t getCalMinA();
    uint32_t getCalMaxA();
    uint32_t getCalMinB();
    uint32_t getCalMaxB();
    uint8_t getPotAPin();
    uint8_t getPotBPin();
    uint8_t getBtnAPin();
    uint8_t getBtnBPin();
    void resetCalibration();
};
