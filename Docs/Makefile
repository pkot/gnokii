#
# Makefile for the GNOKII tool suite's documentation directory
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=../
include ${TOPDIR}/Makefile.global

#
# Documentation for gnokii (xgnokii install documentation files itself).
#

INSTALL_DOCS = 	README \
		README-3810 \
		README-6110 \
		CREDITS \
		DataCalls-QuickStart \
		README-WIN32 \
		gettext-howto \
		gnokii.nol \
		gnokii-ir-howto \
		packaging-howto \
		sample.gnokiirc

GNOKII1_MAN=man/gnokii.1 man/todologo.1
GNOKII8_MAN=man/gnokiid.8 man/mgnokiidev.8
XGNOKII_MAN=man/xgnokii.1x

all:
	@echo

install:
	$(INSTALL) -d $(docdir)
	@for xxx in $(INSTALL_DOCS); do \
	    if [ -e $$xxx ]; then \
		$(INSTALL) -m 0444 $$xxx $(docdir)/$$xxx; \
	    fi; \
	done
	$(INSTALL) $(GNOKII1_MAN) $(man1dir)
	$(INSTALL) $(GNOKII8_MAN) $(man8dir)
	if [ "x$(HAVE_XGNOKII)" = "xyes" ]; then \
		$(INSTALL) $(XGNOKII_MAN) $(xmandir); \
	fi
	@echo "done"

clean:
	$(RM) *~

depend dep:
	@echo

.PHONY: all install clean dep depend
