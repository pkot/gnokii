/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 7110 series. 
  See README for more details on supported mobile phones.

  The various routines are called P7110_(whatever).

  $Log$
  Revision 1.4  2001-01-29 17:14:44  chris
  dprintf now in misc.h (and fiddling with 7110 code)

  Revision 1.3  2001/01/23 15:32:44  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.2  2001/01/17 02:54:56  chris
  More 7110 work.  Use with care! (eg it is not possible to delete phonebook entries)
  I can now edit my phonebook in xgnokii but it is 'work in progress'.

  Revision 1.1  2001/01/14 22:47:01  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#ifndef __phone_7110_h
#define __phone_7110_h

#include <gsm-common.h>

extern GSM_Functions P7110_Functions;
extern bool P7110_LinkOK;
extern GSM_Information P7110_Information;

/* Limits for IMEI, Revision and Model string storage. */

#define P7110_MAX_IMEI_LENGTH     (20)
#define P7110_MAX_REVISION_LENGTH (6)
#define P7110_MAX_MODEL_LENGTH    (6)


/* Phone Memory types */

#define P7110_MEMORY_MT 0x09 /* ?? combined ME and SIM phonebook */
#define P7110_MEMORY_ME 0x05 /* ME (Mobile Equipment) phonebook */
#define P7110_MEMORY_SM 0x06 /* SIM phonebook */
#define P7110_MEMORY_FD 0x04 /* ?? SIM fixdialling-phonebook */
#define P7110_MEMORY_ON 0x07 /* ?? SIM (or ME) own numbers list */
#define P7110_MEMORY_EN 0x08 /* ?? SIM (or ME) emergency number */
#define P7110_MEMORY_DC 0x01 /* ME dialled calls list */
#define P7110_MEMORY_RC 0x03 /* ME received calls list */
#define P7110_MEMORY_MC 0x02 /* ME missed (unanswered received) calls list */
#define P7110_MEMORY_VOICE 0x0b /* Voice Mailbox */
/* This is used when the memory type is unknown. */
#define P7110_MEMORY_XX 0xff

/* Entry Types for the enhanced phonebook */

#define P7110_ENTRYTYPE_NUMBER 0x0b   /* Phonenumber */
#define P7110_ENTRYTYPE_NOTE   0x0a   /* Note (Text) */
#define P7110_ENTRYTYPE_POSTAL 0x09   /* Postal Address (Text) */
#define P7110_ENTRYTYPE_EMAIL  0x08   /* Email Adress (TEXT) */
#define P7110_ENTRYTYPE_NAME   0x07   /* Name always the only one */
#define P7110_ENTRYTYPE_DATE   0x13   /* Date for a Called List */
#define P7110_ENTRYTYPE_GROUP  0x1e   /* Group number for phonebook entry */


#ifdef __phone_7110_c  /* Prototype functions for phone-7110.c only */

static GSM_Error P7110_Initialise(char *port_device, char *initlength,
		 GSM_ConnectionType connection,
		 void (*rlp_callback)(RLP_F96Frame *frame));
static GSM_Error P7110_GenericCRHandler(int messagetype, unsigned char *buffer, int length);
static GSM_Error P7110_GetIMEI(char *imei);
static GSM_Error P7110_GetRevision(char *revision);
static GSM_Error P7110_GetModel(char *model);
static GSM_Error P7110_ReadPhonebook(GSM_PhonebookEntry *entry);
static GSM_Error P7110_WritePhonebookLocation(GSM_PhonebookEntry *entry);
static GSM_Error P7110_GetMemoryStatus(GSM_MemoryStatus *status);
static GSM_Error P7110_GetBatteryLevel(GSM_BatteryUnits *units, float *level);
static GSM_Error P7110_GetRFLevel(GSM_RFUnits *units, float *level);
static GSM_Error P7110_GetBitmap(GSM_Bitmap *bitmap);
static GSM_Error P7110_SetBitmap(GSM_Bitmap *bitmap);
static GSM_Error P7110_DialVoice(char *Number);

static void P7110_Terminate();
static bool P7110_SendRLPFrame( RLP_F96Frame *frame, bool out_dtx );

static int GetMemoryType(GSM_MemoryType memory_type);

#endif  /* #ifdef __phone_7110_c */

#endif  /* #ifndef __phone_7110_h */



