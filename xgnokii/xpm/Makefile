#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=../..
include $(TOPDIR)/Makefile.global

all:
	@echo

# FIXME: these files should be in xkeyb files, but I leave this to Jano to
# consider if xgnokii should contain xkeyb. Jano? PJml
install: all
	$(INSTALL) -d $(libdir)
	$(INSTALL) -m 0444 6110.xpm $(libdir)/6110.xpm
	$(INSTALL) -m 0444 6150.xpm $(libdir)/6150.xpm

clean:
	$(RM) *~

depend dep:
	@echo

.PHONY: all install clean dep depend
