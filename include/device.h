/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file device access code.

  Last modification: Wed Apr 12 18:29:55 PDT 2000
  Modified by Hugh Blemings <hugh@linuxcare.com>

*/

#ifndef __device_h
#define __device_h

#include <unistd.h>
#include "misc.h"

int device_getfd(void);

int device_open(__const char *__file, int __with_odd_parity);
void device_close(void);
void device_reset(void);

void device_setdtrrts(int __dtr, int __rts);
void device_changespeed(int __speed);

size_t device_read(__ptr_t __buf, size_t __nbytes);
size_t device_write(__const __ptr_t __buf, size_t __n);

#endif  /* __device_h */
