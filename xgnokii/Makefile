XVERSION = 19991207

XGNOKIIDIR=${DESTDIR}/lib/xgnokii

# DEBUG += -DXDEBUG 

#
# Set up compilation/linking flags for Linux.
#

COMMON=-Wall -O2 \
       ${MODEL} ${PORT} \
       ${GETTEXT} \
       ${SECURITY} \
       ${DEBUG} \
       -DVERSION=\"${VERSION}\" \
       -DXVERSION=\"${XVERSION}\" \
       -DXGNOKIIDIR=\"${XGNOKIIDIR}\"

CFLAGS = -D_REENTRANT ${COMMON} ${GTKCFLAGS}
LDFLAGS = -s -lpthread ${GTKLDFLAGS}

#
# For FreeBSD uncomment the following lines
#

# CFLAGS= -D_THREAD_SAFE ${COMMON} -I/usr/local/include
# LDFLAGS= -pthread -L/usr/local/lib -lintl

################## Nothing interesting after this line ##################

#
# Common objects - these files are needed for all utilities
#

COMMON_OBJS = ../gsm-api.o \
              ../fbus-3810.o \
              ../mbus-2110.o \
              ../fbus-6110.o ../fbus-6110-auth.o ../fbus-6110-ringtones.o \
              ../device.o ../unixserial.o ../win32/winserial.o \
              ../gsm-networks.o ../cfgreader.o

#
# Object files for each utility
#

XGNOKII_OBJS = xgnokii.o xgnokii_common.o xgnokii_contacts.o xgnokii_sms.o \
               xgnokii_netmon.o xgnokii_dtmf.o xgnokii_cfg.o

# Build executable
all: xgnokii

xgnokii: $(XGNOKII_OBJS) $(COMMON_OBJS)

install: $(XGNOKII_OBJS) $(COMMON_OBJS)
	@echo "Installing xgnokii files..."

	@mkdir -p $(DESTDIR)/bin $(XGNOKIIDIR)/help

	@$(INSTALL) -m 755 -s xgnokii $(DESTDIR)/bin
	@echo " $(DESTDIR)/bin/xgnokii"

	@echo "Installing help files for xgnokii..."
	@$(INSTALL) -m 0444 docs/help/*.html $(XGNOKIIDIR)/help

	@echo "done xgnokii."

	
# Misc targets
clean:
	@rm -f core *~ *% *.bak xgnokii $(XGNOKII_OBJS)

# Dependencies - simplified for now
xgnokii.o: xgnokii.c xgnokii.h xgnokii_common.h xgnokii_netmon.h xgnokii_dtmf.h xgnokii_cfg.h ../gsm-api.c ../gsm-api.h ../misc.h ../gsm-common.h
xgnokii_contacts.o: xgnokii_contacts.c xgnokii_contacts.h xgnokii_common.h xgnokii.h ../misc.h ../gsm-api.c ../gsm-api.h ../gsm-common.h
xgnokii_sms.o: xgnokii_sms.c xgnokii_sms.h xgnokii_common.h xgnokii.h ../misc.h ../gsm-api.h ../gsm-api.c ../gsm-common.h
xgnokii_common.o: xgnokii_common.c xgnokii_common.h
xgnokii_cfg.o: xgnokii_cfg.c xgnokii_cfg.h xgnokii.h ../misc.h
xgnokii_netmon.o: xgnokii_netmon.c xgnokii_netmon.h xgnokii.h xgnokii_common.h ../misc.h ../gsm-api.h
xgnokii_dtmf.o: xgnokii_dtmf.c xgnokii_dtmf.h xgnokii_common.h ../misc.h ../gsm-api.h
