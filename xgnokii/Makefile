XVERSION = 19990914
BIN = ../bin
LN = /bin/ln

#DEBUG += -DXDEBUG 

GTKCFLAGS=`gtk-config --cflags`
GTKLDFLAGS=`gtk-config --libs`

#
# Set up compilation/linking flags for Linux.
#

COMMON=-Wall -g -O0 \
       ${MODEL} ${PORT} \
       ${GETTEXT} \
       ${DEBUG} \
       -DVERSION=\"${VERSION}\" \
       -DXVERSION=\"${XVERSION}\"

CFLAGS = -D_REENTRANT ${COMMON} ${GTKCFLAGS}
LDFLAGS = -lpthread ${GTKLDFLAGS}

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
              ../gsm-networks.o ../cfgreader.o

#
# Object files for each utility
#

XGNOKII_OBJS = xgnokii.o xgnokii_common.o xgnokii_contacts.o xgnokii_sms.o

# Build executable
all: xgnokii

xgnokii: $(XGNOKII_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $(XGNOKII_OBJS) $(COMMON_OBJS) $(LDFLAGS)
	if [ ! -d $(BIN) ]; then mkdir $(BIN); fi 
	if [ -f $(BIN)/xgnokii ]; then rm -f $(BIN)/xgnokii; fi
	$(LN) xgnokii $(BIN)/xgnokii

# Misc targets
clean:
	@rm -f core *~ *% *.bak \
               xgnokii $(BIN)/xgnokii $(XGNOKII_OBJS)

# Dependencies - simplified for now
xgnokii.o: xgnokii.c xgnokii.h xgnokii_common.h ../gsm-api.c ../gsm-api.h ../misc.h ../gsm-common.h
xgnokii_contacts.o: xgnokii_contacts.c xgnokii_contacts.h xgnokii_common.h xgnokii.h ../misc.h ../gsm-api.c ../gsm-api.h ../gsm-common.h
xgnokii_sms.o: xgnokii_sms.c xgnokii_sms.h xgnokii_common.h xgnokii.h ../misc.h ../gsm-api.h ../gsm-api.c ../gsm-common.h
xgnokii_common.o: xgnokii_common.c xgnokii_common.h
