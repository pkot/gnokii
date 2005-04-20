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

  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>
  Copyright (C) 2003       BORBELY Zoltan
  Copyright (C) 2004       Pawel Kot, Phil Ashby

  Fake definitions for the bluetooth handling functions.

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "devices/unixbluetooth.h"

#ifndef HAVE_BLUETOOTH

int bluetooth_open(const char *addr, uint8_t channel, struct gn_statemachine *state) { return -1; }
int bluetooth_close(int fd, struct gn_statemachine *state) { return -1; }
int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state) { return -1; }
int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state) { return -1; }
int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state) { return -1; }

#endif /* HAVE_BLUETOOTH */
