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

#include "config.h"
#include "gnokii.h"

#ifdef HAVE_SOCKETPHONET

int socketphonet_close(struct gn_statemachine *state);
int socketphonet_open(const char *iface, int with_async, struct gn_statemachine *state);
size_t socketphonet_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state);
size_t socketphonet_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state);
int socketphonet_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#else

int socketphonet_close(struct gn_statemachine *state)
{
	return -1;
}

int socketphonet_open(const char *iface, int with_async, struct gn_statemachine *state)
{
	return -1;
}

size_t socketphonet_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	return -1;
}

size_t socketphonet_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	return -1;
}

int socketphonet_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	return -1;
}

#endif

#endif /* _gnokii_devices_linuxphonet_h */
