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

  Copyright (C) 2002, BORBELY Zoltan

  Various compatibility functions.

*/

#ifdef WIN32
#  include <windows.h>
#  include <sys/timeb.h>
#  include <time.h>
#  define ftime _ftime
#  define timeb _timeb
#else
#  include <sys/time.h>
#endif

#include "config.h"
#include "compat.h"

#ifndef	HAVE_GETTIMEOFDAY

int gettimeofday(struct timeval *tv, void *tz)
{
	struct timeb t;

	ftime(&t);

	if (tv) {
		tv->tv_sec = t.time;
		tv->tv_usec = 1000 * t.millitm;
	}

	return 0;
}

#endif
