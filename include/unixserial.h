/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Wed Dec 15 00:17:17 MET 1999
  Modified by Marcel Holtmann <marcel@rvs.uni-bielefeld.de>

*/

#ifndef __unixserial_h
#define __unixserial_h

#ifdef WIN32
#include <stddef.h>

    #define __const const
    typedef void * __ptr_t;
#else
    #include <unistd.h>
#endif	/* WIN32 */

#define HAVE_TERMIOS_CSPEED
#define HAVE_CFSETSPEED

int serial_open(__const char *__file, int __oflag);
int serial_close(int __fd);

int serial_opendevice(__const char *__file);

void serial_setdtrrts(int __fd, int __dtr, int __rts);
void serial_changespeed(int __fd, int __speed);

size_t serial_read(int __fd, __ptr_t __buf, size_t __nbytes);
size_t serial_write(int __fd, __const __ptr_t __buf, size_t __n);

#endif  /* __unixserial_h */
