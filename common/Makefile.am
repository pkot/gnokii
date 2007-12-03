# REMEMBER: if you change the numbers below, please change them in gnokii.h as well.
# Change it in the following manner
#  - if you change the code and don't change API, increase GNOKII_RELEASE
#  - if you change API in backward compatible manner (adding things),
#    increase GNOKII_MAJOR, GNOKII_MINOR, set GNOKII_RELEASE to 0
#  - if you change API in backward incompatible manner (removing things,
#    changing things), increase GNOKII_MAJOR, set GNOKII_RELEASE to 0
# Change is just once per release!
GNOKII_MAJOR = 8
GNOKII_MINOR = 5
GNOKII_RELEASE = 0

#if WIN32_CROSS
#LIBTOOL_UNDEF = -no-undefined
#LIBTOOL_VER = -version-info $(GNOKII_MAJOR):$(GNOKII_RELEASE):$(GNOKII_MINOR) -avoid-version
#LIBTOOL_RPATH=-rpath $(libdir)
#LDLIBS = -lws2_32 -lwinmm
#else
LIBTOOL_VER = -version-info $(GNOKII_MAJOR):$(GNOKII_RELEASE):$(GNOKII_MINOR)
LIBTOOL_RPATH=-rpath $(libdir)
#endif

if WIN32
DATA_DIR = 
else
DATA_DIR = data
endif

lib_LTLIBRARIES = libgnokii.la

SUBDIRS = phones \
	links \
	devices \
	$(DATA_DIR)

LIBS   += $(LIBPCSCLITE_LIBS)

pkgconfig_DATA = gnokii.pc
pkgconfigdir = $(libdir)/pkgconfig

libgnokii_la_CFLAGS = -DLIBDIR="\"$(libdir)\""
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
	gsm-call.c \
	gsm-encoding.c \
	gsm-common.c \
	sms-nokia.c \
	nokia-decoding.c \
	pkt.c \
	compat.c \
	misc.c \
	snprintf.c \
	localcharset.c \
	localcharset.h

libgnokii_la_LIBADD = \
	$(builddir)/phones/libPHONES.la \
	$(builddir)/links/libLINKS.la \
	$(builddir)/devices/libDEVICES.la \
	$(XPM_LIBS)

gnvcal.c: vcal.lx
	$(LEX) -ognvcal.c vcal.lx

CLEANFILES = gnvcal.c

