/*
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 *
 */

#ifndef __devices_tekram_h
#define __devices_tekram_h

#include "compat.h"
#include "gnokii.h"

#define TEKRAM_B115200 0x00
#define TEKRAM_B57600  0x01
#define TEKRAM_B38400  0x02
#define TEKRAM_B19200  0x03
#define TEKRAM_B9600   0x04

#define TEKRAM_PW      0x10 /* Pulse select bit */

void* tekram_open(gn_config *cfg, int with_odd_parity, int with_async);
void tekram_close(void *instance);

void tekram_setdtrrts(void *instance, int dtr, int rts);
void tekram_changespeed(void *instance, int speed);

size_t tekram_read(void *instance, __ptr_t buf, size_t nbytes);
size_t tekram_write(void *instance, const __ptr_t buf, size_t n);

int tekram_select(void *instance, struct timeval *timeout);

#endif  /* __devices_tekram_h */
