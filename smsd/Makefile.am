bin_PROGRAMS = smsd

plugindir = $(libdir)/smsd
plugin_LTLIBRARIES = libfile.la

modules_flags = -export_dynamic -avoid-version -module

if HAVE_POSTGRES
plugin_LTLIBRARIES += libpq.la
libpq_la_SOURCES = pq.c db.h
libpq_la_LDFLAGS = $(modules_flags)
endif

if HAVE_MYSQL
plugin_LTLIBRARIES += libmysql.la
libmysql_la_SOURCES = mysql.c db.h
libmysql_la_LDFLAGS = $(modules_flags)
endif

libfile_la_SOURCES = utils.h utils.c file.c
libfile_la_LDFLAGS = $(modules_flags)

smsd_SOURCES = smsd.h smsd.c lowlevel.c
smsd_LDADD = $(GLIB_LIBS) $(PTHREAD_LIBS) $(top_builddir)/common/libgnokii.la

AM_CFLAGS =				\
	$(GLIB_CFLAGS)			\
	-DMODULES_DIR=\"${pkglibdir}\"	\
	$(PTHREAD_CFLAGS)

man_MANS = man/smsd.8

EXTRA_DIST = pq.c mysql.c README ChangeLog action README.MySQL sms.tables.mysql.sql sms.tables.pq.sql README.Tru64
