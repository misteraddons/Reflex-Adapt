/*******************************************************************************
 * PlayStation (JOGCON) input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 *
 * Handles a single input port.
 *
 * Based on MiSTer-A1 JogCon (JogConUSB) by sorgelig.
 * https://github.com/MiSTer-devel/Retro-Controllers-USB-MiSTer/tree/master/JogConUSB
 *
 * Uses PsxNewLib
 * https://github.com/SukkoPera/PsxNewLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

// JogCon settings
// The logical output mode is selected from persisted menu_jogcon_mode.
// Legacy shoulder-button + JogCon MODE chords remain available as session
// shortcuts and update the same live selection.
// Force can still be set via menu_jogcon_force.

#define SP_MAX  160

void update_jogcon_display_name();
void init_jogcon();
bool handleJogconData();
void jogconSetup();
void jogconSetup2();
