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

  Copyright (C) 2002 Jan Kratochvil

*/

#ifndef __devices_tcp_h
#define __devices_tcp_h

#include "compat.h"
#include "misc.h"
#include "gnokii.h"

int tcp_opendevice(const char *file, int with_async, struct gn_statemachine *state);
int tcp_close(int fd, struct gn_statemachine *state);

size_t tcp_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state);
size_t tcp_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state);

int tcp_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#endif  /* __devices_tcp_h */
