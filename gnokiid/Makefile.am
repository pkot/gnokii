DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
bin_PROGRAMS = gnokiid
gnokiid_SOURCES = gnokiid.c
gnokiid_LDADD = $(top_builddir)/common/libgnokii.la $(top_builddir)/common/data/libDATA.la $(INTLLIBS)
gnokiid_CFLAGS = -I$(top_srcdir)/include
