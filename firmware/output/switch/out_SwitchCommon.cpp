#include "out_SwitchCommon.h"

#include <stdlib.h>
#include <string.h>

extern void rumble_callback(uint8_t index, uint8_t left, uint8_t right);

void SwitchCommon::setSwitchRequestReport(uint8_t *report, int report_size) {
  memcpy(_switchRequestReport, report, report_size);
}

uint8_t *SwitchCommon::generate_report() {
  set_empty_report();
  _report[0] = 0xa1;
  switch (_switchRequestReport[10]) {
    case BLUETOOTH_PAIR_REQUEST:
      set_subcommand_reply();
      set_bt();
      break;
    case REQUEST_DEVICE_INFO:
      _device_info_queried = true;
      set_subcommand_reply();
      set_device_info();
      break;
    case SET_SHIPMENT:
      set_subcommand_reply();
      set_shipment();
      break;
    case SPI_READ:
      set_subcommand_reply();
      spi_read();
      break;
    case SET_MODE:
      set_subcommand_reply();
      set_mode();
      break;
    case TRIGGER_BUTTONS:
      set_subcommand_reply();
      set_trigger_buttons();
      break;
    case TOGGLE_IMU:
      set_subcommand_reply();
      toggle_imu();
      break;
    case IMU_SENSITIVITY:
      set_subcommand_reply();
      imu_sensitivity();
      break;
    case ENABLE_VIBRATION:
      set_subcommand_reply();
      enable_vibration();
      break;
    case SET_PLAYER:
      set_subcommand_reply();
      set_player_lights();
      break;
    case SET_NFC_IR_STATE:
      set_subcommand_reply();
      set_nfc_ir_state();
      break;
    case SET_NFC_IR_CONFIG:
      set_subcommand_reply();
      set_nfc_ir_config();
      break;
    default:
      set_full_input_report();
      break;
  }
  return _report;
}

void SwitchCommon::set_empty_report() {
  memset(_report, 0x00, sizeof(_report));
}

void SwitchCommon::set_empty_switch_request_report() {
  memset(_switchRequestReport, 0x00, sizeof(_switchRequestReport));
}

void SwitchCommon::set_subcommand_reply() {
  // Input Report ID
  _report[1] = 0x21;

  // TODO: Find out what the vibrator byte is doing.
  // This is a hack in an attempt to semi-emulate
  // actions of the vibrator byte as it seems to change
  // when a subcommand reply is sent.
  if (_vibration_enabled) {
    _vibration_idx = (_vibration_idx + 1) % 4;
    _vibration_report = VIB_OPTS[_vibration_idx];
  }

  set_standard_input_report();
}

void SwitchCommon::set_unknown_subcommand(uint8_t subcommand_id) {
  // Set NACK
  _report[14] = 0x80;

  // Set unknown subcommand ID
  _report[15] = subcommand_id;
}

void SwitchCommon::set_timer() {
  // If the timer hasn't been set before
  if (_timestamp == 0) {
    _timestamp = to_ms_since_boot(get_absolute_time());
    _report[2] = 0x00;
    return;
  }

  // Get the time that has passed since the last timestamp
  // in milliseconds
  uint32_t now = to_ms_since_boot(get_absolute_time());
  uint32_t delta_t = (now - _timestamp);

  // Get how many ticks have passed in hex with overflow at 255
  // Joy-Con uses 4.96ms as the timer tick rate
  uint32_t elapsed_ticks = int(delta_t * 4);
  _timer = (_timer + elapsed_ticks) & 0xFF;

  _report[2] = _timer;
  _timestamp = now;
}

void SwitchCommon::set_full_input_report() {
  // Setting Report ID to full standard input report ID
  _report[1] = 0x30;

  set_standard_input_report();
  set_imu_data();
}

void SwitchCommon::set_standard_input_report() {
  set_timer();

  //_controller->getSwitchReport(&_switchReport);
  memcpy(_report + 3, (uint8_t *)&_switchReport, sizeof(SwitchReport));
  _report[13] = _vibration_report;
}

void SwitchCommon::set_controller_rumble(uint8_t itf, uint8_t left, uint8_t right) {
  //_controller->setRumble(rumble);
  rumble_callback(itf, left, right); //todo pass interface index
  //uint16_t rumbling_combo = (amp_motor_right << 8) | amp_motor_left;
  _rumble = (right << 8) | left;
}

uint16_t SwitchCommon::get_controller_rumble() {
  return _rumble;
}
//Static members
switchpro_mode_enum SwitchCommon::switchpro_mode { SWITCHPRO_PRO };




//helper functions for analog stick data
//https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/spi_flash_notes.md#analog-stick-factory-and-user-calibration
//typedef struct {
//    uint16_t XAxisCenter;
//    uint16_t XAxisMinbelowcenter;
//    uint16_t XAxisMaxabovecenter;
//    uint16_t YAxisCenter;
//    uint16_t YAxisMinbelowcenter;
//    uint16_t YAxisMaxabovecenter;
//} stick_cal_t;
//
//stick_cal_t switch_analog_stick_decode(bool right, uint8_t *encoded) {
//  uint16_t data[6];
//  data[0] = ((encoded[1] << 8) & 0x0F00) | (encoded[0]);
//  data[1] =  (encoded[2] << 4)           | (encoded[1] >> 4);
//  data[2] = ((encoded[4] << 8) & 0x0F00) | (encoded[3]);
//  data[3] =  (encoded[5] << 4)           | (encoded[4] >> 4);
//  data[4] = ((encoded[7] << 8) & 0x0F00) | (encoded[6]);
//  data[5] =  (encoded[8] << 4)           | (encoded[7] >> 4);
//  stick_cal_t ret = {
//    .XAxisCenter         = right ? data[0] : data[2],
//    .XAxisMinbelowcenter = right ? data[2] : data[4],
//    .XAxisMaxabovecenter = right ? data[4] : data[0],
//    .YAxisCenter         = right ? data[1] : data[3],
//    .YAxisMinbelowcenter = right ? data[3] : data[5],
//    .YAxisMaxabovecenter = right ? data[5] : data[1],
//  };
//  return ret;
//}
//
//void switch_analog_stick_encode(uint16_t valueX, uint16_t valueY, uint8_t *out) {
//  out[0] =  (valueX & 0x00FF);
//  out[1] = ((valueY & 0x000F) << 4) | ((valueX & 0x0F00) >> 8);
//  out[2] = ((valueY & 0x0FF0) >> 4);
//}
//
//void switch_analog_stick_encode(bool right, stick_cal_t decoded, uint8_t *out) {
//  if (right) {
//    switch_analog_stick_encode(decoded.XAxisCenter        , decoded.YAxisCenter        , &out[0]);
//    switch_analog_stick_encode(decoded.XAxisMinbelowcenter, decoded.YAxisMinbelowcenter, &out[3]);
//    switch_analog_stick_encode(decoded.XAxisMaxabovecenter, decoded.YAxisMaxabovecenter, &out[6]);
//  } else {
//    switch_analog_stick_encode(decoded.XAxisMaxabovecenter, decoded.YAxisMaxabovecenter, &out[0]);
//    switch_analog_stick_encode(decoded.XAxisCenter        , decoded.YAxisCenter        , &out[3]);
//    switch_analog_stick_encode(decoded.XAxisMinbelowcenter, decoded.YAxisMinbelowcenter, &out[6]);
//  }
//}
//
//void generate_calibrated_data() {
//    //min and max value. in range of 128
//    //uint8_t minmax = 0x80 + 78; // n64?
//    //uint8_t minmax = 0x80 + 88; // ngc?
//    //uint8_t minmax = 0x80 + 94; // switch?
//    
//    uint16_t switchcen = 2048;
//    uint16_t switchmax = (((minmax) << 4) | (minmax & 0x0F)) - switchcen;
//
//    stick_cal_t switchnewcalibration = {
//        .XAxisCenter         = switchcen,
//        .XAxisMinbelowcenter = switchmax,
//        .XAxisMaxabovecenter = switchmax,
//        .YAxisCenter         = switchcen,
//        .YAxisMinbelowcenter = switchmax,
//        .YAxisMaxabovecenter = switchmax
//    };
//
//    uint8_t calibrated_data_l[9] {0};
//    uint8_t calibrated_data_r[9] {0};
//
//    switch_analog_stick_encode(0, switchnewcalibration, &calibrated_data_l[0]);
//    switch_analog_stick_encode(1, switchnewcalibration, &calibrated_data_r[0]);
//
//    printf("LStick ");
//    for (uint8_t i = 0; i < sizeof(calibrated_data_l); ++i)
//        printf("0x%02x, ", calibrated_data_l[i]);
//    printf("\n");
//    
//    printf("RStick ");
//    for (uint8_t i = 0; i < sizeof(calibrated_data_r); ++i)
//        printf("0x%02x, ", calibrated_data_r[i]);
//    printf("\n");
//}
