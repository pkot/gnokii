/*
 *
 * $Id$
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for Nokia mobile phones.
 *
 * Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __unix_irda_h_
#define __unix_irda_h_

#ifdef HAVE_IRDA

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/irda.h>

#endif

#include "misc.h"
#include "gsm-api.h"

int irda_open(struct gn_statemachine *state);
int irda_close(int fd, struct gn_statemachine *state);
int irda_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state);
int irda_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state);
int irda_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#endif
