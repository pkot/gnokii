/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Jul  4 23:26:47 CEST 2000
  Modified by Pawe³ Kot <pkot@linuxnews.pl>

*/

#include <string.h>
#include "misc.h"

#ifndef HAVE_TIMEOPS

/* FIXME: I have timersub defined in sys/time.h :-( PJ
   FIXME: Jano wants this function too... PJ

int timersub(struct timeval *a, struct timeval *b, struct timeval *result) {
  do {
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;
    if ((result)->tv_usec < 0) {
      --(result)->tv_sec;
      (result)->tv_usec += 1000000;
    }
  } while (0);
}
*/

#endif

int GetLine(FILE *File, char *Line, int count) {

  char *ptr;

  if (fgets(Line, count, File)) {
    ptr=Line+strlen(Line)-1;

    while ( (*ptr == '\n' || *ptr == '\r') && ptr>=Line)
      *ptr--='\0';

      return strlen(Line);
  }
  else
    return 0;
}
