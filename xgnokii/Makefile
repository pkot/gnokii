

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
    LDFLAGS += $(XPM_LIBS)
    CFLAGS  += $(XPM_CFLAGS)
endif

CFLAGS += $(PTHREAD_CFLAGS) \
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
	xgnokii_logos.o \
	xgnokii_lowlevel.o \
	xgnokii_data.o

HELP_DIR = docs/help


all: xgnokii


xgnokii: $(OBJS) $(TOPDIR)/common/COMMON.o $(TOPDIR)/common/data/DATA.o $(TOPDIR)/common/gsm-filetypes.o

$(TOPDIR)/common/COMMON.o:
	$(MAKE) -C $(TOPDIR)/common COMMON.o

$(TOPDIR)/common/data/DATA.o:
	$(MAKE) -C $(TOPDIR)/common/data DATA.o

$(TOPDIR)/common/gsm-filetypes.o:
	$(MAKE) -C $(TOPDIR)/common gsm-filetypes.o

clean:
	$(RM) $(OBJS) *~ xpm/*~ depend xgnokii

install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) xgnokii $(bindir)
	$(INSTALL) -d $(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -d $(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL) -m 0444 {} $(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL) -m 0444 ../Docs/CREDITS $(xgnokii_libdir)/help/en_US
	$(INSTALL) -m 0444 ../COPYING $(xgnokii_libdir)/help/en_US
	$(INSTALL) -d $(xgnokii_libdir)/help/en_US/sample
	$(INSTALL) -m 0444 ../Docs/sample/gnokiirc $(xgnokii_libdir)/help/en_US/sample
	(cd $(xgnokii_libdir)/help; ln -sf en_US C; ln -sf en_US POSIX)
	$(INSTALL) -d $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/5110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/Preview_6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/Preview_6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/Preview_6210.xpm $(xgnokii_libdir)/xpm

install-strip: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) -s xgnokii $(bindir)
	$(INSTALL) -d $(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -d $(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL) -m 0444 {} $(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL) -m 0444 ../Docs/CREDITS $(xgnokii_libdir)/help/en_US
	$(INSTALL) -m 0444 ../COPYING $(xgnokii_libdir)/help/en_US
	$(INSTALL) -d $(xgnokii_libdir)/help/en_US/sample
	$(INSTALL) -m 0444 ../Docs/sample/gnokiirc $(xgnokii_libdir)/help/en_US/sample
	(cd $(xgnokii_libdir)/help; ln -sf en_US C; ln -sf en_US POSIX)
	$(INSTALL) -d $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/5110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/Preview_6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/Preview_6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -m 0444 xpm/Preview_6210.xpm $(xgnokii_libdir)/xpm
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(bindir)
	$(INSTALL) -o root -g gnokii -m 0750 xgnokii $(bindir)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -m 0440 {} $(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/CREDITS $(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -m 0440 ../COPYING $(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/help/en_US/sample
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/sample/gnokiirc $(xgnokii_libdir)/help/en_US/sample
	(cd $(xgnokii_libdir)/help; ln -sf en_US C; ln -sf en_US POSIX)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/5110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6210.xpm $(xgnokii_libdir)/xpm
	@echo

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -o root -g gnokii -d $(bindir)
	$(INSTALL) -o root -g gnokii -m 0750 -s xgnokii $(bindir)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/help
	( cd $(HELP_DIR); \
	  $(FIND) . -type d \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/help/{} \; ; \
	  $(FIND) . -type f \! -path "*CVS*" \
	       -exec $(INSTALL) -o root -g gnokii -m 0440 {} $(xgnokii_libdir)/help/{} \; \
	)
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/CREDITS $(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -m 0440 ../COPYING $(xgnokii_libdir)/help/en_US
	$(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/help/en_US/sample
	$(INSTALL) -o root -g gnokii -m 0440 ../Docs/sample/gnokiirc $(xgnokii_libdir)/help/en_US/sample
	(cd $(xgnokii_libdir)/help; ln -sf en_US C; ln -sf en_US POSIX)
	$(INSTALL) -o root -g gnokii -d -m 0750 $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/5110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6110.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6150.xpm $(xgnokii_libdir)/xpm
	$(INSTALL) -o root -g gnokii -m 0440 xpm/Preview_6210.xpm $(xgnokii_libdir)/xpm
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
