
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += $(PTHREAD_CFLAGS)
LDLIBS += $(PTHREAD_LIBS)

LDLIBS += -L$(TOPDIR)/common -lgnokii

OBJS = gnokiid.o

ifdef NO_SHARED
        GNOKII_LIB=libgnokii.a
else
        GNOKII_LIB=libgnokii.so
endif

all: gnokiid

gnokiid: $(OBJS) $(TOPDIR)/common/data/DATA.o $(TOPDIR)/common/$(GNOKII_LIB)
	$(CC) $(LDFLAGS) $(OBJS) $(TOPDIR)/common/data/DATA.o $(LDLIBS) -o $@

clean:
	$(RM) $(OBJS) *~ depend gnokiid *.exe core *.bak

install: all
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) gnokiid $(DESTDIR)/$(sbindir)

install-strip: all
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) -s gnokiid $(DESTDIR)/$(sbindir)
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) -o root -g gnokii -m 4750 gnokiid $(DESTDIR)/$(sbindir)
	@echo "done"

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(DESTDIR)/$(sbindir)
	$(INSTALL) -o root -g gnokii -m 4750 -s gnokiid $(DESTDIR)/$(sbindir)
	@echo "done"

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
