
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
	$(INSTALL) -m 755 todologo $(DESTDIR)$(bindir)
	$(INSTALL) -m 755 sendsms $(DESTDIR)$(bindir)
	$(INSTALL) -m 755 ppm2nokia $(DESTDIR)$(bindir)
	@if [ -f gnapplet.sis ]; then \
		$(INSTALL) -d $(DESTDIR)$(docdir); \
		$(INSTALL_DATA) gnapplet.sis $(DESTDIR)$(docdir); \
	fi

install: install-generic
	$(LIBTOOL) --mode=install $(INSTALL) mgnokiidev $(DESTDIR)$(sbindir)

install-strip: install-generic
	$(LIBTOOL) --mode=install $(INSTALL) -s mgnokiidev $(DESTDIR)$(sbindir)

install-suid: install-generic
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 4750 mgnokiidev $(DESTDIR)$(sbindir)

install-ss: install-generic
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 4750 -s mgnokiidev $(DESTDIR)$(sbindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

