/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

*/

#ifndef __devices_winserial_h_
#define __devices_winserial_h_

#include "misc.h"
#include "gsm-error.h"

int serial_open(char *file, int oflag);
int serial_close(int fd);

int serial_opendevice(char *file, int with_odd_parity, int with_async, int with_hw_handshake);

void serial_setdtrrts(int fd, int dtr, int rts);
GSM_Error serial_changespeed(int fd, int speed);

size_t serial_read(int fd, __ptr_t buf, size_t nbytes);
size_t serial_write(int fd, __ptr_t buf, size_t n);

int serial_select(int fd, struct timeval *timeout);

#endif  /* __devices_winserial_h_ */
