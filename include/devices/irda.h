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

#include "config.h"
#include "gnokii.h"

#ifdef HAVE_IRDA

int irda_open(struct gn_statemachine *state);
int irda_close(int fd, struct gn_statemachine *state);
int irda_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state);
int irda_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state);
int irda_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#else

int irda_open(struct gn_statemachine *state)
{
	return -1;
}

int irda_close(int fd, struct gn_statemachine *state)
{
	return -1;
}

int irda_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return -1;
}

int irda_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return -1;
}

int irda_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	return -1;
}

#endif

#endif /* __gnokii_irda_h_ */
