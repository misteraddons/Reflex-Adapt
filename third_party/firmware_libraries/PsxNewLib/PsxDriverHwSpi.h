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
 * \file PsxDriverHwSpi.h
 * \author SukkoPera <software@sukkology.net>
 * \date 22 Mar 2021
 * \brief Playstation Controller Hardware SPI Driver
 * 
 * Please refer to the GitHub page and wiki for any information:
 * https://github.com/SukkoPera/PsxNewLib
 */

#include "PsxDriver.h"
#include <SPI.h>
// #include <DigitalIO.h>


//template <uint8_t PIN_ATT>
class PsxDriverHwSpi: public PsxDriver {
private:
	// DigitalPin<PIN_ATT> att;
	// DigitalPin<MOSI> cmd;
	// DigitalPin<MISO> dat;
	// DigitalPin<SCK> clk;
	const uint8_t att;
	const uint8_t ack;
	const uint8_t cmd; // mosi
	const uint8_t dat; // miso
	const uint8_t clk;

	unsigned long byteFinishTime;

	// Set up the speed, data order and data mode
	static const SPISettings spiSettings;

    const uint8_t spi;

protected:
	virtual byte shiftInOut (const byte out) override {
		byte b = spi ? SPI1.transfer (out) : SPI.transfer (out);
		byteFinishTime = micros ();

		return b;
	}

public:
	PsxDriverHwSpi (uint8_t SPI_NUMBER, uint8_t PIN_ATT, uint8_t PIN_ACK, uint8_t PIN_CMD, uint8_t PIN_DAT, uint8_t PIN_CLK)
		: att{PIN_ATT}, ack{PIN_ACK}, cmd{PIN_CMD}, dat{PIN_DAT}, clk{PIN_CLK}, spi{SPI_NUMBER}
	{ }
	virtual void attention () override {
		digitalWrite(att, LOW);//att.low ();

		spi ? SPI1.beginTransaction (spiSettings) : SPI.beginTransaction (spiSettings);
	}
	
	virtual void noAttention () override {
		spi ? SPI1.endTransaction () : SPI.endTransaction ();

		// Make sure CMD and CLK sit high
		// cmd.high ();    // This actually does nothing as pin stays under SPI control, I guess
		// clk.high ();    // Ditto
		digitalWrite(att, HIGH);//att.high ();
	}

	virtual boolean acknowledged () override {
		// We just wait a bit, hoping the ACK goes by...
		return micros () - byteFinishTime > INTER_CMD_BYTE_DELAY;
	}
	
	virtual boolean begin () override {
		pinMode(att, OUTPUT); digitalWrite(att, HIGH);//att.config (OUTPUT, HIGH);    // HIGH -> Controller not selected

		/* We need to force these at startup, that's why we need to know which
		 * pins are used for HW SPI. It's a sort of "start condition" the
		 * controller needs.
		 */
		// cmd.config (OUTPUT, HIGH);
		// clk.config (OUTPUT, HIGH);
		// dat.config (INPUT, HIGH);     // Enable pull-up

        pinMode(ack, INPUT);

		spi ? SPI1.setMISO(dat) : SPI.setMISO(dat); // setRX
		//spi ? SPI.setCS(pin_size_t pin) : SPI.setCS(pin_size_t pin);
		spi ? SPI1.setSCK(clk) : SPI.setSCK(clk);
		spi ? SPI1.setMOSI(cmd) : SPI.setMOSI(cmd); // setTX

		spi ? SPI1.begin (false) : SPI.begin (false);

		spi ? SPI1.begin (false) : SPI.begin (false);

		return PsxDriver::begin ();
	}
};

// Init static data members
//template <uint8_t PIN_ATT>
const SPISettings PsxDriverHwSpi::spiSettings (250000, LSBFIRST, SPI_MODE3);
