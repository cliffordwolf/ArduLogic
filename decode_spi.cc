/*
 *  ArduLogic - Low Speed Logic Analyzer using the Arduino Hardware
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
 */

#include "ardulogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static bool last_cs;
static uint8_t bitcount;
static uint8_t wordcount;

static void decoder_spi_vcd_defs(FILE *f)
{
	fprintf(f, "$var reg 8 %s1 %sMOSI_DATA $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg 8 %s2 %sMISO_DATA $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg 8 %sb %sBITCOUNT $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg 8 %sw %sWORDCOUNT $end\n", vcd_prefix, vcd_prefix);
}

static void decoder_spi_vcd_init(FILE *f)
{
	fprintf(f, " bzzzzzzzz 1");
	fprintf(f, " bzzzzzzzz 2");
	fprintf(f, " bzzzzzzzz b");
	fprintf(f, " bzzzzzzzz w");
	last_cs = false;
}

static void printbyte(FILE *f, uint8_t data, const char *name)
{
	fprintf(f, " b");
	for (int i = 0; i < 8; i++)
		fprintf(f, "%d", (data & (0x80 >> i)) != 0);
	fprintf(f, " %s", name);
}

static void decoder_spi_vcd_step(FILE *f, size_t i)
{
	bool cs = (samples[i] & (1 << decode_config[CFG_SPI_CS])) != 0;
	if (decode_config[CFG_SPI_CSNEG] != 0)
		cs = !cs;
	if (cs == true && last_cs == false) {
#if 0
		fprintf(f, " bxxxxxxxx 1");
		fprintf(f, " bxxxxxxxx 2");
		fprintf(f, " bxxxxxxxx b");
		fprintf(f, " bxxxxxxxx w");
#endif
		last_cs = true;
		wordcount = 0;
		bitcount = 0;
	}
	else if (cs == false && last_cs == true) {
		fprintf(f, " bzzzzzzzz 1");
		fprintf(f, " bzzzzzzzz 2");
		fprintf(f, " bzzzzzzzz b");
		fprintf(f, " bzzzzzzzz w");
		last_cs = false;
		bitcount = 0;
	}
	else {
		if (bitcount == 0) {
			uint8_t mosi_byte = 0, miso_byte = 0;
			for (int j = 0; j < 8; j++) {
				bool mosi = (samples[i+j] & (1 << decode_config[CFG_SPI_MOSI])) != 0;
				bool miso = (samples[i+j] & (1 << decode_config[CFG_SPI_MISO])) != 0;
				int bitidx = decode_config[CFG_SPI_MSB] ? 8-j : j;
				mosi_byte |= mosi << bitidx;
				miso_byte |= miso << bitidx;
			}
			printbyte(f, mosi_byte, "1");
			printbyte(f, miso_byte, "2");
			printbyte(f, wordcount, "w");
		}
		printbyte(f, bitcount, "b");
		if (++bitcount == 8) {
			bitcount = 0;
			wordcount++;
		}
	}
}

struct decoder_desc decoder_spi = {
	&decoder_spi_vcd_defs,
	&decoder_spi_vcd_init,
	&decoder_spi_vcd_step
};

