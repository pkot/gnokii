#!/bin/sh

autopoint -f
libtoolize -c -f
AC_LOCAL_FLAGS="-I m4"
if [ "`uname -s`"x = "FreeBSD"x ]; then
	AC_LOCAL_FLAGS="${AC_LOCAL_FLAGS} -I /usr/local/share/aclocal"
fi
aclocal ${AC_LOCAL_FLAGS}
autoheader
autoconf
./configure "$@"
