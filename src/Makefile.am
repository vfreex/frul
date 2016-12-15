AM_CPPFLAGS = -I $(top_srcdir)/include

lib_LTLIBRARIES = libfrul.la
libfrul_la_SOURCES = frul.c

bin_PROGRAMS = frulrun

frulrun_SOURCES = main.c
frulrun_LDADD = libfrul.la
