
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

#
# For this common directory is used "subsystem.o" .o files concept.
#   (the list of object files to be linked together (to COMMON.o),
#   and other dirs is used this _one_ file (instead of all OBJS)
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

CFLAGS += $(PTHREAD_CFLAGS)

OBJS =	cfgreader.o \
	device.o \
	fbus-3810.o \
	fbus-6110-auth.o \
	fbus-6110-ringtones.o \
	fbus-6110.o \
	gsm-api.o \
	gsm-networks.o \
	mbus-2110.o \
	mbus-640.o \
	rlp-common.o \
	rlp-crc24.o

ifdef WIN32
	OBJS += $(TOPDIR)/win32/winserial.o
else
	OBJS += unixserial.o
endif

ifdef XPM_CFLAGS
    CFLAGS += $(XPM_CFLAGS)
endif

all: COMMON.o

COMMON.o: $(OBJS)
	$(LD) $(LDREL) $(LDOUT) COMMON.o $(OBJS)

gsm-filetypes.o:
	$(CC) $(CFLAGS) -c gsm-filetypes.c

clean:
	$(RM) $(OBJS) *~ depend COMMON.o

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend
