#include "../../product_config.h"

#include "input_dreamcast_debug_runtime_state.h"

#ifdef ENABLE_INPUT_DREAMCAST
Adafruit_USBD_CDC dreamcastDebugCdc;
bool dreamcastDebugCdcEnabled = false;
#endif
