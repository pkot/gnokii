
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

OBJS =	gnokii.o

ifdef OWN_GETOPT
	OBJS += $(TOPDIR)/getopt/libgetopt.a
endif

ifndef WIN32_CROSS
PRGNAME=gnokii
else
PRGNAME=gnokii.exe
TARGET=-mconsole
CFLAGS+=-DGNOKIIDLL_IMPORTS
LDLIBS+=-lws2_32 -lwinmm
endif

LDLIBS += $(TOPDIR)/common/libgnokii.la

all: gnokii

gnokii: $(OBJS) $(TOPDIR)/common/libgnokii.la
	$(LIBTOOL) --mode=link $(CC) $(TARGET) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(PRGNAME)

$(TOPDIR)/getopt/libgetopt.a:
	$(MAKE) -C $(TOPDIR)/getopt

clean:
	$(LIBTOOL) --mode=clean $(RM) gnokii $(OBJS)
	$(RM) *~ depend *.exe core *.bak

install: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) $(PRGNAME) $(DESTDIR)$(bindir)

install-strip: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -s gnokii $(DESTDIR)$(bindir)
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 0750 gnokii $(DESTDIR)$(bindir)
	@echo

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -o root -g gnokii -m 0750 -s gnokii $(DESTDIR)$(bindir)
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
