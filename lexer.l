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
#include <list>
#include "ardulogic.h"
#include "parser.hh"
%}

%option yylineno
%option noyywrap

%%

A[0-5] {
	yylval.num = yytext[1]-'0';
	return TOK_PIN;
}

D[2-7] {
	yylval.num = 6 + yytext[1]-'2';
	return TOK_PIN;
}

\"[#-~]+\" {
	yylval.str = strdup(yytext+1);
	yylval.str[strlen(yylval.str)-1] = 0;
	return TOK_STRING;
}

[0-9]+Hz {
	yylval.num = atoi(yytext);
	return TOK_FREQ;
}

[0-9]+kHz {
	yylval.num = 1000 * atoi(yytext);
	return TOK_FREQ;
}

"trigger"	{ return TOK_TRIGGER; }
"posedge"	{ return TOK_POSEDGE; }
"negedge"	{ return TOK_NEGEDGE; }
"capture"	{ return TOK_CAPTURE; }
"pullup"	{ return TOK_PULLUP; }
"label"		{ return TOK_LABEL; }

"decode"	{ return TOK_DECODE; }
"spi"		{ return TOK_SPI; }
"i2c"		{ return TOK_I2C; }
"jtag"		{ return TOK_JTAG; }

"msb"		{ return TOK_MSB; }
"lsb"		{ return TOK_LSB; }

[\n\r]		{ return TOK_EOL; }
"#"[^\n\r]*	/* comment */
[\t ]		/* ignore whitespace*/
.		{ return yytext[0]; }

%%

extern void *yyunput_usused_ptr;
void *yyunput_usused_ptr = (void*)&yyunput;

