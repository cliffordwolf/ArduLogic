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
int trigger_freq;
int pins[TOTAL_PIN_NUM];
const char *pin_names[TOTAL_PIN_NUM] = {
	"A0", "A1", "A2", "A3", "A4", "A5",
	"D2", "D3", "D4", "D5", "D6", "D7" };
std::vector<uint16_t> samples;

const char *vcd_prefix = "";
bool dont_cleanup_fwsrc;
bool verbose;

void help(const char *progname)
{
	fprintf(stderr, "Usage: %s [-v] [-p [-n]] [-P vcd_prefix] [-t <dev>] \\\n", progname);
	fprintf(stderr, "     %*.s [-V vcd_ file] [-R raw_file] \\\n", int(strlen(progname)+2), "");
	fprintf(stderr, "     %*.s configfile [ raw_file ]\n", int(strlen(progname)+2), "");
	exit(1);
}

int main(int argc, char **argv)
{
	int opt;
	const char *ttydev = "/dev/ttyACM0";
	bool programm_arduino = false;
	const char *vcd_file = NULL;
	const char *raw_file = NULL;

	while ((opt = getopt(argc, argv, "vpnP:t:V:R:")) != -1) {
		switch (opt) {
		case 'v':
			verbose = true;
			break;
		case 'p':
			programm_arduino = true;
			break;
		case 'n':
			dont_cleanup_fwsrc = true;
			break;
		case 'P':
			vcd_prefix = optarg;
			break;
		case 't':
			ttydev = optarg;
			break;
		case 'V':
			vcd_file = optarg;
			break;
		case 'R':
			raw_file = optarg;
			break;
		default:
			help(argv[0]);
		}
	}

	if (optind >= argc-2)
		help(argv[0]);

	config(argv[optind]);

	if (programm_arduino)
		genfirmware(ttydev);

	if (optind == argc-2)
		readrawfile(argv[optind+1]);
	else
		readdata(ttydev, !programm_arduino);

	if (vcd_file)
		writevcd(vcd_file);

	if (raw_file)
		writerawfile(argv[optind+1]);

	return 0;
}
