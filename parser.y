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

%{

#include "ardulogic.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <list>

extern int yylex(void);
extern int yyget_lineno(void);

void check_decode(int v) {
	if (decode != 0 && decode != v) {
		fprintf(stderr, "Config error in line %d: Conflicting decode and/or trigger statements\n", yyget_lineno());
		exit(1);
	}
}

void yyerror (char const *s) {
        fprintf(stderr, "Parser error in line %d: %s\n", yyget_lineno(), s);
        exit(1);
}

%}

%union {
	int num;
	char *str;
}

%token <num> TOK_PIN
%token <str> TOK_STRING

%token TOK_TRIGGER TOK_POSEDGE TOK_NEGEDGE
%token TOK_DECODE TOK_SPI TOK_I2C TOK_JTAG
%token TOK_CAPTURE TOK_PULLUP TOK_LABEL TOK_EOL

%type <num> edge neg

%%

config:
	/* empty */ |
	config TOK_EOL |
	config stmt TOK_EOL;

stmt:
	stmt_trigger | stmt_capture | stmt_pullup | stmt_decode | stmt_label;

stmt_trigger:
	TOK_TRIGGER edge TOK_PIN {
		check_decode(DECODE_TRIGGER);
		pins[$3] |= $2;
		decode = DECODE_TRIGGER;
	};

stmt_capture:
	TOK_CAPTURE capture_list;

capture_list:
	capture_list TOK_PIN {
		pins[$2] |= PIN_CAPTURE;
	} |
	TOK_PIN {
		pins[$1] |= PIN_CAPTURE;
	};

stmt_pullup:
	TOK_PULLUP pullup_list;

pullup_list:
	pullup_list TOK_PIN {
		pins[$2] |= PIN_PULLUP;
	} |
	TOK_PIN {
		pins[$1] |= PIN_PULLUP;
	};

stmt_decode:
	TOK_DECODE TOK_SPI edge TOK_PIN neg TOK_PIN TOK_PIN TOK_PIN {
		check_decode(0);
		decode_config[CFG_SPI_CSNEG] = $5;
		decode_config[CFG_SPI_CS]    = $6;
		decode_config[CFG_SPI_MOSI]  = $7;
		decode_config[CFG_SPI_MISO]  = $8;
		pins[$4] |= $3; // SCK
		pins[$5] |= PIN_CAPTURE | PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE; // CS
		pins[$6] |= PIN_CAPTURE; // MOSI
		pins[$7] |= PIN_CAPTURE; // MISO
		pin_names[$4] = "SCK";
		pin_names[$5] = "CS";
		pin_names[$6] = "MOSI";
		pin_names[$7] = "MISO";
		decode = DECODE_SPI;
	} |
	TOK_DECODE TOK_I2C TOK_PIN TOK_PIN {
		check_decode(0);
		decode_config[CFG_I2C_SCL] = $3;
		decode_config[CFG_I2C_SDA] = $4;
		pins[$3] |= PIN_CAPTURE | PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE; // SCL
		pins[$4] |= PIN_CAPTURE | PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE; // SDA
		pin_names[$3] = "SCL";
		pin_names[$4] = "SDA";
		decode = DECODE_I2C;
	} |
	TOK_DECODE TOK_JTAG TOK_PIN TOK_PIN TOK_PIN TOK_PIN {
		check_decode(0);
		decode_config[CFG_JTAG_TMS] = $4;
		decode_config[CFG_JTAG_TDI] = $5;
		decode_config[CFG_JTAG_TDO] = $6;
		pins[$3] |= PIN_TRIGGER_POSEDGE; // TCK
		pins[$4] |= PIN_CAPTURE; // TMS
		pins[$5] |= PIN_CAPTURE; // TDI
		pins[$6] |= PIN_CAPTURE; // TDO
		pin_names[$3] = "TCK";
		pin_names[$4] = "TMS";
		pin_names[$5] = "TDI";
		pin_names[$6] = "TDO";
		decode = DECODE_JTAG;
	};

stmt_label:
	TOK_LABEL TOK_PIN TOK_STRING {
		pin_names[$2] = $3;
	};

edge:
	TOK_POSEDGE {
		$$ = PIN_TRIGGER_POSEDGE;
	} |
	TOK_NEGEDGE {
		$$ = PIN_TRIGGER_NEGEDGE;
	};

neg:
	'!' {
		$$ = 1;
	} |
	/* empty */ {
		$$ = 0;
	};

%%

extern FILE *yyin;

void config(const char *file)
{
	decode = 0;
	memset(decode_config, 0, sizeof(decode_config));
	memset(pins, 0, sizeof(pins));

	yyin = fopen(file, "r");
	if (yyin == NULL) {
		fprintf(stderr, "Can't open config file `%s': %s\n", file, strerror(errno));
		exit(1);
	}

	yyparse();
	fclose(yyin);

	printf("Capture configuration:");
	for (int i = 0; i < TOTAL_PIN_NUM; i++) {
		if (pins[i] == 0)
			continue;
		printf(" %s:", pin_names[i]);
		if ((pins[i] & PIN_CAPTURE) != 0)
			printf("c");
		if ((pins[i] & PIN_TRIGGER_POSEDGE) != 0)
			printf("p");
		if ((pins[i] & PIN_TRIGGER_NEGEDGE) != 0)
			printf("n");
	}
	printf("\n");

	printf("Decode configuration: %d", decode);
	for (int i = 0; i < CFG_WORDS; i++)
		printf(" %d", decode_config[i]);
	printf("\n");
}

