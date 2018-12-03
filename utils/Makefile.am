DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

sbin_PROGRAMS = mgnokiidev
mgnokiidev_SOURCES = mgnokiidev.c
mgnokiidev_LDADD = $(INTLLIBS) $(TCP_LIBS)
mgnokiidev_LDFLAGS = $(SUID_LDFLAGS) $(AM_LDFLAGS)
mgnokiidev_CFLAGS = -I$(top_srcdir)/include $(SUID_CFLAGS) $(AM_CFLAGS)

scripts_DATA = sendsms
scriptsdir = $(bindir)

EXTRA_DIST = gnapplet.sis $(scripts_DATA)

gnapplet.sis:
	wget -q -N -O $@ http://www.gnokii.org/download/gnapplet/gnapplet.sis
