/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for miscellaneous defines, typedefs etc.

  Last modification: Mon May  1 10:18:04 CEST 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __misc_h
#define __misc_h    

#include "config.h"

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

/* This one is for NLS. */

#ifdef USE_NLS
  #include <libintl.h>
  #define _(x) gettext(x)
#else
  #define _(x) (x)
#endif /* USE_NLS */

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

/* This one is for FreeBSD and similar systems without __ptr_t_ */
/* FIXME: autoconf should take care of this. */

#ifndef __ptr_t
  typedef void * __ptr_t;
#endif /* __ptr_t */


/* Add here any timer operations which are not supported by libc5 */

#ifndef HAVE_TIMEOPS
#include <sys/time.h>

#ifndef timersub
#define timersub(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif

#endif /* HAVE_TIMEOPS */


#include <stdio.h>
extern int GetLine(FILE *File, char *Line, int count);

/* jano: Functions for detecting phone capabiltes. For explanation how  */
/* to use these functions look into xgnokii_lowlevel.c                  */

/* For models table */
typedef struct {
  char *model;
  char *number;
} PhoneModel;

extern char *GetModel (const char *);
extern int CallerGroupSupported (const char *);
extern int NetmonitorSupported (const char *);
extern int KeyboardSupported (const char *);
extern int SMSSupported (const char *);
extern int CalendarSupported (const char *);
extern int DTMFSupported (const char *);
extern int DataSupported (const char *);
extern int SpeedDialSupported (const char *);
#endif /* __misc_h */
