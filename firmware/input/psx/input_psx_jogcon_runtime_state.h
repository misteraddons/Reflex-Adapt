#pragma once

#include <Arduino.h>
#include <PsxNewLib/PsxPublicTypes.h>

extern byte ff;
extern byte mode;
extern byte force;
extern byte mouse_axis;
extern int16_t sp_step;
extern uint8_t sp_div;
extern int16_t sp_max;
extern int16_t sp_half;
extern uint16_t jogcon_counter;
extern uint16_t jogcon_newcnt;
extern uint16_t jogcon_cleancnt;
extern uint16_t jogcon_newbtn;
extern uint16_t jogcon_oldbtn;
extern int8_t jogcon_oldspinner;
extern uint8_t jogcon_oldpaddle;
extern int32_t jogcon_pdlpos;
extern int16_t jogcon_wheelCenter;
extern bool jogcon_wheelCenterSet;
extern uint16_t jogcon_prevcnt;
extern int8_t jogcon_btntimeout;
extern JogconDirection jogcon_lastDirection;
