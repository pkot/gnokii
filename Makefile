
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
DATA_DIR = common/data
endif

DIRS =  common/phones \
	common/links \
	common/devices \
	$(DATA_DIR) \
	po \
	common \
	$(BIN_DIRS)

GTK_DIRS =	xgnokii

INSTALL_DIRS =	$(BIN_DIRS) \
		common

INSTALL_SIMPLE =	po \
			intl \
			include

DOCS_DIR = 	Docs

all: intl $(DIRS)
	@if [ "$(GTK_LIBS)" ]; then \
		for dir in $(GTK_DIRS); do \
		    if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir; \
		    fi; \
		done \
	fi
	@echo "done"
	@echo "##########################################"
	@echo "###"
	@echo "### It is strongly recommended to run:"
	@echo "### $(MAKE) install"
	@echo "### now. Otherwise gnokii may not work."
	@echo "###"
	@echo "##########################################"

dummy:

intl: dummy
	-ln -sf include/config.h config.h
	$(MAKE) -C intl
	$(MAKE) -C intl localcharset.lo
	-$(RM) config.h

$(DIRS): dummy
	$(MAKE) -C $@

clean:
	$(RM) *~ *.orig *.rej include/*~ include/*.orig include/*.rej testsuite/myout*
	@for dir in intl $(DIRS); do \
	    if [ -e $$dir/Makefile ]; then \
		$(MAKE) -C $$dir clean; \
	    fi; \
	done

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
	$(RM) Makefile.global config.cache config.log config.status \
		include/config.h \
		packaging/RedHat/gnokii.spec \
		packaging/Slackware/SlackBuild \
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

install: all
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
	@echo "#####################################################"
	@echo "###"
	@echo "### Please make sure to have $(libdir) in"
	@echo "### the system defaults or in /etc/ld.so.conf and run"
	@echo "### /sbin/ldconfig at some time. Otherwise gnokii may"
	@echo "### not work."
	@echo "###"
	@echo "#####################################################"

install-docs:
	$(MAKE) -C $(DOCS_DIR) install
	@echo "done"

install-strip:
	@for dir in $(INSTALL_DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-strip; \
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
				$(MAKE) -C $$dir install-strip; \
			fi; \
		done \
	fi

	@echo "done"

install-suid:
	@for dir in $(INSTALL_DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-suid; \
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
				$(MAKE) -C $$dir install-suid; \
			fi; \
		done \
	fi

	@echo "done"

install-ss:
	@for dir in $(INSTALL_DIRS); do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir install-ss; \
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
				$(MAKE) -C $$dir install-ss; \
			fi; \
		done \
	fi

	@echo "done"

.PHONY: all install clean distclean dep depend install-docs
