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

  Header file for various platform compatibility.

*/

#ifndef	__gnokii_compat_h
#define	__gnokii_compat_h

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#ifdef WIN32
#  include <windows.h>
#  include <string.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif


#ifndef	HAVE_TIMEOPS

#undef timerisset
#undef timerclear
#undef timercmp
#undef timeradd
#undef timersub

/* The following code is borrowed from glibc, please don't reindent it */

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
int gettimeofday(struct timeval *tv, void *tz);
#endif

#ifndef	HAVE_STRSEP
char *strsep(char **stringp, const char *delim);
#endif

#if !defined(HAVE_SNPRINTF) && !defined(HAVE_C99_SNPRINTF)
int snprintf(char *str, size_t size, const char *format, ...);
#endif

#if !defined(HAVE_VSNPRINTF) && !defined(HAVE_C99_VSNPRINTF)
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#ifndef HAVE_ASPRINTF
int asprintf(char **ptr, const char *format, ...);
#endif

#ifndef HAVE_VASPRINTF
int vasprintf(char **ptr, const char *format, va_list ap);
#endif

#ifdef WIN32
/*
 * This is inspired by Marcin Wiacek <marcin-wiacek@topnet.pl>, it should help
 * windows compilers (MS VC 6)
 */
#  define inline /* Not supported */
#  define strcasecmp stricmp
#  define strncasecmp strnicmp
#  ifndef HAVE_UNISTD_H
#    define sleep(x) Sleep((x) * 1000)
#    define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
#  endif /* HAVE_UNISTD_H */
#endif

#endif
