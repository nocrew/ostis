CC=gcc
AR=ar
#CFLAGS=-Os -mcpu=603 -mtune=603 -I.. -Wall -Werror -Wno-unused-function
#LDFLAGS=-O3
CFLAGS=-g -Wall -Werror -Wno-unused-function -I..
LDFLAGS=-g

SRC=ea.c move_to_sr.c reset.c cmpi.c bcc.c lea.c suba.c jmp.c move.c btst.c \
    moveq.c cmp.c dbcc.c clr.c movea.c add.c cmpa.c lsr.c adda.c addq.c \
    sub.c scc.c movep.c movem.c rts.c and.c exg.c or.c subq.c bclr.c asl.c \
    addi.c bset.c move_from_sr.c ori.c ori_to_sr.c andi.c andi_to_sr.c \
    move_usp.c lsl.c trap.c tst.c jsr.c mulu.c divu.c link.c rte.c unlk.c \
    swap.c pea.c ext.c muls.c asr.c eor.c eori.c nop.c linef.c divs.c \
    eori_to_sr.c rol.c roxl.c not.c ror.c neg.c subi.c move_to_ccr.c \
    ori_to_ccr.c addx.c cmpm.c subx.c eori_to_ccr.c negx.c linea.c roxr.c \
    bchg.c abcd.c
OBJ=ea.o move_to_sr.o reset.o cmpi.o bcc.o lea.o suba.o jmp.o move.o btst.o \
    moveq.o cmp.o dbcc.o clr.o movea.o add.o cmpa.o lsr.o adda.o addq.o \
    sub.o scc.o movep.o movem.o rts.o and.o exg.o or.o subq.o bclr.o asl.o \
    addi.o bset.o move_from_sr.o ori.o ori_to_sr.o andi.o andi_to_sr.o \
    move_usp.o lsl.o trap.o tst.o jsr.o mulu.o divu.o link.o rte.o unlk.o \
    swap.o pea.o ext.o muls.o asr.o eor.o eori.o nop.o linef.o divs.o \
    eori_to_sr.o rol.o roxl.o not.o ror.o neg.o subi.o move_to_ccr.o \
    ori_to_ccr.o addx.o cmpm.o subx.o eori_to_ccr.o negx.o linea.o roxr.o \
    bchg.o abcd.o
LIB=

LIBCPU=libcpu.a

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(LIBCPU)

$(LIBCPU): $(OBJ)
	$(AR) cru $@ $(OBJ)

clean:
	rm -f *.o *~
