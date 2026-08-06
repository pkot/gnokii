/*
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 *
 */

#ifndef __gnokii_irda_h_
#define __gnokii_irda_h_

#include "compat.h"
#include "gnokii.h"

void* irda_open(gn_config *cfg, int with_odd_parity, int with_async);
void irda_close(void *instance);
int irda_select(void *instance, struct timeval *timeout);
size_t irda_read(void *instance, __ptr_t bytes, size_t size);
size_t irda_write(void *instance, const __ptr_t bytes, size_t size);
int irda_getfd(void *instance);

#endif /* __gnokii_irda_h_ */
