
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=..
include $(TOPDIR)/Makefile.global

OBJS =	gnokii.o gnokii-calendar.o gnokii-dial.o gnokii-file.o gnokii-logo.o gnokii-monitor.o \
	gnokii-other.o gnokii-phonebook.o gnokii-profile.o gnokii-ringtone.o gnokii-security.o \
	gnokii-settings.o gnokii-sms.o gnokii-todo.o gnokii-utils.o gnokii-wap.o

ifdef OWN_GETOPT
	OBJS += $(TOPDIR)/getopt/libgetopt.a
endif

ifndef WIN32_CROSS
PRGNAME=gnokii
else
PRGNAME=gnokii.exe
TARGET=-mconsole
CFLAGS+=-DGNOKIIDLL_IMPORTS
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

install: gnokii
	echo $(DESTDIR)
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) $(PRGNAME) $(DESTDIR)$(bindir)

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif

.PHONY: all install clean dep depend
