%{
  #include <stdlib.h>
  #include <string.h>
  #include "common.h"
  void exprerror(char *);
  #include "expr.tab.h"
  
  static char *inputstr;
  static int inputpos = 0;

  #define YY_INPUT(buf, result, max_size) \
  { \
    if(inputpos > strlen(inputstr)) { \
      result = YY_NULL; \
    } else { \
      buf[0] = inputstr[inputpos++]; \
      result = 1; \
    } \
  }
%}
%option prefix="expr"
%%
[aA][0-7] {
  exprlval.val = exprtext[1]-'0';
  return AREG;
}
[dD][0-7] {
  exprlval.val = exprtext[1]-'0';
  return DREG;
}
\.w {
  return SIZEW;
}
[pP][cC] {
  return PC;
}
[sS][rR] {
  return SR;
}
[sS][pP] {
  return SP;
}
[sS][sS][pP] {
  return SSP;
}
[uU][sS][pP] {
  return USP;
}
[mM][0-9] {
  exprlval.val = exprtext[1]-'0';
  return WIN;
}
\$[0-9a-fA-F]+ {
  exprlval.val = strtol(&exprtext[1], NULL, 16);
  return VAL;
}
[0-9a-fA-F]+ {
  exprlval.val = strtol(exprtext, NULL, 16);
  return VAL;
}
\\[0-9]+ {
  exprlval.val = strtol(&exprtext[1], NULL, 10);
  return VAL;
}
= { return EQ; }
== { return EQ; }
!= { return NE; }
[<] { return LT; }
[<]= { return LE; }
> { return GT; }
>= { return GE; }
& { return BAND; }
&& { return LAND; }
[|] { return BOR; }
[|][|] { return LOR; }
[\^] { return BXOR; }
~ { return BNOT; }
! { return LNOT; }
[a-zA-Z][a-zA-Z0-9_]+ {
  exprlval.str = strdup(exprtext);
  return LABEL;
}
[()\[\]+\-*/\n] {
  return *exprtext;
}
%%
int exprwrap(void)
{
  return 1;
}
void expr_set_inputstr(char *str)
{
  inputpos = 0;
  inputstr = str;
}


