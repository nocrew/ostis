CC=gcc
AR=ar
RANLIB=ranlib

SRC=debug.c display.c draw.c edit.c layout.c win.c
OBJ=debug.o display.o draw.o edit.o layout.o win.o
LIB=

LIBDEBUG=libdebug.a

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(LIBDEBUG)

$(LIBDEBUG): $(OBJ)
	$(AR) cru $@ $(OBJ)
	$(RANLIB) $@

clean:
	rm -f *.o *~
