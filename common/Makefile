
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

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += $(PTHREAD_CFLAGS)
OBJS =	cfgreader.o \
	device.o \
	fbus-3810.o \
	fbus-6110-auth.o \
	gsm-ringtones.o \
	fbus-6110.o \
	gsm-api.o \
	gsm-networks.o \
	mbus-2110.o \
	mbus-6160.o \
	mbus-640.o \
	vcal.o \
	misc.o \
	gsm-bitmaps.o \
	gsm-common.o

all: COMMON.o gsm-filetypes.o

phones/PHONES.o:
	$(MAKE) -C phones PHONES.o

links/LINKS.o:
	$(MAKE) -C links LINKS.o

devices/DEVICES.o:
	$(MAKE) -C devices DEVICES.o

COMMON.o: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	$(LD) $(LDREL) $(LDOUT) COMMON.o $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o

gsm-filetypes.o: gsm-filetypes.c 
	$(CC) $(CFLAGS) $(XPM_CFLAGS) -c gsm-filetypes.c

vcal.c: vcal.lx
	$(LEX) -ovcal.c vcal.lx

clean:
	$(RM) $(OBJS) *~ depend COMMON.o gsm-filetypes.o vcal.c phones/PHONES.o links/LINKS.o devices/DEVICES.o

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

