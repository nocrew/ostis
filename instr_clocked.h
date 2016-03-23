#ifndef INSTR_CLOCKED_H
#define INSTR_CLOCKED_H

void move_to_sr_instr_init(void *[], void *[]);
void reset_instr_init(void *[], void *[]);
void cmpi_instr_init(void *[], void *[]);
void bcc_instr_init(void *[], void *[]);
void lea_instr_init(void *[], void *[]);
void suba_instr_init(void *[], void *[]);
void jmp_instr_init(void *[], void *[]);
void move_instr_init(void *[], void *[]);
void btst_instr_init(void *[], void *[]);
void moveq_instr_init(void *[], void *[]);
void cmp_instr_init(void *[], void *[]);
void dbcc_instr_init(void *[], void *[]);
void clr_instr_init(void *[], void *[]);
void movea_instr_init(void *[], void *[]);
void add_instr_init(void *[], void *[]);
void cmpa_instr_init(void *[], void *[]);
void lsr_instr_init(void *[], void *[]);
void adda_instr_init(void *[], void *[]);
void addq_instr_init(void *[], void *[]);
void sub_instr_init(void *[], void *[]);
void scc_instr_init(void *[], void *[]);
void movep_instr_init(void *[], void *[]);
void movem_instr_init(void *[], void *[]);
void rts_instr_init(void *[], void *[]);
void and_instr_init(void *[], void *[]);
void exg_instr_init(void *[], void *[]);
void or_instr_init(void *[], void *[]);
void subq_instr_init(void *[], void *[]);
void bclr_instr_init(void *[], void *[]);
void asl_instr_init(void *[], void *[]);
void addi_instr_init(void *[], void *[]);
void bset_instr_init(void *[], void *[]);
void move_from_sr_instr_init(void *[], void *[]);
void ori_instr_init(void *[], void *[]);
void ori_to_sr_instr_init(void *[], void *[]);
void andi_instr_init(void *[], void *[]);
void andi_to_sr_instr_init(void *[], void *[]);
void move_usp_instr_init(void *[], void *[]);
void lsl_instr_init(void *[], void *[]);
void trap_instr_init(void *[], void *[]);
void tst_instr_init(void *[], void *[]);
void jsr_instr_init(void *[], void *[]);
void mulu_instr_init(void *[], void *[]);
void divu_instr_init(void *[], void *[]);
void link_instr_init(void *[], void *[]);
void rte_instr_init(void *[], void *[]);
void rtr_instr_init(void *[], void *[]);
void unlk_instr_init(void *[], void *[]);
void move_from_ccr_instr_init(void *[], void *[]);
void movec_instr_init(void *[], void *[]);
void swap_instr_init(void *[], void *[]);
void pea_instr_init(void *[], void *[]);
void ext_instr_init(void *[], void *[]);
void muls_instr_init(void *[], void *[]);
void asr_instr_init(void *[], void *[]);
void eor_instr_init(void *[], void *[]);
void eori_instr_init(void *[], void *[]);
void eori_to_sr_instr_init(void *[], void *[]);
void nop_instr_init(void *[], void *[]);
void linef_instr_init(void *[], void *[]);
void divs_instr_init(void *[], void *[]);
void rol_instr_init(void *[], void *[]);
void roxl_instr_init(void *[], void *[]);
void not_instr_init(void *[], void *[]);
void ror_instr_init(void *[], void *[]);
void neg_instr_init(void *[], void *[]);
void subi_instr_init(void *[], void *[]);
void move_to_ccr_instr_init(void *[], void *[]);
void ori_to_ccr_instr_init(void *[], void *[]);
void addx_instr_init(void *[], void *[]);
void cmpm_instr_init(void *[], void *[]);
void subx_instr_init(void *[], void *[]);
void eori_to_ccr_instr_init(void *[], void *[]);
void negx_instr_init(void *[], void *[]);
void linea_instr_init(void *[], void *[]);
void roxr_instr_init(void *[], void *[]);
void bchg_instr_init(void *[], void *[]);
void abcd_instr_init(void *[], void *[]);
void stop_instr_init(void *[], void *[]);
void sbcd_instr_init(void *[], void *[]);
void tas_instr_init(void *[], void *[]);
void andi_to_ccr_instr_init(void *[], void *[]);

/*
 * Unimplemented:
 * 
 * NBCD
 * CHK
 * TRAPV
 * 
 */

#endif
