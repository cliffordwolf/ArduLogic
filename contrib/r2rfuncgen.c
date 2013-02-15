/*
 *  A simple ATMega328 (Arduino) Firmware for generating a 440 Hz sine
 *  wave using an external R-2R network connected to B0..B5 (Arduino
 *  pins 8..13).
 *
 *  Copyright (C) 2013  Clifford Wolf <clifford@clifford.at>
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
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <math.h>

// perl -e 'for $i (0..270) { printf("%d, ", 40+20*sin($i*2*3.14159/271)); }'
uint8_t samples[] = {
40, 40, 40, 41, 41, 42, 42, 43, 43, 43, 44, 44, 45, 45, 45, 46, 46, 47, 47, 48,
48, 48, 49, 49, 49, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 53, 54, 54, 54, 54,
55, 55, 55, 56, 56, 56, 56, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 58, 59, 59,
59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59,
59, 59, 59, 59, 59, 59, 59, 59, 58, 58, 58, 58, 58, 58, 57, 57, 57, 57, 57, 56,
56, 56, 56, 55, 55, 55, 54, 54, 54, 54, 53, 53, 53, 52, 52, 52, 51, 51, 51, 50,
50, 49, 49, 49, 48, 48, 48, 47, 47, 46, 46, 45, 45, 45, 44, 44, 43, 43, 43, 42,
42, 41, 41, 40, 40, 40, 39, 39, 38, 38, 37, 37, 36, 36, 36, 35, 35, 34, 34, 34,
33, 33, 32, 32, 31, 31, 31, 30, 30, 30, 29, 29, 28, 28, 28, 27, 27, 27, 26, 26,
26, 25, 25, 25, 25, 24, 24, 24, 23, 23, 23, 23, 22, 22, 22, 22, 22, 21, 21, 21,
21, 21, 21, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 22,
22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 27, 27,
27, 28, 28, 28, 29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 33, 33, 34, 34, 34, 35,
35, 36, 36, 36, 37, 37, 38, 38, 39, 39 };

int main()
{
	DDRB = 0x3f;

	uint16_t t = 0;
	while (1) {
		while (t >= sizeof(samples))
			t -= sizeof(samples);
		PORTB = samples[t++];
		_delay_us(7);
	}

	return 0;
}

