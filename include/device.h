/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file device access code.

*/

#ifndef __device_h_
#define __device_h_

#include <unistd.h>
#include "misc.h"
#include "gsm-common.h"

int device_getfd(void);

int device_open(const char *file, int with_odd_parity, int with_async, int with_hw_handshake, GSM_ConnectionType device_type);
void device_close(void);
void device_reset(void);

void device_setdtrrts(int dtr, int rts);
void device_changespeed(int speed);

size_t device_read(__ptr_t buf, size_t nbytes);
size_t device_write(const __ptr_t buf, size_t n);

int device_select(struct timeval *timeout);

#ifndef WIN32
#  include "devices/unixserial.h"
#  include "devices/unixirda.h"
#  include "devices/tekram.h"
#else
#  include "devices/winserial.h"
#endif

#endif  /* __device_h_ */
