%token <str> PC SR LABEL SIZEW
%token <val> AREG DREG VAL WIN
%type <val> expr
%left '-' '+'
%left '*' '/'
%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "cpu.h"
#include "expr.h"
#include "debug/debug.h"
#include "mmu.h"
  int yydebug = 1;
  void yyerror(char *);
  int yylex(void);
  static LONG addr;
  static long parseerror;
%}
%union {
  char *str;
  LONG val;
};
%%
input:
	| input line
	;

line:   '\n'
	| expr '\n' { addr = $1; }
	;

expr:	VAL { $$ = $1; }
	| AREG { $$ = cpu->a[$1]; }
	| DREG { $$ = cpu->d[$1]; }
	| PC { $$ = cpu->pc; }
	| SR { $$ = cpu->sr; }
	| WIN { $$ = debug_win_addr($1); }
	| LABEL { if(cprint_label_exists($1)) {
                    $$ = cprint_label_addr($1);
		  } else {
		    yyerror("foobar");
		  }
	          free($1);
		}
	| expr SIZEW { $$ = $1 & 0xffff; }
	| expr '-' expr { $$ = $1 - $3; }
	| expr '+' expr { $$ = $1 + $3; }
	| expr '/' expr { $$ = $1 / $3; }
	| expr '*' expr { $$ = $1 * $3; }
	| '(' expr ')' { $$ = $2; }
	| '[' expr ']' { $$ = mmu_read_long_print($2); }
	| '[' expr ']' SIZEW { $$ = mmu_read_word_print($2); }
	;
%% 
void yyerror(char *s)
{
  parseerror = EXPR_FAILURE;
}
int expr_parse(LONG *ret, char *str)
{
  char *tmp;

  parseerror = EXPR_SUCCESS;

  tmp = (char *)malloc(strlen(str)+2);
  strcpy(tmp, str);
  strcat(tmp, "\n");
  expr_set_inputstr(tmp);
  yyparse();
  free(tmp);

  if(parseerror == EXPR_FAILURE) return EXPR_FAILURE;
  *ret = addr;
  return EXPR_SUCCESS;
}
