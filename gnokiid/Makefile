
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

LDLIBS += $(TOPDIR)/common/libgnokii.la

OBJS = gnokiid.o

all: gnokiid

gnokiid: $(OBJS) $(TOPDIR)/common/data/libDATA.la $(TOPDIR)/common/libgnokii.la
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(OBJS) $(TOPDIR)/common/data/libDATA.la $(LDLIBS) -o $@

clean:
	$(LIBTOOL) --mode=clean $(RM) gnokiid $(OBJS)
	$(RM) *~ depend core *.bak

install: gnokiid
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) gnokiid $(DESTDIR)$(sbindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
