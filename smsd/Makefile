#
# Makefile for the xGNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. & Jan Derfinak
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += $(PTHREAD_CFLAGS) \
	  `glib-config --cflags`

LDFLAGS += $(PTHREAD_LIBS) \
	   `glib-config --libs` -lpq

ifdef XPM_LIBS
    LDFLAGS +=$(XPM_LIBS)
endif

OBJS = 	db.o \
	smsd.o \
        lowlevel.o

all: smsd


smsd: $(OBJS) $(TOPDIR)/common/COMMON.o $(TOPDIR)/common/data/DATA.o $(TOPDIR)/common/gsm-filetypes.o

$(TOPDIR)/common/COMMON.o:
	$(MAKE) -C $(TOPDIR)/common COMMON.o

$(TOPDIR)/common/data/DATA.o:
	$(MAKE) -C $(TOPDIR)/common/data DATA.o

$(TOPDIR)/common/gsm-filetypes.o:
	$(MAKE) -C $(TOPDIR)/common gsm-filetypes.o

clean:
	$(RM) $(OBJS) *~ depend smsd

install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) -m 755 -s smsd $(bindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
