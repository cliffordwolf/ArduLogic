/*
 *  ArduLogic - Low Speed Logic Analyzer using the Arduino HArdware
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

void yyerror (char const *s) {
        fprintf(stderr, "Parser error in line %d: %s\n", yyget_lineno(), s);
        exit(1);
}

%}

%union {
	int num;
	std::list<int> *numarray;
}

%token <num> TOK_NUM

%token TOK_TRIGGER TOK_ON TOK_POSEDGE TOK_NEGEDGE
%token TOK_MONITOR TOK_DECODE TOK_SPI TOK_I2C TOK_JTAG
%token TOK_EOL

%%

config:
	/* empty */ |
	config TOK_EOL |
	config stmt TOK_EOL;

stmt:
	stmt_trigger | stmt_monitor | stmt_decode;

stmt_trigger:
	TOK_TRIGGER TOK_NUM |
	TOK_TRIGGER TOK_ON TOK_POSEDGE TOK_NUM |
	TOK_TRIGGER TOK_ON TOK_NEGEDGE TOK_NUM;

stmt_monitor:
	TOK_MONITOR monitor_list;

monitor_list:
	monitor_list TOK_NUM |
	TOK_NUM;

stmt_decode:
	TOK_DECODE TOK_SPI neg TOK_NUM TOK_NUM TOK_NUM edge TOK_NUM |
	TOK_DECODE TOK_I2C TOK_NUM TOK_NUM |
	TOK_DECODE TOK_JTAG TOK_NUM TOK_NUM TOK_NUM TOK_NUM;

edge:
	TOK_POSEDGE | TOK_NEGEDGE;

neg:
	'!' | /* empty */;

%%

extern FILE *yyin;

void config(const char *file)
{
	yyin = fopen(file, "r");
	if (yyin == NULL) {
		fprintf(stderr, "Can't open config file `%s': %s\n", file, strerror(errno));
		exit(1);
	}
	yyparse();
	fclose(yyin);
}

