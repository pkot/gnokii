#!/bin/sh

# comment the next line when you have the system without NLS
gettextize -c -f --intl --no-changelog

# I don't like when this file gets automatically changed
if [ -f configure.in~ ]; then
	mv configure.in~ configure.in
fi

if [ -f po/Makevars.template ]; then
	mv po/Makevars.template po/Makevars
fi
aclocal
autoheader
autoconf
./configure "$@"
