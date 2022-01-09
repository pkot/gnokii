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

int serial_open(const char *file, int oflag);
int serial_close(int fd, struct gn_statemachine *state);

int serial_opendevice(const char *file, int with_odd_parity, int with_async, int with_hw_handshake, struct gn_statemachine *state);

void serial_setdtrrts(int fd, int dtr, int rts, struct gn_statemachine *state);
gn_error serial_changespeed(int fd, int speed, struct gn_statemachine *state);

size_t serial_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state);
size_t serial_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state);

int serial_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

gn_error serial_nreceived(int fd, int *n, struct gn_statemachine *state);
gn_error serial_flush(int fd, struct gn_statemachine *state);

#endif  /* __devices_serial_h */
