/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Wed Apr 12 18:29:55 PDT 2000
  Modified by Hugh Blemings <hugh@linuxcare.com>

*/

#ifndef WIN32

#include "unixserial.h"
#include "device.h"

/*
 * Structure to store the filedescriptor we use.
 *
 */

int device_portfd = -1;

int device_getfd(void) {

  return device_portfd;
}

int device_open(__const char *__file, int __with_odd_parity, int __with_async) {

  device_portfd = serial_opendevice(__file, __with_odd_parity, __with_async);

  return (device_portfd >= 0);
}

void device_close(void) {

  serial_close(device_portfd);
}

void device_reset(void) {
}

void device_setdtrrts(int __dtr, int __rts) {

  serial_setdtrrts(device_portfd, __dtr, __rts);
}

void device_changespeed(int __speed) {

  serial_changespeed(device_portfd, __speed);
}

size_t device_read(__ptr_t __buf, size_t __nbytes) {

  return (serial_read(device_portfd, __buf, __nbytes));
}

size_t device_write(__const __ptr_t __buf, size_t __n) {

  return (serial_write(device_portfd, __buf, __n));
}

#endif /* WIN32 */

