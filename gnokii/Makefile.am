bin_PROGRAMS = gnokii
gnokii_SOURCES = gnokii.c gnokii-calendar.c gnokii-dial.c gnokii-file.c gnokii-logo.c gnokii-monitor.c \
	gnokii-other.c gnokii-phonebook.c gnokii-profile.c gnokii-ringtone.c gnokii-security.c \
	gnokii-settings.c gnokii-sms.c gnokii-todo.c gnokii-utils.c gnokii-wap.c gnokii-app.h

gnokii_LDADD = $(top_builddir)/common/libgnokii.la

#if OWN_GETOPT
#	gnokii_LDADD += $(TOPDIR)/getopt/libgetopt.la
#endif

#FIXME cross compiling
#if WIN32_CROSS
#PRGNAME=gnokii.exe
#TARGET=-mconsole
#AM_CFLAGS=-DGNOKIIDLL_IMPORTS
#else
PRGNAME=gnokii
#endif

