sbin_PROGRAMS = mgnokiidev
mgnokiidev_SOURCES = mgnokiidev.c

scripts_DATA = sendsms
scriptsdir = $(bindir)

EXTRA_DIST = gnapplet.sis

gnapplet.sis:
	wget -q --no-clobber -O $@ http://www.gnokii.org/download/gnapplet/gnapplet.sis
