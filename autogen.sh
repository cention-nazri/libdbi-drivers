#!/bin/sh
# autogen.sh - generates configure using the autotools
# $Id: autogen.sh,v 1.2 2003/02/04 22:37:53 mhoenicka Exp $
aclocal
automake --add-missing -f

## autoconf 2.53 will not work, at least on FreeBSD. Change the following
## line appropriately to call autoconf 2.13 instead. This one works for
## FreeBSD 4.7:
autoconf213

autoheader