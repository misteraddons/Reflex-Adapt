#include "input_psx_jogcon_runtime_state.h"

byte ff = 0;
byte mode = 0;
byte force = 0;
byte mouse_axis = 0;
int16_t sp_step = 0;
uint8_t sp_div = 0;
int16_t sp_max = 0;
int16_t sp_half = 0;
uint16_t jogcon_counter = 0;
uint16_t jogcon_newcnt = 0;
uint16_t jogcon_cleancnt = 0;
uint16_t jogcon_newbtn = 0;
uint16_t jogcon_oldbtn = 0;
int8_t jogcon_oldspinner = 0;
uint8_t jogcon_oldpaddle = 0;
int32_t jogcon_pdlpos = 0;
int16_t jogcon_wheelCenter = 0;
bool jogcon_wheelCenterSet = false;
uint16_t jogcon_prevcnt = 0;
int8_t jogcon_btntimeout = 0;
JogconDirection jogcon_lastDirection = JOGCON_DIR_NONE;
