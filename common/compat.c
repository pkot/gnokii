/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2002, BORBELY Zoltan

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Various compatibility functions.

*/

#include <sys/time.h>

#include "config.h"
#include "compat.h"

#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h>
#endif

#ifndef	HAVE_GETTIMEOFDAY

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	struct timeb t;

	_ftime(&t);

	if (tv) {
		tv->tv_sec = t.time;
		tv->tv_usec = 1000 * t.millitm;
	}
	if (tz) {			/* historic */
		tz->tz_dsttime = t.dstflag;
		tz->tz_minuteswest = t.timezone;
	}

	return 0;
}

#endif
