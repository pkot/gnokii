
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

OBJS =	gnokii.o

all: gnokii

gnokii: $(OBJS) $(TOPDIR)/common/COMMON.o

$(TOPDIR)/common/COMMON.o: 
	$(MAKE) -C $(TOPDIR)/common COMMON.o

clean:
	$(RM) $(OBJS) *~ depend gnokii *.exe core *.bak

install:
	$(INSTALL) -d $(bindir)
	$(INSTALL) gnokii $(bindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
