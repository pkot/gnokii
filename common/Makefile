
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

MAJOR_NUMBER = 1
MINOR_NUMBER = 2

OBJS =	gsm-api.o \
	gsm-error.o \
	gsm-statemachine.o \
	cfgreader.o \
	device.o \
	vcal.o \
	vcard.o \
	ldif.o \
	gsm-networks.o \
	gsm-filetypes.o \
	gsm-ringtones.o \
	gsm-bitmaps.o \
	gsm-sms.o \
	gsm-call.o \
	gsm-encoding.o \
	gsm-common.o \
	sms-nokia.o \
	nokia-decoding.o \
	compat.o \
	misc.o \
	snprintf.o

ifdef NO_SHARED
GNOKII_LIB=libgnokii.a
else
GNOKII_LIB  =libgnokii.so
GNOKII_LIBM =$(GNOKII_LIB).$(MAJOR_NUMBER)
GNOKII_LIBMM=$(GNOKII_LIBM).$(MINOR_NUMBER)
endif

all: $(GNOKII_LIB)

phones/PHONES.o:
	$(MAKE) -C phones PHONES.o

links/LINKS.o:
	$(MAKE) -C links LINKS.o

devices/DEVICES.o:
	$(MAKE) -C devices DEVICES.o

gnokii-debug: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o ../gnokii/gnokii.o
	$(CC) -o gnokii-debug $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o ../gnokii/gnokii.o

libgnokii.so: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	$(CC) -shared -Wl,-soname,$(GNOKII_LIBM) -o $(GNOKII_LIBMM) $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	ln -sf $(GNOKII_LIBMM) $(GNOKII_LIBM)
	ln -sf $(GNOKII_LIBM) $(GNOKII_LIB)

libgnokii.a: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	rm -f libgnokii.a
	$(AR) rc libgnokii.a $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o 
	ranlib libgnokii.a

vcal.c: vcal.lx
	$(LEX) -ovcal.c vcal.lx

clean:
	$(RM) $(OBJS) *~ depend libgnokii.a libgnokii.so* phones/PHONES.o links/LINKS.o devices/DEVICES.o

distclean:
	$(RM) vcal.c

install:
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) $(GNOKII_LIBMM) $(DESTDIR)$(libdir)
	cd $(DESTDIR)$(libdir) && ln -sf $(GNOKII_LIBMM) $(GNOKII_LIBM)
	cd $(DESTDIR)$(libdir) && ln -sf $(GNOKII_LIBM) $(GNOKII_LIB)
	$(INSTALL) -d $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL_DATA) gnokii.pc $(DESTDIR)$(libdir)/pkgconfig
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
