%{
#define YYSTYPE double
#include "funceval.tab.h"
#include <stdlib.h>
#include <math.h>
%}

white [ \t]+
digit [0-9]
integer {digit}+
exponent [eE][+-]?{integer}
real {integer}("."{integer})?{exponent}?
epsilon e
%%

{white} { }
{real} { yylval=atof(yytext); 
 return NUMBER;
}
{epsilon} { yylval=M_E; return NUMBER; }

"+" return PLUS;
"-" return MINUS;
"*" return TIMES;
"/" return DIVIDE;
"^" return POWER;
"(" return LEFT;
")" return RIGHT;
"\n" return END;
"log" return LOG;
"ln" return LN;
