/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002      BORBELY Zoltan, Ladis Michl, Pawel Kot, Chris Kemp
  Header file device access code.

*/

#ifndef _gnokii_device_h
#define _gnokii_device_h

#include "compat.h"
#include "misc.h"
#include "gnokii.h"

GNOKII_API int device_getfd(struct gn_statemachine *state);

int device_open(const char *file, int with_odd_parity, int with_async,
		int with_hw_handshake, gn_connection_type device_type,
		struct gn_statemachine *state);
void device_close(struct gn_statemachine *state);
void device_reset(struct gn_statemachine *state);

void device_setdtrrts(int dtr, int rts, struct gn_statemachine *state);
void device_changespeed(int speed, struct gn_statemachine *state);

size_t device_read(__ptr_t buf, size_t nbytes, struct gn_statemachine *state);
size_t device_write(const __ptr_t buf, size_t n, struct gn_statemachine *state);

int device_select(struct timeval *timeout, struct gn_statemachine *state);

gn_error device_nreceived(int *n, struct gn_statemachine *state);
gn_error device_flush(struct gn_statemachine *state);

#endif  /* _gnokii_device_h */
