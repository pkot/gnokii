/*
  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 6100/5100 series. 
  See README for more details on supported mobile phones.

  $Log$
  Revision 1.4  2001-12-31 09:35:47  pkot
  libsms update. Will break sms reading/sending on all models not using libsms. libsms is getting now a middle layer. More updates soon. Cleanups.

  Revision 1.3  2001/11/17 16:44:07  pkot
  Cleanup. Reading SMS for 6100 series. Not that it has some bugs more and does not support UDH yet

  Revision 1.2  2001/11/15 12:15:04  pkot
  smslib updates. begin work on sms in 6100 series

  Revision 1.1  2001/07/09 23:55:36  pkot
  Initial support for 6100 series in the new structure (me)

*/

#ifndef __phones_nk6100_h
#define __phones_nk6100_h

#include "gsm-data.h"

/* Phone Memory types */

#define P6100_MEMORY_MT 0x09 /* ?? combined ME and SIM phonebook */
#define P6100_MEMORY_ME 0x05 /* ME (Mobile Equipment) phonebook */
#define P6100_MEMORY_SM 0x06 /* SIM phonebook */
#define P6100_MEMORY_FD 0x04 /* ?? SIM fixdialling-phonebook */
#define P6100_MEMORY_ON 0x07 /* ?? SIM (or ME) own numbers list */
#define P6100_MEMORY_EN 0x08 /* ?? SIM (or ME) emergency number */
#define P6100_MEMORY_DC 0x01 /* ME dialled calls list */
#define P6100_MEMORY_RC 0x03 /* ME received calls list */
#define P6100_MEMORY_MC 0x02 /* ME missed (unanswered received) calls list */
#define P6100_MEMORY_VOICE 0x0b /* Voice Mailbox */
/* This is used when the memory type is unknown. */
#define P6100_MEMORY_XX 0xff

#ifdef __phones_nk6100_c  /* Prototype functions for phone-6100.c only */

static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Initialise(GSM_Statemachine *state);
static GSM_Error GetModelName(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error IncomingModelInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error IncomingSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error Incoming0x03(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error Incoming0x0a(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error Incoming0x17(int messagetype, unsigned char *buffer, int length, GSM_Data *data);


static int GetMemoryType(GSM_MemoryType memory_type);

#endif  /* #ifdef __phones_nk6100_c */

#endif  /* #ifndef __phones_nk6100_h */
