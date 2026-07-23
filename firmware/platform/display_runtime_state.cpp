#include "display_runtime_state.h"

#ifdef USE_I2C_DISPLAY
SSD1306AsciiWire display(Wire);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif
