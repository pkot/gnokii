

#
# $Id$
#
# Makefile for the xGNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. & Jan Derfinak
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += $(PTHREAD_CFLAGS) \
	  $(GTK_CFLAGS) -DGTK_ENABLE_BROKEN

LDLIBS += $(PTHREAD_LIBS) \
	  $(GTK_LIBS)

GLDLIBS += $(TOPDIR)/common/libgnokii.la

OBJS = 	xgnokii.lo \
	xgnokii_common.lo \
	xgnokii_contacts.lo \
	xgnokii_sms.lo \
	xgnokii_netmon.lo \
	xgnokii_dtmf.lo \
	xgnokii_cfg.lo \
	xgnokii_speed.lo \
	xgnokii_xkeyb.lo \
	xgnokii_calendar.lo \
	xgnokii_logos.lo \
	xgnokii_xring.lo \
	xgnokii_lowlevel.lo \
	xgnokii_data.lo

HELP_DIR = docs/help

all: xgnokii


xgnokii: $(OBJS) $(TOPDIR)/common/data/libDATA.la $(TOPDIR)/common/libgnokii.la
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(OBJS) $(GLDLIBS) $(TOPDIR)/common/data/libDATA.la $(LDLIBS) -o $@

clean:
	$(LIBTOOL) --mode=clean $(RM) xgnokii $(OBJS)
	$(RM) *~ xpm/*~ depend

install: xgnokii
	$(INSTALL) -d $(DESTDIR)$(xbindir)
	$(LIBTOOL) --mode=install $(INSTALL) xgnokii $(DESTDIR)$(xbindir)
	$(INSTALL_DATA) xgnokii.pc $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL_DATA) {} $(DESTDIR)$(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL_DATA) ../Docs/CREDITS $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL_DATA) ../COPYING $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	$(INSTALL_DATA) ../Docs/sample/gnokiirc $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	(cd $(DESTDIR)$(xgnokii_libdir)/help; \
	 rm -f C ; ln -sf en_US C; \
	 rm -f POSIX ; ln -sf en_US POSIX)
	$(INSTALL) -d $(DESTDIR)$(datadir)/applications
	$(INSTALL_DATA) xgnokii.desktop $(DESTDIR)$(datadir)/applications

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
