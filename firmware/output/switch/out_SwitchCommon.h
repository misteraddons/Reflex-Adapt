#ifndef SwitchCommon_h
#define SwitchCommon_h

//#include "Controller.h"
#include "out_SwitchConsts.h"

// typedef struct __attribute((packed, aligned(1))) {
struct SwitchRumbleData {
    float high_band_freq;
    float high_band_amp;
    float low_band_freq;
    float low_band_amp;
};

typedef enum {
  BLUETOOTH_PAIR_REQUEST = 0x01,
  REQUEST_DEVICE_INFO = 0x02,
  SET_SHIPMENT = 0x08,
  SPI_READ = 0x10,
  SET_MODE = 0x03,
  TRIGGER_BUTTONS = 0x04,
  TOGGLE_IMU = 0x40,
  ENABLE_VIBRATION = 0x48,
  SET_PLAYER = 0x30,
  SET_NFC_IR_STATE = 0x22,
  SET_NFC_IR_CONFIG = 0x21,
  IMU_SENSITIVITY = 0x41
} SwitchRequest;

typedef enum {
  SWITCHPRO_PRO = 0,
  SWITCHPRO_NES,
  SWITCHPRO_SNES,
  SWITCHPRO_N64,
} switchpro_mode_enum;

const uint8_t VIB_OPTS[4] = {0x0a, 0x0c, 0x0b, 0x09};

class SwitchCommon {
 public:
  virtual void init() = 0;
  void setSwitchRequestReport(uint8_t *report, int report_size);
  void set_empty_switch_request_report();
  void set_controller_rumble(uint8_t itf, uint8_t left, uint8_t right);
  uint16_t get_controller_rumble();
  SwitchRumbleData rumble_data[2];
  static switchpro_mode_enum switchpro_mode;
  uint8_t *generate_report();
  SwitchReport _switchReport = {
      .batteryConnection = 0x91,
      .buttons = {0x0},
      .lx = SWITCH_JOYSTICK_MID,
      .ly = SWITCH_JOYSTICK_MID,
      .rx = SWITCH_JOYSTICK_MID,
      .ry = SWITCH_JOYSTICK_MID,
      //.l = {0xff,0xf7,0x7f},
      //.r = {0xff,0xf7,0x7f}
  };
  //Controller *_controller;
  virtual uint8_t *generate_usb_report() = 0;

 protected:
  void set_empty_report();
  void set_subcommand_reply();
  void set_unknown_subcommand(uint8_t subcommand_id);
  void set_timer();
  void set_full_input_report();
  void set_standard_input_report();
  void set_bt();
  void set_device_info();
  void set_shipment();
  void toggle_imu();
  void imu_sensitivity();
  void set_imu_data();
  uint8_t spi_read_byte(uint8_t addr_top, uint8_t addr_bottom);
  void spi_read();
  void set_mode();
  void set_trigger_buttons();
  void enable_vibration();
  void set_player_lights();
  void set_nfc_ir_state();
  void set_nfc_ir_config();
  uint8_t _report[100] = {0x0};
  uint8_t _switchRequestReport[100] = {0x0};
  uint8_t _addr[6] = {0x0};
  bool _vibration_enabled = false;
  uint8_t _vibration_report = 0x00;
  uint8_t _vibration_idx = 0x00;
  bool _imu_enabled = false;
  uint8_t _player_number = 0x00;
  bool _device_info_queried = false;
  uint32_t _timer = 0;
  uint32_t _timestamp = 0;
  uint16_t _rumble = false;
};

void hid_report_data_callback(uint8_t itf, SwitchCommon *inst, uint16_t report_id,
                              uint8_t *report, int report_size);
bool is_switch_output_report(uint16_t report_id, const uint8_t *report, int report_size);
#endif
