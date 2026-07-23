#pragma once

#include "../output_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

bool authOutputModeRequiresAuth(outputMode_t mode);
bool authOutputModeHasProvider(outputMode_t mode);
bool authOutputModeCanRun(outputMode_t mode);
bool authOutputModeShouldShowLock(outputMode_t mode);
bool authOutputModeShouldShowKey(outputMode_t mode);

#ifdef __cplusplus
}
#endif
