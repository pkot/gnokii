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
# LDLIBS += -s -rdynamic -L$(TOPDIR)/common -lgnokii

ifdef XPM_LIBS
	LDLIBS +=$(XPM_LIBS)
endif

ifdef ICONV_LIBS
	LDLIBS += $(ICONV_LIBS)
endif

OBJS = 	smsd.o \
	lowlevel.o

all: smsd


smsd: $(OBJS) $(TOPDIR)/common/libgnokii.la
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@


### DB Modules
# Change this according to your system
# FIXME: detect it in configure

# PostgreSQL support
libpq.la: smsd.h pq.c
	$(LIBTOOL) --mode=link $(CC) -o libpq.la $(CFLAGS) -I../include \
	-I/usr/include/pgsql pq.c $(shell $(GLIBLDLIBS)) -lpq -avoid-version -rpath $(libdir)/smsd

# MySQL support
libmysql.la: smsd.h mysql.c
	$(LIBTOOL) --mode=link $(CC) -o libmysql.la $(CFLAGS) -I../include \
	$(shell mysql_config --cflags) mysql.c $(shell $(GLIBLDLIBS)) $(shell mysql_config --libs)

# File support
libfile.la: smsd.h file.c
	$(LIBTOOL) --mode=link $(CC) -o libfile.so $(CFLAGS) -I../include \
        file.c $(shell $(GLIBLDLIBS))

### End of DB Modules

$(TOPDIR)/common/libgnokii.la:
	$(MAKE) -C $(TOPDIR)/common libgnokii.la

clean:
	$(LIBTOOL) --mode=clean $(RM) smsd libpq.la libmysql.la libfile.la $(OBJS)
	$(RM) *~ depend *.so core*

install: all
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 755 smsd $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)${libdir}/smsd
	for f in lib*.la ; do \
	$(LIBTOOL) --mode=install $(INSTALL) $$f $(DESTDIR)${libdir}/smsd ; \
	done
	$(INSTALL) -d $(DESTDIR)$(man8dir)
	$(INSTALL_DATA) $(SMSD_MAN) $(DESTDIR)$(man8dir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
