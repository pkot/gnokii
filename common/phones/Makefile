
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

OBJS =	generic.lo \
	nokia.lo \
	nk7110.lo \
	nk6510.lo \
	nk6160.lo \
	nk6100.lo \
	nk3110.lo \
	atgen.lo \
	atbosch.lo \
	ateric.lo \
	atnok.lo \
	atsie.lo \
	gnapplet.lo \
	fake.lo

#	nk2110.lo
#	dc2711.lo

all: libPHONES.la

libPHONES.la: $(OBJS)
	$(LIBTOOL) --mode=link $(CC) -o libPHONES.la $(OBJS)

clean:
	$(LIBTOOL) --mode=clean $(RM) libPHONES.la $(OBJS)
	$(RM) *~ depend

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

