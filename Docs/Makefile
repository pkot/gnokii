#
# Makefile for the GNOKII tool suite's documentation directory
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

TOPDIR=../
include ${TOPDIR}/Makefile.global

#
# Documentation for gnokii (xgnokii install documentation files itself).
#

INSTALL_DOCS = 	README \
		README-2110 \
		README-3810 \
		README-6110 \
		README-6510 \
		README-7110 \
		README-MacOSX \
		README-WIN32 \
		README-dancall \
		README-ericsson \
		README-siemens \
		README.libsms \
		CREDITS \
		Bugs \
		DataCalls-QuickStart \
		gettext-howto \
		gnokii.nol \
		packaging-howto \
		gnokii-ir-howto \
		gnokii-IrDA-Linux \
		gnokii-hackers-howto \
		ringtones.txt \
		logos.txt \
		sample/gnokiirc \
		sample/magic \
		sample/logo/pacman.xpm \
		sample/logo/gnokii.xpm \
		sample/logo/gnokiiop.xpm \
		sample/logo/horse.xpm \
		sample/logo/bronto.xpm \
		sample/logo/horse2.xpm \
		sample/ringtone/star.imelody \
		sample/ringtone/star.rtttl \
		sample/vCalendar/test.vcs \
		sample/ppp/cimd-connect \
		sample/ppp/ppp-FILES \
		sample/ppp/options \
		sample/ppp/pap-secrets \
		sample/ppp/ppp-hscsd \
		sample/ppp/ppp-6210-modem \
		sample/ppp/ppp-gnokii \
		sample/ppp/ppp-on

GNOKII1_MAN=man/gnokii.1 man/todologo.1
GNOKII8_MAN=man/gnokiid.8 man/mgnokiidev.8
XGNOKII_MAN=man/xgnokii.1x

all:
	@echo

install:
	$(INSTALL) -d $(DESTDIR)$(docdir)
	$(INSTALL) -d $(DESTDIR)$(docdir)/sample/logo
	$(INSTALL) -d $(DESTDIR)$(docdir)/sample/ringtone
	$(INSTALL) -d $(DESTDIR)$(docdir)/sample/vCalendar
	$(INSTALL) -d $(DESTDIR)$(docdir)/sample/vCard
	$(INSTALL) -d $(DESTDIR)$(docdir)/sample/ppp
	@for xxx in $(INSTALL_DOCS); do \
	    if [ -e $$xxx ]; then \
		$(INSTALL_DATA) $$xxx $(DESTDIR)$(docdir)/$$xxx; \
	    fi; \
	done
	$(INSTALL) -d $(DESTDIR)$(man1dir)
	$(INSTALL) -d $(DESTDIR)$(man8dir)
	$(INSTALL_DATA) $(GNOKII1_MAN) $(DESTDIR)$(man1dir)
	$(INSTALL_DATA) $(GNOKII8_MAN) $(DESTDIR)$(man8dir)
	if [ "x$(HAVE_XGNOKII)" = "xyes" ]; then \
		$(INSTALL) -d $(DESTDIR)$(xmandir); \
		$(INSTALL_DATA) $(XGNOKII_MAN) $(DESTDIR)$(xmandir); \
	fi
	@echo "done"

clean:
	$(RM) *~

depend dep:
	@echo

.PHONY: all install clean dep depend
