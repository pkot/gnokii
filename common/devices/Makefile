
#
# $Id$
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

#
# For this common directory is used "subsystem.la" .la files concept.
#   (the list of object files to be linked together (to COMMON.la),
#   and other dirs is used this _one_ file (instead of all OBJS)
#

TOPDIR=../..
include $(TOPDIR)/Makefile.global

OBJS =	tekram.lo \
	irda.lo \
	dku2libusb.lo \
	bluetooth.lo

ifdef WIN32
	OBJS += winserial.lo \
		winirda.lo \
		winbluetooth.lo
else
	OBJS += unixserial.lo \
		unixbluetooth.lo \
		osxbluetooth.lo \
		unixirda.lo \
		tcp.lo
endif

all: libDEVICES.la

libDEVICES.la: $(OBJS)
	$(LIBTOOL) --mode=link $(CC) -o libDEVICES.la $(OBJS)

clean:
	$(LIBTOOL) --mode=clean $(RM) libDEVICES.la $(OBJS)
	$(RM) *~ depend

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

