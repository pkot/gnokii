#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings

# Compiler to use.
CC = gcc

# Set up compilation/linking flags.
CFLAGS = -D_REENTRANT -Wall -g -O0 -pthread 

LDFLAGS = -lpthread

GNOKII_OBJS = gnokii.o gsm-api.o fbus-3810.o fbus-6110.o

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
