SUBDIRS = getopt include common gnokii po Docs

if WIN32
else
SUBDIRS += gnokiid utils
endif

if HAVE_GTK
SUBDIRS += xgnokii
endif

if SMSD
SUBDIRS += smsd
endif

DIST_SUBDIRS = include Docs common gnokii gnokiid utils xgnokii po smsd Windows testsuite getopt

EXTRA_DIST =				\
		ChangeLog		\
		COPYING			\
		COPYRIGHT		\
		MAINTAINERS		\
		TODO			\
		intltool-extract.in	\
		intltool-merge.in	\
		intltool-update.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

ACLOCAL_AMFLAGS = -I m4
