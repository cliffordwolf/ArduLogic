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

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

static int fd;
static struct termios tcattr_old;
static const char *tts_name;

static void sigint_hdl(int dummy)
{
	char ch = 0;
	if (write(fd, &ch, 1) != 1) {
		fprintf(stderr, "I/O Error on tts `%s': %s\n", tts_name, strerror(errno));
		tcsetattr(fd, TCSAFLUSH, &tcattr_old);
		exit(1);
	}
}

static void sigalrm_hdl(int dummy)
{
	printf("\n");
	printf("-- no response from device --\n");
	printf("\n");
	printf("This could have multiple reasons:\n");
	printf("\n");
	printf("1. There is no ArduLogic firmware on the device yet.\n");
	printf("Solution: Re-run with the `-p' command line option set.\n");
	printf("\n");
	printf("2. The Arduino is connected to a different serial device.\n");
	printf("Solution: Pass the correct serial device name using the `-t'\n");
	printf("command line option.\n");
	printf("\n");
	printf("3. Automatic reset via USB is disabled on your Arduino.\n");
	printf("solution: Push the reset button on the Arduino now.\n");
	printf("\n");
}

static unsigned char serialread()
{
	unsigned char ch;
	int rc = read(fd, &ch, 1);
	if (rc == 1) {
		if (verbose) {
			if (32 < ch && ch < 127)
				printf("<0x%02x:'%c'>\n", ch, ch);
			else if (ch > 127)
				printf("<0x%02x:0b%d%d%d%d%d%d%d%d>\n", ch,
						(ch & 0x80) != 0, (ch & 0x40) != 0,
						(ch & 0x20) != 0, (ch & 0x10) != 0,
						(ch & 0x08) != 0, (ch & 0x04) != 0,
						(ch & 0x02) != 0, (ch & 0x01) != 0);
			else
				printf("<0x%02x>\n", ch);
		}
		return ch;
	}
	if (rc == 0)
		fprintf(stderr, "I/O Error on tts `%s': EOF\n", tts_name);
	else
		fprintf(stderr, "I/O Error on tts `%s': %s\n", tts_name, strerror(errno));
	tcsetattr(fd, TCSAFLUSH, &tcattr_old);
	exit(1);
}

static size_t get_numbits(std::vector<uint8_t> &data)
{
	return (data.size()-1) * 7 - (data.back() & ~0x80);
}

static bool get_bit(std::vector<uint8_t> &data, size_t num)
{
	size_t byte_num = num / 7;
	size_t bit_num = num % 7;
	return (data[byte_num] & (1 << bit_num)) != 0;
}

static size_t get_numwords(std::vector<uint8_t> &data, size_t bits)
{
	size_t total_bits = get_numbits(data);
	if (total_bits % bits != 0) {
		fprintf(stderr, "Data encoding boundary error on tts `%s' (total_bits=%d, chunk_bits=%d).\n",
				tts_name, total_bits, bits);
		exit(1);
	}
	return total_bits / bits;
}

static uint16_t get_word(std::vector<uint8_t> &data, size_t num, size_t bits)
{
	uint16_t value = 0;
	for (size_t i = 0; i < bits; i++)
		value |= get_bit(data, num*bits + i) ? 1 << i : 0;
	return value;
}

void readdata(const char *tts, bool autoprog)
{
	printf("Connecting to Arduino on `%s'..\n", tts);
	tts_name = tts;

	fd = open(tts, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Failed to open tts device `%s': %s\n", tts, strerror(errno));
		exit(1);
	}

	tcgetattr(fd, &tcattr_old);
	struct termios tcattr = tcattr_old;
	tcattr.c_iflag = IGNBRK | IGNPAR;
	tcattr.c_oflag = 0;
	tcattr.c_cflag = CS8 | CREAD | CLOCAL;
	tcattr.c_lflag = 0;
	cfsetspeed(&tcattr, B2000000);
	tcsetattr(fd, TCSAFLUSH, &tcattr);

	char header[100 + TOTAL_PIN_NUM] = "..ARDULOGIC:";
	int hp = strlen(header), hdrlen = hp;
	header[0] = header[1] = 0;
	for (int i = 0; i < TOTAL_PIN_NUM; i++)
		header[hp++] = pins[i] + '0';
	hp += sprintf(header + hp, ":%x:\r\n", trigger_freq);

	sighandler_t old_hdl = signal(SIGALRM, &sigalrm_hdl);
	alarm(3);

	int idx = 0;
	while (idx != hp) {
		unsigned char ch = serialread();
		if (header[idx] != ch) {
			if (idx >= hdrlen && ch != 0) {
				if (autoprog) {
					alarm(0);
					signal(SIGALRM, old_hdl);
					close(fd);
					fprintf(stderr, "Firmware doesn't match configuration. Reprogramming probe.\n");
					genfirmware(tts);
					readdata(tts, false);
					return;
				}
				fprintf(stderr, "Firmware doesn't match configuration. Re-run with -p.\n");
				exit(1);
			}
			idx = header[0] == ch ? 1 : 0;
		} else
			idx++;
	}

	alarm(0);
	signal(SIGALRM, old_hdl);

	printf("Recording. Press Ctrl-C to stop.\n");
	old_hdl = signal(SIGINT, &sigint_hdl);

	uint8_t error_code;
	int disp_count = 0;
	int disp_mode = 0;
	std::vector<uint8_t> data;
	while (1)
	{
		unsigned char ch = serialread();
		if (ch == 0) {
			ch = serialread();
			if (ch == 1) {
				error_code = serialread();
				break;
			}
			goto encoder_error;
		}
		if ((ch & 0x80) == 0) {
	encoder_error:
			fprintf(stderr, "Data encoding error on tts `%s'.\n", tts_name);
			tcsetattr(fd, TCSAFLUSH, &tcattr_old);
			exit(1);
		}
		data.push_back(ch);
		if (!verbose) {
			putchar(disp_mode[".,*#="]);
			if (++disp_count >= 64) {
				if (data.size() > 1e6)
					printf(" %.2f MB   \r", data.size() / double(1024*1024));
				else
					printf(" %.2f kB\r", data.size() / double(1024));
				disp_mode = (disp_mode + 1) % 5;
				disp_count = 0;
			}
			fflush(stdout);
		}
	}

	printf("\nRecording finished. Got %d bytes tts payload.\n", (int)data.size());
	signal(SIGINT, old_hdl);

	tcsetattr(fd, TCSAFLUSH, &tcattr_old);
	close(fd);

	if (error_code) {
		fprintf(stderr, "Probe reported error 0x%02x.\n", error_code);
		exit(1);
	}

	int num_bits = 0;
	int bit2pin[16] = { /* zeros */ };
	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
		if ((pins[i] & PIN_CAPTURE) == 0)
			continue;
		bit2pin[num_bits++] = i;
	}

	int num_words = get_numwords(data, num_bits);
	for (int i = 0; i < num_words; i++) {
		uint16_t word = get_word(data, i, num_bits);
		uint16_t sample = 0;
		for (int j = 0; j < num_bits; j++) {
			if ((word & (1 << j)) == 0)
				continue;
			sample |= 1 << bit2pin[j];
		}
		if (verbose)
			printf("Decode: word=0x%04x -> sample=0x%04x\n", word, sample);
		samples.push_back(sample);
	}

	printf("Decoded %d samples from captured data.\n", (int)samples.size());
}

