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

#ifndef	_gnokii_compat_h
#define	_gnokii_compat_h

#include "config.h"

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#ifdef WIN32
#  include <windows.h>
#  include <string.h>
#endif

#ifdef HAVE_STDARG_H
#  include <stdarg.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

/*
 * The following ifdef block is the standard way of creating macros which make
 * exporting from a DLL simpler. All files within this DLL are compiled with 
 * the GNOKIIDLL_EXPORTS symbol defined on the command line. this symbol should
 * not be defined on any project that uses this DLL. This way any other project
 * whose source files include this file see API functions as being imported 
 * from a DLL, wheras this DLL sees symbols defined with this macro as being 
 * exported.
 *
 */

#if defined(WIN32) && defined(_USRDLL)
#  ifdef GNOKIIDLL_EXPORTS
#    define API __declspec(dllexport)
#  else
#    define API __declspec(dllimport)
#  endif
#else /* !WIN32 */
#  define API
#endif /* WIN32 */

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
API char *strsep(char **stringp, const char *delim);
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

/*
 * The following code was taken from W. Richard Stevens'
 * "UNIX Network Programming", Volume 1, Second Edition.
 *
 * We need the newer CMSG_LEN() and CMSG_SPACE() macros, but few
 * implementations support them today.  These two macros really need
 * an ALIGN() macro, but each implementation does this differently.
 */

#ifndef CMSG_LEN
#  define CMSG_LEN(size) (sizeof(struct cmsghdr) + (size))
#endif

#ifndef CMSG_SPACE
#  define CMSG_SPACE(size) (sizeof(struct cmsghdr) + (size))
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

#ifndef HAVE_PTR_T
	typedef void * __ptr_t;
#endif

/* Get rid of long defines. Use #if __unices__ */
#define __unices__ defined(__svr4__) || defined(__FreeBSD__) || defined(__bsdi__) || defined(__MACH__)
#if __unices__
#  include <strings.h>
#  include <sys/file.h>
#endif

/* This one is for NLS. */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(x) gettext(x)
#  define N_(x) gettext_noop(x)
#else
#  define _(x) (x)
#  define N_(x) (x)
#endif /* ENABLE_NLS */

/* Definitions for u8, u16 and u32, borrowed from
   /usr/src/linux/include/asm-i386/types.h */
#ifndef u8
	typedef unsigned char u8;
#endif

#ifndef u16
	typedef unsigned short u16;
#endif

#ifndef u32
	typedef unsigned int u32;
#endif

#endif
