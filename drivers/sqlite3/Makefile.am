docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

if HAVE_SQLITE3

if BUILDDOCS
htdocdir = $(docdir)/dbd_sqlite3
doc_DATA = dbd_sqlite3.pdf
htdoc_DATA = dbd_sqlite3/* 
else
htdocdir =
doc_DATA =
htdoc_DATA =
endif

sqlite3_ltlibs = libdbdsqlite3.la
sqlite3_ldflags = -module -avoid-version
sqlite3_sources = dbd_sqlite3.c

else

htdocdir =
doc_DATA =
htdoc_DATA =

sqlite3_ltlibs =
sqlite3_ldflags =
sqlite3_sources =

endif

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include @DBI_INCLUDE@ @SQLITE3_INCLUDE@

driver_LTLIBRARIES = $(sqlite3_ltlibs)
libdbdsqlite3_la_LDFLAGS = $(sqlite3_ldflags)
libdbdsqlite3_la_LIBADD = @SQLITE3_LDFLAGS@ @SQLITE3_LIBS@ @LIBADD_LIBDBI@
libdbdsqlite3_la_SOURCES = $(sqlite3_sources)
libdbdsqlite3_la_DEPENDENCIES = dbd_sqlite3.h

AM_CPPFLAGS += -DDBDIR=\"@dbi_dbdir@/sqlite3\"

EXTRA_DIST = dbd_sqlite3.c dbd_sqlite3.h AUTHORS TODO README dbd_sqlite3.sgml dbd_sqlite3.pdf dbd_sqlite3/*

# jadetex spews out tons of useless crap. shut it up:
dbd_sqlite3.pdf: dbd_sqlite3.sgml
	@echo "Converting dbd_sqlite3.sgml to PDF...";
	$(myjade) -t tex -V tex-backend -d $(top_srcdir)/doc/include/doc-print.dsl $<
# run jadetex three times to get the references right
	$(mypdftex) "&pdfjadetex" dbd_sqlite3.tex
	$(mypdftex) "&pdfjadetex" dbd_sqlite3.tex
	$(mypdftex) "&pdfjadetex" dbd_sqlite3.tex

dbd_sqlite3/*: dbd_sqlite3.sgml
	@echo "Converting dbd_sqlite3.sgml to HTML...";
	mkdir -p dbd_sqlite3 && cd dbd_sqlite3 && $(myjade) -t sgml -d $(top_srcdir)/../doc/include/doc-html.dsl ${abs_srcdir}/dbd_sqlite3.sgml


maintainer-clean: clean
	rm -f dbd_sqlite3.pdf dbd_sqlite3/*

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)@dbi_dbdir@/sqlite3
	chmod 1777 $(DESTDIR)@dbi_dbdir@/sqlite3
