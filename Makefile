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

SRC=mmu.c mmu_fallback.c ram.c rom.c cpu.c cartridge.c psg.c mfp.c shifter.c screen.c \
    midi.c ikbd.c fdc.c rtc.c floppy.c floppy_st.c floppy_msa.c floppy_stx.c event.c \
    state.c prefs.c cprint.c diag.c
SRC_TEST=tests/test_main.c
OBJ=$(SRC:.c=.o)

EMU_SRC=main.c $(SRC)
EMU_OBJ=$(EMU_SRC:.c=.o)
EMU_TEST_SRC=$(EMU_SRC) $(SRC_TEST)
EMU_TEST_OBJ=$(EMU_TEST_SRC:.c=.o)

LIBCPU=cpu/libcpu.a
LIBDEBUG=debug/libdebug.a
LIBTESTS=tests/libtests.a

DEPS = $(EMU_OBJ) $(LIBCPU) $(LIBDEBUG) $(PARSEROBJ)
DEPS_TEST = $(EMU_TEST_OBJ) $(LIBCPU) $(LIBDEBUG) $(PARSEROBJ) $(LIBTESTS)

LIB=$(LIBCPU) $(LIBDEBUG) `sdl2-config --libs`

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

all:	default

default:
	$(MAKE) ostis CFLAGS_EXTRA="-O3"

gdb:
	$(MAKE) ostis-gdb CFLAGS_EXTRA="-ggdb"

test:
	$(MAKE) ostis-test CFLAGS_EXTRA="-ggdb -DTEST_BUILD"

ostis ostis-gdb: $(DEPS)
	$(CC) $(LDFLAGS) -o $@ $(EMU_OBJ) $(PARSEROBJ) $(LIB)

ostis-test:	$(DEPS_TEST)
	$(CC) $(LDFLAGS) -o $@ $(EMU_TEST_OBJ) $(PARSEROBJ) $(LIB) $(LIBTESTS)

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

include cpu/cpu.mk
include debug/debug.mk
include tests/tests.mk

clean::
	rm -f *.o *~ $(PARSERSRC) expr.tab.h
