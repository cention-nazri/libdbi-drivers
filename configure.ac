AC_INIT([libdbi-drivers], [0.9.0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([drivers/sqlite/dbd_sqlite.c])
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE([foreign dist-zip subdir-objects serial-tests])
AM_CONFIG_HEADER(config.h)
AC_LIBTOOL_WIN32_DLL
AC_CONFIG_MACRO_DIR([m4])

dnl library versioning currently unnecessary...
dnl dnl Library versioning
dnl LIB_CURRENT=0
dnl LIB_REVISION=4
dnl LIB_AGE=0
dnl AC_SUBST(LIB_CURRENT)
dnl AC_SUBST(LIB_REVISION)
dnl AC_SUBST(LIB_AGE)

AC_CANONICAL_HOST

dnl ====================================
dnl Fix CFLAGS to avoid the default -g option
dnl CFLAGS will still be picked up from the environment
dnl ====================================
: ${CFLAGS=""}

dnl ====================================
dnl Check for programs
dnl ====================================

AC_PROG_CC
AC_PROG_LD
AC_PROG_INSTALL
LT_INIT([dlopen])
dnl AM_PROG_LIBTOOL
AC_PROG_CPP

dnl ====================================
dnl Set some general compile options
dnl ====================================
if test -z "$GCC"; then
        case $host in
        *-*-irix*)
                if test -z "$CC"; then
                        CC=cc
                fi
                DEBUG="-g -signed"
                CFLAGS="-O2 -w -signed $CFLAGS"
                PROFILE="-p -g3 -O2 -signed" ;;
        sparc-sun-solaris*)
                DEBUG="-v -g"
                CFLAGS="-xO4 -fast -w -fsimple -native -xcg92 $CFLAGS"
                PROFILE="-v -xpg -g -xO4 -fast -native -fsimple -xcg92 -Dsuncc" ;;
	*-*-freebsd*)
                DEBUG="-g"
                CFLAGS="-O -I/usr/local/include $CFLAGS"
                CPPFLAGS="-I/usr/local/include $CPPFLAGS"
		LDFLAGS="-L/usr/local/lib $LDFLAGS"
                PROFILE="-g -p" ;;
        *)
                DEBUG="-g"
                CFLAGS="-O $CFLAGS"
                PROFILE="-g -p" ;;
        esac
else

        case $host in
        *-*-linux*)
                DEBUG="-g -Wall -D_REENTRANT -D__NO_MATH_INLINES -fsigned-char"
                CFLAGS="-O20 -ffast-math -D_REENTRANT -fsigned-char $CFLAGS"
                PROFILE="-pg -g -O20 -ffast-math -D_REENTRANT -fsigned-char";;
        sparc-sun-*)
                DEBUG="-g -Wall -D__NO_MATH_INLINES -fsigned-char -mv8"
                CFLAGS="-O20 -ffast-math -D__NO_MATH_INLINES -fsigned-char -mv8 $CFLAGS"
                PROFILE="-pg -g -O20 -D__NO_MATH_INLINES -fsigned-char -mv8" ;;
	*-*-freebsd*)
                DEBUG="-g -Wall -D__NO_MATH_INLINES -fsigned-char"
                CFLAGS="-O20 -D__NO_MATH_INLINES -fsigned-char -I/usr/local/include $CFLAGS"
                CPPFLAGS="-I/usr/local/include $CPPFLAGS"
		LDFLAGS="-L/usr/local/lib $LDFLAGS"
                PROFILE="-O20 -g -pg -D__NO_MATH_INLINES -fsigned-char" ;;
        *)
                DEBUG="-g -Wall -D__NO_MATH_INLINES -fsigned-char"
                CFLAGS="-O20 -D__NO_MATH_INLINES -fsigned-char $CFLAGS"
                PROFILE="-O20 -g -pg -D__NO_MATH_INLINES -fsigned-char" ;;
        esac
fi


AC_SUBST(DEBUG)
AC_SUBST(PROFILE)

case $host in
	*-*-*bsd*)
		dlopen_flag="RTLD_LAZY" ;;
	*)
		dlopen_flag="RTLD_NOW" ;;
esac

AC_DEFINE_UNQUOTED(DLOPEN_FLAG, $dlopen_flag, [ Specifies the mode of dlopen behavior ])

dnl ==============================
dnl Check for libraries
dnl ==============================

dnl framework and database engine client libraries
AC_CHECK_DBI
AC_CHECK_MYSQL
AC_CHECK_PGSQL
AC_CHECK_SQLITE
AC_CHECK_SQLITE3
AC_CHECK_MSQL
AC_CHECK_ORACLE
AC_CHECK_FIREBIRD_INTERBASE
AC_CHECK_FREETDS
AC_CHECK_INGRES
AC_CHECK_DB2

dnl functions to load dynamic libraries
use_ltdl=no
AC_ARG_WITH(ltdl,
	[  --with-ltdl	          Use libtool's libltdl to load runtime modules [no]],
	[  use_ltdl=$withval ])
	
if test "x$use_ltdl" = xyes; then
AC_CHECK_HEADERS([ltdl.h], [AC_CHECK_LIB([ltdl],[lt_dlopen],[LIBADD_DL=-lltdl])])
else
AC_CHECK_HEADERS([mach-o/dyld.h],[AC_CHECK_FUNCS([NSLinkModule], [
       LIBADD_DL=""
       dlsym_prefix=_
       ])],
[
AC_CHECK_HEADERS([dlfcn.h])
AC_CHECK_FUNCS([dlopen],[LIBADD_DL=],
		[AC_CHECK_LIB([dl],[dlopen],[LIBADD_DL=-ldl],[])])])
fi
AC_SUBST(LIBADD_DL)

dnl ==============================
dnl Check for functions
dnl ==============================

case $host in
	*-*-win32*)
		shlib_ext=.dll ;;
	*-*-cygwin*)
		shlib_ext=.dll ;;
	*-*-mingw32*)
		shlib_ext=.dll ;;
	*)
		shlib_ext=.so ;;
esac

AC_ARG_WITH(shlib-ext,
	[  --with-shlib-ext	  Specifies the filename extension of loadable modules (usually not necessary).],
	[  shlib_ext=$withval ])
AC_ARG_WITH([driverdir], AS_HELP_STRING([--with-driverdir=PATH],
	[Path to put libdbi plugins into [[LIBDIR/dbd]]]),
	[driverdir="$withval"], [driverdir='${libdir}/dbd'])
AC_SUBST([driverdir])

AC_DEFINE_UNQUOTED(DRIVER_EXT, "$shlib_ext", [ Specifies the filename extension of loadable modules ])

AC_CHECK_FUNCS(strtoll)
dnl i think we'll eventually get an error here...
AC_CHECK_FUNCS(vasprintf)
AC_REPLACE_FUNCS(asprintf)

dnl ==============================
dnl See whether to build the docs
dnl ==============================

ac_docs="YES"
AC_ARG_ENABLE(docs,
	[  --disable-docs	  do not build and install the documentation.],
	[  if test "$enable_docs" = "no"; then
	       ac_docs="NO"
	   fi])

if test "$ac_docs" = "YES"; then
      docs_subdirs="doc"
      AC_PATH_PROGS(myjade, openjade jade, none)
      AC_PATH_PROG(mypdftex, pdftex, none)
else
   docs_subdirs=""
fi

AC_SUBST(docs_subdirs)

AM_CONDITIONAL(BUILDDOCS, test "x$docs_subdirs" != "x")

dnl ===========================================
dnl See whether to link libdbi into the drivers
dnl     and set -no-undefined
dnl ===========================================

ac_libdbi="YES"
AC_ARG_ENABLE(libdbi,
	[  --disable-libdbi       do not link drivers against libdbi.],
	[  if test "$enable_libdbi" = "no"; then
	       ac_libdbi="NO"
	   fi ])

if test "$ac_libdbi" = "YES"; then
   LIBADD_LIBDBI="-no-undefined -ldbi"
   LIBDBI_TEST="test_libdbi.sh"
   AC_SUBST(LIBDBI_TEST)

else
   LIBADD_LIBDBI="-no-undefined"
fi

dnl ===========================================
dnl the directory for database files (for database engines that
dnl allow to write database files wherever you like)
dnl ===========================================
AC_ARG_WITH(dbi_dbdir,
	[  --with-dbi-dbdir=PATH            Directory for database files],
	[dbi_dbdir=$withval],
	[dbi_dbdir="\$(localstatedir)/lib/libdbi"],
])

AC_SUBST(dbi_dbdir)

dnl ===========================================
dnl check for the libdbi library if required
dnl ===========================================

ac_dbi_libdir="no"

AC_ARG_WITH(dbi-libdir,
	[  --with-dbi-libdir       Specifies where the libdbi library files are.],
	[  ac_dbi_libdir="$withval" ])

if test "$ac_libdbi" = "YES"; then
   AC_MSG_CHECKING(for libdbi library)
   if test "$ac_dbi_libdir" = "no"; then
	   dbi_libdirs="/usr/lib /usr/local/lib /sw/lib"
	   libdbi_libs="libdbi.so libdbi.a"
	   AC_FIND_FILE($libdbi_libs, $dbi_libdirs, ac_dbi_libdir)
	   if test "$ac_dbi_libdir" = "no"; then
		   AC_MSG_RESULT(no)
		   AC_MSG_ERROR([Invalid libdbi directory - include files not found.])
	   fi
   fi

   AC_MSG_RESULT([yes: libs in $ac_dbi_libdir])
   LIBADD_LIBDBI="$LIBADD_LIBDBI -L$ac_dbi_libdir"
	
   AC_SUBST(LIBADD_LIBDBI)

   LIBDBI_LIBDIR="$ac_dbi_libdir"
   AC_SUBST(LIBDBI_LIBDIR)

fi

dnl ==============================
dnl Checks for header files
dnl ==============================

AC_CHECK_HEADERS([string.h strings.h])

AC_OUTPUT([
	libdbi-drivers.spec
	Makefile
	doc/Makefile
	doc/include/Makefile
	drivers/Makefile
	drivers/mysql/Makefile
	drivers/pgsql/Makefile
	drivers/sqlite/Makefile
	drivers/sqlite3/Makefile
	drivers/msql/Makefile
	drivers/oracle/Makefile
	drivers/firebird/Makefile
	drivers/freetds/Makefile
	drivers/ingres/Makefile
	drivers/db2/Makefile
	tests/Makefile
	tests/cgreen/Makefile
	tests/test_dbi.cfg
    tests/plugin_settings.sh
	])
dnl doc/Makefile

if test "$ac_mysql" = "NO" && test "$ac_pgsql" = "NO" && test "$ac_sqlite" = "NO" \
	&& test "$ac_sqlite3" = "NO" && test "$ac_msql" = "NO" && test "$ac_oracle" = "NO" \
	&& test "$ac_firebird" = "NO" && test "$ac_freetds" = "NO" && test "$ac_ingres" = "NO" \
	&& test "$ac_db2" = "NO"; then
   echo
   echo "***WARNING***"
   echo "You did not request any drivers to be built. This is most likely not what you want."
   echo "Please run ./configure --help for a list of switches and select at least one driver."
fi

