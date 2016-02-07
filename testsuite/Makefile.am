SUBDIRS = data

EXTRA_DIST = 			\
	out.00.identify		\
	out.01.sms		\
	out.01.writephonebook	\
	out.02.writephonebook	\
	out.03.writephonebook	\
	out.04.writephonebook	\
	out.05.writephonebook	\
	out.02.concatsms	\
	out.03.ucs2sms		\
	out.08.send161sms	\
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
	out.29.sendsms		\
	out.30.emptysms		\
	out.33.send140sms8bit	\
	out.34.send141sms8bit	\
	out.40.getsms		\
	out.41.getsms		\
	out.42.getsms		\
	out.43.getsms		\
	test.00.identify	\
	test.01.sms		\
	test.01.writephonebook	\
	test.02.writephonebook	\
	test.03.writephonebook	\
	test.04.writephonebook	\
	test.05.writephonebook	\
	test.02.concatsms	\
	test.03.ucs2sms		\
	test.08.send161sms	\
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
	test.29.sendsms		\
	test.30.emptysms	\
	test.33.send140sms8bit	\
	8bit-140.bin		\
	test.34.send141sms8bit	\
	8bit-141.bin		\
	test.40.getsms		\
	test.41.getsms		\
	test.42.getsms		\
	test.43.getsms		\
	.gnokiirc		\
	testit

check: $(EXTRA_DIST)
	$(srcdir)/testit $(srcdir)/ "$(top_builddir)/gnokii/gnokii --config .gnokiirc"

clean-local:
	-rm -f myout.*

noinst_PROGRAMS = dump-vcard
dump_vcard_LDADD = $(top_builddir)/common/libgnokii.la
dump_vcard_SOURCES = dump-vcard.c
dump_vcard_CFLAGS = -I$(top_srcdir)/include
