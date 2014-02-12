%{
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define YYSTYPE double
%}

%token NUMBER
%token PLUS MINUS TIMES DIVIDE POWER
%token LEFT RIGHT
%token LOG
%token LN
%token END

%left PLUS MINUS
%left TIMES DIVIDE
%left NEG
%right POWER

%parse-param {double *val}
%start Input
%%

Input:
    
     | Input Line
;

Line:
     END
     | Expression END { *val = $1; }
;

Expression:
     NUMBER { $$=$1; }
| Expression PLUS Expression { $$=$1+$3; }
| Expression MINUS Expression { $$=$1-$3; }
| Expression TIMES Expression { $$=$1*$3; }
| Expression DIVIDE Expression { $$=$1/$3; }
| MINUS Expression %prec NEG { $$=-$2; }
| Expression POWER Expression { $$=pow($1,$3); }
| LEFT Expression RIGHT { $$=$2; }
| LOG LEFT Expression RIGHT { $$=log10($3); }
| LN LEFT Expression RIGHT { $$=log($3); }
;

%%

int yyerror(char *s) {
  fprintf(stderr, "%s\n", s);
}

/**
 * Parses the expression of load as a function of delay and returns the result.
 * Function is supposed to be the string read from the configuration file, e.g.,
 * e^-(1.08X) +5*e^(2*X). The name of the variable is *strictly* X and will be
 * replaced by the value of the delay argument.
 */
double map_to_load(char *function, double delay, int *err) {
	double retval;
	char *saveptr;
	char *token;
	char strdelay[32];
	memset(strdelay, 0, 32);
	sprintf(strdelay, "%f", delay);

	char function_copy[strlen(function) + 2];
	memset(function_copy, 0, strlen(function) + 2);
	sprintf(function_copy, "%s\n\0", function);

	//function with X replaced with actual value
	char toparse[256];
	memset(toparse, 0, 256);
	char *ptr = toparse;

	//replace variable--split string and concat...
	token = strtok_r(function_copy, "X", &saveptr);
	do {
		if (!token) break;
		strcpy(ptr, token);
		ptr += strlen(token);
		if ( (token = strtok_r(NULL, "X", &saveptr))  || function[strlen(function) - 1] == 'X') {
			strcpy(ptr, strdelay);
			ptr += strlen(strdelay);
		}
	} while (1);

	yy_scan_string(toparse);
	*err = 0;
	if (yyparse(&retval)) *err = 1;
	yylex_destroy();
	return retval;
}

