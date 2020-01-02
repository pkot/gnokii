#!/bin/sh -e

if [ "`uname -s`"x = "Darwin"x ]; then
	glibtoolize -c -f
else
	libtoolize -c -f
fi
glib-gettextize -f -c
intltoolize --force --copy --automake
AC_LOCAL_FLAGS="-I m4/"
if [ "`uname -s`"x = "FreeBSD"x ]; then
	AC_LOCAL_FLAGS="${AC_LOCAL_FLAGS} -I /usr/local/share/aclocal"
fi
aclocal ${AC_LOCAL_FLAGS}
autoheader ${AC_LOCAL_FLAGS}
automake --add-missing
autoconf
./configure "$@"
