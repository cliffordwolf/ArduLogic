/*
 *  A simple ATMega328 (Arduino) Firmware for pulse width triggering
 *  using the external trigger input on an oscilloscope.
 *
 *  Copyright (C) 2011  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  Usage:
 *  - Configure reference high and low periods in us using the _US defines
 *  - Connect the signal to be monitored to Arduino PIN 2 (PIND2)
 *  - Connect the ext trigger of your oscilloscope to Arduino PIN 12 (PINB4)
 *  - Build and upload to arduino: "make ext_pulsewidth.prog"
 *
 *  The LED (Arduino PIN 13) turns on with the first trigger
 *  On startup the LED blinks for 500ms and reference pulses are generated
 */

#define MIN_HIGH_US 400
#define MIN_LOW_US 400

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

volatile bool seen_high_state;
volatile bool seen_low_state;
ISR(INT0_vect)
{
	if ((PIND & 0x04) != 0)
		seen_high_state = true;
	else
		seen_low_state = true;
}

int main()
{
	DDRB |= 0x30;
	PORTB &= ~0x30;

	// blink on startup
	PINB = 0x20;
	_delay_ms(500);
	PINB = 0x20;

	// ref high pulse width
	PORTB |= 0x30;
	_delay_us(MIN_HIGH_US);
	PORTB &= ~0x30;

	_delay_us((MIN_HIGH_US + MIN_LOW_US) / 4);

	// ref low pulse width
	PORTB |= 0x30;
	_delay_us(MIN_LOW_US);
	PORTB &= ~0x30;

	// configure interrupt
	EICRA |= 0x01;
	EIMSK |= 0x01;
	sei();

	// wait for everything to settle
	_delay_ms(1);
	seen_high_state = false;
	seen_low_state = false;

	// do the magic
	while (1)
	{
		if (seen_high_state) {
			seen_high_state = 0;
			_delay_us(MIN_HIGH_US);
			if (seen_high_state || seen_low_state)
				goto trigger;
		}
		if (seen_low_state) {
			seen_low_state = 0;
			_delay_us(MIN_LOW_US);
			if (seen_high_state || seen_low_state)
				goto trigger;
		}
		continue;

	trigger:
		PORTB |= 0x30;
		_delay_ms(100);
		PORTB &= ~0x10;
		seen_high_state = false;
		seen_low_state = false;
	}

	return 0;
}

