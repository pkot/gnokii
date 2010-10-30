/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000      Jan Derfinak
  Copyright (C) 2001-2002 Chris Kemp, Pavel Machek
  Copyright (C) 2001-2004 Pawel Kot, BORBELY Zoltan

  Header file for miscellaneous defines, typedefs etc.

*/

#ifndef _gnokii_misc_h
#define _gnokii_misc_h

#include <stdio.h>
#include <sys/types.h>

#include "compat.h"

/* Some general defines. */

#define ARRAY_LEN(x) (sizeof((x)) / sizeof((x)[0]))

#define GNOKII_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define GNOKII_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* A define to make debug printfs neat */
#ifdef __GNUC__
#  ifndef DEBUG
#    define dprintf(a...) do { } while (0)
#  else
#    define dprintf(a...) do { gn_log_debug(a); } while (0)
#  endif /* DEBUG */
#  /* Use it for error reporting */
#  define dump(a...) do { gn_elog_write(a); } while (0)
#else
#  ifndef DEBUG
#    define dump while (0)
#    define dprintf while (0)
#  else
#    define dump gn_elog_write
#    define dprintf gn_log_debug
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

#define PM_OLD_DEFAULT		PM_SPEEDDIAL | PM_SMS | PM_DTMF | PM_KEYBOARD | PM_CALENDAR
#define PM_DEFAULT		PM_OLD_DEFAULT | PM_CALLERGROUP | PM_EXTPBK | PM_FOLDERS
#define PM_DEFAULT_S40_3RD	PM_DEFAULT | PM_SMSFILE | PM_EXTPBK2 | PM_EXTCALENDAR

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
#define PM_FULLPBK		0x0800
/* Verify if these three might not be coupled into one */
#define PM_SMSFILE		0x1000
#define PM_EXTPBK2		0x2000
#define PM_EXTCALENDAR		0x4000

/* This one indicated reported cases of breaking the phone by xgnokii
 * in FBUS orver IrDA mode */
#define PM_XGNOKIIBREAKAGE	0x8000

char **gnokii_strsplit(const char *string, const char *delimiter, int tokens);
void gnokii_strfreev(char **str_array);
int gnokii_strcmpsep(const char *s1, const char *s2, char sep);

#endif /* _gnokii_misc_h */
