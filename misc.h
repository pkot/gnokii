/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for miscellaneous defines, typedefs etc.

  Last modification: Thu May  6 00:51:04 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef    __misc_h
#define    __misc_h    

/* Some general defines. */

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
    #ifndef u64
        typedef unsigned long long u64;
    #endif

    #ifndef s64
        typedef signed long long s64;
    #endif
#endif 

#endif    /* __misc_h */
