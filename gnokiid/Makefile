
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

ifdef XPM_LIBS
	LDFLAGS +=$(XPM_LIBS)
endif

OBJS =	at-emulator.o \
	datapump.o \
	gnokiid.o \
	virtmodem.o

all: gnokiid

gnokiid: $(OBJS) $(TOPDIR)/common/COMMON.o

$(TOPDIR)/common/COMMON.o: 
	$(MAKE) -C $(TOPDIR)/common COMMON.o

clean:
	$(RM) $(OBJS) *~ depend gnokiid *.exe core *.bak

install:
	$(INSTALL) -d $(sbindir)
	$(INSTALL) gnokiid $(sbindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
