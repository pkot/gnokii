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

aclocal
autoheader
autoconf
./configure "$@"
