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

#ifndef __devices_tekram_h
#define __devices_tekram_h

#ifdef WIN32
#  include <stddef.h>
#else
#  include <unistd.h>
#endif	/* WIN32 */

#include "misc.h"

#define TEKRAM_B115200 0x00
#define TEKRAM_B57600  0x01
#define TEKRAM_B38400  0x02
#define TEKRAM_B19200  0x03
#define TEKRAM_B9600   0x04

#define TEKRAM_PW      0x10 /* Pulse select bit */

int tekram_open(const char *file, struct gn_statemachine *state);
void tekram_close(int fd, struct gn_statemachine *state);

void tekram_setdtrrts(int fd, int dtr, int rts, struct gn_statemachine *state);
void tekram_changespeed(int fd, int speed, struct gn_statemachine *state);

size_t tekram_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state);
size_t tekram_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state);

int tekram_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#endif  /* __devices_tekram_h */
