#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.

#
# Version number of the package.
#

VERSION = 0.2.6-pre2

#
# Compiler to use.
#

CC = gcc

#
# Debug - uncomment this to see debugging output. It is useful only for
# developers and testers.
#

# DEBUG=-DDEBUG

#
# Model of the mobile phone
#

MODEL=-DMODEL="\"3810\""

#
# For Nokia 6110/5110 uncomment the next line
#

# MODEL=-DMODEL="\"6110\""

#
# Serial port for communication
#

PORT=-DPORT="\"/dev/ttyS0\""

#
# For COM2 port uncomment the next line
#

# PORT=-DPORT="\"/dev/ttyS1\""

#
# I18N - comment this line if you do not have GNU gettext installed
#

GETTEXT=-DGNOKII_GETTEXT

#
# For more information about threads see the comp.programming.threads FAQ
# http://www.serpentine.com/~bos/threads-faq/
#

#
# Set up compilation/linking flags for Linux.
#

COMMON=-Wall -g -O0 ${MODEL} ${PORT} ${GETTEXT} ${DEBUG} -DVERSION=\"${VERSION}\"

CFLAGS = -D_REENTRANT ${COMMON}
LDFLAGS = -lpthread

#
# For FreeBSD uncomment the following lines
#

# CFLAGS= -D_THREAD_SAFE ${COMMON} -I/usr/local/include
# LDFLAGS= -pthread -L/usr/local/lib -lintl

################## Nothing interesting after this line ##################

GNOKII_OBJS = gnokii.o gsm-api.o fbus-3810.o fbus-6110.o gsm-networks.o

# Build executable
all: gnokii

gnokii: $(GNOKII_OBJS)

# Misc targets
clean:
	@rm -f core *~ *% *.bak gnokii $(GNOKII_OBJS) gnokii-${VERSION}.tar.gz

dist:	clean
	@mkdir -p /tmp/gnokii-${VERSION}
	@cp -r * /tmp/gnokii-${VERSION}
	@sed 's#@@VERSION@@#${VERSION}#' gnokii.spec >/tmp/gnokii-${VERSION}/gnokii.spec
	@rm -rf /tmp/gnokii-${VERSION}/{patches,CVS}
	@cd /tmp; tar cfz gnokii-${VERSION}.tar.gz gnokii-${VERSION}
	@mv /tmp/gnokii-${VERSION}.tar.gz .

rpm:	dist
	@rpm -ta gnokii-${VERSION}.tar.gz
	@make clean

# Dependencies - simplified for now
gnokii.o: gnokii.c gsm-api.c gsm-api.h misc.h gsm-common.h
gsm-api.o: gsm-api.c fbus-3810.c fbus-3810.h misc.h gsm-common.h
fbus-3810.o: fbus-3810.c fbus-3810.h misc.h gsm-common.h
fbus-6110.o: fbus-6110.c fbus-6110.h misc.h gsm-common.h gsm-networks.h
