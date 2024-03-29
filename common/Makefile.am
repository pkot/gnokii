DEFS = -DLOCALEDIR=\"$(localedir)\" -DCOMPILING_LIBGNOKII @DEFS@

if WIN32
DATA_DIR = 
else
DATA_DIR = data
endif

if HAVE_POSIX_SPAWN
DEVICE_SCRIPT = posixscript.c
endif

lib_LTLIBRARIES = libgnokii.la

SUBDIRS = phones \
	links \
	devices \
	$(DATA_DIR)

LIBS   += $(LIBPCSCLITE_LIBS) $(GLIB_LIBS)

pkgconfig_DATA = gnokii.pc
pkgconfigdir = $(libdir)/pkgconfig

libgnokii_la_CPPFLAGS = -DLIBDIR="\"$(libdir)\""
libgnokii_la_CFLAGS = -I$(top_srcdir)/include $(GLIB_CFLAGS) \
	$(ICAL_CFLAGS)

libgnokii_la_SOURCES = \
	libfunctions.c \
	gsm-api.c \
	gsm-error.c \
	gsm-statemachine.c \
	cfgreader.c \
	device.c \
	vcard.c \
	vcal.c \
	gnvcal.c \
	ldif.c \
	gsm-networks.c \
	gsm-filetypes.c \
	readmidi.c \
	gsm-ringtones.c \
	gsm-bitmaps.c \
	gsm-sms.c \
	gsm-mms.c \
	gsm-call.c \
	gsm-encoding.c \
	gsm-common.c \
	sms-nokia.c \
	nokia-decoding.c \
	pkt.c \
	compat.c \
	misc.c \
	localcharset.c \
	map.c \
	gsm-auth.c \
	$(DEVICE_SCRIPT)

libgnokii_la_LIBADD = \
	$(top_builddir)/snprintf/libsnprintf.la \
	$(top_builddir)/common/phones/libPHONES.la \
	$(top_builddir)/common/links/libLINKS.la \
	$(top_builddir)/common/devices/libDEVICES.la \
	$(XPM_LIBS) \
	$(LTLIBICONV) \
	$(INTLLIBS) \
	$(BLUETOOTH_LIBS) \
	$(USB_LIBS) \
	$(TCP_LIBS) \
	$(ICAL_LIBS)

libgnokii_la_LDFLAGS = 				\
	-version-info $(GNOKII_LT_VERSION)	\
	-no-undefined

gnvcal.c: vcal.lx
	$(LEX) -ognvcal.c '$(srcdir)/vcal.lx'

CLEANFILES = gnvcal.c
EXTRA_DIST = gnokii.pc.in gnvcal.c vcal.lx
