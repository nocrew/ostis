%{
  #include <stdlib.h>
  #include <string.h>
  #include "common.h"
  void yyerror(char *);
  #include "y.tab.h"
  
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
%%
a[0-7] {
  yylval.val = yytext[1]-'0';
  return AREG;
}
d[0-7] {
  yylval.val = yytext[1]-'0';
  return DREG;
}
\.w {
  return SIZEW;
}
pc {
  return PC;
}
sr {
  return SR;
}
m[0-9] {
  yylval.val = yytext[1]-'0';
  return WIN;
}
\$[0-9a-fA-F]+ {
  yylval.val = strtol(&yytext[1], NULL, 16);
  return VAL;
}
[0-9a-fA-F]+ {
  yylval.val = strtol(yytext, NULL, 16);
  return VAL;
}
\\[0-9]+ {
  yylval.val = strtol(&yytext[1], NULL, 10);
  return VAL;
}
[a-zA-Z][a-zA-Z0-9_]+ {
  yylval.str = strdup(yytext);
  return LABEL;
}
[()\[\]+\-*/=\n] {
  return *yytext;
}
%%
int yywrap(void)
{
  return 1;
}
void expr_set_inputstr(char *str)
{
  inputpos = 0;
  inputstr = str;
}


