
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

LDLIBS += $(TOPDIR)/common/libgnokii.la

ifdef XPM_LIBS
	LDLIBS += $(XPM_LIBS)
endif

ifdef ICONV_LIBS
	LDLIBS += $(ICONV_LIBS)
endif

OBJS = gnokiid.o

all: gnokiid

gnokiid: $(OBJS) $(TOPDIR)/common/data/libDATA.la $(TOPDIR)/common/libgnokii.la
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(OBJS) $(TOPDIR)/common/data/libDATA.la $(LDLIBS) -o $@

clean:
	$(LIBTOOL) --mode=clean $(RM) gnokiid $(OBJS)
	$(RM) *~ depend core *.bak

install: all
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) gnokiid $(DESTDIR)/$(sbindir)

install-strip: all
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -s gnokiid $(DESTDIR)/$(sbindir)
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 4750 gnokiid $(DESTDIR)/$(sbindir)
	@echo "done"

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 4750 -s gnokiid $(DESTDIR)/$(sbindir)
	@echo "done"

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
