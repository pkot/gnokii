
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

OBJS =	atbus.lo \
	fbus.lo \
	fbus-phonet.lo \
	fbus-3110.lo \
	m2bus.lo \
	utils.lo

#	cbus.lo

all: libLINKS.la

libLINKS.la: $(OBJS)
	$(LIBTOOL) --mode=link $(CC) -o libLINKS.la $(OBJS)

clean:
	$(LIBTOOL) --mode=clean $(RM) libLINKS.la $(OBJS)
	$(RM) *~ depend

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

