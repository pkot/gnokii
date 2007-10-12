
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

OBJS = mgnokiidev.lo

all: mgnokiidev

mgnokiidev: $(OBJS)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

clean:
	$(LIBTOOL) --mode=clean $(RM) mgnokiidev $(OBJS)
	$(RM) *~ depend *.exe core *.bak

install-generic:
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -m 755 sendsms $(DESTDIR)$(bindir)
	@if [ -f gnapplet.sis ]; then \
		$(INSTALL) -d $(DESTDIR)$(docdir); \
		$(INSTALL_DATA) gnapplet.sis $(DESTDIR)$(docdir); \
	fi

install: install-generic
	$(LIBTOOL) --mode=install $(INSTALL) mgnokiidev $(DESTDIR)$(sbindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

