
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

ifdef XPM_LIBS
	LDFLAGS +=$(XPM_LIBS)
endif

OBJS =	gnokii.o

ifdef OWN_GETOPT
	OBJS += $(TOPDIR)/getopt/libgetopt.a
endif

LDFLAGS += -L$(TOPDIR)/common -lgnokii

all: gnokii

gnokii: $(OBJS) $(TOPDIR)/common/gsm-filetypes.o

$(TOPDIR)/common/gsm-filetypes.o:
	$(MAKE) -C $(TOPDIR)/common gsm-filetypes.o

$(TOPDIR)/getopt/libgetopt.a:
	$(MAKE) -C $(TOPDIR)/getopt

clean:
	$(RM) $(OBJS) *~ depend gnokii *.exe core *.bak

install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) gnokii $(bindir)

install-strip: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) -s gnokii $(bindir)
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(bindir)
	$(INSTALL) -o root -g gnokii -m 0750 gnokii $(bindir)
	@echo

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(bindir)
	$(INSTALL) -o root -g gnokii -m 0750 -s gnokii $(bindir)
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
