docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

if HAVE_SQLITE

if BUILDDOCS
htdocdir = $(docdir)/dbd_sqlite
doc_DATA = dbd_sqlite.pdf
htdoc_DATA = dbd_sqlite/* 
else
htdocdir =
doc_DATA =
htdoc_DATA =
endif

sqlite_ltlibs = libdbdsqlite.la
sqlite_ldflags = -module -avoid-version
sqlite_sources = dbd_sqlite.c

else

htdocdir =
doc_DATA =
htdoc_DATA =

sqlite_ltlibs =
sqlite_ldflags =
sqlite_sources =

endif

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include @DBI_INCLUDE@ @SQLITE_INCLUDE@

driver_LTLIBRARIES = $(sqlite_ltlibs)
libdbdsqlite_la_LDFLAGS = $(sqlite_ldflags) @SQLITE_LDFLAGS@ @SQLITE_LIBS@ @LIBADD_LIBDBI@
libdbdsqlite_la_SOURCES = $(sqlite_sources)
libdbdsqlite_la_DEPENDENCIES = dbd_sqlite.h

AM_CPPFLAGS += -DDBDIR=\"@dbi_dbdir@/sqlite\"

EXTRA_DIST = dbd_sqlite.c dbd_sqlite.h AUTHORS TODO README dbd_sqlite.sgml dbd_sqlite.pdf dbd_sqlite/*

# jadetex spews out tons of useless crap. shut it up:
dbd_sqlite.pdf: dbd_sqlite.sgml
	@echo "Converting dbd_sqlite.sgml to PDF...";
	$(myjade) -t tex -V tex-backend -d $(top_srcdir)/doc/include/doc-print.dsl $<
# run jadetex three times to get the references right
	$(mypdftex) "&pdfjadetex" dbd_sqlite.tex
	$(mypdftex) "&pdfjadetex" dbd_sqlite.tex
	$(mypdftex) "&pdfjadetex" dbd_sqlite.tex

dbd_sqlite/*: dbd_sqlite.sgml
	@echo "Converting dbd_sqlite.sgml to HTML...";
	mkdir -p dbd_sqlite && cd dbd_sqlite && $(myjade) -t sgml -d $(top_srcdir)/../doc/include/doc-html.dsl ${abs_srcdir}/dbd_sqlite.sgml


maintainer-clean: clean
	rm -f dbd_sqlite.pdf dbd_sqlite/*

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)@dbi_dbdir@/sqlite	
	chmod 1777 $(DESTDIR)@dbi_dbdir@/sqlite
