/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2010 Daniele Forsi

  This file provides an API for accessing functions via the phonet Linux kernel module.
  See README for more details on supported mobile phones.

  The various routines are called socketphonet_(whatever).

*/

#ifndef _gnokii_devices_linuxphonet_h
#define _gnokii_devices_linuxphonet_h

#include "compat.h"
#include "gnokii.h"

void* socketphonet_open(gn_config *cfg, int with_odd_parity, int with_async);
void socketphonet_close(void *instance);
int socketphonet_select(void *instance, struct timeval *timeout);
size_t socketphonet_read(void *instance, __ptr_t buf, size_t nbytes);
size_t socketphonet_write(void *instance, const __ptr_t buf, size_t n);
int socketphonet_getfd(void *instance);

#endif /* _gnokii_devices_linuxphonet_h */
