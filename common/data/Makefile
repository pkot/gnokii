
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

CFLAGS += $(PTHREAD_CFLAGS)

OBJS =	at-emulator.o \
	datapump.o \
	rlp-common.o \
	rlp-crc24.o \
	virtmodem.o



all: DATA.o

DATA.o: $(OBJS)
	$(LD) $(LDREL) $(LDOUT) DATA.o $(OBJS)

clean:
	$(RM) $(OBJS) *~ depend DATA.o

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

