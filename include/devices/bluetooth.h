/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>

*/

#ifndef _gnokii_bluetooth_h
#define _gnokii_bluetooth_h

#include "compat.h"
#include "gnokii.h"

void* bluetooth_open(gn_config *cfg, int with_odd_parity, int with_async);
void bluetooth_close(void *instance);
int bluetooth_select(void *instance, struct timeval *timeout);
size_t bluetooth_read(void *instance, __ptr_t bytes, size_t size);
size_t bluetooth_write(void *instance, const __ptr_t bytes, size_t size);
int bluetooth_getfd(void *instance);

#endif /* _gnokii_bluetooth_h */
