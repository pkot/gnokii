#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.

#
# Version number of the package.
#

VERSION = 0.3.1_pre32

#
# Compiler to use.
#

CC = gcc

#
# Debug - uncomment this to see debugging output. It is useful only for
# developers and testers.
#

DEBUG=-DDEBUG

#
# Model of the mobile phone.  This is now only used as a default
# if ~/.gnokiirc cannot be read.
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
# Under Windows, uncomment this
#

# PORT=-DPORT="\"COM1:\""

#
# Under FreeBSD, uncomment this
#

# PORT=-DPORT="\"/dev/ttyd0\""

#
# I18N - comment this line if you do not have GNU gettext installed
#

GETTEXT=-DGNOKII_GETTEXT

#
# WIN32 support for Cygwin. Uncomment when compiling under Cygwin/WIN32
#

# WIN32=-DWIN32

#
# Security - compile with -DSECURITY to enable all security features
#

# SECURITY=-DSECURITY

#
# GTK - you need this only for GUI support, if you do not need GUI, you can
# comment this out
#

GTKCFLAGS=`gtk-config --cflags`
GTKLDFLAGS=`gtk-config --libs`

#
# Destination directory
#

DESTDIR=/usr/local

#
# Install utility
#

INSTALL=install

#
# For more information about threads see the comp.programming.threads FAQ
# http://www.serpentine.com/~bos/threads-faq/
#

#
# For more information about threads see the comp.programming.threads FAQ
# http://www.serpentine.com/~bos/threads-faq/
#

#
# Set up compilation/linking flags for Linux.
#

export CC INSTALL MODEL PORT GETTEXT DEBUG VERSION WIN32 SECURITY GTKCFLAGS GTKLDFLAGS DESTDIR

COMMON=-Wall -O2 \
       ${MODEL} ${PORT} \
       ${GETTEXT} \
       ${SECURITY} \
       ${WIN32} \
       ${DEBUG} \
       -DVERSION=\"${VERSION}\"

CFLAGS = -D_REENTRANT ${COMMON} ${GTKCFLAGS}

#
# Comment this for WIN32 ;-)
#

LDFLAGS = -s -lpthread ${GTKLDFLAGS}

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
              mbus-2110.o \
              fbus-6110.o fbus-6110-auth.o fbus-6110-ringtones.o \
              gsm-networks.o cfgreader.o gsm-filetypes.o \
              win32/winserial.o

#
# RLP objects - only needed for data calls
#

RLP_OBJS = rlp-common.o rlp-crc24.o

#
# Object files for each utility
#

GNOKII_OBJS = gnokii.o ${RLP_OBJS}

XLOGOS_OBJS = xlogos.o

XKEYB_OBJS = xkeyb.o

GNOKIID_OBJS = gnokiid.o at-emulator.o virtmodem.o datapump.o

MGNOKIIDEV_OBJS = mgnokiidev.o

# Build executable
all: gnokii gnokiid mgnokiidev xgnokii xlogos xkeyb

gnokii: $(GNOKII_OBJS) $(COMMON_OBJS)

gnokiid: $(GNOKIID_OBJS) $(COMMON_OBJS)

xgnokii: $(COMMON_OBJS)
	@make -C xgnokii

xlogos: $(XLOGOS_OBJS) $(COMMON_OBJS)

xkeyb: $(XKEYB_OBJS) $(COMMON_OBJS)

mgnokiidev: $(MGNOKIIDEV_OBJS) 

.PHONY: xgnokii

# Misc targets
clean:
	@rm -f core *.exe *~ *% *.bak \
               $(COMMON_OBJS) \
               gnokii $(GNOKII_OBJS) \
               gnokiid $(GNOKIID_OBJS) \
               xlogos $(XLOGOS_OBJS) \
               xkeyb $(XKEYB_OBJS) \
               mgnokiidev $(MGNOKIIDEV_OBJS) \
               gnokii-${VERSION}.tar.gz
	@make -sC xgnokii clean

dist:	clean
	@mkdir -p /tmp/gnokii-${VERSION}
	@cp -r * /tmp/gnokii-${VERSION}
	@sed 's#@@VERSION@@#${VERSION}#' gnokii.spec >/tmp/gnokii-${VERSION}/gnokii.spec
	@rm -rf /tmp/gnokii-${VERSION}/CVS /tmp/gnokii-${VERSION}/*/CVS 
	@cd /tmp; tar cfz gnokii-${VERSION}.tar.gz gnokii-${VERSION}
	@rm -rf /tmp/gnokii-${VERSION}
	@mv /tmp/gnokii-${VERSION}.tar.gz .

install: gnokii gnokiid xgnokii xlogos xkeyb
	@echo "Installing files..."

	@mkdir -p $(DESTDIR)/bin $(DESTDIR)/sbin $(DESTDIR)/lib/gnokii

	@$(INSTALL) gnokii $(DESTDIR)/bin
	@echo " $(DESTDIR)/bin/gnokii"

	@$(INSTALL) gnokiid $(DESTDIR)/sbin
	@echo " $(DESTDIR)/sbin/gnokiid"

	@$(INSTALL) mgnokiidev $(DESTDIR)/sbin
	@echo " $(DESTDIR)/sbin/mgnokiidev"

	@$(INSTALL) xlogos $(DESTDIR)/bin
	@echo " $(DESTDIR)/bin/xlogos"

	@$(INSTALL) xkeyb $(DESTDIR)/bin
	@echo " $(DESTDIR)/bin/xkeyb"

	@$(INSTALL) pixmaps/6110.xpm $(DESTDIR)/lib/gnokii/6110.xpm
	@echo " $(DESTDIR)/lib/gnokii/6110.xpm"

	@$(INSTALL) pixmaps/6150.xpm $(DESTDIR)/lib/gnokii/6150.xpm
	@echo " $(DESTDIR)/lib/gnokii/6150.xpm"

	@make -sC xgnokii install

	@echo "done."

rpm:	dist
	@rpm -ta gnokii-${VERSION}.tar.gz
	@make clean

# Dependencies - simplified for now
gnokii.o: gnokii.c gnokii.h gsm-api.c gsm-api.h misc.h gsm-common.h
gnokiid.o: gnokiid.c gsm-api.c gsm-api.h misc.h gsm-common.h
xlogos.o: xlogos.c gsm-api.c gsm-api.h misc.h gsm-common.h
xkeyb.o: xkeyb.c gsm-api.c gsm-api.h misc.h gsm-common.h
virtmodem.o: virtmodem.c at-emulator.c gsm-api.c gsm-api.h misc.h gsm-common.h
datapump.o: datapump.c gsm-api.c gsm-api.h misc.h gsm-common.h
at-emulator.o: at-emulator.c gsm-api.c gsm-api.h misc.h gsm-common.h
gsm-api.o: gsm-api.c fbus-3810.c fbus-3810.h mbus-2110.c mbus-2110.h fbus-6110.c fbus-6110.h misc.h gsm-common.h
fbus-3810.o: fbus-3810.c fbus-3810.h misc.h gsm-common.h
mbus-2110.o: mbus-2110.c mbus-2110.h misc.h gsm-common.h
fbus-6110.o: fbus-6110.c fbus-6110.h misc.h gsm-common.h gsm-networks.h
fbus-6110-auth.o: fbus-6110-auth.c fbus-6110-auth.h
fbus-6110-ringtones.o: fbus-6110-ringtones.c fbus-6110-ringtones.h
cfgreader.o: cfgreader.c cfgreader.h
win32/winserial.o: win32/winserial.c win32/winserial.h
