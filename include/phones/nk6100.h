/*
  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 6100/5100 series.
  See README for more details on supported mobile phones.

*/

#ifndef __phones_nk6100_h
#define __phones_nk6100_h

#include "gsm-data.h"

/* Phone Memory types */

#define P6100_MEMORY_MT 0x01 /* ?? combined ME and SIM phonebook */
#define P6100_MEMORY_ME 0x02 /* ME (Mobile Equipment) phonebook */
#define P6100_MEMORY_SM 0x03 /* SIM phonebook */
#define P6100_MEMORY_FD 0x04 /* ?? SIM fixdialling-phonebook */
#define P6100_MEMORY_ON 0x05 /* ?? SIM (or ME) own numbers list */
#define P6100_MEMORY_EN 0x06 /* ?? SIM (or ME) emergency number */
#define P6100_MEMORY_DC 0x07 /* ME dialled calls list */
#define P6100_MEMORY_RC 0x08 /* ME received calls list */
#define P6100_MEMORY_MC 0x09 /* ME missed (unanswered received) calls list */
#define P6100_MEMORY_VOICE 0x0b /* Voice Mailbox */
/* This is used when the memory type is unknown. */
#define P6100_MEMORY_XX 0xff

#define	P6100_MAX_SMS_MESSAGES	12 /* maximum number of sms messages */

typedef struct {
	GSM_KeyCode Key;
	int Repeat;
} NK6100_Keytable;

#endif  /* #ifndef __phones_nk6100_h */
