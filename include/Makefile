#
# Makefile for the GNOKII tool includes directory
#
# Copyright (C) 2003 Pawe³ Kot
#

TOPDIR=../
include ${TOPDIR}/Makefile.global

#
# Header files to be installed
#

INSTALL_HEADERS = gnokii.h \
		gnokii/bitmaps.h \
		gnokii/common.h \
		gnokii/data.h \
		gnokii/error.h \
		gnokii/ringtones.h \
		gnokii/sms.h \
		gnokii/virtmodem.h \
		gnokii/call.h \
		gnokii/encoding.h \
		gnokii/networks.h \
		gnokii/rlp-common.h \
		gnokii/statemachine.h

all:
	@echo

install:
	$(INSTALL) -d $(DESTDIR)$(includedir)/gnokii
	@for xxx in $(INSTALL_HEADERS); do \
	    if [ -e $$xxx ]; then \
		$(INSTALL) -m 0644 $$xxx $(DESTDIR)$(includedir)/$$xxx; \
	    fi; \
        done
	@echo "done"

clean:
	@echo

depend dep:
	@echo

.PHONY: all install clean dep depend
