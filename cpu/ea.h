#ifndef EA_H
#define EA_H

#include "common.h"
#include "cpu.h"
#include "mmu.h"

#define MODESWAP(x) (((x&0x38)>>3)|((x&0x7)<<3))
#define ADD_CYCLE_EA(x) do { if(!rmw) { cpu->icycle += x; } } while(0);

BYTE ea_read_byte(struct cpu *,int,int);
WORD ea_read_word(struct cpu *,int,int);
LONG ea_read_long(struct cpu *,int,int);
void ea_write_byte(struct cpu *,int,BYTE);
void ea_write_word(struct cpu *,int,WORD);
void ea_write_long(struct cpu *,int,LONG);
LONG ea_get_addr(struct cpu *,int);
void ea_print(struct cprint *,int, int);

#endif
