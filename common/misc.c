/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Jul  4 23:26:47 CEST 2000
  Modified by Pawe³ Kot <pkot@linuxnews.pl>

*/

#include "misc.h"

#ifndef HAVE_TIMEOPS

/* FIXME: I have timersub defined in sys/time.h :-( PJ
   FIXME: Jano wants this function too... PJ

int timersub(struct timeval *t1, struct timeval *t2, struct timeval *t3) {
  t3->tv_usec = t1->tv_usec - t2->tv_usec;
  t3->tv_sec = t1->tv_sec - t2->tv_sec;
  if (t3->tv_usec < 0) {
    t3->tv_usec = 100 - t3->tv_usec;
	 t3->tv_sec--;
  }
  return 0;
}
*/

#endif
