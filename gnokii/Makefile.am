DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

bin_PROGRAMS = gnokii
gnokii_SOURCES = gnokii.c gnokii-calendar.c gnokii-dial.c gnokii-file.c gnokii-logo.c gnokii-mms.c \
	gnokii-monitor.c \
	gnokii-other.c gnokii-phonebook.c gnokii-profile.c gnokii-ringtone.c gnokii-security.c \
	gnokii-settings.c gnokii-sms.c gnokii-todo.c gnokii-utils.c gnokii-wap.c gnokii-app.h

gnokii_LDADD = $(top_builddir)/common/libgnokii.la $(INTLLIBS) $(TERMLDFLAGS) $(TERMLIBS)
gnokii_CFLAGS = -I$(top_srcdir)/include

# Following http://sourceware.org/ml/automake/2000-08/msg00014.html:
# Compile getopt1.c and getopt.c unconditionally on all platforms, it
# has its own magic for the platforms that already have it.
gnokii_LDADD += $(top_builddir)/getopt/libgetopt.a

#FIXME cross compiling
#if WIN32_CROSS
#PRGNAME=gnokii.exe
#TARGET=-mconsole
#AM_CFLAGS=-DGNOKIIDLL_IMPORTS
#else
PRGNAME=gnokii
#endif

