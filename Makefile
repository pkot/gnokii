
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=.

#
# Makefile.global contains gnokii global settings
#
include ${TOPDIR}/Makefile.global


DIRS =		common \
		gnokii
#
# For now gnokiid and utils only make sense on Unix like systems.
# Some other stuff that makes only sense on Win32 platform.
#

ifndef WIN32
DIRS +=		gnokiid \
		utils
endif

GTK_DIRS =	xgnokii \
		xlogos

PO_DIR   = 	po
DOCS_DIR = 	Docs

all:
	@for dir in $(DIRS); do \
	    if [ -e $$dir/Makefile ]; then \
		$(MAKE) -C $$dir; \
	    fi; \
	done
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR); \
	fi

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir; \
		    fi; \
		done \
	fi
	@echo "done"

clean:
	$(RM) *~
	@for dir in $(DIRS); do \
	    if [ -e $$dir/Makefile ]; then \
		$(MAKE) -C $$dir clean; \
	    fi; \
	done
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) clean; \
	fi

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir clean; \
		    fi; \
		done \
	fi
	@echo "done"

distclean:	clean
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) distclean; \
	fi
	$(RM) Makefile.global config.cache config.log config.status \
        packaging/RedHat/gnokii.spec

dep: 
	@for dir in $(DIRS); do \
	    if [ -e $$dir/Makefile ]; then \
		$(MAKE) -C $$dir dep; \
	    fi; \
	done

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir dep; \
		    fi; \
		done \
	fi
	@echo "done"

install: all
	@for dir in $(DIRS); do \
	    if [ -e $$dir/Makefile ]; then \
		$(MAKE) -C $$dir install; \
	    fi; \
	done
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) install; \
	fi

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install; \
		    fi; \
		done \
	fi
	@echo "done"

install-docs:
	$(MAKE) -C $(DOCS_DIR) install
	@echo "done"

.PHONY: all install clean distclean dep depend install-docs
