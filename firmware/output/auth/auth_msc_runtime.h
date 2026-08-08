#pragma once

#include "../output_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

bool auth_msc_should_enable(outputMode_t effectiveOutputMode);
void auth_msc_configure(outputMode_t effectiveOutputMode);
bool auth_msc_is_enabled();
void auth_msc_task();

#ifdef __cplusplus
}
#endif
