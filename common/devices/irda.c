/*
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * Copyright (C) 1999-2000  Hugh Blemings & Pavel Janík ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 * Copyright (C) 2004       Phil Ashby
 *
 * Fake definitions for irda handling functions.
 */

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"

#ifndef HAVE_IRDA

void* irda_open(gn_config *cfg, int with_odd_parity, int with_async) { return NULL; }
void irda_close(void *instance) { }
size_t irda_write(void *instance, const __ptr_t bytes, size_t size) { return -1; }
size_t irda_read(void *instance, __ptr_t bytes, size_t size) { return -1; }
int irda_select(void *instance, struct timeval *timeout) { return -1; }

#endif /* HAVE_IRDA */
