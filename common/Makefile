
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
	sms-nokia.o \
	gsm-bitmaps.o \
	gsm-common.o \
	gsm-encoding.o \
	gsm-error.o \
	gsm-statemachine.o \
	compat.o \
	snprintf.o

ifdef NO_SHARED
GNOKII_LIB=libgnokii.a
else
GNOKII_LIB=libgnokii.so
endif

all: $(GNOKII_LIB) gsm-filetypes.o

phones/PHONES.o:
	$(MAKE) -C phones PHONES.o

links/LINKS.o:
	$(MAKE) -C links LINKS.o

devices/DEVICES.o:
	$(MAKE) -C devices DEVICES.o

gnokii-debug: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o gsm-filetypes.o ../gnokii/gnokii.o
	$(CC) -o gnokii-debug $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o gsm-filetypes.o ../gnokii/gnokii.o

libgnokii.so: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	$(CC) -shared -o libgnokii.so $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o

libgnokii.a: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	rm -f libgnokii.a
	$(AR) rc libgnokii.a $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o 
	ranlib libgnokii.a

gsm-filetypes.o: gsm-filetypes.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XPM_CFLAGS) -c gsm-filetypes.c

vcal.c: vcal.lx
	$(LEX) -ovcal.c vcal.lx

clean:
	$(RM) $(OBJS) *~ depend libgnokii.a libgnokii.so gsm-filetypes.o phones/PHONES.o links/LINKS.o devices/DEVICES.o

distclean:
	$(RM) vcal.c

install:
	$(INSTALL) -d $(DESTDIR)/$(libdir)
	$(INSTALL) libgnokii.so $(DESTDIR)/$(libdir)
	@echo

install-strip: install
	@echo

install-suid: install
	@echo

install-ss: install
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
