CC=gcc
AR=ar
include ../Makefile.include
#CFLAGS=-O3 -I.. -Wall -Werror -Wno-unused-function
#LDFLAGS=-O3
#CFLAGS=-g -Wall -Werror -Wno-unused-function -I..
#LDFLAGS=-g

SRC=move_immediate.c
OBJ=move_immediate.o
LIB=

LIBCPUTEST=libtests.a

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(LIBCPUTEST)

$(LIBCPUTEST): $(OBJ)
	$(AR) cru $@ $(OBJ)

clean:
	rm -f *.o *~
