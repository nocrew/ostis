#ifndef HDC_H
#define HDC_H

#include "common.h"

BYTE hdc_get_status();
void hdc_add_cmddata(BYTE);
void hdc_set_cmd(BYTE);
void hdc_init(char *);

#endif
