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

DIST_SUBDIRS = 		\
	common		\
	Docs		\
	getopt		\
	gnokii		\
	gnokiid		\
	include		\
	po		\
	smsd		\
	snprintf	\
	testsuite	\
	utils		\
	Windows		\
	xgnokii

EXTRA_DIST =		\
	ChangeLog	\
	COPYING		\
	COPYRIGHT	\
	MAINTAINERS	\
	TODO

DISTCLEANFILES =

ACLOCAL_AMFLAGS = -I m4
