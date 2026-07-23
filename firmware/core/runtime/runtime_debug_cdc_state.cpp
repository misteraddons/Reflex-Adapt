#include "../../product_config.h"

#include "runtime_debug_cdc_state.h"

#ifdef ENABLE_RUNTIME_SERIAL_DEBUG_CDC
Adafruit_USBD_CDC runtimeDebugCdc;
bool runtimeDebugCdcEnabled = false;
#endif
