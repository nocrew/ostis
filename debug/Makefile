CC=gcc
AR=ar
#CFLAGS=-Os -mcpu=603 -mtune=603 -I.. -Wall -Werror -Wno-unused-function
#LDFLAGS=-O3
CFLAGS=-g -Wall -Werror -Wno-unused-function -I..
LDFLAGS=-g

SRC=debug.c display.c draw.c edit.c layout.c win.c
OBJ=debug.o display.o draw.o edit.o layout.o win.o
LIB=

LIBDEBUG=libdebug.a

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(LIBDEBUG)

$(LIBDEBUG): $(OBJ)
	$(AR) cru $@ $(OBJ)

clean:
	rm -f *.o *~
