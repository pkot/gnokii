#
# Makefile for the smsd.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. & Jan Derfinak
#               2000 Karel Zak
# $Id$
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += -DMODULES_DIR=\"${libdir}/smsd\" $(PTHREAD_CFLAGS) \
	  `glib-config --cflags`

LDLIBS += $(PTHREAD_LIBS) \
	   `glib-config --libs`

LDLIBS += -s -rdynamic -L$(TOPDIR)/common -lgnokii -ldl
# Some systems, eg. FreeBSD don't have libdl. Use this:
# LDLIBS += -s -rdynamic -L$(TOPDIR)/common -lgnokii

ifdef XPM_LIBS
	LDLIBS +=$(XPM_LIBS)
endif

OBJS = 	smsd.o \
	lowlevel.o

all: smsd


smsd: $(OBJS) $(TOPDIR)/common/libgnokii.so
	gcc $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@


### DB Modules
# Change this according to your system
# FIXME: detect it in configure

# PostgreSQL support
libpq.so: smsd.h pq.c
	gcc -s -shared -o libpq.so $(CFLAGS) -I../include \
	-I/usr/include/pgsql pq.c `glib-config --libs` -lpq

# MySQL support
libmysql.so: smsd.h mysql.c
	gcc -s -shared -o libmysql.so $(CFLAGS) -I../include \
	$(shell mysql_config --cflags) mysql.c `glib-config --libs` $(shell mysql_config --libs)

# File support
libfile.so: smsd.h file.c
	gcc -s -shared -o libfile.so $(CFLAGS) -I../include \
        file.c `glib-config --libs`

### End of DB Modules

$(TOPDIR)/common/libgnokii.so:
	$(MAKE) -C $(TOPDIR)/common libgnokii.so

clean:
	$(RM) $(OBJS) *~ depend smsd *.so core*

install: all
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) -m 755 -s smsd $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)${libdir}/smsd
	$(INSTALL) lib*.so $(DESTDIR)${libdir}/smsd

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
