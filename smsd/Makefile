#
# Makefile for the smsd.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. & Jan Derfinak
#               2000 Karel Zak
# $Id$
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += -DXDEBUG -g $(PTHREAD_CFLAGS) \
	  `glib-config --cflags`

LDFLAGS += $(PTHREAD_LIBS) \
	   `glib-config --libs`

LDFLAGS += -g -L$(TOPDIR)/common -lgnokii

# Change this according to your system
# FIXME: detect it in configure
CFLAGS += -I/usr/include/pgsql

LDFLAGS += -lpq
#LDFLAGS += -L/usr/lib/pgsql -lpq

ifdef XPM_LIBS
    LDFLAGS +=$(XPM_LIBS)
endif

OBJS = 	db.o \
	smsd.o \
        lowlevel.o

all: smsd


smsd: $(OBJS) $(TOPDIR)/common/gsm-filetypes.o $(TOPDIR)/common/libgnokii.so
	gcc $(LDFLAGS) $(OBJS) $(TOPDIR)/common/gsm-filetypes.o -o $@

$(TOPDIR)/common/gsm-filetypes.o:
	$(MAKE) -C $(TOPDIR)/common gsm-filetypes.o

$(TOPDIR)/common/libgnokii.so:
	$(MAKE) -C $(TOPDIR)/common libgnokii.so

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
