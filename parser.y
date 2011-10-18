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
}

%token <num> TOK_PIN

%token TOK_TRIGGER TOK_POSEDGE TOK_NEGEDGE
%token TOK_DECODE TOK_SPI TOK_I2C TOK_JTAG
%token TOK_MONITOR TOK_EOL

%type <num> edge neg

%%

config:
	/* empty */ |
	config TOK_EOL |
	config stmt TOK_EOL;

stmt:
	stmt_trigger | stmt_monitor | stmt_decode;

stmt_trigger:
	TOK_TRIGGER edge TOK_PIN {
		check_decode(DECODE_TRIGGER);
		pins[$3] |= $2;
		decode = DECODE_TRIGGER;
	};

stmt_monitor:
	TOK_MONITOR monitor_list;

monitor_list:
	monitor_list TOK_PIN {
		pins[$2] |= PIN_MONITOR;
	} |
	TOK_PIN {
		pins[$1] |= PIN_MONITOR;
	};

stmt_decode:
	TOK_DECODE TOK_SPI edge TOK_PIN neg TOK_PIN TOK_PIN TOK_PIN {
		check_decode(0);
		decode_config[CFG_SPI_CSNEG] = $5;
		decode_config[CFG_SPI_CS]    = $6;
		decode_config[CFG_SPI_MOSI]  = $7;
		decode_config[CFG_SPI_MISO]  = $8;
		pins[$4] |= $3; // SCK
		pins[$5] |= PIN_MONITOR | PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE; // CS
		pins[$6] |= PIN_MONITOR; // MOSI
		pins[$7] |= PIN_MONITOR; // MISO
		decode = DECODE_SPI;
	} |
	TOK_DECODE TOK_I2C TOK_PIN TOK_PIN {
		check_decode(0);
		decode_config[CFG_I2C_SCL] = $3;
		decode_config[CFG_I2C_SDA] = $4;
		pins[$3] |= PIN_MONITOR | PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE; // SCL
		pins[$4] |= PIN_MONITOR | PIN_TRIGGER_POSEDGE | PIN_TRIGGER_NEGEDGE; // SDA
		decode = DECODE_I2C;
	} |
	TOK_DECODE TOK_JTAG TOK_PIN TOK_PIN TOK_PIN TOK_PIN {
		check_decode(0);
		decode_config[CFG_JTAG_TMS] = $4;
		decode_config[CFG_JTAG_TDI] = $5;
		decode_config[CFG_JTAG_TDO] = $6;
		pins[$3] |= PIN_TRIGGER_POSEDGE; // TCK
		pins[$4] |= PIN_MONITOR; // TMS
		pins[$5] |= PIN_MONITOR; // TDI
		pins[$6] |= PIN_MONITOR; // TD0
		decode = DECODE_JTAG;
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
		if ((pins[i] & PIN_MONITOR) != 0)
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

