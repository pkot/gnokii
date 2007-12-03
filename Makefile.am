SUBDIRS = include common gnokii po

# FIXME add Docs

if WIN32
else
SUBDIRS += gnokiid utils
endif

if HAVE_GTK
SUBDIRS += xgnokii
endif

if GLIB
# FIXME
#SUBDIRS += smsd
endif

DIST_SUBDIRS = include Docs common gnokii gnokiid utils xgnokii smsd po

EXTRA_DIST = ChangeLog \
		COPYING \
		COPYRIGHT \
		MAINTAINERS \
		TODO


ACLOCAL_AMFLAGS = -I m4
