%token <str> PC SR LABEL SIZEW SIZEB SP SSP USP
%token <val> AREG DREG VAL WIN
%token EQ NE LT LE GT GE BAND LAND BOR LOR BXOR BNOT LNOT
%type <val> expr
%name-prefix = "expr"
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
  int exprdebug = 1;
  void exprerror(char *);
  int exprlex(void);
  static LONG addr;
  static long parseerror;

  int exprparse();
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
	| SP { $$ = cpu->a[7]; }
	| SSP { if(CHKS) { $$ = cpu->a[7]; } else { $$ = cpu->ssp; } }
	| USP { if(!CHKS) { $$ = cpu->a[7]; } else { $$ = cpu->usp; } }
	| WIN { $$ = debug_win_addr($1); }
	| LABEL { if(cprint_label_exists($1)) {
                    $$ = cprint_label_addr($1);
		  } else {
		    exprerror("foobar");
		  }
	          free($1);
		}
	| expr SIZEW { $$ = $1 & 0xffff; }
	| expr SIZEB { $$ = $1 & 0xff; }
	| expr '-' expr { $$ = $1 - $3; }
	| expr '+' expr { $$ = $1 + $3; }
	| expr '/' expr { $$ = $1 / $3; }
	| expr '*' expr { $$ = $1 * $3; }
	| '(' expr ')' { $$ = $2; }
	| '[' expr ']' { $$ = mmu_read_long_print($2); }
	| '[' expr ']' SIZEW { $$ = mmu_read_word_print($2); }
	| '[' expr ']' SIZEB { $$ = mmu_read_byte_print($2); }
	| expr EQ expr { $$ = ($1 == $3); }
	| expr NE expr { $$ = ($1 != $3); }
	| expr LT expr { $$ = ($1 < $3); }
	| expr LE expr { $$ = ($1 <= $3); }
	| expr GT expr { $$ = ($1 > $3); }
	| expr GE expr { $$ = ($1 >= $3); }
	| expr BXOR expr { $$ = ($1 ^ $3); }
	| expr BOR expr { $$ = ($1 | $3); }
	| expr BAND expr { $$ = ($1 & $3); }
	| expr LOR expr { $$ = ($1 || $3); }
	| expr LAND expr { $$ = ($1 && $3); }
	| BNOT expr { $$ = ~ $2; }
	| LNOT expr { $$ = ! $2; }
	;
%% 
void exprerror(char *s)
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
  exprparse();
  free(tmp);

  if(parseerror == EXPR_FAILURE) return EXPR_FAILURE;
  *ret = addr;
  return EXPR_SUCCESS;
}
