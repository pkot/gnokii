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

GNOKII_MAN = man/gnokii.1

all: 
	@echo

install:
	$(INSTALL) -d $(docdir)
	@for xxx in $(INSTALL_DOCS); do \
	    if [ -e $$xxx ]; then \
		$(INSTALL) -m 0444 $$xxx $(docdir)/$$xxx; \
	    fi; \
	done
	$(INSTALL) $(GNOKII_MAN) $(mandir)
	@echo "done"

clean:
	$(RM) *~ 

depend dep:
	@echo

.PHONY: all install clean dep depend
