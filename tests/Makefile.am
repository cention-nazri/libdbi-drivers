## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = cgreen

TESTS = test_dbi
check_PROGRAMS = test_dbi
test_dbi_SOURCES = test_dbi.c
test_dbi_LDADD = $(top_srcdir)/tests/cgreen/libcgreen.a -L@libdir@ -lm -ldbi
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/tests/cgreen/include -I@includedir@
CFLAGS = -DDBI_DRIVER_DIR=\"@driverdir@\" -g
AM_CPPFLAGS = -DDBDIR=\"@dbi_dbdir@\"

EXTRA_DIST =

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

