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

int irda_open(struct gn_statemachine *state) { return -1; }
int irda_close(int fd, struct gn_statemachine *state) { return -1; }
int irda_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state) { return -1; }
int irda_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state) { return -1; }
int irda_select(int fd, struct timeval *timeout, struct gn_statemachine *state) { return -1; }

#endif /* HAVE_IRDA */
