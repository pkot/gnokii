
#
# $Id$
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

BIN_DIRS = gnokii

ifndef WIN32
BIN_DIRS += gnokiid utils
endif

DIRS =  common/phones \
	common/links \
	common/devices \
	common/data \
	common \
	$(BIN_DIRS)

GTK_DIRS =  xgnokii

PO_DIR   = 	po
DOCS_DIR = 	Docs

all: $(DIRS)
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

dummy:

$(DIRS): dummy
	$(MAKE) -C $@

clean:
	$(RM) *~ *.orig *.rej include/*~ include/*.orig include/*.rej testsuite/myout*
	@for dir in $(DIRS); do \
	    if [ -e $$dir/Makefile ]; then \
		$(MAKE) -C $$dir clean; \
	    fi; \
	done
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) clean; \
	fi

ifdef OWN_GETOPT
		$(MAKE) -C getopt clean
endif

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir clean; \
		    fi; \
		done \
	fi

	$(MAKE) -C Docs clean

	@echo "done"

distclean: clean
	$(MAKE) -C common distclean
	@if [ -e $(PO_DIR)/Makefile ]; then \
		$(MAKE) -C $(PO_DIR) distclean; \
	fi
	$(RM) Makefile.global config.cache config.log config.status \
		include/config.h \
		packaging/RedHat/gnokii.spec \
		packaging/Slackware/SlackBuild \
		po/Makefile.in \
		debian
	$(RM) `$(FIND) . -name "*~"`
	@echo "done"

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

test:
	( cd testsuite; ./testit )

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

install-strip:
	@for dir in $(DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-strip; \
		fi; \
	done
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) install; \
	fi

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
			if [ -e $$dir/Makefile ]; then \
				$(MAKE) -C $$dir install-strip; \
			fi; \
		done \
	fi
	@echo "done"

install-suid:
	@for dir in $(DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-suid; \
		fi; \
	done
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) install; \
	fi

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
			if [ -e $$dir/Makefile ]; then \
				$(MAKE) -C $$dir install-suid; \
			fi; \
		done \
	fi
	@echo "done"

install-ss:
	@for dir in $(DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-ss; \
		fi; \
	done
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) install; \
	fi

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
			if [ -e $$dir/Makefile ]; then \
				$(MAKE) -C $$dir install-ss; \
			fi; \
		done \
	fi
	@echo "done"

.PHONY: all install clean distclean dep depend install-docs
