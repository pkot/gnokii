#!/bin/sh

# comment the next line when you have the system without NLS
gettextize -c -f
if [ -f po/Makevars.template ]; then
	mv po/Makevars.template po/Makevars
fi
aclocal
autoheader
autoconf
./configure "$@"
