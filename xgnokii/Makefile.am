SUBDIRS = xpm

bin_PROGRAMS = xgnokii

AM_CFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(GTK_CFLAGS) -DGTK_ENABLE_BROKEN \
	-DLOCALEDIR=\"$(localedir)\"

xgnokii_LDADD = \
	$(PTHREAD_LIBS) $(GTK_LIBS) $(INTLLIBS) \
	$(top_builddir)/common/libgnokii.la $(top_builddir)/common/data/libDATA.la
xgnokii_CFLAGS = -I$(top_srcdir)/include $(AM_CFLAGS)

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

pkgconfig_DATA = xgnokii.pc
pkgconfigdir = $(libdir)/pkgconfig

desktop_in_files = xgnokii.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(desktop_DATA)

EXTRA_DIST = xgnokii.pc.in $(desktop_in_files) README.vcard
