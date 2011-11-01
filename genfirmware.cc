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
#include <assert.h>
#include <math.h>

static void gen_pack(FILE *f)
{
	uint8_t ashift[32] = { };
	uint8_t dshift[32] = { };
	int idx = 0;

	for (int i = PIN_A(0); i < PIN_D(2); i++) {
		if ((pins[i] & PIN_CAPTURE) == 0)
			continue;
		int off = 16 + (idx++) - (i - PIN_A(0));
		ashift[off] |= 1 << i;
	}
	for (int i = PIN_D(2); i < TOTAL_PIN_NUM; i++) {
		if ((pins[i] & PIN_CAPTURE) == 0)
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

static void gen_fifo(FILE *f, int num_bits)
{
	fprintf(f, "static void serio_send();\n");
	fprintf(f, "volatile uint8_t fifo_data[256];\n");
	fprintf(f, "volatile uint8_t fifo_in = 0, fifo_out = 0;\n");
	fprintf(f, "uint8_t fifo_bits = 7;\n");
	fprintf(f, "static inline void fifo_next() {\n");
	fprintf(f, "	while (fifo_in+1 == fifo_out) {\n");
	fprintf(f, "		error_code |= 0x01;\n");
	fprintf(f, "		PORTB |= 0x20;\n");
	fprintf(f, "		serio_send();\n");
	fprintf(f, "	}\n");
	fprintf(f, "	fifo_data[++fifo_in] = 0x80;\n");
	fprintf(f, "	fifo_bits = 7;\n");
	fprintf(f, "}\n");
	fprintf(f, "volatile bool fifo_push_en = 0;\n");
	fprintf(f, "static inline void fifo_push(smplword_t w) {\n");
	fprintf(f, "	uint8_t bits = %d;\n", num_bits);
	fprintf(f, "	if (!fifo_push_en)\n");
	fprintf(f, "		return;\n");
	fprintf(f, "	do {\n");
	fprintf(f, "		uint8_t bc = bits > fifo_bits ? fifo_bits : bits;\n");
	fprintf(f, "		fifo_data[fifo_in] |= w << (7-fifo_bits);\n");
	fprintf(f, "		fifo_bits -= bc;\n");
	fprintf(f, "		if (fifo_bits == 0)\n");
	fprintf(f, "			fifo_next();\n");
	fprintf(f, "		w = w >> bc;\n");
	fprintf(f, "		bits -= bc;\n");
	fprintf(f, "	} while (bits > 0);\n");
	fprintf(f, "}\n");
	fprintf(f, "static inline void fifo_close() {\n");
	fprintf(f, "	while (fifo_in != fifo_out)\n");
	fprintf(f, "		serio_send();\n");
	fprintf(f, "	fifo_data[++fifo_in] = 0x80 | fifo_bits;\n");
	fprintf(f, "	fifo_next();\n");
	fprintf(f, "}\n");
}

static void gen_serio(FILE *f)
{
	fprintf(f, "#define BAUD_RATE 115200\n");
	fprintf(f, "static void serio_setup() {\n");
	fprintf(f, "	// See AtMega168 Datasheet Table 19-12: 115200 baud @16MHz\n");
	fprintf(f, "	UBRR0H = 0;\n");
	fprintf(f, "	UBRR0L = 8;\n");
	fprintf(f, "	UCSR0A = 0;\n");
	fprintf(f, "	UCSR0B = _BV(RXEN0) | _BV(TXEN0);\n");
	fprintf(f, "	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);\n");
	fprintf(f, "	PORTD |= _BV(1);\n");
	fprintf(f, "	DDRD |= _BV(1);\n");
	fprintf(f, "}\n");
	fprintf(f, "static void serio_send() {\n");
	fprintf(f, "	if (fifo_in == fifo_out)\n");
	fprintf(f, "		return;\n");
	fprintf(f, "	PORTB |= 0x04;\n");
	fprintf(f, "	if ((UCSR0A & _BV(UDRE0)) == 0)\n");
	fprintf(f, "		return;\n");
	fprintf(f, "	UDR0 = fifo_data[fifo_out];\n");
	fprintf(f, "	fifo_out++;\n");
	fprintf(f, "	PINB = 0x0c;\n");
	fprintf(f, "}\n");
#if 0
	fprintf(f, "static void serio_sendbyte(uint8_t ch) {\n");
	fprintf(f, "	while ((UCSR0A & _BV(UDRE0)) == 0) { /* wait */ }\n");
	fprintf(f, "	UDR0 = ch;\n");
	fprintf(f, "}\n");
#endif
}

static void gen_freq_trigger(FILE *f)
{
	fprintf(f, "ISR(TIMER1_COMPA_vect) {\n");
	fprintf(f, "	uint8_t value_pinc = PINC;\n");
	fprintf(f, "	uint8_t value_pind = PIND;\n");
	fprintf(f, "	PORTB |= 0x02;\n");
	fprintf(f, "	fifo_push(pack(value_pinc, value_pind));\n");
	fprintf(f, "	PORTB &= ~0x02;\n");
	fprintf(f, "}\n");
}

static void gen_irq_trigger(FILE *f)
{
	fprintf(f, "// volatile uint8_t value_pinc;\n");
	fprintf(f, "// volatile uint8_t value_pind;\n");
	for (int i=0; i<2; i++) {
		fprintf(f, "ISR(INT%d_vect) {\n", i);
		fprintf(f, "	uint8_t value_pinc = PINC;\n");
		fprintf(f, "	uint8_t value_pind = PIND;\n");
		fprintf(f, "	PORTB |= 0x02;\n");
		fprintf(f, "	fifo_push(pack(value_pinc, value_pind));\n");
		fprintf(f, "	PORTB &= ~0x02;\n");
		fprintf(f, "}\n");
	}
}

static void gen_trigger(FILE *f)
{
	fprintf(f, "static uint8_t trigger_state = 0x80;\n");

	fprintf(f, "static void check_trigger() {\n");
	fprintf(f, "	uint8_t value_pinc = PINC;\n");
	fprintf(f, "	uint8_t value_pind = PIND;\n");
	fprintf(f, "	PORTB |= 0x01;\n");
	fprintf(f, "	uint8_t new_state = 0;\n");

	int idx = 0;
	uint8_t posedge_mask = 0x80;
	uint8_t negedge_mask = 0x80;
	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
		if ((pins[i] & (PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE)) == 0)
			continue;
		if ((pins[i] & PIN_TRIGGER_POSEDGE) != 0)
			posedge_mask |= 1 << idx;
		if ((pins[i] & PIN_TRIGGER_NEGEDGE) != 0)
			negedge_mask |= 1 << idx;
		if (PIN_A(0) <= i && i <= PIN_A(5))
			fprintf(f, "	new_state |= (value_pinc & 0x%02x) != 0 ? 0x%02x : 0;\n", 1 << (i-PIN_A(0)), 1 << idx);
		if (PIN_D(2) <= i && i <= PIN_D(7))
			fprintf(f, "	new_state |= (value_pind & 0x%02x) != 0 ? 0x%02x : 0;\n", 1 << (i-PIN_D(0)), 1 << idx);
		idx++;
	}

	fprintf(f, "	if ((new_state & ~trigger_state & 0x%02x) != 0)\n", posedge_mask);
	fprintf(f, "		goto triggered;\n");
	fprintf(f, "	if ((~new_state & trigger_state & 0x%02x) != 0)\n", negedge_mask);
	fprintf(f, "		goto triggered;\n");
	fprintf(f, "	trigger_state = new_state;\n");
	fprintf(f, "	PORTB &= ~0x03;\n");
	fprintf(f, "	return;\n");

	fprintf(f, "triggered:\n");
	fprintf(f, "	PORTB |= 0x02;\n");
	fprintf(f, "	trigger_state = new_state;\n");
	fprintf(f, "	fifo_push(pack(value_pinc, value_pind));\n");
	fprintf(f, "	PORTB &= ~0x03;\n");
	fprintf(f, "}\n");
}

void genfirmware(const char *tts)
{
	bool use_irq_trigger = true;
	int num_trigger = 0;
	int num_bits = 0;

	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
		if ((pins[i] & (PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE)) != 0)
			num_trigger++;
		if ((pins[i] & PIN_CAPTURE) != 0)
			num_bits++;
		if (i == PIN_D(2) || i == PIN_D(3))
			continue;
		if ((pins[i] & (PIN_TRIGGER_POSEDGE|PIN_TRIGGER_NEGEDGE)) != 0)
			use_irq_trigger = false;
	}

	if (num_trigger > 7) {
		fprintf(stderr, "A maximum of 7 trigger pins is supported by the firmware generator.\n");
		exit(1);
	}

	if (num_trigger > 0 && trigger_freq > 0) {
		fprintf(stderr, "Conflicting trigger configuration: found trigger pins and trigger frequency.\n");
		exit(1);
	}

	if (!use_irq_trigger)
		printf("WARNING: IRQs are only used when all triggers are on D2 and/or D3!\n");

	FILE *f = fopen(".ardulogic_tmp.firmware.c", "w");
	if (!f) {
		fprintf(stderr, "Can't create firmware source file `.ardulogic_tmp.firmware.c': %s\n", strerror(errno));
		exit(1);
	}

	fprintf(f, "#include <stdint.h>\n");
	fprintf(f, "#include <stdbool.h>\n");
	fprintf(f, "#include <avr/io.h>\n");
	fprintf(f, "#include <avr/sleep.h>\n");
	fprintf(f, "#include <avr/interrupt.h>\n");
	fprintf(f, "typedef uint%d_t smplword_t;\n", num_bits <= 8 ? 8 : 16);
	fprintf(f, "volatile uint8_t error_code = 0;\n");

	gen_pack(f);
	gen_fifo(f, num_bits);
	gen_serio(f);

	if (trigger_freq > 0)
		gen_freq_trigger(f);
	else if (use_irq_trigger)
		gen_irq_trigger(f);
	else
		gen_trigger(f);

	char header[100 + TOTAL_PIN_NUM] = "..ARDULOGIC:";
	int hp = strlen(header);
	header[0] =  header[1] = 0;
	for (int i = 0; i < TOTAL_PIN_NUM; i++)
		header[hp++] = pins[i] + '0';
	hp += sprintf(header + hp, ":%x:\r\n", trigger_freq);

	uint8_t pullupc = 0, pullupd = 0;
	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
		if ((pins[i] & PIN_PULLUP) == 0)
			continue;
		if (PIN_A(0) <= i && i <= PIN_A(5))
			pullupc |= 1 << (i-PIN_A(0));
		if (PIN_D(2) <= i && i <= PIN_D(7))
			pullupd |= 1 << (i-PIN_D(0));
	}

	fprintf(f, "int main() {\n");
	fprintf(f, "	DDRB = 0x3f;\n");
	fprintf(f, "	DDRC = 0;\n");
	fprintf(f, "	DDRD = 0;\n");
	fprintf(f, "	PORTB = 0;\n");
	fprintf(f, "	PORTC = 0x%02x;\n", pullupc);
	fprintf(f, "	PORTD = 0x%02x;\n", pullupd);
	fprintf(f, "	serio_setup();\n");
	for (int i = 0; i < hp; i++)
		fprintf(f, "	fifo_data[fifo_in++] = 0x%02x;\n", header[i]);
	fprintf(f, "	fifo_data[fifo_in] |= 0x80;\n");

	if (trigger_freq > 0)
	{
		uint8_t tccr1a = 0, tccr1b = 0, tccr1c = 0;
		uint16_t tcnt1 = 0, ocr1a = 0, ocr1b = 0, icr1 = 0;
		uint8_t timsk1 = 0, tifr1 = 0;

		int prescale = 1;
		int prescale_bits = 1;
		int cycles;

		while (1)
		{
			cycles = round((16e6 / prescale) / trigger_freq);
			if (cycles < 64000)
				break;

			switch (prescale)
			{
			case 1:
				prescale = 8;
				prescale_bits = 2;
				break;
			case 8:
				prescale = 64;
				prescale_bits = 3;
				break;
			case 64:
				prescale = 256;
				prescale_bits = 4;
				break;
			case 256:
				prescale = 1024;
				prescale_bits = 5;
				break;
			default:
				assert(!"This should never happen");
				exit(1);
			}
		}

		printf("Configure trigger for %.2f kHz (prescale=%d, cycles=%d).\n",
				16e6 / (prescale * cycles), prescale, cycles);

		// CTC mode: set CTC1/WGM12 in tccr1b
		tccr1b |= 0x08;

		// configure prescaler and cycles
		tccr1b |= prescale;
		ocr1a = cycles;

		// enable ionterrupt (timer 1 comp A)
		timsk1 |= 0x02;

		fprintf(f, "	fifo_push_en = true;\n");
		fprintf(f, "	fifo_push(pack(PINC, PIND));\n");
		fprintf(f, "	PORTB |= 0x10;\n");

		fprintf(f, "	TCCR1A = 0x%02x;\n", tccr1a);
		fprintf(f, "	TCCR1B = 0x%02x;\n", tccr1b);
		fprintf(f, "	TCCR1C = 0x%02x;\n", tccr1c);
		fprintf(f, "	TCNT1H = 0x%02x;\n", tcnt1 >> 8);
		fprintf(f, "	TCNT1L = 0x%02x;\n", tcnt1 & 0xff);
		fprintf(f, "	OCR1AH = 0x%02x;\n", ocr1a >> 8);
		fprintf(f, "	OCR1AL = 0x%02x;\n", ocr1a & 0xff);
		fprintf(f, "	OCR1BH = 0x%02x;\n", ocr1b >> 8);
		fprintf(f, "	OCR1BL = 0x%02x;\n", ocr1b & 0xff);
		fprintf(f, "	ICR1H = 0x%02x;\n", icr1 >> 8);
		fprintf(f, "	ICR1L = 0x%02x;\n", icr1 & 0xff);
		fprintf(f, "	TIMSK1 = 0x%02x;\n", timsk1);
		fprintf(f, "	TIFR1 = 0x%02x;\n", tifr1);
		fprintf(f, "	sei();\n");

		fprintf(f, "	while ((UCSR0A & _BV(RXC0)) == 0) {\n");
		fprintf(f, "		PINB = 0x01;\n");
		fprintf(f, "		serio_send();\n");
		fprintf(f, "	}\n");

		fprintf(f, "	PORTB &= ~0x10;\n");
		fprintf(f, "	fifo_push_en = false;\n");

		fprintf(f, "	// EICRA = 0;\n");
		fprintf(f, "	// EIMSK = 0;\n");
		fprintf(f, "	// cli();\n");
	}
	else if (use_irq_trigger)
	{
		uint8_t eicra = 0;
		uint8_t eimsk = 0;

		switch (pins[PIN_D(2)] & (PIN_TRIGGER_POSEDGE|PIN_TRIGGER_NEGEDGE))
		{
		case PIN_TRIGGER_POSEDGE|PIN_TRIGGER_NEGEDGE:
			eicra |= 0x01;
			eimsk |= 0x01;
			break;
		case PIN_TRIGGER_NEGEDGE:
			eicra |= 0x02;
			eimsk |= 0x01;
			break;
		case PIN_TRIGGER_POSEDGE:
			eicra |= 0x03;
			eimsk |= 0x01;
			break;
		}

		switch (pins[PIN_D(3)] & (PIN_TRIGGER_POSEDGE|PIN_TRIGGER_NEGEDGE))
		{
		case PIN_TRIGGER_POSEDGE|PIN_TRIGGER_NEGEDGE:
			eicra |= 0x04;
			eimsk |= 0x02;
			break;
		case PIN_TRIGGER_NEGEDGE:
			eicra |= 0x06;
			eimsk |= 0x02;
			break;
		case PIN_TRIGGER_POSEDGE:
			eicra |= 0x0c;
			eimsk |= 0x02;
			break;
		}

		fprintf(f, "	fifo_push_en = true;\n");
		fprintf(f, "	fifo_push(pack(PINC, PIND));\n");
		fprintf(f, "	fifo_push_en = false;\n");

		fprintf(f, "	EICRA = 0x%02x;\n", eicra);
		fprintf(f, "	EIMSK = 0x%02x;\n", eimsk);
		fprintf(f, "	sei();\n");

		fprintf(f, "	fifo_push_en = true;\n");
		fprintf(f, "	PORTB |= 0x10;\n");
		fprintf(f, "	while ((UCSR0A & _BV(RXC0)) == 0) {\n");
		fprintf(f, "		PINB = 0x01;\n");
		fprintf(f, "		// value_pinc = PINC;\n");
		fprintf(f, "		// value_pind = PIND;\n");
		fprintf(f, "		serio_send();\n");
		fprintf(f, "	}\n");
		fprintf(f, "	PORTB &= ~0x10;\n");
		fprintf(f, "	fifo_push_en = false;\n");

		fprintf(f, "	// EICRA = 0;\n");
		fprintf(f, "	// EIMSK = 0;\n");
		fprintf(f, "	// cli();\n");
	}
	else
	{
		fprintf(f, "	fifo_push_en = true;\n");
		fprintf(f, "	PORTB |= 0x10;\n");
		fprintf(f, "	while ((UCSR0A & _BV(RXC0)) == 0) {\n");
		fprintf(f, "		serio_send();\n");
		fprintf(f, "		check_trigger();\n");
		fprintf(f, "	}\n");
		fprintf(f, "	PORTB &= ~0x10;\n");
		fprintf(f, "	fifo_push_en = false;\n");
	}

	fprintf(f, "	fifo_close();\n");
	fprintf(f, "	while (fifo_in != fifo_out)\n");
	fprintf(f, "		serio_send();\n");
	fprintf(f, "	fifo_data[fifo_in++] = 0;\n");
	fprintf(f, "	fifo_data[fifo_in++] = 1;\n");
	fprintf(f, "	fifo_data[fifo_in++] = error_code;\n");
	fprintf(f, "	while (1)\n");
	fprintf(f, "		serio_send();\n");
	fprintf(f, "	return 0;\n");
	fprintf(f, "}\n");

	fclose(f);

	if (tts) {
		setenv("ARDUINO_TTY", tts, 1);
		int rc = system("set -x; avr-gcc -Wall -std=gnu99 -O3 -o .ardulogic_tmp.firmware.elf -mmcu=atmega328p -DF_CPU=16000000L .ardulogic_tmp.firmware.c");
		rc = rc ?: system("set -x; avr-objcopy -j .text -j .data -O ihex .ardulogic_tmp.firmware.elf .ardulogic_tmp.firmware.hex");
		rc = rc ?: system("set -x; avrdude -p m328p -b 115200 -c arduino -P \"$ARDUINO_TTY\" -v -U \"flash:w:.ardulogic_tmp.firmware.hex\"");
		if (dont_cleanup_fwsrc == false) {
			remove(".ardulogic_tmp.firmware.c");
			remove(".ardulogic_tmp.firmware.elf");
			remove(".ardulogic_tmp.firmware.hex");
		}
		if (rc) {
			fprintf(stderr, "Error while compiling firmware or programming the arduino!\n");
			exit(1);
		}
	}
}

