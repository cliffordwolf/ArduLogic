%{
#include <list>
#include "ardulogic.h"
#include "parser.hh"
%}

%option yylineno
%option noyywrap

%%

[0-9]+ {
	yylval.num = atoi(yytext);
	return TOK_NUM;
}

"trigger"	{ return TOK_TRIGGER; }
"on"		{ return TOK_ON; }
"posedge"	{ return TOK_POSEDGE; }
"negedge"	{ return TOK_NEGEDGE; }

"monitor"	{ return TOK_MONITOR; }

"decode"	{ return TOK_DECODE; }
"spi"		{ return TOK_SPI; }
"i2c"		{ return TOK_I2C; }
"jtag"		{ return TOK_JTAG; }

[\n\r]		{ return TOK_EOL; }
"#"[^\n\r]*	/* comment */
[\t ]		/* ignore whitespace*/

%%

extern void *yyunput_usused_ptr;
void *yyunput_usused_ptr = (void*)&yyunput;

