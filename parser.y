
%{

#include "ardulogic.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

