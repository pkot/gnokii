/*

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

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

  Copyright (C) 2003 Marcus Godehardt

  This is the config.h file for GNOKII for MSVC8 compiler, you can
  edit this manually. This is not auto generated.

*/

#ifndef _config_h
#define _config_h

#if defined(_MSC_VER) && defined(WIN32)
#	pragma once
#else
#	error This config.h is only for MSVC8 compiler !!!
#endif

#undef VERSION
#define VERSION "0.6.30"

/* We support Bluetooth and IRDA on MSVC8 */
#define HAVE_BLUETOOTH 1
#define HAVE_IRDA 1

/* Define if you have the following headers */
#define HAVE_STDARG_H 1
#define HAVE_STRING_H 1
#undef HAVE_STRINGS_H
#undef HAVE_STDINT_H
#undef HAVE_INTTYPES_H
#define HAVE_SYS_TYPES_H 1
#undef HAVE_SYS_FILE_H
#undef HAVE_SYS_TIME_H
#undef HAVE_UNISTD_H
#define HAVE_CTYPE_H 1
#define HAVE_STDLIB_H 1
#define HAVE_DIRECT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SYS_STAT_H 1

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
#undef HAVE_SNPRINTF
#undef HAVE_C99_SNPRINTF

/* Define both if you have ISO C99 compliant vsnprintf */
#define HAVE_VSNPRINTF 1
#define HAVE_C99_VSNPRINTF 1

/* Define if you have asprintf */
#define HAVE_ASPRINTF 1

/* Define if you have vasprintf */
#define HAVE_VASPRINTF 1

/* Define if you have strsep */
#undef HAVE_STRSEP

/* Define debug level */
#define DEBUG 1
#define XDEBUG 1
#define RLP_DEBUG 1

/* Decide if you want security options enabled */
#define SECURITY

/* Define if you have XPM components */
#undef XPM

/* Define if you want NLS */
#undef ENABLE_NLS

/* Define if you have <langinfo.h> and nl_langinfo(CODESET). */
#undef HAVE_LANGINFO_CODESET

/* Define if you want to use Unix98 PTYs */
#undef USE_UNIX98PTYS

/* Define if struct msghdr has msg_control field */
#undef HAVE_MSGHDR_MSG_CONTROL

/* FIXME: Disable some odd warnings, comment these lines if u wanna fix the bad code lines */
#pragma warning(disable : 4244) // conversion from 'int ' to 'float ', possible loss of data
#pragma warning(disable : 4761) // integral size mismatch in argument; conversion supplied
#pragma warning(disable : 4018) // signed/unsigned mismatch
#pragma warning(disable : 4305) // truncation from 'const int ' to 'char '
#pragma warning(disable : 4996) // 'strnicmp' was declared deprecated

/* Suppress CRT warnings */
#define _CRT_SECURE_NO_DEPRECATE

#endif // _config_h
