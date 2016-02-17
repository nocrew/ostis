CPU_SRC=$(addprefix cpu/,ea.c move_to_sr.c reset.c cmpi.c bcc.c lea.c	\
    suba.c jmp.c move.c btst.c moveq.c cmp.c dbcc.c clr.c movea.c	\
    add.c cmpa.c lsr.c adda.c addq.c sub.c scc.c movep.c movem.c	\
    rts.c and.c exg.c or.c subq.c bclr.c asl.c addi.c bset.c		\
    move_from_sr.c ori.c ori_to_sr.c andi.c andi_to_sr.c move_usp.c	\
    lsl.c trap.c tst.c jsr.c mulu.c divu.c link.c rte.c unlk.c swap.c	\
    pea.c ext.c muls.c asr.c eor.c eori.c nop.c linef.c divs.c		\
    eori_to_sr.c rol.c roxl.c not.c ror.c neg.c subi.c move_to_ccr.c	\
    ori_to_ccr.c addx.c cmpm.c subx.c eori_to_ccr.c negx.c linea.c	\
    roxr.c bchg.c abcd.c stop.c sbcd.c tas.c andi_to_ccr.c rtr.c)
CPU_OBJ=$(CPU_SRC:.c=.o)

$(LIBCPU): $(CPU_OBJ)
	$(AR) cru $@ $(CPU_OBJ)
	$(RANLIB) $@

clean::
	rm -f $(CPU_OBJ) $(LIBCPU) cpu/*~
