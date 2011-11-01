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

struct tap_state_desc {
	char name[12];
	int next[2];
};

static struct tap_state_desc tap_states[] = {
	/*  0 */ { "RESET",      {  1,  0 } },
	/*  1 */ { "IDLE",       {  1,  2 } },
	/*  2 */ { "SELECT DR",  {  3,  9 } },
	/*  3 */ { "CAPTURE DR", {  4,  5 } },
	/*  4 */ { "SHIFT DR",   {  4,  5 } },
	/*  5 */ { "EXIT1 DR",   {  6,  8 } },
	/*  6 */ { "PAUSE DR",   {  6,  7 } },
	/*  7 */ { "EXIT2 DR",   {  4,  8 } },
	/*  8 */ { "UPDATE DR",  {  1,  2 } },
	/*  9 */ { "SELECT IR",  { 10,  0 } },
	/* 10 */ { "CAPTURE IR", { 11, 12 } },
	/* 11 */ { "SHIFT IR",   { 11, 12 } },
	/* 12 */ { "EXIT1 IR",   { 13, 15 } },
	/* 13 */ { "PAUSE IR",   { 13, 14 } },
	/* 14 */ { "EXIT2 IR",   { 11, 15 } },
	/* 15 */ { "UPDATE IR",  {  1,  2 } },
	/* 16 */ { "UNKNOWN 1",  { 16, 17 } },
	/* 17 */ { "UNKNOWN 2",  { 16, 18 } },
	/* 18 */ { "UNKNOWN 3",  { 16, 19 } },
	/* 19 */ { "UNKNOWN 4",  { 16, 20 } },
	/* 20 */ { "UNKNOWN 5",  { 16,  0 } }
};

static int state_idx;

static void decoder_jtag_vcd_defs(FILE *f)
{
	state_idx = 16;
	fprintf(f, "$var reg 8 %sn %sTAPID $end\n", vcd_prefix, vcd_prefix);
	fprintf(f, "$var reg %d %st %sTAP $end\n", 12*8, vcd_prefix, vcd_prefix);
}

static void bytef(FILE *f, uint8_t byte)
{
	for (int i = 7; i >= 0; i--)
		fprintf(f, "%d", (byte & (1 << i)) != 0);
}

static void decoder_jtag_vcd_init(FILE *f)
{
	fprintf(f, " b");
	bytef(f, state_idx);
	fprintf(f, " %sn b", vcd_prefix);
	for (int i = 0; i < 12; i++)
		bytef(f, tap_states[state_idx].name[i]);
	fprintf(f, " %st", vcd_prefix);
}

static void decoder_jtag_vcd_step(FILE *f, size_t i)
{
	int tms = (samples[i-1] & (1 << decode_config[CFG_JTAG_TMS])) != 0;
	state_idx = tap_states[state_idx].next[tms];
	decoder_jtag_vcd_init(f);
}

struct decoder_desc decoder_jtag = {
	&decoder_jtag_vcd_defs,
	&decoder_jtag_vcd_init,
	&decoder_jtag_vcd_step
};

