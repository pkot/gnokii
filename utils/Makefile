
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

install:
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) mgnokiidev $(DESTDIR)/$(sbindir)
	$(INSTALL) todologo $(DESTDIR)/$(bindir)

install-strip:
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) -s mgnokiidev $(DESTDIR)/$(sbindir)
	$(INSTALL) todologo $(DESTDIR)/$(bindir)

install-suid:
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) -o root -g gnokii -m 4750 mgnokiidev $(DESTDIR)/$(sbindir)
	$(INSTALL) todologo $(DESTDIR)/$(bindir)

install-ss:
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) -o root -g gnokii -m 4750 -s mgnokiidev $(DESTDIR)/$(sbindir)
	$(INSTALL) todologo $(DESTDIR)/$(bindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

