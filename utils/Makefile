
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
	$(INSTALL) -d $(sbindir)
	$(INSTALL) -o root -g gnokii -m 4750 mgnokiidev $(sbindir)
	$(INSTALL) todologo $(bindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

