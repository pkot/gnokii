/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Copyright (C) 2003 Marcus Godehard

*/

/* DO EDIT MANUALLY !!! This is not auto generated. */
#undef VERSION
#define VERSION "0.5.0pre6"

/* Define if you have the following headers */
#undef HAVE_UNISTD_H
#define HAVE_STRING_H 1
#undef HAVE_STRINGS_H
#define HAVE_CTYPE_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STDARG_H 1

/* Define if your compiler supports long long */
#undef HAVE_LONG_LONG

/* Define if your compiler supports long double */
#undef HAVE_LONG_DOUBLE

/* Define if you have timersub() */
#undef HAVE_TIMEOPS

/* Define if you have gettimeofday() */
#undef HAVE_GETTIMEOFDAY

/* Define if you have tm_gmtoff field in tm structure */
#undef HAVE_TM_GMTON

/* Define if you have cfsetspeed, cfsetispeed, cfsetospeed, c_ispeed and c_ospeed in struct termios */
#undef HAVE_CFSETSPEED
#undef HAVE_CFSETOSPEED
#undef HAVE_TERMIOS_CSPEED

/* Define both if you have ISO C99 compliant snprintf */
#define HAVE_SNPRINTF
#undef HAVE_C99_SNPRINTF

/* Define both if you have ISO C99 compliant vsnprintf */
#define HAVE_VSNPRINTF 1
#undef HAVE_C99_VSNPRINTF

/* Define if you have asprintf */
#define HAVE_ASPRINTF 1

/* Define if you have vasprintf */
#define HAVE_VASPRINTF 1

/* Define if you have strsep */
#undef HAVE_STRSEP

/* Define debug level */
#undef DEBUG
#undef XDEBUG
#undef RLP_DEBUG

/* Define debug level */
#ifdef _DEBUG
#   define	DEBUG
#endif

/* Decide if you want security options enabled */
#define SECURITY

/* Define if you have XPM components */
#undef XPM

/* Define if you want NLS */
#undef ENABLE_NLS

/* Define if you want to use Unix98 PTYs */
#undef USE_UNIX98PTYS

/* Define if struct msghdr has msg_control field */
#undef HAVE_MSGHDR_MSG_CONTROL

/* FIXME: Disable some odd warnings, comment these lines if u wanna fix the bad code lines */
#pragma warning(disable : 4244) // conversion from 'int ' to 'float ', possible loss of data
#pragma warning(disable : 4761) // integral size mismatch in argument; conversion supplied
#pragma warning(disable : 4018) // signed/unsigned mismatch
#pragma warning(disable : 4305) // truncation from 'const int ' to 'char '
