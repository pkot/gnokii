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

LDFLAGS += -g -rdynamic -L$(TOPDIR)/common -lgnokii -ldl

ifdef XPM_LIBS
    LDFLAGS +=$(XPM_LIBS)
endif

OBJS = 	smsd.o \
        lowlevel.o

all: smsd


smsd: $(OBJS) $(TOPDIR)/common/gsm-filetypes.o $(TOPDIR)/common/libgnokii.so
	gcc $(LDFLAGS) $(OBJS) $(TOPDIR)/common/gsm-filetypes.o -o $@


### DB Modules
# Change this according to your system
# FIXME: detect it in configure

# PostgreSQL support
libpq.so: smsd.h pq.c
	gcc -shared -o libpq.so `glib-config --cflags` -I../include \
        -I/usr/include/pgsql pq.c `glib-config --libs` -lpq

### End of DB Modules

$(TOPDIR)/common/gsm-filetypes.o:
	$(MAKE) -C $(TOPDIR)/common gsm-filetypes.o

$(TOPDIR)/common/libgnokii.so:
	$(MAKE) -C $(TOPDIR)/common libgnokii.so

clean:
	$(RM) $(OBJS) *~ depend smsd libpq.so core*

install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) -m 755 -s smsd $(bindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
