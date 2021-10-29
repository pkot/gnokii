/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2002-2004 BORBELY Zoltan, Pawel Kot
  Copyright (C) 2002      Feico de Boer, Markus Plail
  Copyright (C) 2003      Marcus Godehardt, Ladis Michl

  Header file for various platform compatibility.

*/

#ifndef	_gnokii_compat_h
#define	_gnokii_compat_h

#ifdef HAVE_CONFIG_H
#  include "config.h"
#else
#  error "compat.h requires config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#  include <windows.h>
#  include <locale.h>
#endif

#ifdef HAVE_DIRECT_H
#  include <direct.h>
#endif

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#ifdef HAVE_STDARG_H
#  ifndef _VA_LIST
#    include <stdarg.h>
#  endif
#endif

#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif

#ifdef HAVE_STRING_H
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE 1
#  endif
#  include <string.h>
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif

#ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

#ifdef HAVE_WCHAR_H
#  include <wchar.h>
#endif

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef HAVE_TERMIOS_H
#  include <termios.h>
#endif

#ifdef HAVE_DIRENT_H
#  include <dirent.h>
#endif

#ifdef HAVE_ERROR_H
#  include <error.h>
#endif

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#if !defined(INT_MAX)
#  define INT_MAX 2147483647
#endif

/*
 * The following ifdef block is the standard way of creating macros which make
 * exporting from a DLL simpler. All files within this DLL are compiled with
 * the GNOKIIDLL_EXPORTS symbol defined on the command line. this symbol should
 * not be defined on any project that uses this DLL. This way any other project
 * whose source files include this file see API functions as being imported
 * from a DLL, whereas this DLL sees symbols defined with this macro as being
 * exported.
 */
#if defined(WIN32)
#  if defined(GNOKIIDLL_EXPORTS) || defined(_USRDLL) || defined(DLL_EXPORT)
#    define GNOKII_API __declspec(dllexport)
#  elif defined(GNOKIIDLL_IMPORTS)
#    define GNOKII_API __declspec(dllimport)
#  else
#    define GNOKII_API
#  endif
#elif (__GNUC__ - 0 > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 3)
#    define GNOKII_API __attribute__ ((visibility("default")))
#else
#    define GNOKII_API
#endif /* WIN32 */

/* I assume that HAVE_STRNDUP always implies HAVE_STRING_H */
#ifndef HAVE_STRNDUP
extern char *strndup(const char *src, size_t n);
#endif

#ifndef HAVE_SETENV
extern int setenv(const char *name, const char *value, int overwrite);
extern int unsetenv(const char *name);
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
# define timercmp(a, b, CMP)						      \
  (((a)->tv_sec == (b)->tv_sec) ?					      \
   ((a)->tv_usec CMP (b)->tv_usec) :					      \
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

#ifndef PRINTF_ATTRIBUTE
#  ifdef HAVE___ATTRIBUTE__
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format. and a2 the index of the first
 * argument. Not that some gcc 2.x versions don't handle this
 * properly **/
#    define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#  else
#    define PRINTF_ATTRIBUTE(a1, a2)
#  endif
#endif

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_C99_SNPRINTF)
#  define snprintf rep_snprintf
int rep_snprintf(char *str, size_t size, const char *format, ...) PRINTF_ATTRIBUTE(3, 4);
#endif

#if !defined(HAVE_VSNPRINTF) || !defined(HAVE_C99_VSNPRINTF)
#  define vnsprintf rep_vsnprintf
int rep_vsnprintf(char *str, size_t size, const char *format, va_list ap) PRINTF_ATTRIBUTE(3, 0);
#endif

#ifndef HAVE_ASPRINTF
#  define asprintf rep_asprintf
int rep_asprintf(char **ptr, const char *format, ...) PRINTF_ATTRIBUTE(2, 3);
#endif

#ifndef HAVE_VASPRINTF
#  define vasprintf rep_vasprintf
int rep_vasprintf(char **ptr, const char *format, va_list ap) PRINTF_ATTRIBUTE(2, 0);
#endif

#ifndef HAVE_TIMEGM
time_t timegm(struct tm *tm);
#endif

#if HAVE_MKDIR
# if MKDIR_TAKES_ONE_ARG
   /* Mingw32 */
#  define mkdir(a,b) mkdir(a)
# endif
#else
# if HAVE__MKDIR
   /* plain Win32 */
#  define mkdir(a,b) _mkdir(a)
# else
#  error "Don't know how to create a directory on this system."
# endif
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
#  ifdef _MSC_VER
#    define inline __inline
#    define strcasecmp _stricmp
#    define strncasecmp _strnicmp
#    define __const const
#  endif /* _MSC_VER */
#  if !defined(HAVE_UNISTD_H) || defined(__MINGW32__)
#    define sleep(x) Sleep((x) * 1000)
#    define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
#  endif /* HAVE_UNISTD_H */
#endif

#ifndef HAVE_PTR_T
	typedef void * __ptr_t;
#endif

/* Get rid of long defines. Use #if __unices__ */
#define __unices__ defined(__svr4__) || defined(__FreeBSD__) || defined(__bsdi__) || defined(__MACH__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__HAIKU__)

/* This one is for NLS. */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  ifdef COMPILING_LIBGNOKII
#    define _(x) dgettext(GETTEXT_PACKAGE, x)
#    define N_(x) dgettext_noop(GETTEXT_PACKAGE, x)
#  else
#    define _(x) gettext(x)
#    define N_(x) gettext_noop(x)
#  endif
#else
#  define _(x) (x)
#  define N_(x) (x)
#  define dgettext(y, x) (x)
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

/* for Linux Bluetooth compatibility */
#if !defined(HAVE_STDINT_H) && !defined(HAVE_INTTYPES_H)
	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned int uint32_t;
	typedef char int8_t;
	typedef short int16_t;
	typedef int int32_t;
#endif

#ifndef HAVE_U_INT8_T
	typedef unsigned char u_int8_t;
#endif

#ifdef HAVE_WCRTOMB
#  define MBSTATE mbstate_t
#  define MBSTATE_ENC_CLEAR(x) memset(&(x), 0, sizeof(mbstate_t))
#  define MBSTATE_DEC_CLEAR(x) memset(&(x), 0, sizeof(mbstate_t))
#else
#  define MBSTATE char
#  define MBSTATE_ENC_CLEAR(x) mbtowc(NULL, NULL, 0)
#  define MBSTATE_DEC_CLEAR(x) wctomb(NULL, 0)
#endif

/* MS Visual C does not have va_copy */
#ifndef va_copy
#  ifdef __va_copy
#    define va_copy(dest, src)    __va_copy((dest), (src))
#  else
#    define va_copy(dest, src)    memcpy(&(dest), &(src), sizeof(va_list))
#  endif
#endif

/* Bool type */
#ifdef HAVE_STDBOOL_H
#  include <stdbool.h>
#else
#  if !HAVE__BOOL
#    ifdef __cplusplus
	typedef bool _Bool;
#    else
#define _Bool signed char
#    endif
#  endif
#  define bool _Bool
#  define false 0
#  define true 1
#  define __bool_true_false_are_defined 1
#endif

#ifdef __HAIKU__
#  define FIOASYNC FIONBIO
#endif

#endif
