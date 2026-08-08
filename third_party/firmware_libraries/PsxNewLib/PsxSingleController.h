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
 * \file PsxNewLib.h
 * \author SukkoPera <software@sukkology.net>
 * \date 27 Jan 2020
 * \brief Playstation controller interface library for Arduino
 * 
 * Please refer to the GitHub page and wiki for any information:
 * https://github.com/SukkoPera/PsxNewLib
 */

#pragma once

#include "PsxDriver.h"
#include "PsxPublicTypes.h"
#include "PsxOptions.h"
#include "PsxCommands.h"


/** \brief PSX Controller Interface
 * 
 * This is the base class implementing interactions with PSX controllers. It is
 * partially abstract, so it is not supposed to be instantiated directly.
 */
class PsxSingleController {
protected:
	PsxDriver *driver;

	PsxControllerData controller;

public:
	/** \brief Initialize library
	 * 
	 * This function shall be called before any others, it will initialize the
	 * communication and return if a supported controller was found. It shall
	 * also be called to reinitialize the communication whenever the controller
	 * is unplugged.
	 * 
	 * Derived classes can override this function if they need to perform
	 * additional initializations, but shall call it on return.
	 * 
	 * \return true if a supported controller was found, false otherwise
	 */
	virtual boolean begin (PsxDriver& drv) {
		driver = &drv;

		controller.clear ();

		return read ();
	}

	//! \name Configuration Mode Functions
	//! @{
	
	/** \brief Enter Configuration Mode
	 * 
	 * Some controllers can be configured in several aspects. For instance,
	 * DualShock controllers can return analog stick data. This function puts
	 * the controller in configuration mode.
	 * 
	 * Note that <i>Configuration Mode</i> is sometimes called <i>Escape Mode</i>.
	 * 
	 * \return true if Configuration Mode was entered successfully
	 */
	boolean enterConfigMode () {
		boolean ret = false;

		unsigned long start = millis ();
		do {
			driver -> selectController ();
			byte *in = driver -> autoShift (enter_config, 4);
			driver -> deselectController ();

			ret = in != NULL && isConfigReply (in);

			if (!ret) {
				delay (COMMAND_RETRY_INTERVAL);
			}
		} while (!ret && millis () - start <= COMMAND_TIMEOUT);
		delay (MODE_SWITCH_DELAY);

		return ret;
	}

	/** \brief Enable (or disable) analog sticks
	 * 
	 * This function enables or disables the analog sticks that were introduced
	 * with DualShock controllers. When they are enabled, the getLeftAnalog()
	 * and getRightAnalog() functions can be used to retrieve their positions.
	 * Also, button presses for L3 and R3 will be available through the
	 * buttonPressed() and similar functions.
	 * 
	 * When analog sticks are enabled, the \a ANALOG led will light up (in red)
	 * on the controller.
	 * 
	 * Note that on some third-party controllers, when analog sticks are
	 * disabled the analog levers will "emulate" the D-Pad and possibly the
	 * []/^/O/X buttons. This does not happen on official Sony controllers.
	 * 
	 * This function will only work if when the controller is in Configuration
	 * Mode.
	 * 
	 * \param[in] enabled true to enable, false to disable
	 * \param[in] locked If true, the \a ANALOG button on the controller will be
	 *                   disabled and the user will not be able to turn off the
	 *                   analog sticks.
	 * \return true if the command was ackowledged by the controller. Note that
	 *         this does not fully guarantee that the analog sticks were enabled
	 *         as this can only be checked after Configuration Mode is exited.
	 */
	boolean enableAnalogSticks (bool enabled = true, bool locked = false) {
		boolean ret = false;
		byte out[sizeof (set_mode)];

		memcpy (out, set_mode, sizeof (set_mode));
		out[3] = enabled ? 0x01 : 0x00;
		out[4] = locked ? 0x03 : 0x00;

		unsigned long start = millis ();
		byte cnt = 0;
		do {
			driver -> selectController ();
			byte *in = driver -> autoShift (out, 5);
			driver -> deselectController ();

			/* We can't know if we have successfully enabled analog mode until
			 * we get out of config mode, so let's just be happy if we get a few
			 * consecutive valid replies
			 */
			if (in != nullptr) {
				++cnt;
			}
			ret = cnt >= 3;

			if (!ret) {
				delay (COMMAND_RETRY_INTERVAL);
			}
		} while (!ret && millis () - start <= COMMAND_TIMEOUT);
		delay (MODE_SWITCH_DELAY);

		return ret;
	}

	/** \brief Enable (or disable) the vibration capability of the DualShock / DualShock 2
	 * 
	 * This function enables or disables the rumble feature of the DualShock / DualShock 2 controllers.
	 *  NOTE that this function does nothing on its own - the vibration on/off must be set using 
	 *  setRumble() and the controller will begin to vibrate when the read() function is 
	 *  next called.
	 *
	 * This function will only work if when the controller is in Configuration
	 * Mode.
	 * 
	 * \param[in] enabled true to enable both motors, false to disable them.
	 *
	 * \return true if we got bytes back. Eventually we should wait for ACK from the controller.
	 */
	boolean enableRumble(bool enabled = true) {
		boolean ret = true;
		byte out[sizeof (enable_rumble)];

		memcpy (out, enable_rumble, sizeof (enable_rumble));
		out[3] = enabled ? 0x00 : 0xff;
		out[4] = enabled ? 0x01 : 0xff;

		unsigned long start = millis ();
		byte cnt = 0;
		do {
			driver -> selectController ();
			byte *in = driver -> autoShift (out, 5);
			driver -> deselectController ();
			// attention ();
			// byte *in = autoShift (out, 5);
			// noAttention ();

			if (in != nullptr) {
				++cnt;
			}
			ret = cnt >= 3;

			if (!ret) {
				delay (COMMAND_RETRY_INTERVAL);
			}
		} while (!ret && millis () - start <= COMMAND_TIMEOUT);
		delay (MODE_SWITCH_DELAY);
		
		controller.rumbleEnabled = ret;
		return ret;
	}

	/** \brief Set the requested power output of the rumble motors on DualShock / DualShock 2 controllers.
	 * 
	 * This function sets internal variables that set the requested motor power of the rumble motors.
	 *  NOTE this does nothing if rumble has not been enabled with enableRumble(), rumble motors will 
	 *  activate or deactivate to match the arguments of this function with the next call to read()
	 *  
	 *  NOTE it's possible to use single motor rumble on the japanese SCPH-1150.
	 *  DualShock is also backwards compatible with this mode. This function will use the "old rumble"
	 * 	when rumbleEnabled is not set. After entering Config Mode, the old rumble will not work
	 *  anymore until the controller is powered off and on again.
	 *
	 * \param[in] enabled true to activate motor 1, false to deactivate.
	 * \param[in] requested motor power of motor 2, where 0x00 to 0xFF corresponds to 0 to 100%.
	 */
	void setRumble(bool motor1Active = true, byte motor2Power = 0xff) {
		if (controller.rumbleEnabled) {
			controller.motor1Level = motor1Active ? 0xff : 0x00;
			controller.motor2Level = motor2Power;
		} else { //Old rumble method. Single motor
			if (motor1Active || motor2Power > 100) {
				controller.motor1Level = MOTOR_OLD_1;
				controller.motor2Level = MOTOR_OLD_2;
			} else {
				controller.motor1Level = 0x0;
				controller.motor2Level = 0x0;
			}
		}
	}

	/** \brief Enable (or disable) analog buttons
	 * 
	 * This function enables or disables the analog buttons that were introduced
	 * with DualShock 2 controllers. When they are enabled, the
	 * getAnalogButton() functions can be used to retrieve how deep/strongly
	 * they are pressed. This applies to the D-Pad buttons, []/^/O/X, L1/2 and
	 * R1/2
	 * 
	 * This function will only work if when the controller is in Configuration
	 * Mode.
	 * 
	 * \param[in] enabled true to enable, false to disable
	 * \return true if the command was ackowledged by the controller. Note that
	 *         this does not fully guarantee that the analog sticks were enabled
	 *         as this can only be checked after Configuration Mode is exited.
	 */
	boolean enableAnalogButtons (bool enabled = true) {
		boolean ret = false;
		byte out[sizeof (set_mode)];

		memcpy (out, set_pressures, sizeof (set_pressures));
		if (!enabled) {
			out[3] = 0x00;
			out[4] = 0x00;
			out[5] = 0x00;
		}

		unsigned long start = millis ();
		byte cnt = 0;
		do {
			driver -> selectController ();
			byte *in = driver -> autoShift (out, sizeof (set_pressures));
			driver -> deselectController ();

			/* We can't know if we have successfully enabled analog mode until
			 * we get out of config mode, so let's just be happy if we get a few
			 * consecutive valid replies
			 */
			if (in != nullptr) {
				++cnt;
			}
			ret = cnt >= 3;

			if (!ret) {
				delay (COMMAND_RETRY_INTERVAL);
			}
		} while (!ret && millis () - start <= COMMAND_TIMEOUT);
		delay (MODE_SWITCH_DELAY);

		return ret;
	}

	/** \brief Retrieve the controller type
	 * 
	 * This function retrieves the controller type. It is not 100% reliable, so
	 * do not rely on it for anything other than a vague indication (for
	 * instance, the DualShock SCPH-1200 controller gets reported as the Guitar
	 * Hero controller...).
	 * 
	 * This function will only work if when the controller is in Configuration
	 * Mode.
	 * 
	 * \return The (tentative) controller type
	 */
	PsxControllerType getControllerType () {
		PsxControllerType ret = PSCTRL_UNKNOWN;

		driver -> selectController ();
		byte *in = driver -> autoShift (type_read, 3);
		driver -> deselectController ();

		if (in != nullptr) {
			const byte& controllerType = in[3];
			if (controllerType == 0x03) {
				ret = PSCTRL_DUALSHOCK2;
			//~ } else if (controllerType == 0x01 && in[1] == 0x42) {
				//~ return 4;		// ???
			} else if (controllerType == 0x01 && in[6] == 0x01) {
				ret = PSCTRL_JOGCON;
			//~ } else if (controllerType == 0x01 && in[1] != 0x42) {
			//~ 	ret = PSCTRL_GUITHERO;
			} else if (controllerType == 0x01) {
				ret = PSCTRL_DUALSHOCK;

				// check if its a waiwai controller
				if (in[6] == 0x02) {
					byte out[sizeof (c46)];
					memcpy (out, c46, sizeof (c46));
					
					// first actuator
					out[3] = 0;
					driver -> selectController ();
					in = driver -> autoShift (out, 4);
					driver -> deselectController ();

					if (in[8] == 0) {
						ret = PSCTRL_WAIWAI;
						// ideally it should also check the next actuator
						// second actuator
						// out[3] = 1;
						// driver -> selectController ();
						// in = driver -> autoShift (out, 4);
						// driver -> deselectController ();

						// if (in[8] == 0)
						// 	ret = PSCTRL_WAIWAI;
					}
				}

			} else if (controllerType == 0x0C) {
				ret = PSCTRL_DSWIRELESS;
			}
		}

		return ret;
	}

	boolean exitConfigMode () {
		boolean ret = false;

		unsigned long start = millis ();
		do {
			driver -> selectController ();
			//~ shiftInOut (poll, in, sizeof (poll));
			//~ shiftInOut (exit_config, in, sizeof (exit_config));
			byte *in = driver -> autoShift (exit_config, 4);
			driver -> deselectController ();

			ret = in != nullptr && !isConfigReply (in);

			if (!ret) {
				delay (COMMAND_RETRY_INTERVAL);
			}
		} while (!ret && millis () - start <= COMMAND_TIMEOUT);
		delay (MODE_SWITCH_DELAY);

		return ret;
	}

	//! @}		// Configuration Mode Functions
	
	//! \name Polling Functions
	//! @{

	/** \brief Retrieve the controller protocol
	 * 
	 * This function retrieves the protocol that was used to interpret
	 * controller data at the last call to read().
	 * 
	 * \return The controller protocol
	 */
	PsxControllerProtocol getProtocol () const {
		return controller.protocol;
	}

	/** \brief Poll the controller
	 * 
	 * This function polls the controller for button and stick data. It self-
	 * adapts to all the supported controller types and populates internal
	 * variables with the retrieved information, which can be later accessed
	 * through the inspection functions.
	 * 
	 * This function must be called quite often in order to keep the controller
	 * alive. Most controllers have some kind of watchdog that will reset them
	 * if they don't get polled at least every so often (like a couple dozen
	 * times per seconds).
	 * 
	 * If this function fails repeatedly, it can safely be assumed that the
	 * controller has been disconnected (or that it is not supported if it
	 * failed right from the beginning).
	 * 
	 * \return true if the read was successful, false otherwise
	 */
	boolean read () {
		boolean ret = false;

		controller.analogSticksValid = false;
		controller.analogButtonDataValid = false;
		controller.mouseDataValid = false;
		controller.protocolExtraDataValid = false;
		memset(controller.protocolExtraData, 0, sizeof(controller.protocolExtraData));

		driver -> selectController ();
		//byte *in = driver -> autoShift (poll, 3);
		//driver -> deselectController ();
		byte *in = nullptr;
		if (controller.rumbleEnabled || (controller.motor1Level == MOTOR_OLD_1 && controller.motor2Level == MOTOR_OLD_2)) {
			byte out[sizeof (poll)];
			memcpy(out, poll, sizeof(poll));
			if (controller.protocol == PSPROTO_JOGCON) {
				out[3] = controller.jogconMotorLevelAndMode;
			} else {
				out[3] = controller.motor1Level;
				out[4] = controller.motor2Level;
			}
			in = driver -> autoShift (out, sizeof(poll));
		}
		else {
			in = driver -> autoShift (poll, 3);
		}
		driver -> deselectController ();

		if (in != NULL) {
			if (isConfigReply (in)) {
				// We're stuck in config mode, try to get out
				exitConfigMode ();
			} else {
				// We surely have buttons
				controller.previousButtonWord = controller.buttonWord;
				controller.buttonWord = ((PsxButtons) in[4] << 8) | in[3];

				// See if we have anything more to read
				if (isDualShock2Reply (in)) {
					controller.protocol = PSPROTO_DUALSHOCK2;
				} else if (isDualShockReply (in)) {
					controller.protocol = PSPROTO_DUALSHOCK;
				} else if (isFlightstickReply (in)) {
					controller.protocol = PSPROTO_FLIGHTSTICK;
				} else if (isNegconReply (in)) {
					controller.protocol = PSPROTO_NEGCON;
				} else if (isJogconReply (in)) {
					controller.protocol = PSPROTO_JOGCON;
				} else if (isGunconReply (in)) {
					controller.protocol = PSPROTO_GUNCON;
				} else if (isMouseReply (in)) {
					controller.protocol = PSPROTO_MOUSE;
				} else if (isFishingReply (in)) {
					controller.protocol = PSPROTO_FISHING;
				} else if (isSpaceballReply (in)) {
					controller.protocol = PSPROTO_SPACEBALL;
				} else {
					controller.protocol = PSPROTO_DIGITAL;
				}

				switch (controller.protocol) {
					case PSPROTO_DUALSHOCK2:
						// We also have analog button data
						controller.analogButtonDataValid = true;
						for (int i = 0; i < PSX_ANALOG_BTN_DATA_SIZE; ++i) {
							controller.analogButtonData[i] = in[i + 9];
						}
						/* Now fall through to DualShock case, the next line
						 * avoids GCC warning
						 */
						/* FALLTHRU */
					case PSPROTO_GUNCON:
						/* The Guncon uses the same reply format as DualShocks,
						 * by just falling through we'll end up with:
						 * - A (Left side) -> Start
						 * - B (Right side) -> Cross
						 * - Trigger -> Circle
						 * - Low byte of HSYNC -> RX
						 * - High byte of HSYNC -> RY
						 * - Low byte of VSYNC -> LX
						 * - High byte of VSYNC -> LY
						 */
					case PSPROTO_DUALSHOCK:
					case PSPROTO_FLIGHTSTICK:
						// We have analog stick data
						controller.analogSticksValid = true;
						controller.rx = in[5];
						controller.ry = in[6];
						controller.lx = in[7];
						controller.ly = in[8];
						break;
					case PSPROTO_NEGCON:
						// Map the twist axis to X axis of left analog
						controller.analogSticksValid = true;
						controller.lx = in[5];

						// Map analog button data to their reasonable counterparts
						controller.analogButtonDataValid = true;
						controller.analogButtonData[PSAB_CROSS] = in[6];
						controller.analogButtonData[PSAB_SQUARE] = in[7];
						controller.analogButtonData[PSAB_L1] = in[8];

						// Make up "missing" digital data
						if (controller.analogButtonData[PSAB_SQUARE] >= NEGCON_I_II_BUTTON_THRESHOLD) {
							controller.buttonWord &= ~PSB_SQUARE;
						}
						if (controller.analogButtonData[PSAB_CROSS] >= NEGCON_I_II_BUTTON_THRESHOLD) {
							controller.buttonWord &= ~PSB_CROSS;
						}
						if (controller.analogButtonData[PSAB_L1] >= NEGCON_L_BUTTON_THRESHOLD) {
							controller.buttonWord &= ~PSB_L1;
						}
						break;
					case PSPROTO_JOGCON:
						/* Map the wheel X axis of left analog, half a rotation
						 * per direction: byte 5 has the wheel position, it is
						 * 0 at startup, then we have 0xFF down to 0x80 for
						 * left/CCW, and 0x01 up to 0x80 for right/CW
						 *
						 * byte 6 is the number of full CW rotations
						 * byte 7 is 0 if wheel is still, 1 if it is rotating CW
						 *        and 2 if rotation CCW
						 * byte 8 seems to stay at 0
						 *
						 * We'll want to cap the movement halfway in each
						 * direction, for ease of use/implementation.
						 */
						controller.analogSticksValid = true;
						if (in[6] < 0x80) {
							// CW up to half
							controller.lx = in[5] < 0x80 ? in[5] : (0x80 - 1);
						} else {
							// CCW down to half
							controller.lx = in[5] > 0x80 ? in[5] : (0x80 + 1);
						}

						// Bring to the usual 0-255 range
						controller.lx += 0x80;

						//Stores the "raw" data to use on getJogconData().
						//TODO: Reusing the analogButtonData array. Need to make a new structure to hold those values.
						controller.analogButtonData[PSAB_PAD_RIGHT] = in[5];
						controller.analogButtonData[PSAB_PAD_LEFT] = in[6];
						controller.analogButtonData[PSAB_PAD_UP] = in[7];
						break;
					case PSPROTO_FISHING:
						// We have analog stick data
						controller.analogSticksValid = true;
						
						//Stick
						controller.lx = in[7];
						controller.ly = in[8];

						// Preserve the raw payload bytes (reply bytes 5-12) for debugging.
						for (byte idx = 0; idx < sizeof(controller.protocolExtraData); ++idx) {
							controller.protocolExtraData[idx] = in[5 + idx];
						}
						controller.protocolExtraDataValid = true;

						//Motion. Not sure about values meaning
						//Stores the motion axis data to use on getFishingMotion().
						//Reusing the analogButtonData array. Need to make a new structure to hold those values.
						controller.analogButtonData[PSAB_PAD_RIGHT] = in[9];  //looks to be z gyro. horizontal rotation
						controller.analogButtonData[PSAB_PAD_LEFT] = in[10]; //Looks to be z acceleration. vertical movement up/down
						controller.analogButtonData[PSAB_PAD_UP] = in[11]; //Looks to be y gyro. vertical rotation

						//Reel. Can't differentiate CW or CCW rotation?
						controller.analogButtonData[PSAB_PAD_DOWN] = in[12];
						break;
					case PSPROTO_SPACEBALL:
						// We have 6DOF analog stick data: x, z, y, Rx, Rz, Ry
						controller.analogSticksValid = true;

						controller.lx = in[5];
						//controller.lz = in[6];
						controller.ly = in[7];
						controller.rx = in[8];
						//controller.rz = in[9];
						controller.ry = in[10];

						//Stores the extra axis data.
						//Reusing the analogButtonData array. Need to make a new structure to hold those values.
						//Currently there's no way dedicated method to read those values.
						controller.analogButtonData[PSAB_PAD_RIGHT] = in[6]; // lz
						controller.analogButtonData[PSAB_PAD_LEFT] = in[9]; // rz
						break;
					case PSPROTO_MOUSE:
						/* PSX Mouse (SCPH-1030) data format:
						 * Byte 3-4: Buttons (inverted, like other controllers)
						 *   Bit 0 = Right button, Bit 1 = Left button
						 * Byte 5: X movement (signed, positive = right)
						 * Byte 6: Y movement (signed, positive = down)
						 */
						controller.mouseDataValid = true;
						// Extract button state (un-invert for easier use)
						// PSX mouse: bit 0 = right, bit 1 = left (inverted in raw data)
						controller.mouseButtons = (~in[3]) & 0x03;
						// Extract relative movement
						controller.mouseX = (int8_t)in[5];
						controller.mouseY = (int8_t)in[6];
						break;
					default:
						// We are already done
						break;
				}
				
				ret = true;
			}
		}

		return ret;
	}

	/** \brief Poll the IR remote receiver (SCPH-10160)
	 *
	 * This function polls the PS2 IR remote receiver using address 0x61.
	 * The receiver will respond with 20-bit SIRCS IR codes when a button
	 * is pressed on a compatible remote (DVD remote, PS2 remote, etc).
	 *
	 * SIRCS 20-bit format: 8 bits device + 5 bits subdevice + 7 bits command
	 * DVD remote uses device code 0x93.
	 *
	 * \return true if an IR receiver is detected, false otherwise
	 */
	boolean detectIR () {
		driver -> selectController ();
		byte *in = driver -> autoShift (ir_detect, sizeof(ir_detect));
		driver -> deselectController ();

		if (in != NULL && isIRRemoteReply (in)) {
			controller.protocol = PSPROTO_IRREMOTE;
			return true;
		}

		return false;
	}

	/** \brief Poll IR remote receiver for button input
	 *
	 * Polls the IR receiver using standard controller poll (0x42).
	 * The receiver acts as a digital controller, mapping IR codes
	 * to controller buttons internally.
	 *
	 * \return true if the receiver responded, false otherwise
	 */
	boolean readIR () {
		boolean ret = false;

		driver -> selectController ();
		// Use ir_detect (0x04 command) for polling - returns button data in 0x12 format
		byte *in = driver -> autoShift (ir_detect, sizeof(ir_detect));
		driver -> deselectController ();

		if (in != NULL && isIRRemoteReply(in)) {
			// IR receiver responds with 0x12 format:
			// Byte 3: Low byte of button data (0xFF if no button)
			// Byte 4: High byte of button data (0xFF if no button)
			controller.protocol = PSPROTO_IRREMOTE;
			controller.buttonWord = ~(((uint16_t)in[4] << 8) | in[3]);
			ret = true;
		}

		return ret;
	}

	/** \brief Check if any button has changed state
	 *
	 * \return true if any button has changed state with regard to the previous
	 *         call to read(), false otherwise
	 */
	boolean buttonsChanged () const {
		return ((controller.previousButtonWord ^ controller.buttonWord) > 0);
	}

	/** \brief Check if a button has changed state
	 * 
	 * \return true if \a button has changed state with regard to the previous
	 *         call to read(), false otherwise
	 */
	boolean buttonChanged (const PsxButtons button) const {
		return (((controller.previousButtonWord ^ controller.buttonWord) & button) > 0);
	}

	/** \brief Check if a button is currently pressed
	 * 
	 * \param[in] button The button to be checked
	 * \return true if \a button was pressed in last call to read(), false
	 *         otherwise
	 */
	boolean buttonPressed (const PsxButton button) const {
		return buttonPressed (~controller.buttonWord, button);
	}

	/** \brief Check if a button is pressed in a Button Word
	 * 
	 * \param[in] buttons The button word to check in
	 * \param[in] button The button to be checked
	 * \return true if \a button is pressed in \a buttons, false otherwise
	 */
	boolean buttonPressed (const PsxButtons buttons, const PsxButton button) const {
		return ((buttons & static_cast<const PsxButtons> (button)) > 0);
	}

	/** \brief Check if a button has just been pressed
	 * 
	 * \param[in] button The button to be checked
	 * \return true if \a button was not pressed in the previous call to read()
	 *         and is now, false otherwise
	 */
	boolean buttonJustPressed (const PsxButton button) const {
		return (buttonChanged (button) & buttonPressed (button));
	}

	/** \brief Check if a button has just been released
	 * 
	 * \param[in] button The button to be checked
	 * \return true if \a button was pressed in the previous call to read() and
	 *         is not now, false otherwise
	 */
	boolean buttonJustReleased (const PsxButton button) const {
		return (buttonChanged (button) & ((~controller.previousButtonWord & button) > 0));
	}

	/** \brief Check if NO button is pressed in a Button Word
	 * 
	 * \param[in] buttons The button word to check in
	 * \return true if all buttons in \a buttons are released, false otherwise
	 */
	boolean noButtonPressed (const PsxButtons buttons) const {
		return buttons == PSB_NONE;
	}

	/** \brief Check if NO button is currently pressed
	 * 
	 * \return true if all buttons were released in the last call to read(),
	 *         false otherwise
	 */
	boolean noButtonPressed (void) const {
		return controller.buttonWord == (uint16_t) ~((uint16_t) PSB_NONE);
	}
	
	/** \brief Retrieve the <em>Button Word</em>
	 * 
	 * The button word contains the status of all digital buttons and can be
	 * retrieved so that it can be inspected later.
	 * 
	 * \sa buttonPressed
	 * \sa noButtonPressed
	 * 
	 * \return the Button Word
	 */
	PsxButtons getButtonWord () const {
		return ~controller.buttonWord;
	}

	/** \brief Retrieve button pressure depth/strength
	 * 
	 * This function will return how deeply/strongly a button is pressed. It
	 * will only work on DualShock 2 controllers after enabling this feature
	 * with enableAnalogButtons().
	 * 
	 * Note that button pressure depth/strength is only available for the D-Pad
	 * buttons, []/^/O/X, L1/2 and R1/2.
	 *
	 * \param[in] button the button the retrieve the pressure depth/strength of
	 * \return the pressure depth/strength [0-255, Fully released to fully
	 *         pressed]
	 */
	byte getAnalogButton (const PsxAnalogButton button) const {
		byte ret = 0;
		
		if (controller.analogButtonDataValid) {
			ret = controller.analogButtonData[button];
		//~ } else if (buttonPressed (button)) {		// FIXME
			//~ // No analog data, assume fully pressed or fully released
			//~ ret = 0xFF;
		}

		return ret;
	}

	/** \brief Retrieve all analog button data
	 */
	const byte* getAnalogButtonData () const {
		return controller.analogButtonDataValid ? controller.analogButtonData : NULL;
	}

	/** \brief Retrieve position of the \a left analog stick
	 * 
	 * This function will return the absolute position of the left analog stick.
	 * 
	 * Note that not all controllers have analog sticks, in which case this
	 * function will return false.
	 * 
	 * \param[in] x A variable where the horizontal position will be stored
	 *              [0-255, L to R]
	 * \param[in] y A variable where the vertical position will be stored
	 *              [0-255, U to D]
	 * \return true if the returned position is valid, false otherwise
	 */
	boolean getLeftAnalog (byte& x, byte& y) const {
		x = controller.lx;
		y = controller.ly;

		return controller.analogSticksValid;
	}

	/** \brief Retrieve position of the \a right analog stick
	 * 
	 * This function will return the absolute position of the right analog
	 * stick.
	 * 
	 * Note that not all controllers have analog sticks, in which case this
	 * function will return false.
	 * 
	 * \param[in] x A variable where the horizontal position will be stored
	 *              [0-255, L to R]
	 * \param[in] y A variable where the vertical position will be stored
	 *              [0-255, U to D]
	 * \return true if the returned position is valid, false otherwise
	 */
	boolean getRightAnalog (byte& x, byte& y) {
		x = controller.rx;
		y = controller.ry;

		return controller.analogSticksValid;
	}

	/** \brief Retrieve Guncon X/Y readings
	 *
	 * According to the Nocash PSX Specifications, the Guncon returns 16-bit X/Y
	 * coordinates of the screen it is aimed at.
	 *
	 * The coordinates are updated in all frames. The absolute min/max may vary
	 * from TV set to TV set.
	 *
	 * Vertical coordinates are counted in scanlines (ie. equal to pixels).
	 * Horizontal coordinates are counted in 8MHz units (which would equal a
	 * resolution of 385 pixels; which can be, for example, converted to 320
	 * pixel resolution as X=X*320/385).
	 *
	 * <em>Caution:</em> The gun only returns meaningful data when read shortly
	 * after begin of VBLANK (ie. AFTER rendering, but still BEFORE vsync), so
	 * make sure to only consider readings returning \a GUNCON_OK;
	 *
	 * \sa GunconStatus
	 */
	GunconStatus getGunconCoordinates (word& x, word& y) const {
		GunconStatus status = GUNCON_OTHER_ERROR;

		if (controller.protocol == PSPROTO_GUNCON && controller.analogSticksValid) {
			status = GUNCON_OK;
			
			x = (((word) controller.ry) << 8) | controller.rx;
			y = (((word) controller.ly) << 8) | controller.lx;

			if (x == 0x0001) {
				if (y == 0x0005) {
					status = GUNCON_UNEXPECTED_LIGHT;
				} else if (y == 0x000A) {
					status = GUNCON_NO_LIGHT;
				}
			}
		}

		return status;
	}


	/** \brief Set Jogcon direction, command and motor power
	 * 
	 * Data will be combined into a single byte as ddccffff, where
	 * dd   = direction (2 bits)
	 * cc   = command (2 bits)
	 * ffff = force (4 bits)
	 *
	 * \param[in] direction The direction for motor rotation
	 * \param[in] command The Command to be sent
	 * \param[in] motorPower The amount of motor power. Max 15 (0x0F).
	 */
	void setJogconMotorMode (JogconDirection direction, JogconCommand command, const uint8_t motorPower) {
		if((byte)direction > 0x3)
			direction = JOGCON_DIR_NONE;

		if((byte)command == JOGCON_CMD_OTHER)
			command = JOGCON_CMD_NONE;

		controller.jogconMotorLevelAndMode = ((byte)direction << 4) | (command | (motorPower & 0x0F));
	}


	/** \brief Retrieve Jogcon state and raw readings
	 * 
	 * \param[in] position A variable where the jog position will be stored
	 * \param[in] revolutions A variable where the jog revolutions will be stored
	 * \param[in] direction A variable where the last direction will be stored
	 * \param[in] cmdResult A variable where the last command sent will be stored
	 * 
	 * \return true if the device is Jogcon and data is valid
	 */
	bool getJogconData (uint8_t& position, uint8_t& revolutions, JogconDirection& direction, JogconCommand& cmdResult) const {
		if (controller.protocol == PSPROTO_JOGCON && controller.analogSticksValid) {
			position = controller.analogButtonData[PSAB_PAD_RIGHT];
			revolutions = controller.analogButtonData[PSAB_PAD_LEFT];
			//state = static_cast<JogconRotation>(analogButtonData[PSAB_PAD_UP]);

			//State byte contains two nibbles with data.
			//Rotation state and command result
			
			//Last rotation direction
			switch (controller.analogButtonData[PSAB_PAD_UP] & 0x0F) {
			case 0x0:
				direction = JOGCON_DIR_NONE;
				break;
			case 0x1:
				direction = JOGCON_DIR_CW;
				break;
			case 0x2:
				direction = JOGCON_DIR_CCW;
				break;
			case 0x4:
				direction = JOGCON_DIR_MAX; //Max value reached (overflow)
				break;
			default:
				direction = JOGCON_DIR_OTHER; //Other - unhandled
				break;
			}

			//Last command result
			switch (controller.analogButtonData[PSAB_PAD_UP] & 0xF0) {
			case 0x00:
				cmdResult = JOGCON_CMD_NONE;
				break;
			case 0x80:
				cmdResult = JOGCON_CMD_DROP_REVOLUTIONS;
				break;
			case 0xC0:
				cmdResult = JOGCON_CMD_NEW_START;
				break;
			default:
				cmdResult = JOGCON_CMD_OTHER; //Other - unhandled
				break;
			}
			return true;
		}

		return false;
	}

	boolean getFishingMotion (byte& reel, byte& gyroZ, byte& gyroY, byte& accZ) const {
		if (controller.protocol == PSPROTO_FISHING && controller.analogSticksValid) {
			reel = controller.analogButtonData[PSAB_PAD_DOWN];
			gyroZ = controller.analogButtonData[PSAB_PAD_RIGHT];
			accZ = controller.analogButtonData[PSAB_PAD_LEFT];
			gyroY = controller.analogButtonData[PSAB_PAD_UP];
			return true;
		}
		return false;
	}

	boolean getFishingRawData (byte *data, byte& len) const {
		len = 0;
		if (controller.protocol == PSPROTO_FISHING && controller.protocolExtraDataValid) {
			if (data != NULL) {
				memcpy(data, controller.protocolExtraData, sizeof(controller.protocolExtraData));
			}
			len = sizeof(controller.protocolExtraData);
			return true;
		}
		return false;
	}

	/** \brief Retrieve Mouse data
	 *
	 * Returns the relative X/Y movement and button state from the PSX Mouse.
	 *
	 * \param[out] x Relative X movement (signed, positive = right)
	 * \param[out] y Relative Y movement (signed, positive = down)
	 * \param[out] buttons Button state (bit 0 = right, bit 1 = left after un-inversion)
	 * \return true if the device is a mouse and data is valid
	 */
	boolean getMouseData (int8_t& x, int8_t& y, uint8_t& buttons) const {
		if (controller.protocol == PSPROTO_MOUSE && controller.mouseDataValid) {
			x = controller.mouseX;
			y = controller.mouseY;
			buttons = controller.mouseButtons;
			return true;
		}
		return false;
	}

	/** \brief Retrieve IR Remote data
	 *
	 * Returns the IR command received from a PS2-compatible remote.
	 *
	 * \param[out] command 7-bit IR command code
	 * \param[out] pressed true if a button is currently being pressed
	 * \return true if the device is an IR receiver and data is valid
	 */
	boolean getIRData (uint8_t& command, boolean& pressed) const {
		if (controller.protocol == PSPROTO_IRREMOTE) {
			command = controller.irCommand;
			pressed = controller.irButtonPressed;
			return controller.irDataValid || !pressed;  // Valid if we have data or explicitly no press
		}
		return false;
	}

	/** \brief Retrieve full IR Remote data
	 *
	 * Returns the complete IR data including device and sub-device codes.
	 *
	 * \param[out] command 7-bit IR command code
	 * \param[out] device 8-bit device code (0x93 for DVD remote)
	 * \param[out] subDevice 5-bit sub-device code
	 * \param[out] pressed true if a button is currently being pressed
	 * \return true if the device is an IR receiver and data is valid
	 */
	boolean getIRDataFull (uint8_t& command, uint8_t& device, uint8_t& subDevice, boolean& pressed) const {
		if (controller.protocol == PSPROTO_IRREMOTE && controller.irDataValid) {
			command = controller.irCommand;
			device = controller.irDevice;
			subDevice = controller.irSubDevice;
			pressed = controller.irButtonPressed;
			return true;
		}
		return false;
	}

	//! @}		// Polling Functions
};
