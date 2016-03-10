DEBUG_SRC=$(addprefix debug/,debug.c display.c draw.c edit.c layout.c win.c)
DEBUG_OBJ=$(DEBUG_SRC:.c=.o)

-include $(DEBUG_SRC:.c=.d)

$(LIBDEBUG): $(DEBUG_OBJ)
	$(AR) cru $@ $(DEBUG_OBJ)
	$(RANLIB) $@

clean::
	rm -f $(DEBUG_OBJ) $(LIBDEBUG) debug/*~ debug/*.d
