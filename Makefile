CC=gcc
CFLAGS_BASE=`sdl2-config --cflags` -I. -I.. -Wall -Werror -Wno-unused-function
LDFLAGS_BASE=

CFLAGS=$(CFLAGS_EXTRA) $(CFLAGS_BASE)
LDFLAGS=$(LDFLAGS_EXTRA) $(LDFLAGS_BASE)
export CFLAGS
export LDFLAGS

YACC=yacc -d
LEX=lex

PARSER=expr
PARSERFILE=expr.y expr.l
PARSERSRC=expr.tab.c lex.expr.c
PARSEROBJ=expr.tab.o lex.expr.o

SRC=mmu.c ram.c rom.c cpu.c cartridge.c psg.c mfp.c shifter.c screen.c \
    midi.c ikbd.c fdc.c rtc.c floppy.c event.c state.c prefs.c
OBJ=mmu.o ram.o rom.o cpu.o cartridge.o psg.o mfp.o shifter.o screen.o \
    midi.o ikbd.o fdc.o rtc.o floppy.o event.o state.o prefs.o

EMU_SRC=main.c $(SRC)
EMU_OBJ=main.o $(OBJ)

TEST_SRC=test_main.c $(SRC)
TEST_OBJ=test_main.o $(OBJ)

LIBCPU=libcpu.a
LIBDEBUG=libdebug.a

DEPS = $(EMU_OBJ) $(LIBCPU) $(LIBDEBUG) $(PARSEROBJ)

LIBTEST=libtests.a

LIB=-Lcpu -lcpu -Ldebug -ldebug `sdl2-config --libs`
TEST_LIB=-Ltests -ltests -Lcpu -lcpu -Ldebug -ldebug `sdl-config --libs`

TEST_PRG=ostistest

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

all:	default

default:
	make ostis CFLAGS_EXTRA="-O3"

gdb:
	make ostis-gdb CFLAGS_EXTRA="-ggdb"

ostis ostis-gdb: $(DEPS)
	$(CC) $(LDFLAGS) -o $@ $(EMU_OBJ) $(PARSEROBJ) $(LIB)

tests:	$(TEST_PRG)

$(LIBCPU):
	make -C cpu

$(LIBTEST):
	make -C tests

$(LIBDEBUG):
	make -C debug

$(PARSEROBJ):	$(PARSERSRC)

$(PARSERSRC): $(PARSERFILE)
	$(LEX) $(PARSER).l
	$(YACC) -b $(PARSER) $(PARSER).y

# $(YSRC): $(YACCFILE)
#	$(YACC) $<

# $(LOBJ): $(LSRC) $(YSRC)
#	$(CC) $(CFLAGS) -c $<

# $(YOBJ): $(YSRC)
# 	$(CC) $(CFLAGS) -c $<

# $(LSRC): $(LEXFILE) expr.tab.h
#	$(LEX) $<

$(TEST_PRG):	$(TEST_OBJ) $(LIBTEST) $(LIBCPU) $(LIBDEBUG) $(LOBJ) $(YOBJ)
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJ) $(LOBJ) $(YOBJ) $(TEST_LIB)

clean:
	make -C cpu clean
	make -C debug clean
	rm -f *.o *~ $(PARSERSRC) expr.tab.h
