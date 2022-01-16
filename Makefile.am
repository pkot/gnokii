SUBDIRS = include getopt snprintf common gnokii po Docs

if WIN32
else
SUBDIRS += gnokiid utils
endif

if XGNOKII
SUBDIRS += xgnokii
endif

if SMSD
SUBDIRS += smsd
endif

SUBDIRS += testsuite

DIST_SUBDIRS = include Docs common gnokii gnokiid utils xgnokii po smsd Windows testsuite getopt

EXTRA_DIST =				\
		ChangeLog		\
		COPYING			\
		COPYRIGHT		\
		MAINTAINERS		\
		TODO

DISTCLEANFILES =

ACLOCAL_AMFLAGS = -I m4
