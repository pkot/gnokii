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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Header file for miscellaneous defines, typedefs etc.

*/

#ifndef __misc_h
#define __misc_h

#include <stdio.h>
#include <sys/types.h>

#include "config.h"
#include "compat.h"

/* Some general defines. */

#ifndef false
#  define false (0)
#endif

#ifndef true
#  define true (!false)
#endif

#ifndef bool
#  define bool int
#endif

#define ARRAY_LEN(x) (sizeof((x)) / sizeof((x)[0]))

#define SAFE_STRNCPY(dest, src, n) do { \
	strncpy((dest), (src), (n)); \
	if ((n) > 0) \
		(dest)[(n)-1] = '\0'; \
	} while (0)

#define SAFE_STRNCPY_SIZEOF(dest,src) \
	SAFE_STRNCPY((dest), (src), sizeof((dest)))

/* Stolen from <glib.h>: */
#if	__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#  define G_GNUC_PRINTF( format_idx, arg_idx )	\
    __attribute__((format (printf, format_idx, arg_idx)))
#else	/* !__GNUC__ */
#  define G_GNUC_PRINTF( format_idx, arg_idx )
#endif	/* !__GNUC__ */

#define GNOKII_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define GNOKII_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* A define to make debug printfs neat */
#ifdef __GNUC__
#  /* Use it for error reporting */
#  define dump(a...) do { dprintf(a); GSM_WriteErrorLog(a); } while (0)
#  ifndef DEBUG
#    define dprintf(a...) do { } while (0)
#  else
#    define dprintf(a...) do { fprintf(stderr, a); fflush(stderr); } while (0)
#  endif /* DEBUG */
#else
#  ifndef DEBUG
#    define dump while (0)
#    define dprintf while (0)
#  else
#    define dump printf
#    define dprintf printf
#  endif /* DEBUG */
#endif /* __GNUC__ */

extern API void (*GSM_ELogHandler)(const char *fmt, va_list ap);
extern void GSM_WriteErrorLog(const char *fmt, ...);

/* Use gsprintf instead of sprintf and sprintf */
#define gsprintf		snprintf
#define gvsprintf		vsnprintf
#define gasprintf		asprintf
#define gvasprintf		vasprintf

/* Get rid of long defines. Use #if __unices__ */
#define __unices__ defined(__svr4__) || defined(__FreeBSD__) || defined(__bsdi__)
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

/* Definitions for u8, u16, u32 and u64, borrowed from
   /usr/src/linux/include/asm-i38/types.h */

#ifndef u8
	typedef unsigned char u8;
#endif

#ifndef u16
	typedef unsigned short u16;
#endif

#ifndef u32
	typedef unsigned int u32;
#endif

#ifndef s32
	typedef int s32;
#endif

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#  ifndef u64
	typedef unsigned long long u64;
#  endif

#  ifndef s64
	typedef signed long long s64;
#  endif
#endif

/* This one is for FreeBSD and similar systems without __ptr_t_ */
/* FIXME: autoconf should take care of this. */
#ifndef __ptr_t
	typedef void * __ptr_t;
#endif /* __ptr_t */

API int GetLine(FILE *File, char *Line, int count);

/* This is for the bitmaps mostly, but may be useful also for the other
 * things. Counts how many octets we need to cover the given ammount of
 * the bits.
 */
#define ceiling_to_octet(x) ((x) + 7) / 8

/* For models table */
typedef struct {
	char *model;
	char *number;
	int flags;
} PhoneModel;

#define PM_CALLERGROUP		0x0001
#define PM_NETMONITOR		0x0002
#define PM_KEYBOARD		0x0004
#define PM_SMS			0x0008
#define PM_CALENDAR		0x0010
#define PM_DTMF			0x0020
#define PM_DATA			0x0040
#define PM_SPEEDDIAL		0x0080
#define PM_EXTPBK		0x0100
#define PM_AUTHENTICATION	0x0200
#define PM_FOLDERS		0x0400

extern char *GetModel (const char *);
extern PhoneModel *GetPhoneModel (const char *);

API char *lock_device(const char*);
API bool unlock_device(char *);

#endif /* __misc_h */
