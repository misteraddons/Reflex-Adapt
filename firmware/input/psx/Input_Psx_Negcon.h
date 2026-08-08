/*******************************************************************************
 * PlayStation (NEGCON) input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 *
 * Handles a single input port.
 *
 * Uses PsxNewLib
 * https://github.com/SukkoPera/PsxNewLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

// Runtime state is owned in input_psx_negcon_runtime_state.cpp.

void negconSetup();
void negconSetup2();
void loopNeGcon(uint8_t i);
