## Makefile.am for examples/form_completion

MAINTAINERCLEANFILES = Makefile.in

DEFAULT_INCLUDES = -I. -I$(top_srcdir)/include

ulib_la = @ULIBS@ $(top_builddir)/src/ulib/lib@ULIB@.la @ULIB_LIBS@

form_completion_LDADD   = $(ulib_la)
form_completion_SOURCES = main.cpp
form_completion_LDFLAGS = $(PRG_LDFLAGS)

noinst_PROGRAMS = form_completion

clean-local:
	-rm -rf core .libs *.bb* *.da *.gc* *.la *.exe gmon.out
