#
# $Id$
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#		2002-2005 Pawel Kot
#

#
# For this common directory is used "subsystem.la" .la files concept.
#   (the list of object files to be linked together (to COMMON.la),
#   and other dirs is used this _one_ file (instead of all OBJS)
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

MAJOR_NUMBER = 2
MINOR_NUMBER = 4
ifdef WIN32_CROSS
LIBTOOL_UNDEF = -no-undefined
LIBTOOL_VER = -version-info $(MAJOR_NUMBER):$(MINOR_NUMBER):0 -avoid-version
LIBTOOL_RPATH=-rpath $(libdir)
else
LIBTOOL_VER = -version-info $(MAJOR_NUMBER):$(MINOR_NUMBER):0
LIBTOOL_RPATH=-rpath $(libdir)
endif

OBJS =	gsm-api.lo \
	gsm-error.lo \
	gsm-statemachine.lo \
	cfgreader.lo \
	device.lo \
	vcard.lo \
	vcal.lo \
	gnvcal.lo \
	ldif.lo \
	gsm-networks.lo \
	gsm-filetypes.lo \
	readmidi.lo \
	gsm-ringtones.lo \
	gsm-bitmaps.lo \
	gsm-sms.lo \
	gsm-call.lo \
	gsm-encoding.lo \
	gsm-common.lo \
	sms-nokia.lo \
	nokia-decoding.lo \
	pkt.lo \
	compat.lo \
	misc.lo \
	snprintf.lo

ifdef NEED_LCHARSET
OBJS += ../intl/localcharset.lo
endif

all: libgnokii.la

phones/libPHONES.la:
	$(MAKE) -C phones libPHONES.la

links/libLINKS.la:
	$(MAKE) -C links libLINKS.la

devices/libDEVICES.la:
	$(MAKE) -C devices libDEVICES.la

libgnokii.la: $(OBJS) phones/libPHONES.la links/libLINKS.la devices/libDEVICES.la
	$(LIBTOOL) --mode=link $(CC) $(LIBTOOL_UNDEF) -o libgnokii.la $(OBJS) phones/libPHONES.la links/libLINKS.la devices/libDEVICES.la $(LIBTOOL_VER) $(LIBTOOL_RPATH) $(LDLIBS)

gnvcal.c: vcal.lx
	$(LEX) -ognvcal.c vcal.lx

clean:
	$(MAKE) -C data clean
	$(MAKE) -C phones clean
	$(MAKE) -C links clean
	$(MAKE) -C devices clean
	$(LIBTOOL) --mode=clean $(RM) libgnokii.la $(OBJS)
	$(RM) *~ depend

distclean:
	$(RM) gnvcal.c

install:
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(LIBTOOL) --mode=install $(INSTALL) libgnokii.la $(DESTDIR)$(libdir)
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
