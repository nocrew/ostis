TESTS_SRC=$(addprefix tests/,test_moveq.c)
TESTS_OBJ=$(TESTS_SRC:.c=.o)

$(LIBTESTS): $(TESTS_OBJ)
	$(AR) cru $@ $(TESTS_OBJ)
	$(RANLIB) $@

clean::
	rm -f $(TESTS_OBJ) $(LIBTESTS) tests/*~
