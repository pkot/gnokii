/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

@TOP@


#undef VERSION
#undef XVERSION
#undef XGNOKIIDIR


/***** Features *****/

/* Define if you want debug */
#undef DEBUG
#undef XDEBUG
#undef RLP_DEBUG

/* Define if you want security options enabled */
#undef SECURITY

/* Define if you want NLS support */
#undef USE_NLS

/* Use UNIX98 style pty support instead of the traditional */
#undef USE_UNIX98PTYS


/***** Compiler specific *****/

/* Define if the `long long' type works.  */
#undef HAVE_LONG_LONG


/**** Platform specific *****/

/* Define if you have gettimeofday */
#undef HAVE_GETTIMEOFDAY

/* Define if you have timerisset, timerclear, timercmp, timeradd and timersub */
#undef HAVE_TIMEOPS

/* Define if you have tm_gmtoff field in tm structure */
#undef HAVE_TM_GMTON

/* Define if you have cfsetspeed */
#undef HAVE_CFSETSPEED

/* Define if you have cfsetispeed and cfsetospeed */
#undef HAVE_CFSETISPEED

/* Define if you have c_ispeed and c_ospeed in struct termios */
#undef HAVE_TERMIOS_CSPEED

/* Define if you have snprintf prototype */
#undef HAVE_SNPRINTF

/* Define if you have vsnprintf prototype */
#undef HAVE_VSNPRINTF

/* Define if your snprintf implementation is fully C99 compliant */
#undef HAVE_C99_SNPRINTF

/* Define if your vsnprintf implementation is fully C99 compliant */
#undef HAVE_C99_VSNPRINTF

/* Define if struct msghdr has msg_control field */
#undef HAVE_MSGHDR_MSG_CONTROL

/* Define if you compile for M$ Windows */
#undef WIN32

/* Define if you have the <libintl.h> header file. */
#undef HAVE_LIBINTL_H

/* Define if you have the intl library (-lintl). */
#undef HAVE_LIBINTL

/* Define if you have XPM components */
#undef XPM


@BOTTOM@

#endif /* __CONFIG_H__ */
