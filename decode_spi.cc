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
	for (int i = 0; i < TOTAL_PIN_NUM; i++)
	{
		if (i == decode_config[CFG_SPI_CS])
			continue;
		if ((pins[i] & PIN_CAPTURE) == 0)
			continue;
		fprintf(f, "$var reg 8 %sd%d %s%s_DATA $end\n",
				vcd_prefix, i, vcd_prefix, pin_names[i]);
	}
	fprintf(f, "$var reg 8 %sb %sBITCOUNT $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg 8 %sw %sWORDCOUNT $end\n", vcd_prefix, vcd_prefix);
}

static void decoder_spi_vcd_init(FILE *f)
{
	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
		if (i == decode_config[CFG_SPI_CS])
			continue;
		if ((pins[i] & PIN_CAPTURE) == 0)
			continue;
		fprintf(f, " bzzzzzzzz %sd%d", vcd_prefix, i);
	}
	fprintf(f, " bzzzzzzzz %sb", vcd_prefix);
	fprintf(f, " bzzzzzzzz %sw", vcd_prefix);
	last_cs = false;
}

static void printbyte(FILE *f, uint8_t data, const char *name)
{
	fprintf(f, " b");
	for (int i = 0; i < 8; i++)
		fprintf(f, "%d", (data & (0x80 >> i)) != 0);
	fprintf(f, " %s%s", vcd_prefix, name);
}

static void decoder_spi_vcd_step(FILE *f, size_t i)
{
	bool cs = (samples[i] & (1 << decode_config[CFG_SPI_CS])) != 0;
	if (decode_config[CFG_SPI_CSNEG] != 0)
		cs = !cs;
	if (cs == true && last_cs == false) {
#if 0
		for (int j = 0; j < TOTAL_PIN_NUM; j++) {
			if (j == decode_config[CFG_SPI_CS])
				continue;
			if ((pins[j] & PIN_CAPTURE) == 0)
				continue;
			fprintf(f, " bxxxxxxxx %sd%d", vcd_prefix, j);
		}
		fprintf(f, " bxxxxxxxx %sb", vcd_prefix);
		fprintf(f, " bxxxxxxxx %sw", vcd_prefix);
#endif
		last_cs = true;
		wordcount = 0;
		bitcount = 0;
	}
	else if (cs == false && last_cs == true) {
		for (int j = 0; j < TOTAL_PIN_NUM; j++) {
			if (j == decode_config[CFG_SPI_CS])
				continue;
			if ((pins[j] & PIN_CAPTURE) == 0)
				continue;
			fprintf(f, " bzzzzzzzz %sd%d", vcd_prefix, j);
		}
		fprintf(f, " bzzzzzzzz %sb", vcd_prefix);
		fprintf(f, " bzzzzzzzz %sw", vcd_prefix);
		last_cs = false;
		bitcount = 0;
	}
	else {
		if (bitcount == 0) {
			for (int j = 0; j < TOTAL_PIN_NUM; j++) {
				if (j == decode_config[CFG_SPI_CS])
					continue;
				if ((pins[j] & PIN_CAPTURE) == 0)
					continue;
				uint8_t byte = 0;
				for (int k = 0; k < 8; k++) {
					bool bit = (samples[i+k] & (1 << j)) != 0;
					int bitidx = decode_config[CFG_SPI_MSB] ? 8-k : k;
					byte |= bit << bitidx;
				}
				char name[5];
				snprintf(name, 5, "d%d", j);
				printbyte(f, byte, name);
			}
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

