
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#


TOPDIR=../
include ${TOPDIR}/Makefile.global

#
# Please, use locales code for languages and for .po files,
# instead full lang names. 
#
# .po file name is: "xx-gnokii.po" (where 'xx' is lang code)
#
LANGUAGES = 	cs \
		fi \
		de \
		nl

.SUFFIXES: .po .mo

MO_FILES = $(LANGUAGES:=-gnokii.mo)


all: $(MO_FILES)
	
.po.mo: $<
	@echo Language: $<
	$(MSGFMT) -o $@ -v $<

install:
	$(INSTALL) -d $(locdir)
	@for lang in $(LANGUAGES); do \
		$(INSTALL) -d $(locdir)/$${lang}/LC_MESSAGES/; \
                $(INSTALL) -m 0444 $${lang}-gnokii.mo $(locdir)/$${lang}/LC_MESSAGES/${PACKAGE}.mo; \
	done
	@echo "done"

clean:
	$(RM) *~ *.mo

depend dep:
	@echo

.PHONY: all install clean dep depend
