#
# Makefile for the smsd.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. & Jan Derfinak
#               2000 Karel Zak
# $Id$
#

GLIBCFLAGS = glib-config --cflags
GLIBLDLIBS = glib-config --libs

# Comment out above lines and uncomment below lines if you want compile smsd
# with glib2 instead of glib1.
#
#GLIBCFLAGS = pkg-config --cflags glib-2.0
#GLIBLDLIBS = pkg-config --libs glib-2.0

TOPDIR=..
include $(TOPDIR)/Makefile.global

SMSD_MAN=man/smsd.8

CFLAGS += -DMODULES_DIR=\"${libdir}/smsd\" $(PTHREAD_CFLAGS) \
	  $(shell $(GLIBCFLAGS))

LDLIBS += $(PTHREAD_LIBS) \
	  $(shell $(GLIBLDLIBS))

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
	-I/usr/include/pgsql pq.c $(shell $(GLIBLDLIBS)) -lpq

# MySQL support
libmysql.so: smsd.h mysql.c
	gcc -s -shared -o libmysql.so $(CFLAGS) -I../include \
	$(shell mysql_config --cflags) mysql.c $(shell $(GLIBLDLIBS)) $(shell mysql_config --libs)

# File support
libfile.so: smsd.h file.c
	gcc -s -shared -o libfile.so $(CFLAGS) -I../include \
        file.c $(shell $(GLIBLDLIBS))

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
	$(INSTALL) -d $(DESTDIR)$(man8dir)
	$(INSTALL_DATA) $(SMSD_MAN) $(DESTDIR)$(man8dir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
