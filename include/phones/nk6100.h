/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

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

#define	P6100_CAP_OLD_CALL_API	1
#define	P6100_CAP_NBS_UPLOAD	2
#define P6100_CAP_PB_UNICODE	4

typedef struct {
	GSM_KeyCode Key;
	int Repeat;
} NK6100_Keytable;

typedef struct {
	void (*OnCellBroadcast)(GSM_CBMessage *Message);
	void (*CallNotification)(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo, GSM_Statemachine *state);
	void (*RLP_RXCallback)(RLP_F96Frame *Frame);
	gn_error (*OnSMS)(GSM_API_SMS *Message);

	unsigned char MagicBytes[4];
	bool sms_notification_in_progress;
	bool sms_notification_lost;
	GSM_DisplayOutput *DisplayOutput;
	NK6100_Keytable Keytable[256];
	int Capabilities;

	char Model[GSM_MAX_MODEL_LENGTH];
	char IMEI[GSM_MAX_IMEI_LENGTH];
	char SWVersion[10];
	char HWVersion[10];
	PhoneModel *PM;
} NK6100_DriverInstance;

void PNOK_GetNokiaAuth(unsigned char *Imei, unsigned char *MagicBytes, unsigned char *MagicResponse);

#endif  /* #ifndef __phones_nk6100_h */
