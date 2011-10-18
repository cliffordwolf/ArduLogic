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

static void gen_pack(FILE *f)
{
	uint8_t ashift[32] = { };
	uint8_t dshift[32] = { };
	int idx = 0;

	for (int i = PIN_A(0); i < PIN_D(2); i++) {
		if ((pins[i] & PIN_MONITOR) == 0)
			continue;
		int off = 16 + (idx++) - (i - PIN_A(0));
		ashift[off] |= 1 << i;
	}
	for (int i = PIN_D(2); i < TOTAL_PIN_NUM; i++) {
		if ((pins[i] & PIN_MONITOR) == 0)
			continue;
		int off = 16 + (idx++) - (i - PIN_D(2) + 2);
		dshift[off] |= 1 << (i - PIN_D(2) + 2);
	}

	fprintf(f, "static inline smplword_t pack(uint8_t adata, uint8_t ddata) {\n");
	fprintf(f, "	smplword_t w = 0;\n");
	for (int i = 0; i < 32; i++) {
		if (ashift[i])
			fprintf(f, "	w |= (adata & 0x%02x) %s %d;\n",
					ashift[i], i < 16 ? ">>" : "<<", abs(i - 16));
		if (dshift[i])
			fprintf(f, "	w |= (ddata & 0x%02x) %s %d;\n",
					dshift[i], i < 16 ? ">>" : "<<", abs(i - 16));
	}
	fprintf(f, "	return w;\n");
	fprintf(f, "}\n");
}

void genfirmware(const char *file)
{
	int use_irq_trigger = 1;
	int num_bits = 0;

	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
		if ((pins[i] & PIN_MONITOR) != 0)
			num_bits++;
		if (i == PIN_D(2) || i == PIN_D(3))
			continue;
		if ((pins[i] & (PIN_TRIGGER_POSEDGE|PIN_TRIGGER_NEGEDGE)) != 0)
			use_irq_trigger = 0;
	}

	if (!use_irq_trigger)
		printf("WARNING: IRQs are only used when all triggers are on D2 and/or D3!\n");

	FILE *f = fopen(file, "w");
	if (!f) {
		fprintf(stderr, "Can't create firmware source file `%s': %s\n", file, strerror(errno));
		exit(1);
	}

	fprintf(f, "#include <stdint.h>\n");
	fprintf(f, "typedef uint%d_t smplword_t;\n", num_bits <= 8 ? 8 : 16);

	gen_pack(f);

	// FIXME
	fprintf(f, "int main() { return 0; }\n");

	fclose(f);
}

