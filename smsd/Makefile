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

LDLIBS += -rdynamic $(TOPDIR)/common/libgnokii.la -ldl
# Some systems, eg. FreeBSD don't have libdl. Use this:
# LDLIBS += -s -rdynamic -L$(TOPDIR)/common -lgnokii -pthread -L../getopt -lgetopt

OBJS = 	smsd.lo \
	lowlevel.lo
DB_OBJS = pq.lo mysql.lo file.lo

# A list of database modules to build.
# FIXME: detect which ones to build in configure
#DB_LIBS = libfile.la libpq.la libmysql.la
DB_LIBS := libfile.la

all: smsd


smsd: $(OBJS) $(TOPDIR)/common/libgnokii.la $(DB_LIBS)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@


### DB Modules

# PostgreSQL support
pq.lo: smsd.h pq.c
	$(LIBTOOL) --mode=compile $(CC) -o pq.lo $(CFLAGS) $(CPPFLAGS) \
	-I../include -I$(shell pg_config --includedir) -c pq.c

libpq.la: pq.lo
	$(LIBTOOL) --mode=link $(CC) -o libpq.la pq.lo \
	$(shell $(GLIBLDLIBS)) -L$(shell pg_config --libdir) -lpq \
	-export-dynamic -avoid-version -rpath ${libdir}/smsd

# MySQL support
mysql.lo: smsd.h mysql.c
	$(LIBTOOL) --mode=compile $(CC) -o mysql.lo $(CFLAGS) $(CPPFLAGS) \
	-I../include $(shell mysql_config --cflags) -c mysql.c

libmysql.la: mysql.lo
	$(LIBTOOL) --mode=link $(CC) -o libmysql.la mysql.lo \
	$(shell $(GLIBLDLIBS)) $(shell mysql_config --libs) \
	-export-dynamic -avoid-version -rpath ${libdir}/smsd

# File support
file.lo: smsd.h file.c
	$(LIBTOOL) --mode=compile $(CC) -o file.lo $(CFLAGS) $(CPPFLAGS) \
	-I../include -c file.c

libfile.la: file.lo
	$(LIBTOOL) --mode=link $(CC) -o libfile.la file.lo \
	$(shell $(GLIBLDLIBS)) \
	-export-dynamic -avoid-version -rpath ${libdir}/smsd

### End of DB Modules

$(TOPDIR)/common/libgnokii.la:
	$(MAKE) -C $(TOPDIR)/common libgnokii.la

clean:
	$(LIBTOOL) --mode=clean $(RM) smsd $(DB_LIBS) $(OBJS) $(DB_OBJS)
	$(RM) *~ depend *.so core*

install: all
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 755 smsd $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)${libdir}/smsd
	for f in $(DB_LIBS) ; do \
	$(LIBTOOL) --mode=install $(INSTALL) $$f $(DESTDIR)${libdir}/smsd ; \
	done
	$(LIBTOOL) --mode=finish ${libdir}/smsd
	$(INSTALL) -d $(DESTDIR)$(man8dir)
	$(INSTALL_DATA) $(SMSD_MAN) $(DESTDIR)$(man8dir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
