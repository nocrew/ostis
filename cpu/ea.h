#ifndef EA_H
#define EA_H

#include "common.h"
#include "cpu.h"
#include "mmu.h"

#define MODESWAP(x) (((x&0x38)>>3)|((x&0x7)<<3))
#define ADD_CYCLE_EA(x) do { if(!rmw) { cpu->icycle += x; } } while(0);

#define EA_INVALID_NONE 0x0
#define EA_INVALID_A    0x1
#define EA_INVALID_D    0x2
#define EA_INVALID_I    0x4
#define EA_INVALID_PC   0x8
#define EA_INVALID_INC  0x10
#define EA_INVALID_DEC  0x20

#define EA_INVALID_DST  (EA_INVALID_I|EA_INVALID_PC)
#define EA_INVALID_MEM  (EA_INVALID_DST|EA_INVALID_A|EA_INVALID_D)
#define EA_INVALID_EA   (EA_INVALID_A|EA_INVALID_D|EA_INVALID_INC| \
			 EA_INVALID_DEC|EA_INVALID_I)

BYTE ea_read_byte(struct cpu *,int,int);
WORD ea_read_word(struct cpu *,int,int);
LONG ea_read_long(struct cpu *,int,int);
void ea_write_byte(struct cpu *,int,BYTE);
void ea_write_word(struct cpu *,int,WORD);
void ea_write_long(struct cpu *,int,LONG);
LONG ea_get_addr(struct cpu *,int);
void ea_print(struct cprint *,int, int);
int ea_valid(int, int);

#endif
