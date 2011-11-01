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

#ifndef ARDULOGIC_H
#define ARDULOGIC_H

#include <stdint.h>
#include <stdio.h>
#include <vector>

#define PIN_A(__n) (__n)
#define PIN_D(__n) (__n+4)
#define TOTAL_PIN_NUM 12

#define PIN_CAPTURE		0x01
#define PIN_TRIGGER_POSEDGE	0x02
#define PIN_TRIGGER_NEGEDGE	0x04
#define PIN_PULLUP		0x08

#define DECODE_NONE	0
#define DECODE_TRIGGER	1
#define DECODE_FREQ	2
#define DECODE_SPI	3
#define DECODE_I2C	4
#define DECODE_JTAG	5

#define CFG_SPI_CSNEG	0
#define CFG_SPI_CS	1
#define CFG_SPI_MOSI	2
#define CFG_SPI_MISO	3

#define CFG_I2C_SCL	0
#define CFG_I2C_SDA	1

#define CFG_JTAG_TMS	0
#define CFG_JTAG_TDI	1
#define CFG_JTAG_TDO	2

#define CFG_WORDS	4

extern int decode;
extern int decode_config[CFG_WORDS];
extern int trigger_freq;
extern int pins[TOTAL_PIN_NUM];
extern const char *pin_names[TOTAL_PIN_NUM];
extern std::vector<uint16_t> samples;

void config(const char *file);
void genfirmware(const char *file);
void readdata(const char *tts);
void writevcd(const char *file);

extern const char *vcd_prefix;
extern bool verbose;

struct decoder_desc {
	void (*vcd_defs)(FILE *f);
	void (*vcd_init)(FILE *f);
	void (*vcd_step)(FILE *f, size_t i);
};

extern struct decoder_desc decoder_spi;
extern struct decoder_desc decoder_i2c;
extern struct decoder_desc decoder_jtag;

#endif
