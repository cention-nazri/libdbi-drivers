#!/bin/sh
# autogen.sh - generates configure using the autotools
# $Id: autogen.sh,v 1.1 2002/11/04 01:04:15 mhoenicka Exp $
aclocal
automake --add-missing
autoconf
autoheader