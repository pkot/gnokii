
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#


TOPDIR=..
include $(TOPDIR)/Makefile.global

OBJS =	getopt.o \
	winserial.o

all: $(OBJS)

clean:
	$(RM) $(OBJS) *~ depend

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

install:
	@echo	

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
