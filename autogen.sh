#!/bin/sh

# comment the next line when you have the system without NLS
if ! gettextize --version | grep -q '0\.10\.' ; then
	gettextize -c -f --intl --no-changelog
else
	gettextize -c -f
fi

# I don't like when this file gets automatically changed
if [ -f configure.in~ ]; then
	mv configure.in~ configure.in
fi

libtoolize -c -f
AC_LOCAL_FLAGS="-I m4"
if [ "`uname -s`"x = "FreeBSD"x ]; then
	AC_LOCAL_FLAGS="${AC_LOCAL_FLAGS} -I /usr/local/share/aclocal"
fi
aclocal ${AC_LOCAL_FLAGS}
autoheader
autoconf
./configure "$@"
