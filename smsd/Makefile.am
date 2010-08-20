bin_PROGRAMS = smsd

plugindir = $(libdir)/smsd
plugin_LTLIBRARIES = libsmsd_file.la

modules_flags = -avoid-version -module

if HAVE_POSTGRES
plugin_LTLIBRARIES += libsmsd_pq.la
libsmsd_pq_la_SOURCES = pq.c db.h utils.h utils.c
libsmsd_pq_la_LDFLAGS = $(modules_flags)
libsmsd_pq_la_LIBADD = $(POSTGRES_LIBS)
endif

if HAVE_MYSQL
plugin_LTLIBRARIES += libsmsd_mysql.la
libsmsd_mysql_la_SOURCES = mysql.c db.h utils.h utils.c
libsmsd_mysql_la_LDFLAGS = $(modules_flags)
libsmsd_mysql_la_LIBADD = $(MYSQL_LIBS)
endif

if HAVE_SQLITE
plugin_LTLIBRARIES += libsmsd_sqlite.la
libsmsd_sqlite_la_SOURCES = sqlite.c db.h utils.h utils.c
libsmsd_sqlite_la_LDFLAGS = $(modules_flags)
libsmsd_sqlite_la_LIBADD = $(SQLITE_LIBS)
endif

libsmsd_file_la_SOURCES = file.c db.h utils.h utils.c
libsmsd_file_la_LDFLAGS = $(modules_flags)
libsmsd_file_la_LIBADD = $(GLIB_LIBS)

smsd_SOURCES = smsd.h smsd.c lowlevel.c lowlevel.h db.h
smsd_LDADD = $(GLIB_LIBS) $(GMODULE_LIBS) $(PTHREAD_LIBS) $(INTLLIBS) $(top_builddir)/common/libgnokii.la

AM_CFLAGS =				\
	-I$(top_srcdir)/include		\
	-I$(srcdir)/			\
	$(GLIB_CFLAGS)			\
	$(POSTGRES_CFLAGS)		\
	$(SQLITE_CFLAGS)		\
	$(MYSQL_CFLAGS)			\
	-DMODULES_DIR=\"${plugindir}\"	\
	-DLOCALEDIR=\"$(localedir)\"	\
	$(PTHREAD_CFLAGS)

man_MANS = man/smsd.8

EXTRA_DIST = sqlite.c pq.c mysql.c README ChangeLog action README.MySQL sms.tables.mysql.sql sms.tables.pq.sql README.Tru64 $(man_MANS)
