#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings

# Compiler to use.
CC = gcc

#
# Model of the mobile phone
#

MODEL=-DMODEL="\"3810\""

#
# For Nokia 6110/5110 uncomment the next line
#

#MODEL=-DMODEL="\"6110\""

#
# Serial port for communication
#

PORT=-DPORT="\"/dev/ttyS0\""

#
# For COM2 port uncomment the next line
#

# PORT=-DPORT="\"/dev/ttyS1\""


#
# For more information about threads see the comp.programming.threads FAQ
# http://www.serpentine.com/~bos/threads-faq/
#

#
# Set up compilation/linking flags for Linux.
#

CFLAGS = -D_REENTRANT -Wall -g -O0 ${MODEL} ${PORT}
LDFLAGS = -lpthread

#
# For FreeBSD uncomment the following lines
#

# CFLAGS= -D_THREAD_SAFE -Wall -g -O2  ${MODEL} ${PORT}
# LDFLAGS= -pthread

GNOKII_OBJS = gnokii.o gsm-api.o fbus-3810.o fbus-6110.o


################## Nothing interesting after this line ##################

# Build executable
all: gnokii

gnokii: $(GNOKII_OBJS)

# Misc targets
clean:
	rm -f core *~ *% *.bak gnokii $(GNOKII_OBJS)

# Dependencies - simplified for now
gnokii.o: gnokii.c gsm-api.c gsm-api.h  misc.h gsm-common.h
gsm-api.o: gsm-api.c fbus-3810.c fbus-3810.h misc.h gsm-common.h
fbus-3810.o: fbus-3810.c fbus-3810.h misc.h gsm-common.h
fbus-6110.o: fbus-6110.c fbus-6110.h misc.h gsm-common.h
