#include "out_SwitchCommon.h"
#include "out_SwitchUsb.h"


class OutputSwitchPro {
  public:
  SwitchCommon *switchCommon;
  
  OutputSwitchPro() {
    switchCommon = new SwitchUsb();
    switchCommon->init();
  }

  void switchpro_set_report_callback(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) report_id;
    (void) report_type;
    hid_report_data_callback(itf, switchCommon, (uint16_t)buffer[0], (uint8_t *)buffer, bufsize);
  }

};

//SwitchCommon *switchCommon = new SwitchUsb();
//SwitchCommon *switchCommon;


//void switchpro_set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
//  (void) report_id;
//  (void) report_type;
//  hid_report_data_callback(switchCommon, (uint16_t)buffer[0], (uint8_t *)buffer, bufsize);
//}

//void switchpro_setup() {}

//uint8_t xinput_dev_addr = 0;
//uint8_t xinput_instance = 0;


//void switchpro_loop() { }
//void switchprogenericout() {}



//void switchpro_tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report, uint16_t len) {
////  switchprogenericout();
//    //output_generic();
//}
//
//void switchpro_tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
//  //switchprogenericout();
//  //output_generic();
//}
