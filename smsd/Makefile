#
# Makefile for the smsd.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. & Jan Derfinak
#               2000 Karel Zak
# $Id$
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += -DMODULES_DIR=\"${prefix}/share/smsd\" $(PTHREAD_CFLAGS) \
	  `glib-config --cflags`

LDLIBS += $(PTHREAD_LIBS) \
	   `glib-config --libs`

LDLIBS += -s -rdynamic -L$(TOPDIR)/common -lgnokii -ldl

ifdef XPM_LIBS
	LDLIBS +=$(XPM_LIBS)
endif

OBJS = 	smsd.o \
	lowlevel.o

all: smsd


smsd: $(OBJS) $(TOPDIR)/common/gsm-filetypes.o $(TOPDIR)/common/libgnokii.so
	gcc $(LDFLAGS) $(OBJS) $(TOPDIR)/common/gsm-filetypes.o $(LDLIBS) -o $@


### DB Modules
# Change this according to your system
# FIXME: detect it in configure

# PostgreSQL support
libpq.so: smsd.h pq.c
	gcc -s -O2 -shared -o libpq.so `glib-config --cflags` -I../include \
        -I/usr/include/pgsql pq.c `glib-config --libs` -lpq

# MySQL support
libmysql.so: smsd.h mysql.c
	gcc -s -O2 -shared -o libmysql.so `glib-config --cflags` -I../include \
        -I/usr/include/mysql mysql.c `glib-config --libs` -L/usr/lib/mysql -lmysqlclient

# File support
libfile.so: smsd.h file.c
	gcc -s -O2 -shared -o libfile.so `glib-config --cflags` -I../include \
        file.c `glib-config --libs`

### End of DB Modules

$(TOPDIR)/common/gsm-filetypes.o:
	$(MAKE) -C $(TOPDIR)/common gsm-filetypes.o

$(TOPDIR)/common/libgnokii.so:
	$(MAKE) -C $(TOPDIR)/common libgnokii.so

clean:
	$(RM) $(OBJS) *~ depend smsd *.so core*

install: all
	$(INSTALL) -d $(DESTDIR)/$(bindir)
	$(INSTALL) -m 755 -s smsd $(DESTDIR)/$(bindir)
	$(INSTALL) -d $(DESTDIR)/${prefix}/share/smsd
	$(INSTALL) lib*.so $(DESTDIR)/${prefix}/share/smsd

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
