

#
# Makefile for the xGNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. & Jan Derfinak
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += -DXVERSION=\"$(XVERSION)\" \
          -DXGNOKIIDIR=\"$(xgnokii_libdir)\" \
	  $(PTHREAD_CFLAGS) \
	  $(GTK_CFLAGS) 

LDFLAGS += $(PTHREAD_LIBS) \
	   $(GTK_LIBS) 

OBJS = 	xgnokii.o \
	xgnokii_common.o \
	xgnokii_contacts.o \
	xgnokii_sms.o \
	xgnokii_netmon.o \
	xgnokii_dtmf.o \
	xgnokii_cfg.o \
	xgnokii_speed.o \
	xgnokii_xkeyb.o \
	xgnokii_calendar.o \
	xgnokii_lowlevel.o

HELP_DIR = docs/help


all: xgnokii


xgnokii: $(OBJS) $(TOPDIR)/common/COMMON.o

$(TOPDIR)/common/COMMON.o: 
	$(MAKE) -C $(TOPDIR)/common COMMON.o

clean:
	$(RM) $(OBJS) *~ xpm/*~ depend xgnokii

install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) -m 755 -s xgnokii $(bindir)
	$(INSTALL) -d $(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  find ./ -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -d $(xgnokii_libdir)/help/{} \; ; \
	  find ./ -type f \! -path "*CVS*" \
	       -exec $(INSTALL) -m 0444 {} $(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL) -d $(xgnokii_libdir)/help/en_US/{devel,sample}
	$(INSTALL) -m 0444 ../Docs/sample.gnokiirc $(xgnokii_libdir)/help/en_US/sample
	$(INSTALL) -m 0444 ../Docs/{README-3810,README-6110,gettext-howto,packaging-howto} $(xgnokii_libdir)/help/en_US/devel
	(cd $(xgnokii_libdir)/help; ln -s en_US C; ln -s en_US POSIX)
	$(INSTALL) -d $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/5110.xpm $(xgnokii_libdir)/xpm

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
