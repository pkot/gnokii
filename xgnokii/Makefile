
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
	xgnokii_speed.o

HELP_DIR = docs/help


all: xgnokii


xgnokii: $(OBJS) $(TOPDIR)/common/COMMON.o

$(TOPDIR)/common/COMMON.o: 
	$(MAKE) -C $(TOPDIR)/common COMMON.o

clean:
	$(RM) $(OBJS) *~ depend xgnokii

install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) -m 755 -s xgnokii $(bindir)
	$(INSTALL) -d $(xgnokii_libdir)/help
	$(INSTALL) -m 0444 $(HELP_DIR)/*.html $(xgnokii_libdir)/help

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend


	
