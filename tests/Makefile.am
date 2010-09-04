## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = cgreen

TESTS = @MYSQL_TEST@ @PGSQL_TEST@ @SQLITE_TEST@ @SQLITE3_TEST@ @MSQL_TEST@ @ORACLE_TEST@ @FIREBIRD_TEST@ @FREETDS_TEST@ @INGRES_TEST@ @DB2_TEST@
check_PROGRAMS = test_dbi
test_dbi_SOURCES = test_dbi.c
test_dbi_LDADD = $(top_srcdir)/tests/cgreen/libcgreen.a -L@libdir@ -lm -ldbi
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/tests/cgreen/include -I@includedir@
CFLAGS = -DDBI_DRIVER_DIR=\"@driverdir@\" -g
AM_CPPFLAGS = -DDBDIR=\"@dbi_dbdir@\"

EXTRA_DIST = test_mysql.sh test_pgsql.sh test_sqlite.sh test_sqlite3.sh test_msql.sh test_oracle.sh test_firebird.sh test_freetds.sh test_ingres.sh test_db2.sh

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

