
#
# $Id$
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

#
# For this common directory is used "subsystem.o" .o files concept.
#   (the list of object files to be linked together (to COMMON.o),
#   and other dirs is used this _one_ file (instead of all OBJS)
#

TOPDIR=../..
include $(TOPDIR)/Makefile.global

OBJS =	tekram.o

ifdef WIN32
	OBJS += winserial.o
else
	OBJS += unixserial.o \
	        unixirda.o \
		tcp.o
endif

ifdef HAVE_BLUETOOTH
	OBJS += unixbluetooth.o
endif

all: DEVICES.o

DEVICES.o: $(OBJS)
	$(LD) DEVICES.o $(OBJS)

clean:
	$(RM) $(OBJS) *~ depend LINKS.o

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

