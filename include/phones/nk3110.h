
/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 3110 series. 
  See README for more details on supported mobile phones.

  $Log$
  Revision 1.2  2001-12-31 09:35:47  pkot
  libsms update. Will break sms reading/sending on all models not using libsms. libsms is getting now a middle layer. More updates soon. Cleanups.

  Revision 1.1  2001/11/08 16:39:09  pkot
  3810/3110 support for the new structure (Tamas Bondar)


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

#ifdef __phones_nk3110_c  /* Prototype functions for phone-3110.c only */

static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_Initialise(GSM_Statemachine *state);
static GSM_Error P3110_GetSMSInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetPhoneInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetStatusInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_DeleteSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_SendSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_IncomingNothing(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error P3110_IncomingCall(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingCallAnswered(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingCallEstablished(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingEndOfOutgoingCall(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingEndOfIncomingCall(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingEndOfOutgoingCall2(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingRestart(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingInitFrame_0x15(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingInitFrame_0x16(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingInitFrame_0x17(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSUserData(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSSend(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSSendError(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSHeader(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSError(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSDelete(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSDeleteError(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSDelivered(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingNoSMSInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingPINEntered(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingStatusInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingPhoneInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
void P3110_KeepAliveLoop(GSM_Statemachine *state);
void P3110_DecodeTime(unsigned char *b, GSM_DateTime *dt);
int P3110_bcd2int(u8 x);

#endif  /* #ifdef __phones_nk3110_c */

#endif  /* #ifndef __phones_nk3110_h */
