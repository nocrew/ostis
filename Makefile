CC=gcc
#CFLAGS=-Os -I. -I.. -mcpu=603 -mtune=603 -Wall -Werror -Wno-unused-function
#LDFLAGS=-Os
CFLAGS=-g -I. -I.. -Wall -Werror -Wno-unused-function
LDFLAGS=-g
YACC=yacc -d
LEX=lex

YACCFILE=expr.y
LEXFILE=expr.l
YSRC=y.tab.c
LSRC=lex.yy.c
YOBJ=y.tab.o
LOBJ=lex.yy.o

SRC=main.c mmu.c ram.c rom.c cpu.c cartridge.c psg.c mfp.c shifter.c screen.c \
    midi.c ikbd.c fdc.c rtc.c floppy.c
OBJ=main.o mmu.o ram.o rom.o cpu.o cartridge.o psg.o mfp.o shifter.o screen.o \
    midi.o ikbd.o fdc.o rtc.o floppy.o
LIBCPU=libcpu.a
LIBDEBUG=libdebug.a
LIB=-Lcpu -lcpu -Ldebug -ldebug -lSDL -lpthread

PRG=ostis

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(PRG)

$(LIBCPU):
	make -C cpu

$(LIBDEBUG):
	make -C debug

$(PRG): $(OBJ) $(LIBCPU) $(LIBDEBUG) $(LOBJ) $(YOBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LOBJ) $(YOBJ) $(LIB)

y.tab.h: $(YSRC)

$(YSRC): $(YACCFILE)
	$(YACC) $<

$(LOBJ): $(LSRC) $(YSRC)
	$(CC) $(CFLAGS) -c $<

$(YOBJ): $(YSRC)
	$(CC) $(CFLAGS) -c $<

$(LSRC): $(LEXFILE) y.tab.h
	$(LEX) $<

clean:
	make -C cpu clean
	make -C debug clean
	rm -f *.o *~ $(YSRC) $(LSRC) y.tab.h
