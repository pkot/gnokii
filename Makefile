
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
BIN_DIRS += gnokiid
endif

DIRS =  common/phones \
	common/links \
	common/devices \
	common/data \
        common \
	$(BIN_DIRS)

#
# For now gnokiid and utils only make sense on Unix like systems.
# Some other stuff that makes only sense on Win32 platform.
#

ifndef WIN32
DIRS +=	utils
endif

GTK_DIRS =  xgnokii \
            xlogos

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
	$(RM) *~ *.orig *.rej include/*~ include/*.orig include/*.rej
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

distclean:	clean
	@if [ "x$(USE_NLS)" = xyes ]; then \
		$(MAKE) -C $(PO_DIR) distclean; \
	fi
	$(RM) Makefile.global config.cache config.log config.status \
		include/config.h \
		include/config.h.in \
		packaging/RedHat/gnokii.spec \
		packaging/Slackware/SlackBuild \
		po/Makefile.in \
		debian

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

install-strip:
	@for dir in $(BIN_DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-strip; \
		fi; \
	done

	@if [ "$(GTK_LIBS)" ]; then \
		@for dir in $(GTK_DIRS); do \
			if [ -e $$dir/Makefile ]; then \
				$(MAKE) -C $$dir install-strip; \
			fi; \
		done \
	fi
	@echo "done"

install-suid:
	@for dir in $(BIN_DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-suid; \
		fi; \
	done
	@if [ "$(GTK_LIBS)" ]; then \
		@for dir in $(GTK_DIRS); do \
			if [ -e $$dir/Makefile ]; then \
				$(MAKE) -C $$dir install-suid; \
			fi; \
		done \
	fi
	@echo "done"

install-ss:
	@for dir in $(BIN_DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-ss; \
		fi; \
	done

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
			if [ -e $$dir/Makefile ]; then \
				$(MAKE) -C $$dir install-ss; \
			fi; \
		done \
	fi
	@echo "done"

.PHONY: all install clean distclean dep depend install-docs
