#ifndef EXPR_H
#define EXPR_H

#include "common.h"

#define EXPR_SUCCESS 0
#define EXPR_FAILURE 1

void expr_set_inputstr(char *);
int expr_parse(LONG *,char *);

#endif
