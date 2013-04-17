SUBDIRS = data

EXTRA_DIST = 			\
	out.00.identify		\
	out.01.sms		\
	out.02.concatsms	\
	out.10.oplogo		\
	out.11.ringtone		\
	out.12.picture		\
	out.13.emspicture	\
	out.14.oplogo-xpm	\
	out.15.emspicture2	\
	out.16.imelody		\
	out.17.picture2		\
	out.18.emsanimation	\
	out.19.emspicture3	\
	out.26.cimelody		\
	out.27.cimelody2	\
	out.28.cemsanimation	\
	test.00.identify	\
	test.01.sms		\
	test.02.concatsms	\
	test.10.oplogo		\
	test.11.ringtone	\
	test.12.picture		\
	test.13.emspicture	\
	test.14.oplogo-xpm	\
	test.15.emspicture2	\
	test.16.imelody		\
	test.17.picture2	\
	test.18.emsanimation	\
	test.19.emspicture3	\
	test.26.cimelody	\
	test.27.cimelody2	\
	test.28.cemsanimation	\
	testit

check: $(EXTRA_DIST)
	$(srcdir)/testit $(srcdir)/ "$(top_builddir)/gnokii/gnokii --config $(srcdir)/.gnokiirc"

noinst_PROGRAMS = dump-vcard
dump_vcard_LDADD = $(top_builddir)/common/libgnokii.la
dump_vcard_SOURCES = dump-vcard.c
dump_vcard_CFLAGS = -I$(top_srcdir)/include
