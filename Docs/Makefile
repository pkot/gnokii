
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#


TOPDIR=../
include ${TOPDIR}/Makefile.global


#
# Documentation for gnokii (xgnokii install docs itself).
#
INSTALL_DOCS = 	README \
		README-3810 \
		README-6110 \
		CREDITS

SAMPLE_DIR = sample

all: 
	@echo
	
install:
	$(INSTALL) -d $(docdir)
	@for xxx in $(INSTALL_DOCS); do \
	    if [ -e $$xxx ]; then \
		$(INSTALL) -m 0444 $$xxx $(docdir)/$$xxx; \
	    fi; \
	done
	$(INSTALL) -d $(docdir)/sample
	$(INSTALL) -m 0444 $(SAMPLE_DIR)/* $(docdir)/sample
	@echo "done"
	@echo "done"

clean:
	$(RM) *~ 

depend dep:
	@echo

.PHONY: all install clean dep depend
