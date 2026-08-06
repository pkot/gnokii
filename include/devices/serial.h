/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.

*/

#ifndef __devices_serial_h
#define __devices_serial_h

#include "compat.h"
#include "gnokii.h"

void* serial_init(const char *file, int oflag);

void* serial_open(gn_config *cfg, int with_odd_parity, int with_async);
void serial_close(void *instance);

int serial_select(void *instance, struct timeval *timeout);
size_t serial_read(void *instance, __ptr_t buf, size_t nbytes);
size_t serial_write(void *instance, const __ptr_t buf, size_t n);

int serial_getfd(void *instance);

gn_error serial_nreceived(void *instance, int *n);
gn_error serial_flush(void *instance);

gn_error serial_changespeed(void *instance, int speed);
void serial_setdtrrts(void *instance, int dtr, int rts);

#endif  /* __devices_serial_h */
