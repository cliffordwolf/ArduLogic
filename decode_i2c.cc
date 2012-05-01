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

static uint8_t bitstate;
static uint8_t bitcount;
static uint8_t wordcount;
static size_t proc_ptr;

static void decoder_i2c_vcd_defs(FILE *f)
{
	fprintf(f, "$var reg 8 %ss %sSTATE $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg 8 %sd %sDATA $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg 8 %sb %sBITCOUNT $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg 8 %sw %sWORDCOUNT $end\n", vcd_prefix, vcd_prefix);
}

static void decoder_i2c_vcd_init(FILE *f)
{
	fprintf(f, " bzzzzzzzz %ss", vcd_prefix);
	fprintf(f, " bzzzzzzzz %sd", vcd_prefix);
	fprintf(f, " bzzzzzzzz %sb", vcd_prefix);
	fprintf(f, " bzzzzzzzz %sw", vcd_prefix);
	bitstate = 0;
	bitcount = 0;
	wordcount = 0;
	proc_ptr = 0;
}

static void bytef(FILE *f, uint8_t byte)
{
	for (int i = 7; i >= 0; i--)
		fprintf(f, "%d", (byte & (1 << i)) != 0);
}

static bool get_scl(size_t idx)
{
	return (samples[idx] & (1 << decode_config[CFG_I2C_SCL])) != 0;
}

static bool get_sda(size_t idx)
{
	return (samples[idx] & (1 << decode_config[CFG_I2C_SDA])) != 0;
}

static void decoder_i2c_vcd_step(FILE *f, size_t i)
{
	if (i < proc_ptr)
		return;

	bool scl = get_scl(i);
	bool sda = get_sda(i);

	if (scl == true)
	{
		bool sda_high = false, sda_low = false;
		for (proc_ptr = i; proc_ptr < samples.size() && get_scl(proc_ptr); proc_ptr++)
		{
			if (get_sda(proc_ptr))
				sda_high = true;
			else
				sda_low = true;

			// start/restart condition
			if (sda && sda_high && sda_low) {
				fprintf(f, " b");
				bytef(f, 'S');
				fprintf(f, " %ss", vcd_prefix);
				bitstate = 0;
				return;
			}

			// stop condition
			if (!sda && sda_high && sda_low) {
				fprintf(f, " b");
				bytef(f, 'P');
				fprintf(f, " %ss", vcd_prefix);
				return;
			}
		}

		if (i == 0 || get_scl(i-1))
			return;

		while (proc_ptr+1 < samples.size() && get_scl(proc_ptr+1) == false)
			proc_ptr++;

		if (bitstate % 9 == 0)
		{
			uint8_t data = 0;
			for (size_t j = i, k = 0; k < 8 && j < samples.size(); j++)
				if (j > 0 && get_scl(j-1) == false && get_scl(j) == true)
					data = (data << 1) | get_sda(j), k++;

			fprintf(f, " b");
			bytef(f, data);
			fprintf(f, " %sd", vcd_prefix);

			fprintf(f, " b");
			bytef(f, wordcount++);
			fprintf(f, " %sw", vcd_prefix);
			bitcount = 0;
		}

		if (bitstate % 9 != 8)
		{
			fprintf(f, " b");
			bytef(f, bitcount++);
			fprintf(f, " %sb", vcd_prefix);
		}

		// Just a data bit
		fprintf(f, " b");
		if (bitstate < 7)
			bytef(f, 'A');
		else if (bitstate == 7)
			bytef(f, 'R');
		else if (bitstate % 9 == 8)
			bytef(f, 'C');
		else
			bytef(f, 'D');
		fprintf(f, " %ss", vcd_prefix);
		bitstate++;
	}
}

struct decoder_desc decoder_i2c = {
	&decoder_i2c_vcd_defs,
	&decoder_i2c_vcd_init,
	&decoder_i2c_vcd_step
};

