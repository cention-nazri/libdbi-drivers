docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

if HAVE_MYSQL

if BUILDDOCS
# don't use docdir, doc_DATA. This way we keep the HTML in separate subdirs
# for each driver
htdocdir = $(docdir)/dbd_mysql
doc_DATA = dbd_mysql.pdf
htdoc_DATA = dbd_mysql/* 
else
htdocdir =
doc_DATA =
htdoc_DATA =
endif

mysql_ltlibs = libdbdmysql.la
mysql_ldflags = -module -avoid-version
mysql_sources = dbd_mysql.c

else

htdocdir =
doc_DATA =
htdoc_DATA =

mysql_ltlibs =
mysql_ldflags =
mysql_sources =

endif

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include @DBI_INCLUDE@ @MYSQL_INCLUDE@

driver_LTLIBRARIES = $(mysql_ltlibs)
libdbdmysql_la_LDFLAGS = $(mysql_ldflags)
libdbdmysql_la_LIBADD = @MYSQL_LDFLAGS@ @MYSQL_LIBS@ @LIBADD_LIBDBI@
libdbdmysql_la_SOURCES = $(mysql_sources)
libdbdmysql_la_DEPENDENCIES = dbd_mysql.h

EXTRA_DIST = dbd_mysql.c dbd_mysql.h AUTHORS TODO README dbd_mysql.sgml dbd_mysql.pdf $(htdoc_DATA)

# jadetex spews out tons of useless crap. shut it up:
dbd_mysql.pdf: dbd_mysql.sgml
	@echo "Converting dbd_mysql.sgml to PDF...";
	$(myjade) -t tex -V tex-backend -d $(top_srcdir)/doc/include/doc-print.dsl $<
# run jadetex three times to get the references right
	$(mypdftex) "&pdfjadetex" dbd_mysql.tex
	$(mypdftex) "&pdfjadetex" dbd_mysql.tex
	$(mypdftex) "&pdfjadetex" dbd_mysql.tex

dbd_mysql/*: dbd_mysql.sgml
	@echo "Converting dbd_mysql.sgml to HTML...";
	mkdir -p dbd_mysql && cd dbd_mysql && $(myjade) -t sgml -d $(top_srcdir)/../doc/include/doc-html.dsl ${abs_srcdir}/dbd_mysql.sgml


maintainer-clean: clean
	rm -f dbd_mysql.pdf dbd_mysql/*

