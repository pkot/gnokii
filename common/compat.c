/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2002, BORBELY Zoltan

  Released under the terms of the GNU GPL, see file COPYING for more details.

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
