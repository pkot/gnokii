
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
MINOR_NUMBER = 3

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
	readmidi.o \
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

ifndef WIN32_CROSS
ifdef NO_SHARED
GNOKII_LIB=libgnokii.a
else
GNOKII_LIB  =libgnokii.so
GNOKII_LIBM =$(GNOKII_LIB).$(MAJOR_NUMBER)
GNOKII_LIBMM=$(GNOKII_LIBM).$(MINOR_NUMBER)
endif
else
ifdef NO_SHARED
GNOKII_LIB=gnokii.lib
else
GNOKII_LIB=gnokii.dll
endif
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
	$(RM) libgnokii.a
	$(AR) rc libgnokii.a $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o 
	$(RANLIB) libgnokii.a

gnokii.dll: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	$(CC) -mwindows -shared -o $@ $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o -Wl,--output-def,gnokii.def,--out-implib,gnokii.lib

gnokii.lib: $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	$(RM) gnokii.lib
	$(AR) rc gnokii.lib $(OBJS) phones/PHONES.o links/LINKS.o devices/DEVICES.o
	$(RANLIB) gnokii.lib

vcal.c: vcal.lx
	$(LEX) -ovcal.c vcal.lx

clean:
	$(RM) $(OBJS) *~ depend libgnokii.a libgnokii.so* gnokii.lib gnokii.dll gnokii.def phones/PHONES.o links/LINKS.o devices/DEVICES.o

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
