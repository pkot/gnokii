/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 3110 series.
  See README for more details on supported mobile phones.

*/

#ifndef __phones_nk3110_h
#define __phones_nk3110_h

#include "gsm-data.h"

/* Phone Memory Sizes */
#define P3110_MEMORY_SIZE_SM 20
#define P3110_MEMORY_SIZE_ME 0

/* 2 seconds idle timeout */
#define P3110_KEEPALIVE_TIMEOUT 20;

/* Number of times to try resending SMS (empirical) */
#define P3110_SMS_SEND_RETRY_COUNT 4

#endif  /* #ifndef __phones_nk3110_h */
