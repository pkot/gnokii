/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for miscellaneous defines, typedefs etc.

  Last modification: Fri Apr 23 21:12:59 CEST 1999
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

/* For GNU gettext */
#define _(x) gettext(x)

#endif	/* __misc_h */
