DEFS = -DCOMPILING_LIBGNOKII

AM_CFLAGS = $(LIBPCSCLITE_CFLAGS)

noinst_LTLIBRARIES = libPHONES.la
libPHONES_la_SOURCES = \
	generic.c \
	nokia.c \
	nk7110.c \
	nk6510.c \
	nk6160.c \
	nk6100.c \
	nk3110.c \
	atgen.c \
	atbosch.c \
	ateric.c \
	athuawei.c \
	atmot.c \
	atnok.c \
	atsie.c \
	atsoer.c \
	atsam.c \
	atsag.c \
	atlg.c \
	gnapplet.c \
	fake.c \
	pcsc.c

libPHONES_la_CFLAGS =		\
	-I$(top_srcdir)/include	\
	$(AM_CFLAGS)

EXTRA_DIST = nk2110.c dc2711.c


