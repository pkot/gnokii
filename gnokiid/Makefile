
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS	+= $(PTHREAD_CFLAGS)
LDFLAGS += $(PTHREAD_LIBS)

LDFLAGS += -L$(TOPDIR)/common -lgnokii

OBJS =	gnokiid.o

all: gnokiid

gnokiid: $(OBJS) $(TOPDIR)/common/data/DATA.o

$(TOPDIR)/common/data/DATA.o:
	$(MAKE) -C $(TOPDIR)/common/data DATA.o

clean:
	$(RM) $(OBJS) *~ depend gnokiid *.exe core *.bak

install: all
	$(INSTALL) -d $(sbindir)
	$(INSTALL) gnokiid $(sbindir)

install-strip: all
	$(INSTALL) -d $(sbindir)
	$(INSTALL) -s gnokiid $(sbindir)
	@echo "done"

install-suid: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(sbindir)
	$(INSTALL) -o root -g gnokii -m 4750 gnokiid $(sbindir)
	@echo "done"

install-ss: all
	@if [ -z "/bin/grep -e '^gnokii:' /etc/group" ]; then \
		/usr/sbin/groupadd gnokii; \
	fi
	$(INSTALL) -d $(sbindir)
	$(INSTALL) -o root -g gnokii -m 4750 -s gnokiid $(sbindir)
	@echo "done"

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
