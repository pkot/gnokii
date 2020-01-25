/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002 Jan Kratochvil

*/

#ifndef __devices_tcp_h
#define __devices_tcp_h

#include "gnokii.h"

int tcp_opendevice(const char *file, int with_async, struct gn_statemachine *state);
int tcp_close(int fd, struct gn_statemachine *state);

size_t tcp_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state);
size_t tcp_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state);

int tcp_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#endif  /* __devices_tcp_h */
