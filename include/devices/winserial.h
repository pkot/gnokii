/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#ifndef __devices_winserial_h_
#define __devices_winserial_h_

#include "misc.h"
#include "gsm-error.h"

int serial_open(char *file, int oflag);
int serial_close(int fd);

int serial_opendevice(char *file, int with_odd_parity, int with_async, int with_hw_handshake);

void serial_setdtrrts(int fd, int dtr, int rts);
GSM_Error serial_changespeed(int fd, int speed);

size_t serial_read(int fd, __ptr_t buf, size_t nbytes);
size_t serial_write(int fd, __ptr_t buf, size_t n);

int serial_select(int fd, struct timeval *timeout);

#endif  /* __devices_winserial_h_ */
