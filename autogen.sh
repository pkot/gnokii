#!/bin/sh

# comment the next line when you have the system without NLS
gettextize -c -f
aclocal
autoheader
autoconf
./configure "$@"
