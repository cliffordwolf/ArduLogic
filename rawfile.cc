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

void writerawfile(const char *file)
{
	FILE *f = fopen(file, "w");

	if (f == NULL) {
		fprintf(stderr, "Can't open VCD file `%s' for writing: %s\n", file, strerror(errno));
		exit(1);
	}

	for (size_t i = 0; i < samples.size(); i++) {
		fprintf(f, "%04x\n", samples[i]);
	}

	fclose(f);
}

void readrawfile(const char *file)
{
	FILE *f = fopen(file, "r");

	if (f == NULL) {
		fprintf(stderr, "Can't open VCD file `%s' for reading: %s\n", file, strerror(errno));
		exit(1);
	}

	unsigned int sample;
	while (fscanf(f, "%04x\n", &sample) == 1)
		samples.push_back(sample);

	fclose(f);
}

