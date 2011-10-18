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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int decode;
int decode_config[CFG_WORDS];
int pins[TOTAL_PIN_NUM];
const char *pin_names[TOTAL_PIN_NUM] = {
	"A0", "A1", "A2", "A3", "A4", "A5",
	"D2", "D3", "D4", "D5", "D6", "D7" };
std::list<uint16_t> samples;

void help(const char *progname)
{
	fprintf(stderr, "Usage: %s [-p] [-t <dev>] <configfile> <vcdfile>\n", progname);
	exit(1);
}

int main(int argc, char **argv)
{
	int opt;
	const char *ttydev = "/dev/ttyACM0";
	int programm_arduino = 0;

	while ((opt = getopt(argc, argv, "pt:")) != -1) {
		switch (opt) {
		case 'p':
			programm_arduino = 1;
			break;
		case 't':
			ttydev = optarg;
			break;
		default:
			help(argv[0]);
		}
	}

	if (optind != argc-2)
		help(argv[0]);

	config(argv[optind]);

	if (programm_arduino) {
		genfirmware(".ardulogic_tmp.firmware.c");
		setenv("ARDUINO_TTY", ttydev, 1);
		int rc = system("set -x; avr-gcc -Wall -O3 -o .ardulogic_tmp.firmware.elf -mmcu=atmega328p -DF_CPU=16000000L .ardulogic_tmp.firmware.c");
		rc = rc ?: system("set -x; avr-objcopy -j .text -j .data -O ihex .ardulogic_tmp.firmware.elf .ardulogic_tmp.firmware.hex");
		rc = rc ?: system("set -x; avrdude -p m328p -b 115200 -c arduino -P \"$ARDUINO_TTY\" -v -U \"flash:w:.ardulogic_tmp.firmware.hex\"");
		remove(".ardulogic_tmp.firmware.c");
		remove(".ardulogic_tmp.firmware.elf");
		remove(".ardulogic_tmp.firmware.hex");
		if (rc) {
			fprintf(stderr, "Error while compiling firmware or programming the arduino!\n");
			exit(1);
		}
	}

	readdata(ttydev);
	writevcd(argv[optind+1]);

	return 0;
}
