#ifndef _OUT_REPORT_H_
#define _OUT_REPORT_H_

//from https://github.com/jfedor2/hid-remapper

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

//enum class OutType : int8_t {
//    OUTPUT = 0,
//    GET_FEATURE = 1,
//    SET_FEATURE = 2,
//};

void do_queue_out_report(const uint8_t* report, uint16_t len, uint8_t report_id, uint8_t dev_addr, uint8_t interface, bool use_interrupt_edp, uint8_t type);
void do_queue_get_report(uint8_t report_id, uint8_t dev_addr, uint8_t interface, bool use_interrupt_edp, uint8_t type, uint8_t len);
void do_reset_out_report_queue();
void do_send_out_report();

extern void host_get_report_cb(uint8_t dev_addr, uint8_t interface, uint8_t report_id, uint8_t report_type, uint8_t* report, uint16_t len);
extern void host_set_report_complete_cb(uint8_t dev_addr, uint8_t interface, uint8_t report_id);
extern void host_sent_report_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
