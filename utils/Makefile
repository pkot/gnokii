
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

OBJS = mgnokiidev.o

all: mgnokiidev

mgnokiidev: $(OBJS)

clean:
	$(RM) $(OBJS) *~ depend mgnokiidev *.exe core *.bak

install-generic:
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) todologo $(DESTDIR)$(bindir)
	$(INSTALL) ppm2nokia $(DESTDIR)$(bindir)

install: install-generic
	$(INSTALL) mgnokiidev $(DESTDIR)$(sbindir)

install-strip: install-generic
	$(INSTALL) -s mgnokiidev $(DESTDIR)$(sbindir)

install-suid: install-generic
	$(INSTALL) -o root -g gnokii -m 4750 mgnokiidev $(DESTDIR)$(sbindir)

install-ss: install-generic
	$(INSTALL) -o root -g gnokii -m 4750 -s mgnokiidev $(DESTDIR)$(sbindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

