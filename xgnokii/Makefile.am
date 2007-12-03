bin_PROGRAMS = xgnokii

AM_CFLAGS = $(PTHREAD_CFLAGS) \
	  $(GTK_CFLAGS) -DGTK_ENABLE_BROKEN

xgnokii_LDADD = $(PTHREAD_LIBS) $(GTK_LIBS) $(top_builddir)/common/libgnokii.la $(top_builddir)/common/data/libDATA.la

xgnokii_SOURCES = \
	xgnokii.c \
	xgnokii_common.c \
	xgnokii_contacts.c \
	xgnokii_sms.c \
	xgnokii_netmon.c \
	xgnokii_dtmf.c \
	xgnokii_cfg.c \
	xgnokii_speed.c \
	xgnokii_xkeyb.c \
	xgnokii_calendar.c \
	xgnokii_logos.c \
	xgnokii_xring.c \
	xgnokii_lowlevel.c \
	xgnokii_data.c \
	xgnokii_calendar.h \
	xgnokii_cfg.h \
	xgnokii_common.h \
	xgnokii_contacts.h \
	xgnokii_data.h \
	xgnokii_dtmf.h \
	xgnokii.h \
	xgnokii_logos.h \
	xgnokii_lowlevel.h \
	xgnokii_netmon.h \
	xgnokii_sms.h \
	xgnokii_speed.h \
	xgnokii_xkeyb.h \
	xgnokii_xring.h

#FIXME install help
HELP_DIR = docs/help

pkgconfig_DATA = xgnokii.pc
pkgconfigdir = $(libdir)/pkgconfig

EXTRA_DIST = VERSION xgnokii.pc.in
