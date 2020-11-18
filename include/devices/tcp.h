/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002 Jan Kratochvil

*/

#ifndef __devices_tcp_h
#define __devices_tcp_h

#include "compat.h"
#include "gnokii.h"

void* tcp_open(gn_config *cfg, int with_odd_parity, int with_async);
void tcp_close(void *instance);

size_t tcp_read(void *instance, __ptr_t buf, size_t nbytes);
size_t tcp_write(void *instance, const __ptr_t buf, size_t n);

int tcp_select(void *instance, struct timeval *timeout);

#endif  /* __devices_tcp_h */
