/*
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
 * $Log$
 * Revision 1.2  2001-11-08 16:49:20  pkot
 * Cleanups
 *
 * Revision 1.1  2001/02/21 19:57:11  chris
 * More fiddling with the directory layout
 *
 * Revision 1.4  2001/02/20 21:55:12  pkot
 * Small #include updates
 *
 * Revision 1.3  2001/02/06 21:15:37  chris
 * Preliminary irda support for 7110 etc.  Not well tested!
 *
 * Revision 1.2  2001/02/06 15:15:25  pkot
 * FreeBSD unixirda.h fix (Panagiotis Astithas)
 *
 * Revision 1.1  2001/02/03 23:56:18  chris
 * Start of work on irda support (now we just need fbus-irda.c!)
 * Proper unicode support in 7110 code (from pkot)
 *
 */

#ifndef __unix_irda_h_
#define __unix_irda_h_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "linuxirda.h"
#include "misc.h"

int irda_open(void);
int irda_close(int fd);
int irda_write(int __fd, __const __ptr_t __bytes, int __size);
int irda_read(int __fd, __ptr_t __bytes, int __size);
int irda_select(int fd, struct timeval *timeout);

#endif
