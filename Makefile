CC=gcc
CFLAGS_BASE=`sdl2-config --cflags` -I. -I.. -Wall -Werror -Wno-unused-function
LDFLAGS_BASE=

CFLAGS=$(CFLAGS_EXTRA) $(CFLAGS_BASE)
LDFLAGS=$(LDFLAGS_EXTRA) $(LDFLAGS_BASE)
export CFLAGS
export LDFLAGS

YACC=yacc -d
LEX=lex
AR=ar
RANLIB=ranlib

PARSER=expr
PARSERFILE=expr.y expr.l
PARSERSRC=expr.tab.c lex.expr.c
PARSEROBJ=$(PARSERSRC:.c=.o)

SRC=mmu.c ram.c rom.c cpu.c cartridge.c psg.c mfp.c shifter.c screen.c \
    midi.c ikbd.c fdc.c rtc.c floppy.c event.c state.c prefs.c
OBJ=$(SRC:.c=.o)

EMU_SRC=main.c $(SRC)
EMU_OBJ=$(EMU_SRC:.c=.o)

TEST_SRC=test_main.c $(SRC)
TEST_OBJ=$(TEST_SRC:.c=.o)

LIBCPU=cpu/libcpu.a
LIBDEBUG=debug/libdebug.a

DEPS = $(EMU_OBJ) $(LIBCPU) $(LIBDEBUG) $(PARSEROBJ)

LIBTEST=libtests.a

LIB=$(LIBCPU) $(LIBDEBUG) `sdl2-config --libs`
TEST_LIB=-Ltests -ltests $(LIBCPU) $(LIBDEBUG) `sdl-config --libs`

TEST_PRG=ostistest

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

all:	default

default:
	$(MAKE) ostis CFLAGS_EXTRA="-O3"

gdb:
	$(MAKE) ostis-gdb CFLAGS_EXTRA="-ggdb"

ostis ostis-gdb: $(DEPS)
	$(CC) $(LDFLAGS) -o $@ $(EMU_OBJ) $(PARSEROBJ) $(LIB)

tests:	$(TEST_PRG)

$(LIBTEST):
	make -C tests

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

include cpu/cpu.mk
include debug/debug.mk

clean::
	rm -f *.o *~ $(PARSERSRC) expr.tab.h
