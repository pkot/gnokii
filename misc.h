/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for miscellaneous defines, typedefs etc.

  Last modification: Thu May  6 00:51:04 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef	__misc_h
#define	__misc_h	

/* Some general defines. */
#ifndef u8
  typedef unsigned char u8;
#endif	

#ifndef false
  #define false (0)
#endif

#ifndef true
  #define true (!false)
#endif

#ifndef bool	
  #define bool int
#endif

#ifdef GNOKII_GETTEXT
  #include <libintl.h>
  #define _(x) gettext(x)
#else
  #define _(x) (x)
#endif GNOKII_GETTEXT

#endif	/* __misc_h */
