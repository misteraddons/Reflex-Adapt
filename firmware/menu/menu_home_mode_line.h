#pragma once

#ifdef __cplusplus
extern "C" {
#endif
bool tud_connected(void);
bool tud_mounted(void);
#ifdef __cplusplus
}
#endif

const char* getOutputDisplayName();
const char* getOutputDisplayCompactName();
void renderHomeModeLine(bool showScanning, bool force = false);
