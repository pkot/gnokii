
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

ifdef XPM_LIBS
	LDLIBS += $(XPM_LIBS)
endif

OBJS =	gnokii.o

ifdef OWN_GETOPT
	OBJS += $(TOPDIR)/getopt/libgetopt.a
endif

ifdef NO_SHARED
	GNOKII_LIB=libgnokii.a
else
	GNOKII_LIB=libgnokii.so
endif

LDLIBS += -L$(TOPDIR)/common -lgnokii

all: gnokii

gnokii: $(OBJS) $(TOPDIR)/common/$(GNOKII_LIB)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(TOPDIR)/getopt/libgetopt.a:
	$(MAKE) -C $(TOPDIR)/getopt

clean:
	$(RM) $(OBJS) *~ depend gnokii *.exe core *.bak

install: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) gnokii $(DESTDIR)$(bindir)

install-strip: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -s gnokii $(DESTDIR)$(bindir)
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -o root -g gnokii -m 0750 gnokii $(DESTDIR)$(bindir)
	@echo

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -o root -g gnokii -m 0750 -s gnokii $(DESTDIR)$(bindir)
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
