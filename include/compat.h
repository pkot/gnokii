/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2002, BORBELY Zoltan

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for various platform compatibility.

*/

#ifndef	__gnokii_compat_h
#define	__gnokii_compat_h

#ifdef WIN32
#  include <windows.h>
#  include <string.h>
#else
#  include <sys/time.h>
#endif

#ifndef	HAVE_TIMEOPS

/* The following code is borrowed from glibc */

/* Convenience macros for operations on timevals.
   NOTE: `timercmp' does not work for >= or <=.  */
# define timerisset(tvp)	((tvp)->tv_sec || (tvp)->tv_usec)
# define timerclear(tvp)	((tvp)->tv_sec = (tvp)->tv_usec = 0)
# define timercmp(a, b, CMP) 						      \
  (((a)->tv_sec == (b)->tv_sec) ? 					      \
   ((a)->tv_usec CMP (b)->tv_usec) : 					      \
   ((a)->tv_sec CMP (b)->tv_sec))
# define timeradd(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;			      \
    if ((result)->tv_usec >= 1000000)					      \
      {									      \
	++(result)->tv_sec;						      \
	(result)->tv_usec -= 1000000;					      \
      }									      \
  } while (0)
# define timersub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;			      \
    if ((result)->tv_usec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_usec += 1000000;					      \
    }									      \
  } while (0)

#endif	/* HAVE_TIMEOPS */

#ifndef	HAVE_GETTIMEOFDAY
int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#ifdef WIN32
/*
 * This is inspired by Marcin Wiacek <marcin-wiacek@topnet.pl>, it should help
 * windows compilers (MS VC 6)
 */
#  define inline /* Not supported */
#  define snprintf _snprintf
#  define strcasecmp strcmp
#  define strncasecmp _strnicmp
#  define sleep(x) Sleep((x) * 1000)
#  define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
#endif

#endif
