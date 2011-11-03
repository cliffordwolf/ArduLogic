/*
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
 */

#include <stdio.h>
#include <stdint.h>

// decode spi posedge lsb D2 !D3 D4 D5
#define CS_BIT 7
#define MOSI_BIT 8
#define MISO_BIT 9

void print_byte(uint8_t mosi, uint8_t miso)
{
	for (int i = 0; i < 8; i++) {
		int mosi_bit = (mosi & (1<<i)) != 0;
		int miso_bit = (miso & (1<<i)) != 0;
		printf("%04x\n", (mosi_bit << MOSI_BIT) | (miso_bit << MISO_BIT));
	}
}

void print_cs(bool cs)
{
	printf("%04x\n", ((!cs) << CS_BIT));
}

int main()
{
	print_cs(false);
	print_cs(true);
	print_byte(0xaa, 0x00);
	print_byte(0x00, 0xaa);
	print_cs(false);
	print_cs(true);
	print_byte(0x10, 0x20);
	print_byte(0x11, 0x21);
	print_byte(0x12, 0x22);
	print_cs(false);
	print_cs(true);
	print_byte(0xf0, 0x00);
	print_byte(0xf1, 0xa0);
	print_byte(0xf2, 0xa1);
	print_byte(0xf3, 0xa2);
	print_cs(false);
	return 0;
}

