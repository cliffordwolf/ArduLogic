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

void genfirmware(const char *file)
{
	int use_irq_trigger = 1;

	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
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

	// FIXME
	fprintf(f, "int main() { return 0; }\n");

	fclose(f);
}

