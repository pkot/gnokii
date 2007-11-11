
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

DIRS += $(INTL_DIR) \
	common \
	$(BIN_DIRS) \
	$(POSUB)

GTK_DIRS =	xgnokii

INSTALL_DIRS =	$(BIN_DIRS) \
		common

INSTALL_SIMPLE = $(INTL_DIR) \
		 $(POSUB)

INSTALL_INCLUDES = include \
		   common

DOCS_DIR = 	Docs

TOPLEVEL_DOCS = ChangeLog \
		COPYING \
		COPYRIGHT \
		MAINTAINERS \
		TODO

all: compile

compile: $(DIRS)
	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir; \
		    fi; \
		done \
	fi
	@echo "done"

# build the apps after building the library   
$(BIN_DIRS) $(GTK_DIRS): common

$(DIRS):
	$(MAKE) -C $@

clean:
	$(RM) *~ *.orig *.rej include/*~ include/*.orig include/*.rej testsuite/myout*
	@for dir in $(DIRS); do \
	    if [ -e $$dir/Makefile ]; then \
		$(MAKE) -C $$dir clean; \
	    fi; \
	done

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
	$(RM) Makefile.global config.cache config.log config.status \
		include/config.h \
		packaging/RedHat/gnokii.spec \
		packaging/Slackware/SlackBuild \
		packaging/Slackware/SlackBuild-xgnokii \
		po/Makefile.in
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

install-binaries: compile
	@for dir in $(INSTALL_DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install; \
		fi; \
	done

	@for dir in $(INSTALL_SIMPLE); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install; \
		fi; \
	done

	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install; \
		    fi; \
		done \
	fi

	@echo "done"

install-includes:
	@for dir in $(INSTALL_INCLUDES); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install; \
		fi; \
	done

install-docs:
	$(INSTALL) -d $(DESTDIR)$(docdir)
	@for xxx in $(TOPLEVEL_DOCS); do \
	    if [ -e $$xxx ]; then \
		$(INSTALL_DATA) $$xxx $(DESTDIR)$(docdir)/$$xxx; \
	    fi; \
	done
	$(MAKE) -C $(DOCS_DIR) install
	@echo "done"

install-docs-devel:
	$(MAKE) -C $(DOCS_DIR) install-devel
	@echo "done"

install: compile install-binaries install-docs

install-devel: compile install-binaries install-includes install-docs install-docs-devel

.PHONY: all compile install clean distclean dep depend install-binaries install-docs install-docs-devel $(DIRS)
