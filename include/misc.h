	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	   Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  misc.h  Version 0.2.4

	   Header file for miscellaneous defines, typedefs etc. */

#ifndef	__misc_h
#define	__misc_h	

	/* Some general defines. */
#ifndef u8
	typedef	unsigned char		u8;
#endif	

#ifndef 	false
	#define		false (0)
#endif

#ifndef		true
	#define		true (!false)
#endif

#ifndef		bool	
	#define		bool	int
#endif

	/* For GNU gettext */
#define _(x)	gettext(x)

#endif	/* __misc_h */
