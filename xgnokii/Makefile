

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

ifdef XPM_LIBS
	LDLIBS += $(XPM_LIBS)
	CFLAGS += $(XPM_CFLAGS)
endif

ifdef ICONV_LIBS
	LDLIBS += $(ICONV_LIBS)
endif

CFLAGS += $(PTHREAD_CFLAGS) \
	  $(GTK_CFLAGS)

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

install: all
	$(INSTALL) -d $(DESTDIR)$(xbindir)
	$(LIBTOOL) --mode=install $(INSTALL) xgnokii $(DESTDIR)$(xbindir)
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL_DATA) -m 0444 {} $(DESTDIR)$(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL_DATA) -m 0444 ../Docs/CREDITS $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL_DATA) -m 0444 ../COPYING $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	$(INSTALL_DATA) -m 0444 ../Docs/sample/gnokiirc $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	(cd $(DESTDIR)$(xgnokii_libdir)/help; \
	 rm -f C ; ln -sf en_US C; \
	 rm -f POSIX ; ln -sf en_US POSIX)
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_3310.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_3210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6250.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_7110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm

install-strip: all
	$(INSTALL) -d $(DESTDIR)$(xbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -s xgnokii $(DESTDIR)$(xbindir)
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL_DATA) -m 0444 {} $(DESTDIR)$(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL_DATA) -m 0444 ../Docs/CREDITS $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL_DATA) -m 0444 ../COPYING $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	$(INSTALL_DATA) -m 0444 ../Docs/sample/gnokiirc $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	(cd $(DESTDIR)$(xgnokii_libdir)/help; \
	 rm -f C ; ln -sf en_US C; \
	 rm -f POSIX; ln -sf en_US POSIX)
	$(INSTALL) -d $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_3310.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_3210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_6250.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL_DATA) -m 0444 xpm/Preview_7110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)$(xbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 0750 xgnokii $(DESTDIR)$(xbindir)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -m 0440 {} $(DESTDIR)$(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/CREDITS $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -m 0440 ../COPYING $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/sample/gnokiirc $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	(cd $(DESTDIR)$(xgnokii_libdir)/help; \
	 rm -f C ; ln -sf en_US C; \
	 rm -f POSIX; ln -sf en_US POSIX)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_3310.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_3210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6250.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_7110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	@echo

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -o root -g gnokii -d $(DESTDIR)$(xbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 0750 -s xgnokii $(DESTDIR)$(xbindir)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -m 0440 {} $(DESTDIR)$(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/CREDITS $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -m 0440 ../COPYING $(DESTDIR)$(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/sample/gnokiirc $(DESTDIR)$(xgnokii_libdir)/help/en_US/sample
	(cd $(DESTDIR)$(xgnokii_libdir)/help; \
	 rm -f C ; ln -sf en_US C; \
	 rm -f POSIX; ln -sf en_US POSIX)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6150.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_3310.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_5110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_3210.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6250.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_7110.xpm $(DESTDIR)$(xgnokii_libdir)/xpm
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
