CC=gcc
include Makefile.include
#CFLAGS=-O3 -I. -I.. -Wall -Werror -Wno-unused-function
#LDFLAGS=-O3
#CFLAGS=-g -I. -I.. -Wall -Werror -Wno-unused-function
#LDFLAGS=-g
YACC=yacc -d
LEX=lex

YACCFILE=expr.y
LEXFILE=expr.l
YSRC=y.tab.c
LSRC=lex.yy.c
YOBJ=y.tab.o
LOBJ=lex.yy.o

SRC=mmu.c ram.c rom.c cpu.c cartridge.c psg.c mfp.c shifter.c screen.c \
    midi.c ikbd.c fdc.c rtc.c floppy.c event.c state.c
OBJ=mmu.o ram.o rom.o cpu.o cartridge.o psg.o mfp.o shifter.o screen.o \
    midi.o ikbd.o fdc.o rtc.o floppy.o event.o state.o

EMU_SRC=main.c $(SRC)
EMU_OBJ=main.o $(OBJ)

TEST_SRC=test_main.c $(SRC)
TEST_OBJ=test_main.o $(OBJ)

LIBCPU=libcpu.a
LIBDEBUG=libdebug.a

LIBTEST=libtests.a

LIB=-Lcpu -lcpu -Ldebug -ldebug `sdl-config --libs`
TEST_LIB=-Ltests -ltests -Lcpu -lcpu -Ldebug -ldebug `sdl-config --libs`

PRG=ostis
TEST_PRG=ostistest

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(PRG)

tests:	$(TEST_PRG)

$(LIBCPU):
	make -C cpu

$(LIBTEST):
	make -C tests

$(LIBDEBUG):
	make -C debug

$(PRG): $(EMU_OBJ) $(LIBCPU) $(LIBDEBUG) $(LOBJ) $(YOBJ)
	$(CC) $(LDFLAGS) -o $@ $(EMU_OBJ) $(LOBJ) $(YOBJ) $(LIB)

y.tab.h: $(YSRC)

$(YSRC): $(YACCFILE)
	$(YACC) $<

$(LOBJ): $(LSRC) $(YSRC)
	$(CC) $(CFLAGS) -c $<

$(YOBJ): $(YSRC)
	$(CC) $(CFLAGS) -c $<

$(LSRC): $(LEXFILE) y.tab.h
	$(LEX) $<

$(TEST_PRG):	$(TEST_OBJ) $(LIBTEST) $(LIBCPU) $(LIBDEBUG) $(LOBJ) $(YOBJ)
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJ) $(LOBJ) $(YOBJ) $(TEST_LIB)

clean:
	make -C cpu clean
	make -C debug clean
	rm -f *.o *~ $(YSRC) $(LSRC) y.tab.h
