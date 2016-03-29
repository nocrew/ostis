TESTS_SRC=$(addprefix tests/,test_moveq.c test_roxl.c test_roxr.c test_lsl.c test_prefetch1.c test_ccpu_movem.c)
TESTS_OBJ=$(TESTS_SRC:.c=.o)

-include $(TESTS_SRC:.c=.d)

$(LIBTESTS): $(TESTS_OBJ)
	$(AR) cru $@ $(TESTS_OBJ)
	$(RANLIB) $@

clean::
	rm -f $(TESTS_OBJ) $(LIBTESTS) tests/*~ tests/*.d
