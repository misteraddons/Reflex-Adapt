/*******************************************************************************
 * This file is part of PsxNewLib.                                             *
 *                                                                             *
 * Copyright (C) 2019-2020 by SukkoPera <software@sukkology.net>               *
 *                                                                             *
 * PsxNewLib is free software: you can redistribute it and/or                  *
 * modify it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * PsxNewLib is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with PsxNewLib. If not, see http://www.gnu.org/licenses.              *
 ******************************************************************************/
/**
 * \file PsxCommands.h
 * \author SukkoPera <software@sukkology.net>
 * \date 22 Mar 2021
 * \brief Playstation Controller Commands
 * 
 * Please refer to the GitHub page and wiki for any information:
 * https://github.com/SukkoPera/PsxNewLib
 */

#pragma once

#include <Arduino.h>

//! \name Controller Commands
//! @{

/** \brief Enter Configuration Mode
 * 
 * Command used to enter the controller configuration (also known as \a escape)
 * mode
 */
const byte enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
const byte exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
/* These shorter versions of enter_ and exit_config are accepted by all
 * controllers I've tested, even in analog mode, EXCEPT SCPH-1200, so let's use
 * the longer ones
 */
//~ byte enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
//~ byte exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x00};

/** \brief Read Controller Type
 * 
 * Command used to read the controller type.
 * 
 * This does not seem to be 100% reliable, or at least we don't know how to tell
 * all the various controllers apart.
 */
const byte type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
const byte c46[] = {0x01, 0x46, 0x00, /* motor index */ 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A}; //actuators
const byte set_mode[] = {0x01, 0x44, 0x00, /* enabled */ 0x01, /* locked */ 0x03, 0x00, 0x00, 0x00, 0x00};
const byte set_pressures[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00};
//~ byte enable_rumble[] = {0x01, 0x4D, 0x00, 0x00, 0x01};
const byte enable_rumble[] = {0x01, 0x4D, 0x00, /* motor 1 on */ 0x00, /* motor 2 on*/ 0x01, 0xff, 0xff, 0xff, 0xff};

/** \brief Poll all buttons
 *
 * Command used to read the status of all buttons.
 */
const byte poll[] = {0x01, 0x42, 0x00, 0xFF, 0xFF};

/** \brief Poll all controllers
 *
 * Command used to read the status of all controllers when using a MultiTap.
 */
const byte multipoll[] = {0x01, 0x42, 0x01, 0x00 /* Could be 42 */, 0x00};

/** \brief Detect IR remote receiver (SCPH-10160)
 *
 * Command used to detect the PS2 IR remote receiver.
 * Uses address 0x61 (IR mode) with command 0x04.
 * Returns 0x12 in byte 1 if IR receiver is present.
 */
const byte ir_detect[] = {0x61, 0x04, 0x00, 0x00, 0x00};

/** \brief Poll IR remote receiver (SCPH-10160)
 *
 * Command used to poll the PS2 IR remote receiver for button input.
 * Uses address 0x61 (IR mode) with standard poll command 0x42.
 * This makes the IR receiver respond as a digital controller,
 * mapping IR codes to controller buttons internally.
 */
const byte ir_poll[] = {0x61, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//! @}


//! \name Controller Reply Validation
//! @{

/** \brief Check if a reply has the Digital format
 *
 * This is the earliest reply form, which includes data for 14 buttons.
 */
inline static boolean isDigitalReply (const byte *status) {
	return (status[1] & 0xF0) == 0x40;
}

/** \brief Check if a reply has the Flightstick format
 *
 * This is also called "Green Mode" because the led on SCPH-1150/1180 turns
 * green when it is enabled.
 */
inline static boolean isFlightstickReply (const byte *status) {
	return status[1] == 0x53;//return (status[1] & 0xF0) == 0x50;
}

/** \brief Check if a reply has the DualShock format
 *
 * This means it includes data for the two Analog Sticks and L3/R3.
 */
inline static boolean isDualShockReply (const byte *status) {
	return (status[1] & 0xF0) == 0x70;
}

/** \brief Check if a reply has the DualShock2 format
 *
 * This means it includes data for the two Analog Sticks, L3/R3 and analog
 * pressure levels for (almost) all buttons.
 */
inline static boolean isDualShock2Reply (const byte *status) {
	return status[1] == 0x79;
}

/** \brief Check if a reply has the Configuration Mode format
 *
 * This is only supported from DualShock onwards.
 */
inline static boolean isConfigReply (const byte *status) {
	return (status[1] & 0xF0) == 0xF0;
}

/** \brief Check if a reply has the neGcon format
 */
inline static boolean isNegconReply (const byte *status) {
	return status[1] == 0x23;
}

/** \brief Check if a reply has the JogCon format
 */
inline static boolean isJogconReply (const byte *status) {
	return status[1] == 0xE3;
}

/** \brief Check if a reply has the GunCon format
 */
inline boolean isGunconReply (const byte *status) {
	return status[1] == 0x63;
}

/** \brief Check if a reply has the Mouse format (SCPH-1030)
 */
inline boolean isMouseReply (const byte *status) {
	return status[1] == 0x12;
}

/** \brief Check if a reply has the Fishing format
 */
inline boolean isFishingReply (const byte *status) {
	return status[1] == 0xE5;
}

/** \brief Check if a reply has the Sphere 360 (spaceball) format
 */
inline boolean isSpaceballReply (const byte *status) {
	return status[1] == 0x55;
}

/** \brief Check if a reply has the MultiTap format
 *
 * This means it has DualShock-style data for 4 controllers (i.e.: 8 bytes per
 * controller).
 */
inline static boolean isMultiTapReply (const byte *status) {
	return (status[1] & 0xF0) == 0x80;
}

/** \brief Check if a reply has the IR Remote format (SCPH-10160)
 *
 * IR receiver replies with ID 0x12 (same as mouse) when polled with
 * address 0x61. The data format is:
 * Byte 0: 0xFF (always)
 * Byte 1: 0x12 (device ID)
 * Byte 2: 0x5A (always)
 * Byte 3: Low byte of IR data (or 0xFF if no button pressed)
 * Byte 4: High byte of IR data (or 0xFF if no button pressed)
 *
 * Note: This is only valid when using ir_poll[] command (address 0x61).
 */
inline static boolean isIRRemoteReply (const byte *status) {
	return status[1] == 0x12;
}
//! @}
