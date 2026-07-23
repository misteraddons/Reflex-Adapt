#pragma once

#include <stdint.h>

int normalize_switch_out_report(
    uint16_t report_id,
    uint8_t* report,
    int report_size,
    uint8_t* out,
    int out_cap);

bool is_switch_output_report(uint16_t report_id, const uint8_t* report, int report_size);
