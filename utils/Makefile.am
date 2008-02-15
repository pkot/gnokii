sbin_PROGRAMS = mgnokiidev
mgnokiidev_SOURCES = mgnokiidev.c
mgnokiidev_LDADD = $(INTLLIBS)
mgnokiidev_CFLAGS = -I$(top_srcdir)/include

scripts_DATA = sendsms
scriptsdir = $(bindir)

EXTRA_DIST = gnapplet.sis $(scripts_DATA)

gnapplet.sis:
	wget -q -r -O $@ http://www.gnokii.org/download/gnapplet/gnapplet.sis
