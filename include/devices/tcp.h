/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2002 Jan Kratochvil

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#ifndef __devices_tcp_h
#define __devices_tcp_h

#ifdef WIN32
  #include <stddef.h>
  /* FIXME: this should be solved in config.h in 0.4.0 */
  #define __const const
  typedef void * __ptr_t;
#else
  #include <unistd.h>
#endif	/* WIN32 */

#include "misc.h"

int tcp_open(__const char *__file);
int tcp_close(int __fd);

int tcp_opendevice(__const char *__file, int __with_async);

size_t tcp_read(int __fd, __ptr_t __buf, size_t __nbytes);
size_t tcp_write(int __fd, __const __ptr_t __buf, size_t __n);

int tcp_select(int fd, struct timeval *timeout);

#endif  /* __devices_tcp_h */
