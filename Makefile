#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.

#
# Version number of the package.
#

VERSION = 0.3.0-pre5

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
# IR communication - uncomment this line if you have IR port that can be setup
# to emulate COM port in BIOS.
#

# INFRARED=-DINFRARED

#
# I18N - comment this line if you do not have GNU gettext installed
#

GETTEXT=-DGNOKII_GETTEXT

#
# GTK - you need this only for GUI support, if you do not need GUI, you can
# comment this out
#

GTKCFLAGS=`gtk-config --cflags`
GTKLDFLAGS=`gtk-config --libs`

#
# For more information about threads see the comp.programming.threads FAQ
# http://www.serpentine.com/~bos/threads-faq/
#

#
# Set up compilation/linking flags for Linux.
#

COMMON=-Wall -g -O0 \
       ${MODEL} ${PORT} \
       ${INFRARED} \
       ${GETTEXT} \
       ${DEBUG} \
       -DVERSION=\"${VERSION}\"

CFLAGS = -D_REENTRANT ${COMMON} ${GTKCFLAGS}
LDFLAGS = -lpthread ${GTKLDFLAGS}

#
# For FreeBSD uncomment the following lines
#

# CFLAGS= -D_THREAD_SAFE ${COMMON} -I/usr/local/include
# LDFLAGS= -pthread -L/usr/local/lib -lintl

################## Nothing interesting after this line ##################

#
# Common objects - these files are needed for all utilities
#

COMMON_OBJS = gsm-api.o \
              fbus-3810.o \
              fbus-6110.o fbus-6110-auth.o \
              gsm-networks.o

#
# Object files for each utility
#

GNOKII_OBJS = gnokii.o

XGNOKII_OBJS = xgnokii.o

GNOKIID_OBJS = gnokiid.o at-emulator.o virtmodem.o datapump.o

# Build executable
all: gnokii gnokiid xgnokii

gnokii: $(GNOKII_OBJS) $(COMMON_OBJS)

gnokiid: $(GNOKIID_OBJS) $(COMMON_OBJS)

xgnokii: $(XGNOKII_OBJS) $(COMMON_OBJS)

# Misc targets
clean:
	@rm -f core *~ *% *.bak \
               $(COMMON_OBJS) \
               gnokii $(GNOKII_OBJS) \
               gnokiid $(GNOKIID_OBJS) \
               xgnokii $(XGNOKII_OBJS) \
               gnokii-${VERSION}.tar.gz

dist:	clean
	@mkdir -p /tmp/gnokii-${VERSION}
	@cp -r * /tmp/gnokii-${VERSION}
	@sed 's#@@VERSION@@#${VERSION}#' gnokii.spec >/tmp/gnokii-${VERSION}/gnokii.spec
	@rm -rf /tmp/gnokii-${VERSION}/CVS /tmp/gnokii-${VERSION}/*/CVS 
	@cd /tmp; tar cfz gnokii-${VERSION}.tar.gz gnokii-${VERSION}
	@rm -rf /tmp/gnokii-${VERSION}
	@mv /tmp/gnokii-${VERSION}.tar.gz .

rpm:	dist
	@rpm -ta gnokii-${VERSION}.tar.gz
	@make clean

# Dependencies - simplified for now
gnokii.o: gnokii.c gsm-api.c gsm-api.h misc.h gsm-common.h
gnokiid.o: gnokiid.c gsm-api.c gsm-api.h misc.h gsm-common.h
virtmodem.o: virtmodem.c at-emulator.c gsm-api.c gsm-api.h misc.h gsm-common.h
datapump.o: datapump.c gsm-api.c gsm-api.h misc.h gsm-common.h
at-emulator.o: at-emulator.c gsm-api.c gsm-api.h misc.h gsm-common.h
gsm-api.o: gsm-api.c fbus-3810.c fbus-3810.h misc.h gsm-common.h
fbus-3810.o: fbus-3810.c fbus-3810.h misc.h gsm-common.h
fbus-6110.o: fbus-6110.c fbus-6110.h misc.h gsm-common.h gsm-networks.h
fbus-6110-auth.o: fbus-6110-auth.c fbus-6110-auth.h
xgnokii.o: xgnokii.c gsm-api.c gsm-api.h misc.h gsm-common.h
