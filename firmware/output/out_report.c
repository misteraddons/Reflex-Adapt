#include <stdio.h>
#include <string.h>

#include <tusb.h>

#include "out_report.h"

typedef struct {
    uint8_t dev_addr;
    uint8_t interface;
    bool use_interrupt_edp; //interrupt or control
    uint8_t report_id;
    uint8_t len; // uint16_t
    uint8_t request;
    uint8_t type; //OutType type;
    uint8_t report[64];  // XXX
} outgoing_out_report_t;

#define OOR_BUFSIZE 8
static outgoing_out_report_t outgoing_out_reports[OOR_BUFSIZE];
static uint8_t oor_head = 0;
static uint8_t oor_tail = 0;
static uint8_t oor_items = 0;

static uint8_t get_buffer[64];

static bool ready_to_send = true;

void do_reset_out_report_queue() {
    oor_head = 0;
    oor_tail = 0;
    oor_items = 0;
    ready_to_send = true;
}

void do_queue_out_report(const uint8_t* report, uint16_t len, uint8_t report_id, uint8_t dev_addr, uint8_t interface, bool use_interrupt_edp, uint8_t type) {
    if (oor_items == OOR_BUFSIZE) {
        printf("out overflow!\n");
        return;
    }
    size_t queued_len = (size_t) len + ((report_id != 0) ? 1u : 0u);
    if (queued_len > sizeof(outgoing_out_reports[oor_tail].report)) {
        return;
    }
    outgoing_out_reports[oor_tail].dev_addr = dev_addr;
    outgoing_out_reports[oor_tail].interface = interface;
    outgoing_out_reports[oor_tail].use_interrupt_edp = use_interrupt_edp;
    outgoing_out_reports[oor_tail].report_id = report_id;
    outgoing_out_reports[oor_tail].request = HID_REQ_CONTROL_SET_REPORT;
    outgoing_out_reports[oor_tail].len = (uint16_t) queued_len;
    outgoing_out_reports[oor_tail].type = type;
    if (report_id != 0) {
        outgoing_out_reports[oor_tail].report[0] = report_id;
    }
    memcpy(outgoing_out_reports[oor_tail].report + ((report_id != 0) ? 1 : 0), report, len);
    oor_tail = (oor_tail + 1) % OOR_BUFSIZE;
    oor_items++;
}

void do_queue_get_report(uint8_t report_id, uint8_t dev_addr, uint8_t interface, bool use_interrupt_edp, uint8_t type, uint8_t len) {
    if (oor_items == OOR_BUFSIZE) {
        printf("out overflow!\n");
        return;
    }
    outgoing_out_reports[oor_tail].dev_addr = dev_addr;
    outgoing_out_reports[oor_tail].interface = interface;
    outgoing_out_reports[oor_tail].use_interrupt_edp = use_interrupt_edp;
    outgoing_out_reports[oor_tail].report_id = report_id;
    outgoing_out_reports[oor_tail].request = HID_REQ_CONTROL_GET_REPORT;
    outgoing_out_reports[oor_tail].type = type;
    outgoing_out_reports[oor_tail].len = len;
    oor_tail = (oor_tail + 1) % OOR_BUFSIZE;
    oor_items++;
}

void do_send_out_report() {
    if ((oor_items > 0) && ready_to_send) {
        outgoing_out_report_t* out = &(outgoing_out_reports[oor_head]);
        if (out->request == HID_REQ_CONTROL_SET_REPORT) {
          if (out->use_interrupt_edp) {
              if (tuh_hid_send_report(out->dev_addr, out->interface, out->report_id, out->report, out->len)) {
                  ready_to_send = false;
                  oor_head = (oor_head + 1) % OOR_BUFSIZE;
                  oor_items--;
              }
          } else {
              if (tuh_hid_set_report(out->dev_addr, out->interface, out->report_id, out->type, out->report, out->len)) {
                  ready_to_send = false;
                  oor_head = (oor_head + 1) % OOR_BUFSIZE;
                  oor_items--;
              }
          }
        } else if (out->request == HID_REQ_CONTROL_GET_REPORT) {
            if (tuh_hid_get_report(out->dev_addr, out->interface, out->report_id, out->type, get_buffer, out->len)) {
                ready_to_send = false;
                oor_head = (oor_head + 1) % OOR_BUFSIZE;
                oor_items--;
            }
        }
    }
}

// Invoked when sent report to device successfully via interrupt endpoint
void tuh_hid_report_sent_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report, uint16_t len) {
    ready_to_send = true;
    host_sent_report_cb(dev_addr, idx, report, len);
}

// Invoked when Sent Report to device via either control endpoint
// len = 0 indicate there is error in the transfer e.g stalled response
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {
    ready_to_send = true;
    host_set_report_complete_cb(dev_addr, instance, report_id);
}

// Invoked when Get Report to device via either control endpoint
// len = 0 indicate there is error in the transfer e.g stalled response
void tuh_hid_get_report_complete_cb(uint8_t dev_addr, uint8_t idx, uint8_t report_id, uint8_t report_type, uint16_t len) {
    ready_to_send = true;
    host_get_report_cb(dev_addr, idx, report_id, report_type, get_buffer, len);
}
