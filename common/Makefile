
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

OBJS =	cfgreader.o \
	device.o \
	gsm-ringtones.o \
	gsm-api.o \
	gsm-networks.o \
	vcal.o \
	misc.o \
        gsm-sms.o \
	gsm-bitmaps.o \
	gsm-common.o \
	gsm-encoding.o \
	gsm-statemachine.o

all: libgnokii.o gsm-filetypes.o

phones/PHONES.o:
	$(MAKE) -C phones PHONES.o

links/LINKS.o:
	$(MAKE) -C links LINKS.o

devices/DEVICES.o:
	$(MAKE) -C devices DEVICES.o

libgnokii.o: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	$(CC) -shared -o libgnokii.so $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o

gsm-filetypes.o: gsm-filetypes.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XPM_CFLAGS) -c gsm-filetypes.c

vcal.c: vcal.lx
	$(LEX) -ovcal.c vcal.lx

clean:
	$(RM) $(OBJS) *~ depend COMMON.o gsm-filetypes.o vcal.c phones/PHONES.o links/LINKS.o devices/DEVICES.o

install:
	$(INSTALL) -d $(libdir)
	$(INSTALL) libgnokii.so $(libdir)
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

