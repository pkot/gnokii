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

#ifndef _gnokii_misc_h
#define _gnokii_misc_h

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

/* If glib.h is included, G_GNUC_PRINTF is already defined. */
#ifndef G_GNUC_PRINTF
/* Stolen from <glib.h>: */
#  if	__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#    define G_GNUC_PRINTF( format_idx, arg_idx )	\
      __attribute__((format (printf, format_idx, arg_idx)))
#  else	/* !__GNUC__ */
#    define G_GNUC_PRINTF( format_idx, arg_idx )
#  endif	/* !__GNUC__ */
#endif


#define GNOKII_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define GNOKII_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* A define to make debug printfs neat */
#ifdef __GNUC__
#  ifndef DEBUG
#    define dprintf(a...) do { } while (0)
#  else
#    define dprintf(a...) do { fprintf(stderr, a); fflush(stderr); } while (0)
#  endif /* DEBUG */
#  /* Use it for error reporting */
#  define dump(a...) do { dprintf(a); gn_elog_write(a); } while (0)
#else
#  ifndef DEBUG
#    define dump while (0)
#    define dprintf while (0)
#  else
#    define dump printf
#    define dprintf printf
#  endif /* DEBUG */
#endif /* __GNUC__ */

/* Use gsprintf instead of sprintf and sprintf */
#define gsprintf		snprintf
#define gvsprintf		vsnprintf
#define gasprintf		asprintf
#define gvasprintf		vasprintf

/* This is for the bitmaps mostly, but may be useful also for the other
 * things. Counts how many octets we need to cover the given ammount of
 * the bits.
 */
#define ceiling_to_octet(x) ((x) + 7) / 8

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

#endif /* _gnokii_misc_h */
