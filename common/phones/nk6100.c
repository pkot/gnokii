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
  Copyright (C) 2002 BORBELY Zoltan

  This file provides functions specific to the 6100 series.
  See README for more details on supported mobile phones.

*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk6100.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

/* Some globals */

static unsigned char MagicBytes[4] = { 0x00, 0x00, 0x00, 0x00 };
/* FIXME: we should get rid on it */
static GSM_Statemachine *State = NULL;
static void (*OnCellBroadcast)(GSM_CBMessage *Message) = NULL;
static void (*CallNotification)(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo) = NULL;
static void (*RLP_RXCallback)(RLP_F96Frame *Frame) = NULL;
static GSM_Error (*OnSMS)(GSM_API_SMS *Message) = NULL;
static bool sms_notification_in_progress = false;
static bool sms_notification_lost = false;
static NK6100_Keytable Keytable[256];

/* static functions prototypes */
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Initialise(GSM_Statemachine *state);
static GSM_Error Authentication(GSM_Statemachine *state, char *imei);
static GSM_Error BuildKeytable(GSM_Statemachine *state);
static GSM_Error GetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetOnSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SaveSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error DeleteSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetBitmap(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetBitmap(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error WritePhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetPowersource(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSMSStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetWelcomeMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetOperatorLogo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetDateTime(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetDateTime(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetAlarm(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetAlarm(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetProfile(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetProfile(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetDisplayStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error DisplayOutput(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error PollDisplay(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetCellBroadcast(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error NetMonitor(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error MakeCall(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AnswerCall(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error CancelCall(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetCallNotification(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SendRLPFrame(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetRLPRXCallback(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SendDTMF(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Reset(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetRingtone(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRawRingtone(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetRawRingtone(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error PressOrReleaseKey(GSM_Data *data, GSM_Statemachine *state, bool press);
static GSM_Error EnterChar(GSM_Data *data, GSM_Statemachine *state);

#ifdef  SECURITY
static GSM_Error EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error ChangeSecurityCode(GSM_Data *data, GSM_Statemachine *state);
#endif

static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingPhoneInfo2(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingSMS1(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingProfile(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingPhoneClockAndAlarm(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingDisplay(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingCallInfo(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingRLPFrame(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingKey(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingMisc(int messagetype, unsigned char *message, int length, GSM_Data *data);

#ifdef  SECURITY
static GSM_Error IncomingSecurityCode(int messagetype, unsigned char *message, int length, GSM_Data *data);
#endif

static int GetMemoryType(GSM_MemoryType memory_type);
static void FlushLostSMSNotifications(GSM_Statemachine *state);

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 0x04, IncomingPhoneStatus },
	{ 0x0a, IncomingNetworkInfo },
	{ 0x03, IncomingPhonebook },
	{ 0x05, IncomingProfile },
	{ 0x11, IncomingPhoneClockAndAlarm },
	{ 0x13, IncomingCalendar },
	{ 0x0d, IncomingDisplay },
	{ 0x02, IncomingSMS1 },
	{ 0x14, IncomingSMS },
	{ 0x64, IncomingPhoneInfo },
	{ 0xd2, IncomingPhoneInfo2 },
	{ 0x40, IncomingSecurity },
	{ 0x01, IncomingCallInfo },
	{ 0xf1, IncomingRLPFrame },
	{ 0x0c, IncomingKey },
	{ 0x06, PNOK_IncomingCallDivert },
	{ 0xda, IncomingMisc },
#ifdef	SECURITY
	{ 0x08, IncomingSecurityCode },
#endif
	{ 0, NULL}
};

GSM_Phone phone_nokia_6100 = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"6110|6130|6150|6190|5110|5130|5190|3210|3310|3330|3360|8210", /* Supported models */
		4,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Arbitrary,         /* RF level units */
		4,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Arbitrary,         /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		48, 84,                /* Startup logo size */
		14, 72,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	Functions
};

static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return Initialise(state);
	case GOP_Terminate:
		return PGEN_Terminate(data, state);
	case GOP_GetSpeedDial:
		return GetSpeedDial(data, state);
	case GOP_SetSpeedDial:
		return SetSpeedDial(data, state);
	case GOP_GetImei:
	case GOP_GetModel:
	case GOP_GetRevision:
	case GOP_GetManufacturer:
	case GOP_Identify:
		return Identify(data, state);
	case GOP_GetBitmap:
		return GetBitmap(data, state);
	case GOP_SetBitmap:
		return SetBitmap(data, state);
	case GOP_GetBatteryLevel:
		return GetBatteryLevel(data, state);
	case GOP_GetRFLevel:
		return GetRFLevel(data, state);
	case GOP_GetMemoryStatus:
		return GetMemoryStatus(data, state);
	case GOP_ReadPhonebook:
		return ReadPhonebook(data, state);
	case GOP_WritePhonebook:
		return WritePhonebook(data, state);
	case GOP_GetPowersource:
		return GetPowersource(data, state);
	case GOP_GetSMSStatus:
		return GetSMSStatus(data, state);
	case GOP_GetNetworkInfo:
		return GetNetworkInfo(data, state);
	case GOP_SendSMS:
		return PNOK_FBUS_SendSMS(data, state);
	case GOP_OnSMS:
		return SetOnSMS(data, state);
	case GOP_PollSMS:
		SM_Loop(state, 1);
		return GE_NONE;
	case GOP_GetSMS:
		return GetSMSMessage(data, state);
	case GOP_SaveSMS:
		return SaveSMSMessage(data, state);
	case GOP_DeleteSMS:
		return DeleteSMSMessage(data, state);
	case GOP_GetDateTime:
		return GetDateTime(data, state);
	case GOP_SetDateTime:
		return SetDateTime(data, state);
	case GOP_GetAlarm:
		return GetAlarm(data, state);
	case GOP_SetAlarm:
		return SetAlarm(data, state);
	case GOP_GetProfile:
		return GetProfile(data, state);
	case GOP_SetProfile:
		return SetProfile(data, state);
	case GOP_GetCalendarNote:
		return GetCalendarNote(data, state);
	case GOP_WriteCalendarNote:
		return WriteCalendarNote(data, state);
	case GOP_DeleteCalendarNote:
		return DeleteCalendarNote(data, state);
	case GOP_GetDisplayStatus:
		return GetDisplayStatus(data, state);
	case GOP_DisplayOutput:
		return DisplayOutput(data, state);
	case GOP_PollDisplay:
		return PollDisplay(data, state);
	case GOP_GetSMSCenter:
		return GetSMSCenter(data, state);
	case GOP_SetSMSCenter:
		return SetSMSCenter(data, state);
	case GOP_SetCellBroadcast:
		return SetCellBroadcast(data, state);
	case GOP_NetMonitor:
		return NetMonitor(data, state);
	case GOP_MakeCall:
		return MakeCall(data, state);
	case GOP_AnswerCall:
		return AnswerCall(data, state);
	case GOP_CancelCall:
		return CancelCall(data, state);
	case GOP_SetCallNotification:
		return SetCallNotification(data, state);
	case GOP_SendRLPFrame:
		return SendRLPFrame(data, state);
	case GOP_SetRLPRXCallback:
		return SetRLPRXCallback(data, state);
#ifdef	SECURITY
	case GOP_EnterSecurityCode:
		return EnterSecurityCode(data, state);
	case GOP_GetSecurityCodeStatus:
		return GetSecurityCodeStatus(data, state);
	case GOP_ChangeSecurityCode:
		return ChangeSecurityCode(data, state);
#endif
	case GOP_SendDTMF:
		return SendDTMF(data, state);
	case GOP_Reset:
		return Reset(data, state);
	case GOP_GetRingtone:
		return GE_NOTSUPPORTED;
	case GOP_SetRingtone:
		return SetRingtone(data, state);
	case GOP_GetRawRingtone:
		return GetRawRingtone(data, state);
	case GOP_SetRawRingtone:
		return SetRawRingtone(data, state);
	case GOP_PressPhoneKey:
		return PressOrReleaseKey(data, state, true);
	case GOP_ReleasePhoneKey:
		return PressOrReleaseKey(data, state, false);
	case GOP_EnterChar:
		return EnterChar(data, state);
	case GOP_CallDivert:
		return PNOK_CallDivert(data, state);
	default:
		dprintf("NK61xx unimplemented operation: %d\n", op);
		return GE_NOTIMPLEMENTED;
	}
}

/* Nokia authentication protocol is used in the communication between Nokia
   mobile phones (e.g. Nokia 6110) and Nokia Cellular Data Suite software,
   commercially sold by Nokia Corp.

   The authentication scheme is based on the token send by the phone to the
   software. The software does it's magic (see the function
   FB61_GetNokiaAuth()) and returns the result back to the phone. If the
   result is correct the phone responds with the message "Accessory
   connected!" displayed on the LCD. Otherwise it will display "Accessory not
   supported" and some functions will not be available for use.

   The specification of the protocol is not publicly available, no comment.

   The auth code is written specially for gnokii project by Odinokov Serge.
   If you have some special requests for Serge just write him to
   apskaita@post.omnitel.net or serge@takas.lt

   Reimplemented in C by Pavel Janík ml.
*/

void PNOK_GetNokiaAuth(unsigned char *Imei, unsigned char *MagicBytes, unsigned char *MagicResponse)
{
	int i, j, CRC = 0;
	unsigned char Temp[16]; /* This is our temporary working area. */

	/* Here we put FAC (Final Assembly Code) and serial number into our area. */

	memcpy(Temp, Imei + 6, 8);

	/* And now the TAC (Type Approval Code). */

	memcpy(Temp + 8, Imei + 2, 4);

	/* And now we pack magic bytes from the phone. */

	memcpy(Temp + 12, MagicBytes, 4);

	for (i = 0; i <= 11; i++)
		if (Temp[i + 1] & 1)
			Temp[i]<<=1;

	switch (Temp[15] & 0x03) {
	case 1:
	case 2:
		j = Temp[13] & 0x07;
		for (i = 0; i <= 3; i++)
			Temp[i+j] ^= Temp[i+12];
		break;

	default:
		j = Temp[14] & 0x07;
		for (i = 0; i <= 3; i++)
			Temp[i + j] |= Temp[i + 12];
	}

	for (i = 0; i <= 15; i++)
		CRC ^= Temp[i];

	for (i = 0; i <= 15; i++) {
		switch (Temp[15 - i] & 0x06) {
		case 0:
			j = Temp[i] | CRC;
			break;

		case 2:
		case 4:
			j = Temp[i] ^ CRC;
			break;

		case 6:
			j = Temp[i] & CRC;
			break;
		}

		if (j == CRC) j = 0x2c;

		if (Temp[i] == 0) j = 0;

		MagicResponse[i] = j;
	}
}

/* Initialise is the only function allowed to 'use' state */
static GSM_Error Initialise(GSM_Statemachine *state)
{
	GSM_Error err;
	char model[GSM_MAX_MODEL_LENGTH+1];
	char imei[GSM_MAX_IMEI_LENGTH+1];
	GSM_Data data;
	PhoneModel *pm;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_6100, sizeof(GSM_Phone));

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
	case GCT_Infrared:
		err = FBUS_Initialise(&(state->Link), state, 0);
		break;
	case GCT_Irda:
		err = PHONET_Initialise(&(state->Link), state);
		break;
	default:
		return GE_NOTSUPPORTED;
	}

	if (err != GE_NONE) {
		dprintf("Error in link initialisation\n");
		return GE_NOTSUPPORTED;
	}

	SM_Initialise(state);

	/* We need to identify the phone first in order to know whether we can
	   authorize or set keytable */
	memset(model, 0, sizeof(model));
	memset(imei, 0, sizeof(imei));
	GSM_DataClear(&data);
	data.Model = model;
	data.Imei = imei;

	if ((err = Identify(&data, state)) != GE_NONE) return err;
	dprintf("model: '%s', imei: '%s'\n", model, imei);
	if ((pm = GetPhoneModel(model)) == NULL) {
		dump(_("Unsupported phone model \"%s\"\n"), model);
		dump(_("Please read Docs/Reporting-HOWTO and send a bug report!\n"));
		return GE_INTERNALERROR;
	}

	if (pm->flags & PM_AUTHENTICATION) {
		/* Now test the link and authenticate ourself */
		if (Authentication(state, imei) != GE_NONE) return GE_NOTSUPPORTED;
	}

	State = state;

	if (pm->flags & PM_KEYBOARD)
		if (BuildKeytable(state) != GE_NONE) return GE_NOTSUPPORTED;

	return GE_NONE;
}


static GSM_Error GetPhoneStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};

	dprintf("Getting phone status...\n");
	if (SM_SendMessage(state, 4, 0x04, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x04);
}

static GSM_Error GetRFLevel(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting rf level...\n");
	return GetPhoneStatus(data, state);
}

static GSM_Error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting battery level...\n");
	return GetPhoneStatus(data, state);
}

static GSM_Error GetPowersource(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting power source...\n");
	return GetPhoneStatus(data, state);
}

static GSM_Error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	float csq_map[5] = {0, 8, 16, 24, 31};

	switch (message[3]) {
	/* Phone status */
	case 0x02:
		dprintf("\tRFLevel: %d\n", message[5]);
		dprintf("\tPowerSource: %d\n", message[7]);
		dprintf("\tBatteryLevel: %d\n", message[8]);
		if (message[5] > 4) return GE_UNHANDLEDFRAME;
		if (message[7] != 1 && message[7] != 2) return GE_UNHANDLEDFRAME;
		if (data->RFLevel && data->RFUnits) {
			switch (*data->RFUnits) {
			case GRF_CSQ:
				*data->RFLevel = csq_map[message[5]];
				break;
			case GRF_Arbitrary:
			default:
				*data->RFUnits = GRF_Arbitrary;
				*data->RFLevel = message[5];
				break;
			}
		}
		if (data->PowerSource) {
			*data->PowerSource = message[7];
		}
		if (data->BatteryLevel && data->BatteryUnits) {
			*data->BatteryUnits = GBU_Arbitrary;
			*data->BatteryLevel = message[8];
		}
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00};

	dprintf("Getting memory status...\n");
	req[4] = GetMemoryType(data->MemoryStatus->MemoryType);
	if (SM_SendMessage(state, 5, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x00, 0x00};

	dprintf("Reading phonebook location (%d/%d)\n", data->PhonebookEntry->MemoryType, data->PhonebookEntry->Location);
	req[4] = GetMemoryType(data->PhonebookEntry->MemoryType);
	req[5] = data->PhonebookEntry->Location;
	if (SM_SendMessage(state, 7, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error WritePhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[FBUS_MAX_FRAME_LENGTH] = {FBUS_FRAME_HEADER, 0x04};
	GSM_PhonebookEntry *pe;
	unsigned char *pos;
	int namelen, numlen;

	pe = data->PhonebookEntry;
	pos = req+4;
	namelen = strlen(pe->Name);
	numlen = strlen(pe->Number);
	dprintf("Writing phonebook location (%d/%d): %s\n", pe->MemoryType, pe->Location, pe->Name);
	if (namelen > GSM_MAX_PHONEBOOK_NAME_LENGTH) {
		dprintf("name too long\n");
		return GE_PHBOOKNAMETOOLONG;
	}
	if (numlen > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GE_PHBOOKNUMBERTOOLONG;
	}
	if (pe->SubEntriesCount > 1) {
		dprintf("61xx doesn't support subentries\n");
		return GE_UNKNOWN;
	}
	if ((pe->SubEntriesCount == 1) && ((pe->SubEntries[0].EntryType != GSM_Number)
		|| (pe->SubEntries[0].NumberType != GSM_General) || (pe->SubEntries[0].BlockNumber != 2)
		|| strcmp(pe->SubEntries[0].data.Number, pe->Number))) {
		dprintf("61xx doesn't support subentries\n");
		return GE_UNKNOWN;
	}
	*pos++ = GetMemoryType(pe->MemoryType);
	*pos++ = pe->Location;
	*pos++ = namelen;
	PNOK_EncodeString(pos, namelen, pe->Name);
	pos += namelen;
	*pos++ = numlen;
	PNOK_EncodeString(pos, numlen, pe->Number);
	pos += numlen;
	*pos++ = (pe->Group == 5) ? 0xff : pe->Group;
	if (SM_SendMessage(state, pos-req, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error GetCallerGroupData(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x00};

	req[4] = data->Bitmap->number;
	if (SM_SendMessage(state, 5, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error GetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Reading bitmap...\n");
	switch (data->Bitmap->type) {
	case GSM_StartupLogo:
		return GetWelcomeMessage(data, state);
	case GSM_WelcomeNoteText:
		return GetWelcomeMessage(data, state);
	case GSM_DealerNoteText:
		return GetWelcomeMessage(data, state);
	case GSM_OperatorLogo:
		return GetOperatorLogo(data, state);
	case GSM_CallerLogo:
		return GetCallerGroupData(data, state);

	case GSM_None:
	case GSM_PictureMessage:
		return GE_NOTSUPPORTED;
	default:
		return GE_INTERNALERROR;
	}
}

static GSM_Error GetSpeedDial(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x16, 0x00};

	req[4] = data->SpeedDial->Number;

	if (SM_SendMessage(state, 5, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error SetSpeedDial(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x19, 0x00, 0x00, 0x00};

	req[4] = data->SpeedDial->Number;
	req[5] = GetMemoryType(data->SpeedDial->MemoryType);
	req[6] = data->SpeedDial->Location;

	if (SM_SendMessage(state, 7, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_PhonebookEntry *pe;
	GSM_Bitmap *bmp;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	case 0x02:
		if (data->PhonebookEntry) {
			pe = data->PhonebookEntry;
			pos = message+5;
			pe->Empty = false;
			n = *pos++;
			PNOK_DecodeString(pe->Name, sizeof(pe->Name), pos, n);
			pos += n;
			n = *pos++;
			PNOK_DecodeString(pe->Number, sizeof(pe->Number), pos, n);
			pos += n;
			pe->Group = *pos++;
			pos++;
			pe->Date.Year = (pos[0] << 8) + pos[1];
			pos += 2;
			pe->Date.Month = *pos++;
			pe->Date.Day = *pos++;
			pe->Date.Hour = *pos++;
			pe->Date.Minute = *pos++;
			pe->Date.Second = *pos++;
			pe->SubEntriesCount = 0;
		}
		break;
	case 0x03:
		if ((message[4] == 0x7d) || (message[4] == 0x74)) {
			return GE_INVALIDPHBOOKLOCATION;
		}
		return GE_UNHANDLEDFRAME;
	case 0x05:
		break;
	case 0x06:
		switch (message[4]) {
		case 0x7d:
		case 0x90:
			return GE_PHBOOKNAMETOOLONG;
		case 0x74:
			return GE_INVALIDPHBOOKLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}
	case 0x08:
		dprintf("\tMemory location: %d\n", data->MemoryStatus->MemoryType);
		dprintf("\tUsed: %d\n", message[6]);
		dprintf("\tFree: %d\n", message[5]);
		if (data->MemoryStatus) {
			data->MemoryStatus->Used = message[6];
			data->MemoryStatus->Free = message[5];
			return GE_NONE;
		}
		break;
	case 0x09:
		switch (message[4]) {
		case 0x6f:
			return GE_TIMEOUT;
		case 0x7d:
			return GE_INTERNALERROR;
		case 0x8d:
			return GE_INVALIDSECURITYCODE;
		default:
			return GE_UNHANDLEDFRAME;
		}
	case 0x11:
		if (data->Bitmap) {
			bmp = data->Bitmap;
			pos = message+4;
			bmp->number = *pos++;
			n = *pos++;
			PNOK_DecodeString(bmp->text, sizeof(bmp->text), pos, n);
			pos += n;
			bmp->ringtone = *pos++;
			pos++;
			bmp->size = (pos[0] << 8) + pos[1];
			pos += 2;
			pos++;
			bmp->width = *pos++;
			bmp->height = *pos++;
			pos++;
			n = bmp->height * bmp->width / 8;
			if (bmp->size > n) bmp->size = n;
			if (bmp->size > sizeof(bmp->bitmap))
				return GE_UNHANDLEDFRAME;
			memcpy(bmp->bitmap, pos, bmp->size);
		}
		break;
	case 0x12:
		switch (message[4]) {
		case 0x7d:
			return GE_INVALIDPHBOOKLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}
	case 0x14:
		break;
	case 0x15:
		switch (message[4]) {
		case 0x7d:
			return GE_INVALIDPHBOOKLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}

	/* Get speed dial OK */
	case 0x17:
		if (data->SpeedDial) {
			switch (message[4]) {
			case P6100_MEMORY_ME:
				data->SpeedDial->MemoryType = GMT_ME;
				break;
			case P6100_MEMORY_SM:
				data->SpeedDial->MemoryType = GMT_SM;
				break;
			default:
				return GE_UNHANDLEDFRAME;
			}
			data->SpeedDial->Location = message[5];
		}
		break;

	/* Get speed dial error */
	case 0x18:
		return GE_INVALIDSPEEDDIALLOCATION;

	/* Set speed dial OK */
	case 0x1a:
		return GE_NONE;

	/* Set speed dial error */
	case 0x1b:
		return GE_INVALIDSPEEDDIALLOCATION;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}

static GSM_Error PhoneInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x03, 0x00 };
	GSM_Error error;

	dprintf("Getting phone info...\n");
	if (SM_SendMessage(state, 5, 0xd1, req) != GE_NONE) return GE_NOTREADY;
	error = SM_Block(state, data, 0xd2);
	return error;
}

static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10};
	GSM_Error error;

	dprintf("Identifying...\n");
	if (data->Manufacturer) PNOK_GetManufacturer(data->Manufacturer);

	if (SM_SendMessage(state, 4, 0x64, req) != GE_NONE) return GE_NOTREADY;
	if (SM_Block(state, data, 0x64) != GE_NONE)
		if ((error = PhoneInfo(data, state)) != GE_NONE) return error;

	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GE_UNKNOWN;
	return GE_NONE;
}

static GSM_Error Authentication(GSM_Statemachine *state, char *imei)
{
	GSM_Error error;
	GSM_Data data;
	unsigned char connect1[] = {FBUS_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};
	unsigned char connect2[] = {FBUS_FRAME_HEADER, 0x20, 0x02};
	unsigned char connect3[] = {FBUS_FRAME_HEADER, 0x0d, 0x01, 0x00, 0x02};
	unsigned char connect4[] = {FBUS_FRAME_HEADER, 0x10};

	unsigned char magic_connect[] = {FBUS_FRAME_HEADER, 0x12,
					 /* auth response */
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 'N', 'O', 'K', 'I', 'A', '&', 'N', 'O', 'K', 'I', 'A',
					 'a', 'c', 'c', 'e', 's', 's', 'o', 'r', 'y',
					 0x00, 0x00, 0x00, 0x00};

	GSM_DataClear(&data);

	if ((error = SM_SendMessage(state, 7, 0x02, connect1)) != GE_NONE)
		return error;
	if ((error = SM_Block(state, &data, 0x02)) != GE_NONE)
		return error;

	if ((error = SM_SendMessage(state, 5, 0x02, connect2)) != GE_NONE)
		return error;
	if ((error = SM_Block(state, &data, 0x02)) != GE_NONE)
		return error;

	if ((error = SM_SendMessage(state, 7, 0x02, connect3)) != GE_NONE)
		return error;
	if ((error = SM_Block(state, &data, 0x02)) != GE_NONE)
		return error;

	if ((error = SM_SendMessage(state, 4, 0x64, connect4)) != GE_NONE)
		return error;
	if ((error = SM_Block(state, &data, 0x64)) != GE_NONE)
		return error;

	PNOK_GetNokiaAuth(imei, MagicBytes, magic_connect + 4);

	if ((error = SM_SendMessage(state, 45, 0x64, magic_connect)) != GE_NONE)
		return error;

	return GE_NONE;
}

static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	/* Phone ID recvd */
	case 0x11:
		if (data->Imei) {
			snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 9);
			dprintf("Received imei %s\n", data->Imei);
		}
		if (data->Model) {
			snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 25);
			dprintf("Received model %s\n", data->Model);
		}
		if (data->Revision) {
			snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW%s, HW%s", message + 44, message + 39);
			dprintf("Received revision %s\n", data->Revision);
		}

		dprintf("Message: Mobile phone identification received:\n");
		dprintf("\tIMEI: %s\n", message + 9);
		dprintf("\tModel: %s\n", message + 25);
		dprintf("\tProduction Code: %s\n", message + 31);
		dprintf("\tHW: %s\n", message + 39);
		dprintf("\tFirmware: %s\n", message + 44);

		/* These bytes are probably the source of the "Accessory not connected"
		   messages on the phone when trying to emulate NCDS... I hope....
		   UPDATE: of course, now we have the authentication algorithm. */
		dprintf("\tMagic bytes: %02x %02x %02x %02x\n", message[50], message[51], message[52], message[53]);

		memcpy(MagicBytes, message + 50, 4);
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}

static GSM_Error IncomingPhoneInfo2(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (data->Model) {
		snprintf(data->Model, 6, "%s", message + 21);
		data->Model[5] = 0;
	}
	if (data->Revision) {
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW: %s", message + 6);
		data->Revision[GSM_MAX_REVISION_LENGTH-1] = 0;
	}
	dprintf("Phone info:\n%s\n", message + 4);
	return GE_NONE;
}


static int GetMemoryType(GSM_MemoryType memory_type)
{
	int result;

	switch (memory_type) {
	case GMT_MT:
		result = P6100_MEMORY_MT;
		break;
	case GMT_ME:
		result = P6100_MEMORY_ME;
		break;
	case GMT_SM:
		result = P6100_MEMORY_SM;
		break;
	case GMT_FD:
		result = P6100_MEMORY_FD;
		break;
	case GMT_ON:
		result = P6100_MEMORY_ON;
		break;
	case GMT_EN:
		result = P6100_MEMORY_EN;
		break;
	case GMT_DC:
		result = P6100_MEMORY_DC;
		break;
	case GMT_RC:
		result = P6100_MEMORY_RC;
		break;
	case GMT_MC:
		result = P6100_MEMORY_MC;
		break;
	default:
		result = P6100_MEMORY_XX;
		break;
	}
	return (result);
}


static GSM_Error GetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x33, 0x64, 0x00};

	req[5] = data->MessageCenter->No;

	if (SM_SendMessage(state, 6, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x02);
}

static GSM_Error SetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[64] = {FBUS_FRAME_HEADER, 0x30, 0x64};
	SMS_MessageCenter *smsc;
	unsigned char *pos;

	smsc = data->MessageCenter;
	pos = req+5;
	*pos++ = smsc->No;
	pos++;
	switch (smsc->Format) {
		case SMS_FText:
			*pos++ = 0x00;
			break;
		case SMS_FFax:
			*pos++ = 0x22;
			break;
		case SMS_FVoice:
			*pos++ = 0x24;
			break;
		case SMS_FERMES:
			*pos++ = 0x25;
			break;
		case SMS_FPaging:
			*pos++ = 0x26;
			break;
		case SMS_FX400:
			*pos++ = 0x31;
			break;
		case SMS_FEmail:
			*pos++ = 0x32;
			break;
		default:
			return GE_NOTSUPPORTED;
	}
	pos++;
	switch (smsc->Validity) {
		case SMS_V1H:
			*pos++ = 0x0b;
			break;
		case SMS_V6H:
			*pos++ = 0x47;
			break;
		case SMS_V24H:
			*pos++ = 0xa7;
			break;
		case SMS_V72H:
			*pos++ = 0xa9;
			break;
		case SMS_V1W:
			*pos++ = 0xad;
			break;
		case SMS_VMax:
			*pos++ = 0xff;
			break;
		default:
			return GE_NOTSUPPORTED;
	}
	*pos = (SemiOctetPack(smsc->Recipient, pos+1, SMS_Unknown) + 1) / 2 + 1;
	pos += 12;
	*pos = (SemiOctetPack(smsc->Number, pos+1, smsc->Type) + 1) / 2 + 1;
	pos += 12;
	if (smsc->DefaultName < 1) {
		snprintf(pos, 13, "%s", smsc->Name);
		pos += strlen(pos)+1;
	} else
		*pos++ = 0;

	if (SM_SendMessage(state, pos-req, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x02);
}

static GSM_Error SetCellBroadcast(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req_ena[] = {FBUS_FRAME_HEADER, 0x20, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01};
	unsigned char req_dis[] = {FBUS_FRAME_HEADER, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char *req;

	req = data->OnCellBroadcast ? req_ena : req_dis;
	OnCellBroadcast = data->OnCellBroadcast;

	if (SM_SendMessage(state, 10, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x02);
}

static bool CheckIncomingSMS(GSM_Statemachine *state, int pos)
{
	GSM_Data data;
	GSM_API_SMS sms;
	GSM_Error error;

	if (!OnSMS) {
		return false;
	}

	/*
	 * libgnokii isn't reentrant anyway, so this simple trick should be
	 * enough - bozo
	 */
	if (sms_notification_in_progress) {
		sms_notification_lost = true;
		return false;
	}
	sms_notification_in_progress = true;

	memset(&sms, 0, sizeof(sms));
	sms.MemoryType = GMT_SM;
	sms.Number = pos;
	GSM_DataClear(&data);
	data.SMS = &sms;

	dprintf("trying to fetch sms#%hd\n", sms.Number);
	if ((error = GetSMS(&data, state)) != GE_NONE) {
		sms_notification_in_progress = false;
		return false;
	}

	OnSMS(&sms);

	dprintf("deleting sms#%hd\n", sms.Number);
	DeleteSMSMessage(&data, state);

	sms_notification_in_progress = false;

	return true;
}

static void FlushLostSMSNotifications(GSM_Statemachine *state)
{
	int i;

	while (!sms_notification_in_progress && sms_notification_lost) {
		sms_notification_lost = false;
		for (i = 1; i <= P6100_MAX_SMS_MESSAGES; i++)
			CheckIncomingSMS(state, i);
	}
}

static GSM_Error SetOnSMS(GSM_Data *data, GSM_Statemachine *state)
{
	if (data->OnSMS) {
		OnSMS = data->OnSMS;
		sms_notification_lost = true;
		FlushLostSMSNotifications(state);
	} else {
		OnSMS = NULL;
	}

	return GE_NONE;
}

static GSM_Error IncomingSMS1(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	SMS_MessageCenter *smsc;
	GSM_CBMessage cbmsg;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	/* Message sent */
	case 0x02:
		return GE_NONE;

	/* Send failed */
	case 0x03:
		/* I belive this is GE_SMSSENDFAILED in all cases -- pkot */
		switch (message[6]) {
		case 0x02:
		case 0x32: /* I get this error when operator does not allow to send me the SMS.
			    * 01 08 00 03 64 01 32 00
			    */
		case 0x6f: /* maybe bad sms center number */
			return GE_SMSSENDFAILED;
		default:
			return GE_UNHANDLEDFRAME;
		}
		return GE_UNHANDLEDFRAME;

	/* SMS message received */
	case 0x10:
		dprintf("SMS received, location: %d\n", message[5]);
		CheckIncomingSMS(State, message[5]);
		FlushLostSMSNotifications(State);
		return GE_UNSOLICITED;

	/* FIXME: unhandled frame, request: Get HW&SW version !!! */
	case 0x0e:
		if (length == 4) return GE_NONE;
		return GE_UNHANDLEDFRAME;

	/* Set CellBroadcast OK */
	case 0x21:
		break;

	/* Read CellBroadcast */
	case 0x23:
		if (OnCellBroadcast) {
			memset(&cbmsg, 0, sizeof(cbmsg));
			cbmsg.New = true;
			cbmsg.Channel = message[7];
			n = Unpack7BitCharacters(0, length-10, sizeof(cbmsg.Message)-1, message+10, cbmsg.Message);
			DecodeAscii(cbmsg.Message, cbmsg.Message, n);
			OnCellBroadcast(&cbmsg);
		}
		return GE_UNSOLICITED;

	/* Set SMS center OK */
	case 0x31:
		break;

	/* Set SMS center error */
	case 0x32:
		switch (message[4]) {
		case 0x02:
			return GE_EMPTYMEMORYLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}
		return GE_UNHANDLEDFRAME;

	/* SMS center received */
	case 0x34:
		if (data->MessageCenter) {
			smsc = data->MessageCenter;
			pos = message + 4;
			smsc->No = *pos++;
			pos++;
			switch (*pos++) {
			case 0x00:	/* text */
				smsc->Format = SMS_FText;
				break;
			case 0x22:	/* fax */
				smsc->Format = SMS_FFax;
				break;
			case 0x24:	/* voice */
				smsc->Format = SMS_FVoice;
				break;
			case 0x25:	/* ERMES */
				smsc->Format = SMS_FERMES;
				break;
			case 0x26:	/* paging */
				smsc->Format = SMS_FPaging;
				break;
			case 0x31:	/* X.400 */
				smsc->Format = SMS_FX400;
				break;
			case 0x32:	/* email */
				smsc->Format = SMS_FEmail;
				break;
			default:
				return GE_UNHANDLEDFRAME;
			}
			pos++;
			switch (*pos++) {
			case 0x0b:	/*  1 hour */
				smsc->Validity = SMS_V1H;
				break;
			case 0x47:	/*  6 hours */
				smsc->Validity = SMS_V6H;
				break;
			case 0xa7:	/* 24 hours */
				smsc->Validity = SMS_V24H;
				break;
			case 0xa9:	/* 72 hours */
				smsc->Validity = SMS_V72H;
				break;
			case 0xad:	/*  1 week */
				smsc->Validity = SMS_V1W;
				break;
			case 0xff:	/* max.time */
				smsc->Validity = SMS_VMax;
				break;
			default:	/* Probably some error, but better treat it as 24h */
				smsc->Validity = SMS_V24H;
				break;
			}
			snprintf(smsc->Recipient, sizeof(smsc->Recipient), "%s", GetBCDNumber(pos));
			pos += 12;
			snprintf(smsc->Number, sizeof(smsc->Number), "%s", GetBCDNumber(pos));
			smsc->Type = pos[1];
			pos += 12;
			/* FIXME: codepage must be investigated - bozo */
			if (pos[0] == 0x00) {
				snprintf(smsc->Name, sizeof(smsc->Name), _("Set %d"), smsc->No);
				smsc->DefaultName = smsc->No;
			} else {
				snprintf(smsc->Name, sizeof(smsc->Name), "%s", pos);
				smsc->DefaultName = -1;
			}
		}
		break;

	/* SMS center error recv */
	case 0x35:
		switch (message[4]) {
		case 0x01:
			return GE_EMPTYMEMORYLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error GetSMSStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x36, 0x64 };

	if (SM_SendMessage(state, 5, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x07, 0x02 /* Unknown */, 0x00 /* Location */, 0x01, 0x64 };

	if (!data->RawSMS) return GE_INTERNALERROR;

	req[5] = data->RawSMS->Number;
	if (SM_SendMessage(state, 8, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error SaveSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x04, 0x05, 0x02, 0x00, 0x02};
	int length;

	if (!data->RawData || !data->RawData->Data) return GE_INTERNALERROR;
	if (data->RawData->Length < 0) return GE_SMSWRONGFORMAT;

	length = data->RawData->Length - 4;
	if (8 + length > sizeof(req)) return GE_SMSWRONGFORMAT;

	if (data->RawSMS->Status == SMS_Unsent)
		req[4] |= 0x02;
	req[6] = data->RawSMS->Number;
	memcpy(req + 8, data->RawData->Data + 4, length);

	if (SM_SendMessage(state, 8 + length, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error DeleteSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x0a, 0x02, 0x00 /* Location */ };

	if (!data->SMS) return GE_INTERNALERROR;
	req[5] = data->SMS->Number;
	if (SM_SendMessage(state, 6, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int i;

	switch (message[3]) {
	/* save sms succeeded */
	case 0x05:
		dprintf("Message stored at %d\n", message[5]);
		if (data->RawSMS) data->RawSMS->Number = message[5];
		break;

	/* save sms failed */
	case 0x06:
		dprintf("SMS saving failed:\n");
		switch (message[4]) {
		case 0x02:
			dprintf("\tAll locations busy.\n");
			return GE_MEMORYFULL;
		case 0x03:
			dprintf("\tInvalid location!\n");
			return GE_INVALIDSMSLOCATION;
		default:
			dprintf("\tUnknown reason.\n");
			return GE_UNHANDLEDFRAME;
		}

	/* read sms */
	case 0x08:
		for (i = 0; i < length; i++)
			dprintf("[%02x(%d)]", message[i], i);
		dprintf("\n");

		if (!data->RawSMS) return GE_INTERNALERROR;

		memset(data->RawSMS, 0, sizeof(GSM_SMSMessage));

#define	getdata(d,dr,s)	(message[(data->RawSMS->Type == SMS_Deliver) ? (d) : ((data->RawSMS->Type == SMS_Delivery_Report) ? (dr) : (s))])
		switch (message[7]) {
		case 0x00: data->RawSMS->Type	= SMS_Deliver; break;
		case 0x01: data->RawSMS->Type	= SMS_Delivery_Report; break;
		case 0x02: data->RawSMS->Type	= SMS_Submit; break;
		default: return GE_UNHANDLEDFRAME;
		}
		data->RawSMS->Number		= message[6];
		data->RawSMS->MemoryType	= GMT_SM;
		data->RawSMS->Status		= message[4];

		data->RawSMS->DCS		= getdata(22, 21, 23);
		data->RawSMS->Length		= getdata(23, 22, 24);
		data->RawSMS->UDHIndicator	= message[20];
		memcpy(data->RawSMS->UserData, &getdata(43, 22, 44), data->RawSMS->Length);

		if (data->RawSMS->Type == SMS_Delivery_Report) {
			data->RawSMS->ReplyViaSameSMSC = message[11];
			memcpy(data->RawSMS->Time, message + 42, 7);
		}
		if (data->RawSMS->Type != SMS_Submit) {
			memcpy(data->RawSMS->SMSCTime, &getdata(36, 35, 0), 7);
			memcpy(data->RawSMS->MessageCenter, message + 8, 12);
			memcpy(data->RawSMS->RemoteNumber, &getdata(24, 23, 0), 12);
		}
#undef	getdata
		break;

	/* read sms failed */
	case 0x09:
		dprintf("SMS reading failed:\n");
		switch (message[4]) {
		case 0x00:
			dprintf("\tUnknown reason!\n");
			return GE_UNKNOWN;
		case 0x02:
			dprintf("\tInvalid location!\n");
			return GE_INVALIDSMSLOCATION;
		case 0x07:
			dprintf("\tEmpty SMS location.\n");
			return GE_EMPTYSMSLOCATION;
		default:
			dprintf("\tUnknown reason.\n");
			return GE_UNHANDLEDFRAME;
		}

	/* delete sms succeeded */
	case 0x0b:
		dprintf("Message: SMS deleted successfully.\n");
		break;
	
	/* delete sms failed */
	case 0x0c:
		switch (message[4]) {
		case 0x00:
			return GE_UNKNOWN;
		case 0x02:
			return GE_INVALIDSMSLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}

	/* sms status succeded */
	case 0x37:
		dprintf("Message: SMS Status Received\n");
		dprintf("\tThe number of messages: %d\n", message[10]);
		dprintf("\tUnread messages: %d\n", message[11]);
		data->SMSStatus->Unread = message[11];
		data->SMSStatus->Number = message[10];
		break;

	/* sms status failed */
	case 0x38:
		dprintf("Message: SMS Status error, probably not authorized by PIN\n");
		return GE_INTERNALERROR;

	/* unknown */
	default:
		dprintf("Unknown message.\n");
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}


static GSM_Error GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x70 };

	if (SM_SendMessage(state, 4, 0x0a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static GSM_Error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	/* Network info */
	case 0x71:
		if (data->NetworkInfo) {
			data->NetworkInfo->CellID[0] = message[10];
			data->NetworkInfo->CellID[1] = message[11];
			data->NetworkInfo->LAC[0] = message[12];
			data->NetworkInfo->LAC[1] = message[13];
			data->NetworkInfo->NetworkCode[0] = '0' + (message[14] & 0x0f);
			data->NetworkInfo->NetworkCode[1] = '0' + (message[14] >> 4);
			data->NetworkInfo->NetworkCode[2] = '0' + (message[15] & 0x0f);
			data->NetworkInfo->NetworkCode[3] = ' ';
			data->NetworkInfo->NetworkCode[4] = '0' + (message[16] & 0x0f);
			data->NetworkInfo->NetworkCode[5] = '0' + (message[16] >> 4);
			data->NetworkInfo->NetworkCode[6] = 0;
		}
		break;
	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error GetWelcomeMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x16};

	if (SM_SendMessage(state, 4, 0x05, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static GSM_Error GetOperatorLogo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x33, 0x01};

	req[4] = data->Bitmap->number;
	if (SM_SendMessage(state, 5, 0x05, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static GSM_Error SetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[512+GSM_MAX_BITMAP_SIZE] = {FBUS_FRAME_HEADER};
	GSM_Bitmap *bmp;
	unsigned char *pos;
	int len;

	bmp = data->Bitmap;
	pos = req+3;

	switch (bmp->type) {
	case GSM_WelcomeNoteText:
		len = strlen(bmp->text);
		if (len > 255) {
			dprintf("WelcomeNoteText is too long\n");
			return GE_INTERNALERROR;
		}
		*pos++ = 0x18;
		*pos++ = 0x01;	/* one block */
		*pos++ = 0x02;
		*pos = PNOK_EncodeString(pos+1, len, bmp->text);
		pos += *pos+1;
		if (SM_SendMessage(state, pos-req, 0x05, req) != GE_NONE) return GE_NOTREADY;
		return SM_Block(state, data, 0x05);

	case GSM_DealerNoteText:
		len = strlen(bmp->text);
		if (len > 255) {
			dprintf("DealerNoteText is too long\n");
			return GE_INTERNALERROR;
		}
		*pos++ = 0x18;
		*pos++ = 0x01;	/* one block */
		*pos++ = 0x03;
		*pos = PNOK_EncodeString(pos+1, len, bmp->text);
		pos += *pos+1;
		if (SM_SendMessage(state, pos-req, 0x05, req) != GE_NONE) return GE_NOTREADY;
		return SM_Block(state, data, 0x05);

	case GSM_StartupLogo:
		if (bmp->size > GSM_MAX_BITMAP_SIZE) {
			dprintf("StartupLogo is too long\n");
			return GE_INTERNALERROR;
		}
		*pos++ = 0x18;
		*pos++ = 0x01;	/* one block */
		*pos++ = 0x01;
		*pos++ = bmp->height;
		*pos++ = bmp->width;
		memcpy(pos, bmp->bitmap, bmp->size);
		pos += bmp->size;
		if (SM_SendMessage(state, pos-req, 0x05, req) != GE_NONE) return GE_NOTREADY;
		return SM_Block(state, data, 0x05);

	case GSM_OperatorLogo:
		if (bmp->size > GSM_MAX_BITMAP_SIZE) {
			dprintf("OperatorLogo is too long\n");
			return GE_INTERNALERROR;
		}
		*pos++ = 0x30;	/* Store Op Logo */
		*pos++ = 0x01;	/* location */
		*pos++ = ((bmp->netcode[1] & 0x0f) << 4) | (bmp->netcode[0] & 0x0f);
		*pos++ = 0xf0 | (bmp->netcode[2] & 0x0f);
		*pos++ = ((bmp->netcode[5] & 0x0f) << 4) | (bmp->netcode[4] & 0x0f);
		*pos++ = (bmp->size + 4) >> 8;
		*pos++ = (bmp->size + 4) & 0xff;
		*pos++ = 0x00;	/* infofield */
		*pos++ = bmp->width;
		*pos++ = bmp->height;
		*pos++ = 0x01;	/* Just BW */
		memcpy(pos, bmp->bitmap, bmp->size);
		pos += bmp->size;
		if (SM_SendMessage(state, pos-req, 0x05, req) != GE_NONE) return GE_NOTREADY;
		return SM_Block(state, data, 0x05);

	case GSM_CallerLogo:
		len = strlen(bmp->text);
		if (len > 255) {
			dprintf("Callergroup name is too long\n");
			return GE_INTERNALERROR;
		}
		if (bmp->size > GSM_MAX_BITMAP_SIZE) {
			dprintf("CallerLogo is too long\n");
			return GE_INTERNALERROR;
		}
		*pos++ = 0x13;
		*pos++ = bmp->number;
		*pos = PNOK_EncodeString(pos+1, len, bmp->text);
		pos += *pos+1;
		*pos++ = bmp->ringtone;
		*pos++ = 0x01;	/* Graphic on. You can use other values as well:
				   0x00 - Off
				   0x01 - On
				   0x02 - View Graphics
				   0x03 - Send Graphics
				   0x04 - Send via IR
				   You can even set it higher but Nokia phones (my
				   6110 at least) will not show you the name of this
				   item in menu ;-)) Nokia is really joking here. */
		*pos++ = (bmp->size + 4) >> 8;
		*pos++ = (bmp->size + 4) & 0xff;
		*pos++ = 0x00;	/* Future extensions! */
		*pos++ = bmp->width;
		*pos++ = bmp->height;
		*pos++ = 0x01;	/* Just BW */
		memcpy(pos, bmp->bitmap, bmp->size);
		pos += bmp->size;
		if (SM_SendMessage(state, pos-req, 0x03, req) != GE_NONE) return GE_NOTREADY;
		return SM_Block(state, data, 0x03);

	case GSM_None:
	case GSM_PictureMessage:
		return GE_NOTSUPPORTED;
	default:
		return GE_INTERNALERROR;
	}
}

static GSM_Error GetProfileFeature(GSM_Data *data, GSM_Statemachine *state, int id)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x13, 0x01, 0x00, 0x00};

	req[5] = data->Profile->Number;
	req[6] = (unsigned char)id;

	if (SM_SendMessage(state, 7, 0x05, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static GSM_Error SetProfileFeature(GSM_Data *data, GSM_Statemachine *state, int id, int value)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01};

	req[5] = data->Profile->Number;
	req[6] = (unsigned char)id;
	req[7] = (unsigned char)value;
	dprintf("Setting profile %d feature %d to %d\n", req[5], req[6], req[7]);

	if (SM_SendMessage(state, 9, 0x05, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static GSM_Error GetProfile(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x1a, 0x00};
	GSM_Profile *prof;
	GSM_Error error;
	GSM_Data d;
	char model[GSM_MAX_MODEL_LENGTH];
	int i;

	if (!data->Profile)
		return GE_UNKNOWN;
	prof = data->Profile;
	req[4] = prof->Number;

	if (SM_SendMessage(state, 5, 0x05, req) != GE_NONE) return GE_NOTREADY;
	if ((error = SM_Block(state, data, 0x05)) != GE_NONE)
		return error;

	for (i = 0; i <= 0x09; i++) {
		if ((error = GetProfileFeature(data, state, i)) != GE_NONE)
			return error;
	}

	if (prof->DefaultName > -1) {
		/*
		 * FIXME: isn't it should be better if we store the manufacturer
		 *	 and the model in GSM_Phone?
		 */
		GSM_DataClear(&d);
		d.Model = model;
		if ((error = Identify(&d, state)) != GE_NONE) {
			return error;
		}

		/* For N5110 */
		/* FIXME: It should be set for N5130 and 3210 too */
		if (!strcmp(model, "NSE-1")) {
			switch (prof->DefaultName) {
			case 0x00:
				snprintf(prof->Name, sizeof(prof->Name), _("Personal"));
				break;
			case 0x01:
				snprintf(prof->Name, sizeof(prof->Name), _("Car"));
				break;
			case 0x02:
				snprintf(prof->Name, sizeof(prof->Name), _("Headset"));
				break;
			default:
				snprintf(prof->Name, sizeof(prof->Name), _("Unknown (%d)"), prof->DefaultName);
				break;
			}
		} else {
			switch (prof->DefaultName) {
			case 0x00:
				snprintf(prof->Name, sizeof(prof->Name), _("General"));
				break;
			case 0x01:
				snprintf(prof->Name, sizeof(prof->Name), _("Silent"));
				break;
			case 0x02:
				snprintf(prof->Name, sizeof(prof->Name), _("Meeting"));
				break;
			case 0x03:
				snprintf(prof->Name, sizeof(prof->Name), _("Outdoor"));
				break;
			case 0x04:
				snprintf(prof->Name, sizeof(prof->Name), _("Pager"));
				break;
			case 0x05:
				snprintf(prof->Name, sizeof(prof->Name), _("Car"));
				break;
			case 0x06:
				snprintf(prof->Name, sizeof(prof->Name), _("Headset"));
				break;
			default:
				snprintf(prof->Name, sizeof(prof->Name), _("Unknown (%d)"), prof->DefaultName);
				break;
			}
		}
	}

	return GE_NONE;
}

static GSM_Error SetProfile(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[64] = {FBUS_FRAME_HEADER, 0x1c, 0x01, 0x03};
	GSM_Profile *prof;
	GSM_Error error;

	if (!data->Profile)
		return GE_UNKNOWN;
	prof = data->Profile;
	dprintf("Setting profile %d (%s)\n", prof->Number, prof->Name);

	if (prof->Number == 0) {
		/* We cannot rename the General profile! - bozo */

		/*
		 * FIXME: We must to do something. We cannot ask the name
		 * of the General profile, because it's language dependent.
		 * But without SetProfileName we aren't able to set features.
		 */
		dprintf("You cannot rename General profile\n");
		return GE_NOTSUPPORTED;
	} else if (prof->DefaultName > -1) {
		prof->Name[0] = 0;
	}

	req[7] = prof->Number;
	req[8] = PNOK_EncodeString(req+9, 39, prof->Name);
	req[6] = req[8] + 2;

	if (SM_SendMessage(state, req[8]+9, 0x05, req) != GE_NONE) return GE_NOTREADY;
	if ((error = SM_Block(state, data, 0x05)) != GE_NONE)
		return error;

	error  = SetProfileFeature(data, state, 0x00, prof->KeypadTone);
	error |= SetProfileFeature(data, state, 0x01, prof->Lights);
	error |= SetProfileFeature(data, state, 0x02, prof->CallAlert);
	error |= SetProfileFeature(data, state, 0x03, prof->Ringtone);
	error |= SetProfileFeature(data, state, 0x04, prof->Volume);
	error |= SetProfileFeature(data, state, 0x05, prof->MessageTone);
	error |= SetProfileFeature(data, state, 0x06, prof->Vibration);
	error |= SetProfileFeature(data, state, 0x07, prof->WarningTone);
	error |= SetProfileFeature(data, state, 0x08, prof->CallerGroups);
	error |= SetProfileFeature(data, state, 0x09, prof->AutomaticAnswer);

	return (error == GE_NONE) ? GE_NONE : GE_UNKNOWN;
}

static GSM_Error SetRingtone(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[7+GSM_MAX_RINGTONE_PACKAGE_LENGTH] = {FBUS_FRAME_HEADER, 0x36, 0x00, 0x00, 0x78};
	int size;

	if (!data || !data->Ringtone) return GE_INTERNALERROR;

	size = GSM_MAX_RINGTONE_PACKAGE_LENGTH;
	GSM_PackRingtone(data->Ringtone, req + 7, &size);
	req[4] = data->Ringtone->Location;

	if (SM_SendMessage(state, 7 + size, 0x05, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static GSM_Error IncomingProfile(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_Bitmap *bmp;
	GSM_Profile *prof;
	unsigned char *pos;
	int i;
	bool found;

	switch (message[3]) {
	/* Set profile feat. OK */
	case 0x11:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x7d:
			dprintf("Cannot set profile feature\n");
			return GE_UNKNOWN;
		default:
			return GE_UNHANDLEDFRAME;
		}
		break;

	/* Get profile feature */
	case 0x14:
		if (data->Profile) {
			prof = data->Profile;
			switch (message[6]) {
			case 0x00:
				prof->KeypadTone = message[8];
				break;
			case 0x01:
				prof->Lights = message[8];
				break;
			case 0x02:
				prof->CallAlert = message[8];
				break;
			case 0x03:
				prof->Ringtone = message[8];
				break;
			case 0x04:
				prof->Volume = message[8];
				break;
			case 0x05:
				prof->MessageTone = message[8];
				break;
			case 0x06:
				prof->Vibration = message[8];
				break;
			case 0x07:
				prof->WarningTone = message[8];
				break;
			case 0x08:
				prof->CallerGroups = message[8];
				break;
			case 0x09:
				prof->AutomaticAnswer = message[8];
				break;
			default:
				return GE_UNHANDLEDFRAME;
			}
		}
		break;

	/* Get Welcome Message */
	case 0x17:
		if (data->Bitmap) {
			bmp = data->Bitmap;
			pos = message + 5;
			found = false;
			for (i = 0; i < message[4] && !found; i++) {
				switch (*pos++) {
				case 0x01:
					if (bmp->type != GSM_StartupLogo) {
						pos += pos[0] * pos[1] / 8 + 2;
						continue;
					}
					bmp->height = *pos++;
					bmp->width = *pos++;
					bmp->size = bmp->height * bmp->width / 8;
					if (bmp->size > sizeof(bmp->bitmap)) {
						return GE_UNHANDLEDFRAME;
					}
					memcpy(bmp->bitmap, pos, bmp->size);
					pos += bmp->size;
					break;
				case 0x02:
					if (bmp->type != GSM_WelcomeNoteText) {
						pos += *pos + 1;
						continue;
					}
					PNOK_DecodeString(bmp->text, sizeof(bmp->text), pos+1, *pos);
					pos += *pos + 1;
					break;
				case 0x03:
					if (bmp->type != GSM_DealerNoteText) {
						pos += *pos + 1;
						continue;
					}
					PNOK_DecodeString(bmp->text, sizeof(bmp->text), pos+1, *pos);
					pos += *pos + 1;
					break;
				default:
					return GE_UNHANDLEDFRAME;
				}
				found = true;
			}
			if (!found) return GE_NOTSUPPORTED;
		}
		break;

	/* Set welcome ok */
	case 0x19:
		break;

	/* Get profile name */
	case 0x1b:
		if (data->Profile) {
			if (message[9] == 0x00) {
				data->Profile->DefaultName = message[8];
				data->Profile->Name[0] = 0;
			} else {
				data->Profile->DefaultName = -1;
				PNOK_DecodeString(data->Profile->Name, sizeof(data->Profile->Name), message+10, message[9]);
			}
			break;
		} else {
			return GE_UNKNOWN;
		}
		break;

	/* Set profile name OK */
	case 0x1d:
		switch (message[4]) {
		case 0x01:
			break;
		default:
			return GE_UNHANDLEDFRAME;
		}
		break;

	/* Set oplogo ok */
	case 0x31:
		break;

	/* Set oplogo error */
	case 0x32:
		switch (message[4]) {
		case 0x7d:
			return GE_INVALIDPHBOOKLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}

	/* Get oplogo */
	case 0x34:
		if (data->Bitmap) {
			bmp = data->Bitmap;
			pos = message + 5;
			bmp->netcode[0] = '0' + (pos[0] & 0x0f);
			bmp->netcode[1] = '0' + (pos[0] >> 4);
			bmp->netcode[2] = '0' + (pos[1] & 0x0f);
			bmp->netcode[3] = ' ';
			bmp->netcode[4] = '0' + (pos[2] & 0x0f);
			bmp->netcode[5] = '0' + (pos[2] >> 4);
			bmp->netcode[6] = 0;
			pos += 3;
			bmp->size = (pos[0] << 8) + pos[1];
			pos += 2;
			pos++;
			bmp->width = *pos++;
			bmp->height = *pos++;
			pos++;
			i = bmp->height * bmp->width / 8;
			if (bmp->size > i) bmp->size = i;
			if (bmp->size > sizeof(bmp->bitmap)) {
				return GE_UNHANDLEDFRAME;
			}
			memcpy(bmp->bitmap, pos, bmp->size);
		}
		break;

	/* Get oplogo error */
	case 0x35:
		switch (message[4]) {
			case 0x7d:
				return GE_UNKNOWN;
			default:
				return GE_UNHANDLEDFRAME;
		}
		break;
	
	/* Set ringtone OK */
	case 0x37:
		return GE_NONE;

	/* Set ringtone error */
	case 0x38:
		switch (message[4]) {
			case 0x7d:
				return GE_UNKNOWN;
			default:
				return GE_UNHANDLEDFRAME;
		}
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error GetDateTime(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x62};

	if (SM_SendMessage(state, 4, 0x11, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static GSM_Error SetDateTime(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x60, 0x01, 0x01, 0x07,
			       0x00, 0x00,	/* year - H/L */
			       0x00, 0x00,	/* month, day */
			       0x00, 0x00,	/* yours, minutes */
			       0x00};		/* Unknown, but not seconds - try 59 and wait 1 sec. */

	req[7] = data->DateTime->Year >> 8;
	req[8] = data->DateTime->Year & 0xff;
	req[9] = data->DateTime->Month;
	req[10] = data->DateTime->Day;
	req[11] = data->DateTime->Hour;
	req[12] = data->DateTime->Minute;

	if (SM_SendMessage(state, 14, 0x11, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static GSM_Error GetAlarm(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6d};

	if (SM_SendMessage(state, 4, 0x11, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static GSM_Error SetAlarm(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6b, 0x01, 0x20, 0x03,
			       0x02,		/* should be alarm on/off, but it doesn't works */
			       0x00, 0x00,	/* hours, minutes */
			       0x00};		/* Unknown, but not seconds - try 59 and wait 1 sec. */

	if (data->DateTime->AlarmEnabled) {
		req[8] = data->DateTime->Hour;
		req[9] = data->DateTime->Minute;
	} else {
		dprintf("Clearing the alarm clock isn't supported\n");
		return GE_NOTSUPPORTED;
	}

	if (SM_SendMessage(state, 11, 0x11, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static GSM_Error IncomingPhoneClockAndAlarm(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_DateTime *date;
	unsigned char *pos;

	switch (message[3]) {
	/* Date and time set */
	case 0x61:
		switch (message[4]) {
		case 0x01:
			break;
		default:
			return GE_UNHANDLEDFRAME;
		}
		break;

	/* Date and time received */
	case 0x63:
		if (data->DateTime) {
			date = data->DateTime;
			pos = message+8;
			date->Year = (pos[0] << 8) | pos[1];
			pos += 2;
			date->Month = *pos++;
			date->Day = *pos++;
			date->Hour = *pos++;
			date->Minute = *pos++;
			date->Second = *pos++;

			dprintf("Message: Date and time\n");
			dprintf("   Time: %02d:%02d:%02d\n", date->Hour, date->Minute, date->Second);
			dprintf("   Date: %4d/%02d/%02d\n", date->Year, date->Month, date->Day);
		}
		break;

	/* Alarm set */
	case 0x6c:
		switch (message[4]) {
		case 0x01:
			break;
		default:
			return GE_UNHANDLEDFRAME;
		}
		break;

	/* Alarm received */
	case 0x6e:
		if (data->DateTime) {
			date = data->DateTime;
			pos = message + 8;
			date->AlarmEnabled = (*pos++ == 2);
			date->Hour = *pos++;
			date->Minute = *pos++;
			date->Second = 0;

			dprintf("Message: Alarm\n");
			dprintf("   Alarm: %02d:%02d\n", date->Hour, date->Minute);
			dprintf("   Alarm is %s\n", date->AlarmEnabled ? _("on") : _("off"));
		}
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error GetCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x66, 0x00};

	req[4] = data->CalendarNote->Location;

	if (SM_SendMessage(state, 5, 0x13, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x13);
}

static GSM_Error WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x64, 0x01, 0x10,
				 0x00,	/* Length of the rest of the frame. */
				 0x00};	/* The type of calendar note */
	GSM_CalendarNote *note;
	unsigned char *pos;
	unsigned int numlen;

	if (!data->CalendarNote)
		return GE_UNKNOWN;

	note = data->CalendarNote;
	pos = req + 7;
	numlen = strlen(note->Phone);
	if (numlen > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		return GE_UNKNOWN;
	}

	*pos++ = note->Type;

	*pos++ = note->Time.Year >> 8;
	*pos++ = note->Time.Year & 0xff;
	*pos++ = note->Time.Month;
	*pos++ = note->Time.Day;
	*pos++ = note->Time.Hour;
	*pos++ = note->Time.Minute;
	*pos++ = note->Time.Timezone;

	if (note->Alarm.Year) {
		*pos++ = note->Alarm.Year >> 8;
		*pos++ = note->Alarm.Year & 0xff;
		*pos++ = note->Alarm.Month;
		*pos++ = note->Alarm.Day;
		*pos++ = note->Alarm.Hour;
		*pos++ = note->Alarm.Minute;
		*pos++ = note->Alarm.Timezone;
	} else {
		memset(pos, 0x00, 7);
		pos += 7;
	}
	

	*pos = PNOK_EncodeString(pos+1, 255, note->Text);
	pos += *pos+1;

	if (note->Type == GCN_CALL) {
		*pos++ = numlen;
		memcpy(pos, note->Phone, numlen);
		pos += numlen;
	} else {
		*pos++ = 0;
	}

	if (SM_SendMessage(state, pos-req, 0x13, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x13);
}

static GSM_Error DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x68, 0x00};

	req[4] = data->CalendarNote->Location;

	if (SM_SendMessage(state, 5, 0x13, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x13);
}

static GSM_Error IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_CalendarNote *note;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	/* Write cal.note report */
	case 0x65:
		switch (message[4]) {
			case 0x01:
				return GE_NONE;
			case 0x73:
			case 0x7d:
				return GE_UNKNOWN;
			default:
				return GE_UNHANDLEDFRAME;
		}
		break;

	/* Calendar note recvd */
	case 0x67:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x93:
			return GE_EMPTYMEMORYLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}
		if (data->CalendarNote) {
			note = data->CalendarNote;
			pos = message + 8;

			/* FIXME: this supposed to be replaced by general date unpacking function :-) */
			note->Type = *pos++;
			note->Time.Year = (pos[0] << 8) + pos[1];
			pos += 2;
			note->Time.Month = *pos++;
			note->Time.Day = *pos++;
			note->Time.Hour = *pos++;
			note->Time.Minute = *pos++;
			note->Time.Second = *pos++;

			note->Alarm.Year = (pos[0] << 8) + pos[1];
			pos += 2;
			note->Alarm.Month = *pos++;
			note->Alarm.Day = *pos++;
			note->Alarm.Hour = *pos++;
			note->Alarm.Minute = *pos++;
			note->Alarm.Second = *pos++;
			note->Alarm.AlarmEnabled = (note->Alarm.Year != 0);
			n = *pos++;
			PNOK_DecodeString(note->Text, sizeof(note->Text), pos, n);
			pos += n;

			if (note->Type == GCN_CALL) {
				/* This will be replaced later :-) */
				n = *pos++;
				PNOK_DecodeString(note->Phone, sizeof(note->Phone), pos, n);
				pos += n;
			} else {
				note->Phone[0] = 0;
			}
		}
		break;

	/* Del. cal.note report */
	case 0x69:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x93:
			return GE_EMPTYMEMORYLOCATION;
		default:
			return GE_UNHANDLEDFRAME;
		}
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}

static GSM_Error GetDisplayStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x51};

	if (SM_SendMessage(state, 4, 0x0d, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0d);
}

static GSM_Error PollDisplay(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Data dummy;

	GSM_DataClear(&dummy);
	GetDisplayStatus(&dummy, state);

	return GE_NONE;
}

static GSM_Error DisplayOutput(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x53, 0x00};

	req[4] = data->DisplayOutput->OutputFn ? 0x01 : 0x02;

	if (SM_SendMessage(state, 5, 0x0d, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0d);
}

static GSM_Error IncomingDisplay(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int state_table[8] = { 1 << DS_Call_In_Progress, 1 << DS_Unknown,
			       1 << DS_Unread_SMS, 1 << DS_Voice_Call,
			       1 << DS_Fax_Call, 1 << DS_Data_Call,
			       1 << DS_Keyboard_Lock, 1 << DS_SMS_Storage_Full };
	unsigned char *pos;
	int n, x, y, st;
	GSM_DrawMessage drawmsg;
	static GSM_DisplayOutput *disp = NULL;
	struct timeval now, time_diff, time_limit;

	switch (message[3]) {
	/* Display output */
	case 0x50:
		if (disp) {
			switch (message[4]) {
			case 0x01:
				break;
			default:
				return GE_UNHANDLEDFRAME;
			}
			pos = message + 5;
			y = *pos++;
			x = *pos++;
			n = *pos++;
			if (n > DRAW_MAX_SCREEN_WIDTH) return GE_INTERNALERROR;

			/*
			 * WARNING: the following part can damage your mind!
			 * This is so ugly, but there's no other way. Nokia
			 * forgot the clear screen from the protocol, so we
			 * must implement some heuristic here... :-( - bozo
			 */
			time_limit.tv_sec = 0;
			time_limit.tv_usec = 200000;
			gettimeofday(&now, NULL);
			timersub(&now, &disp->Last, &time_diff);
			if (y > 9 && timercmp(&time_diff, &time_limit, >))
				disp->State = true;
			disp->Last = now;

			if (disp->State && y > 9) {
				disp->State = false;
				memset(&drawmsg, 0, sizeof(drawmsg));
				drawmsg.Command = GSM_Draw_ClearScreen;
				disp->OutputFn(&drawmsg);
			}
			/* Maybe this is unneeded, please leave uncommented
			if (x == 0 && y == 46 && pos[1] != 'M') {
				disp->State = true;
			}
			*/

			memset(&drawmsg, 0, sizeof(drawmsg));
			drawmsg.Command = GSM_Draw_DisplayText;
			drawmsg.Data.DisplayText.x = x;
			drawmsg.Data.DisplayText.y = y;
			DecodeUnicode(drawmsg.Data.DisplayText.text, pos, n);
			disp->OutputFn(&drawmsg);

			dprintf("(x,y): %d,%d, len: %d, data: %s\n", x, y, n, drawmsg.Data.DisplayText.text);
		}
		return GE_UNSOLICITED;

	/* Display status */
	case 0x52:
		st = 0;
		pos = message + 4;
		for ( n = *pos++; n > 0; n--, pos += 2) {
			if ((pos[0] < 1) || (pos[0] > 8))
				return GE_UNHANDLEDFRAME;
			if (pos[1] == 0x02)
				st |= state_table[pos[0] - 1];
		}
		if (data->DisplayStatus)
			*data->DisplayStatus = st;
		if (disp) {
			memset(&drawmsg, 0, sizeof(drawmsg));
			drawmsg.Command = GSM_Draw_DisplayStatus;
			drawmsg.Data.DisplayStatus = st;
			disp->OutputFn(&drawmsg);
		}
		break;

	/* Display status ack */
	case 0x54:
		switch (message[4]) {
		case 0x01:
			break;
		default:
			return GE_UNHANDLEDFRAME;
		}
		/* FIXME: remove this hack if data is valid in DisplayOutput */
		disp = data->DisplayOutput->OutputFn ? data->DisplayOutput : NULL;
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error EnableExtendedCmds(GSM_Data *data, GSM_Statemachine *state, unsigned char type)
{
	unsigned char req[] = {0x00, 0x01, 0x64, 0x00};

	if (type == 0x06) {
		dump(_("Tried to activate CONTACT SERVICE\n"));
		return GE_INTERNALERROR;
	}

	req[3] = type;

	if (SM_SendMessage(state, 4, 0x40, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static GSM_Error NetMonitor(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x7e, 0x00};
	GSM_Error error;

	req[3] = data->NetMonitor->Field;

	if ((error = EnableExtendedCmds(data, state, 0x01)) != GE_NONE) return error;

	if (SM_SendMessage(state, 4, 0x40, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static GSM_Error Reset(GSM_Data *data, GSM_Statemachine *state)
{
	if (!data) return GE_INTERNALERROR;
	if (data->ResetType != 0x03 && data->ResetType != 0x04) return GE_INTERNALERROR;

	return EnableExtendedCmds(data, state, data->ResetType);
}

static GSM_Error GetRawRingtone(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x9e, 0x00};
	GSM_Error error;

	if (!data || !data->Ringtone || !data->RawData) return GE_INTERNALERROR;

	req[3] = data->Ringtone->Location;

	if ((error = EnableExtendedCmds(data, state, 0x01)) != GE_NONE) return error;

	if (SM_SendMessage(state, 4, 0x40, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static GSM_Error SetRawRingtone(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[512] = {0x00, 0x01, 0xa0, 0x00, 0x00,
				  0x0c, 0x2c, 0x01,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x02, 0xfc, 0x09};
	GSM_Error error;

	if (!data || !data->Ringtone || !data->RawData || !data->RawData->Data)
		return GE_INTERNALERROR;

	req[3] = data->Ringtone->Location;
	snprintf(req + 8, 13, "%s", data->Ringtone->name);
	memcpy(req + 24, data->RawData->Data, data->RawData->Length);

	if ((error = EnableExtendedCmds(data, state, 0x01)) != GE_NONE) return error;

	if (SM_SendMessage(state, 24 + data->RawData->Length, 0x40, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static GSM_Error IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[2]) {
	/* FIXME: maybe "Enable extended cmds" reply? - bozo */
	case 0x64:
		break;

	/* Netmonitor */
	case 0x7e:
		if (message[3] != 0x00 && data->NetMonitor)
			snprintf(data->NetMonitor->Screen, sizeof(data->NetMonitor->Screen), "%s", message + 4);
		break;

	/* Get bin ringtone */
	case 0x9e:
		switch (message[4]) {
		case 0x00:
			break;
		case 0x0a:
			return GE_UNKNOWN;
		default:
			return GE_UNHANDLEDFRAME;
		}
		if (!data->Ringtone) return GE_INTERNALERROR;
		data->Ringtone->Location = message[3];
		snprintf(data->Ringtone->name, sizeof(data->Ringtone->name), "%s", message + 8);
		if (data->RawData && data->RawData->Data) {
			memcpy(data->RawData->Data, message + 24, length - 24);
			data->RawData->Length = length - 24;
		}
		break;
	
	/* Set bin ringtone result */
	case 0xa0:
		if (message[3] != 0x02) return GE_UNHANDLEDFRAME;
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error MakeCall(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x01};
	/* order: req, voice_end */
	unsigned char voice_end[] = {0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00};
	/* order: req, data_nondigital_end, data_nondigital_final */
	unsigned char data_nondigital_end[]   = { 0x01,  /* make a data call = type 0x01 */
						  0x02,0x01,0x05,0x81,0x01,0x00,0x00,0x01,0x02,0x0a,
						  0x07,0xa2,0x88,0x81,0x21,0x15,0x63,0xa8,0x00,0x00 };
	unsigned char data_nondigital_final[] = { FBUS_FRAME_HEADER, 0x42,0x05,0x01,
						  0x07,0xa2,0xc8,0x81,0x21,0x15,0x63,0xa8,0x00,0x00,
						  0x07,0xa3,0xb8,0x81,0x20,0x15,0x63,0x80,0x01,0x60 };
	/* order: req, data_digital_pred1, data_digital_pred2, data_digital_end */
	unsigned char data_digital_pred1[]    = { FBUS_FRAME_HEADER, 0x42,0x05,0x01,
						  0x07,0xa2,0x88,0x81,0x21,0x15,0x63,0xa8,0x00,0x00,
						  0x07,0xa3,0xb8,0x81,0x20,0x15,0x63,0x80 };
	unsigned char data_digital_pred2[]    = { FBUS_FRAME_HEADER, 0x42,0x05,0x81,
						  0x07,0xa1,0x88,0x89,0x21,0x15,0x63,0xa0,0x00,0x06,
						  0x88,0x90,0x21,0x48,0x40,0xbb,0x07,0xa3,0xb8,0x81,
						  0x20,0x15,0x63,0x80 };
	unsigned char data_digital_end[]      = { 0x01,
						  0x02,0x01,0x05,0x81,0x01,0x00,0x00,0x01,0x02,0x0a,
						  0x07,0xa1,0x88,0x89,0x21,0x15,0x63,0xa0,0x00,0x06,
						  0x88,0x90,0x21,0x48,0x40,0xbb };
	unsigned char *pos;
	int n;

	n = strlen(data->CallInfo->Number);
	if (n > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GE_PHBOOKNUMBERTOOLONG;
	}

	pos = req + 4;
	*pos++ = (unsigned char)n;
	memcpy(pos, data->CallInfo->Number, n);
	pos += n;

	switch (data->CallInfo->Type) {
	case GSM_CT_VoiceCall:
		dprintf("Voice Call\n");
		switch (data->CallInfo->SendNumber) {
		case GSM_CSN_Never:
			voice_end[5] = 0x02;
			break;
		case GSM_CSN_Always:
			voice_end[5] = 0x03;
			break;
		case GSM_CSN_Default:
			voice_end[5] = 0x01;
			break;
		default:
			return GE_INTERNALERROR;
		}
		/* here in the past: voice_end had 8 bytes, memcpied   9 bytes, sent 1+9 bytes
		 * fbus-6110.c:      req_end   had 9 bytes, memcpied  10 bytes, sent   8 bytes
		 * currently:        voice_end has 8 bytes, memcpying  8 bytes, sent   8 bytes
		 */
		memcpy(pos, voice_end, ARRAY_LEN(voice_end));
		pos += ARRAY_LEN(voice_end);
		if (SM_SendMessage(state, pos - req, 0x01, req) != GE_NONE) return GE_NOTREADY;
		break;

	case GSM_CT_NonDigitalDataCall:
		dprintf("Non Digital Data Call\n");
		memcpy(pos, data_nondigital_end, ARRAY_LEN(data_nondigital_end));
		pos += ARRAY_LEN(data_nondigital_end);
		if (SM_SendMessage(state, pos - req, 0x01, req) != GE_NONE) return GE_NOTREADY;
		usleep(10000);
		dprintf("after nondigital1\n");
		if (SM_SendMessage(state, ARRAY_LEN(data_nondigital_final), 0x01, data_nondigital_final) != GE_NONE) return GE_NOTREADY;
		dprintf("after nondigital2\n");
		break;

	case GSM_CT_DigitalDataCall:
		dprintf("Digital Data Call\n");
		if (SM_SendMessage(state, ARRAY_LEN(data_digital_pred1), 0x01, data_digital_pred1) != GE_NONE) return GE_NOTREADY;
		usleep(500000);
		dprintf("after digital1\n");
		if (SM_SendMessage(state, ARRAY_LEN(data_digital_pred2), 0x01, data_digital_pred2) != GE_NONE) return GE_NOTREADY;
		usleep(500000);
		dprintf("after digital2\n");
		memcpy(pos, data_digital_end, ARRAY_LEN(data_digital_end));
		pos += ARRAY_LEN(data_digital_end);
		if (SM_SendMessage(state, pos - req, 0x01, req) != GE_NONE) return GE_NOTREADY;
		dprintf("after digital3\n");
		break;

	default:
		dprintf("Invalid call type %d\n", data->CallInfo->Type);
		return GE_INTERNALERROR;
	}

	return SM_BlockNoRetryTimeout(state, data, 0x01, 500);
}

static GSM_Error AnswerCall(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req1[] = {FBUS_FRAME_HEADER, 0x42, 0x05, 0x01, 0x07,
				0xa2, 0x88, 0x81, 0x21, 0x15, 0x63, 0xa8, 0x00, 0x00,
				0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x06, 0x00, 0x00};

	if (SM_SendMessage(state, sizeof(req1), 0x01, req1) != GE_NONE) return GE_NOTREADY;

	req2[4] = data->CallInfo->CallID;

	if (SM_SendMessage(state, sizeof(req2), 0x01, req2) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x01);
}

static GSM_Error CancelCall(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x08, 0x00, 0x85};

	req[4] = data->CallInfo->CallID;

	if (SM_SendMessage(state, 6, 0x01, req) != GE_NONE) return GE_NOTREADY;
	SM_BlockNoRetry(state, data, 0x01);
	return GE_NONE;
}

static GSM_Error SetCallNotification(GSM_Data *data, GSM_Statemachine *state)
{
	CallNotification = data->CallNotification;

	return GE_NONE;
}

static GSM_Error SendDTMF(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[5+256] = {FBUS_FRAME_HEADER, 0x50};
	int len;

	if (!data || !data->DTMFString) return GE_INTERNALERROR;

	len = strlen(data->DTMFString);
	if (len < 0 || len >= 256) return GE_INTERNALERROR;

	req[4] = len;
	memcpy(req + 5, data->DTMFString, len);

	if (SM_SendMessage(state, 5 + len, 0x01, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x01);
}

static GSM_Error IncomingCallInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_CallInfo cinfo;
	unsigned char *pos;

	switch (message[3]) {
	/* Call going msg */
	case 0x02:
		if (data->CallInfo)
			data->CallInfo->CallID = message[4];
		break;

	/* Call in progress */
	case 0x03:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (CallNotification)
			CallNotification(GSM_CS_Established, &cinfo);
		if (!data->CallInfo) return GE_UNSOLICITED;
		data->CallInfo->CallID = message[4];
		break;

	/* Remote end hang up */
	case 0x04:
		if (data->CallInfo) {
			data->CallInfo->CallID = message[4];
			return GE_UNKNOWN;
		}
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (CallNotification)
			CallNotification(GSM_CS_RemoteHangup, &cinfo);
		return GE_UNSOLICITED;

	/* incoming call alert */
	case 0x05:
		memset(&cinfo, 0, sizeof(cinfo));
		pos = message + 4;
		cinfo.CallID = *pos++;
		pos++;
		if (*pos >= sizeof(cinfo.Number))
			return GE_UNHANDLEDFRAME;
		memcpy(cinfo.Number, pos + 1, *pos);
		pos += *pos + 1;
		if (*pos >= sizeof(cinfo.Name))
			return GE_UNHANDLEDFRAME;
		memcpy(cinfo.Name, pos + 1, *pos);
		pos += *pos + 1;
		if (CallNotification)
			CallNotification(GSM_CS_IncomingCall, &cinfo);
		return GE_UNSOLICITED;
	
	/* answered call */
	case 0x07:
		return GE_UNSOLICITED;

	/* terminated call */
	case 0x09:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (CallNotification)
			CallNotification(GSM_CS_LocalHangup, &cinfo);
		if (!data->CallInfo) return GE_UNSOLICITED;
		data->CallInfo->CallID = message[4];
		break;
	
	/* message after "terminated call" */
	case 0x0a:
		return GE_UNSOLICITED;

	/* call held */
	case 0x23:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (CallNotification)
			CallNotification(GSM_CS_CallHeld, &cinfo);
		return GE_UNSOLICITED;

	/* call resumed */
	case 0x25:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (CallNotification)
			CallNotification(GSM_CS_CallResumed, &cinfo);
		return GE_UNSOLICITED;

	case 0x40:
		return GE_UNSOLICITED;

	/* FIXME: response from "Sent after issuing data call (non digital lines)"
	 * that's what we call data_nondigital_final in MakeCall()
	 */
	case 0x43:
		if (message[4] != 0x02) return GE_UNHANDLEDFRAME;
		return GE_UNSOLICITED;
  	
	/* FIXME: response from answer1? - bozo */
	case 0x44:
		if (message[4] != 0x68) return GE_UNHANDLEDFRAME;
		return GE_UNSOLICITED;
	
	/* DTMF sent */
	case 0x51:
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error SendRLPFrame(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[32] = {0x00, 0xd9};

	/*
	 * Discontinuos transmission (DTX).
	 * See section 5.6 of GSM 04.22 version 7.0.1.
	 */

	if (data->RLP_OutDTX) req[1] = 0x01;
	memcpy(req + 2, (unsigned char *) data->RLP_Frame, 30);

	/*
	 * It's ugly like the hell, maybe we should implement SM_SendFrame
	 * and extend the GSM_Link structure. We should clean this up in the
	 * future... - bozo
	 * return SM_SendFrame(state, 32, 0xf0, req);
	 */

	return FBUS_TX_SendFrame(32, 0xf0, req);
}

static GSM_Error SetRLPRXCallback(GSM_Data *data, GSM_Statemachine *state)
{
	RLP_RXCallback = data->RLP_RX_Callback;

	return GE_NONE;
}

static GSM_Error IncomingRLPFrame(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	RLP_F96Frame frame;

	/*
	 * We do not need RLP frame parsing to be done when we do not have
	 * callback specified.
	 */

	if (!RLP_RXCallback) return GE_NONE;

	/*
	 * Anybody know the official meaning of the first two bytes?
	 * Nokia 6150 sends junk frames starting D9 01, and real frames starting
	 * D9 00. We'd drop the junk frames anyway because the FCS is bad, but
	 * it's tidier to do it here. We still need to call the callback function
	 * to give it a chance to handle timeouts and/or transmit a frame
	 */

	if (message[0] == 0xd9 && message[1] == 0x01) {
		RLP_RXCallback(NULL);
		return GE_NONE;
	}
	
	/*
	 * Nokia uses 240 bit frame size of RLP frames as per GSM 04.22
	 * specification, so Header consists of 16 bits (2 bytes). See section
	 * 4.1 of the specification.
	 */
	
	frame.Header[0] = message[2];
	frame.Header[1] = message[3];

	/*
	 * Next 200 bits (25 bytes) contain the Information. We store the
	 * information in the Data array.
	 */
	
	memcpy(frame.Data, message + 4, 25);
	
	/* The last 24 bits (3 bytes) contain FCS. */
	frame.FCS[0] = message[29];
	frame.FCS[1] = message[30];
	frame.FCS[2] = message[31];

	/* Here we pass the frame down in the input stream. */

	RLP_RXCallback(&frame);

	return GE_NONE;
}


#ifdef  SECURITY
static GSM_Error EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[32] = {FBUS_FRAME_HEADER, 0x0a};
	unsigned char *pos;
	int len;

	if (!data->SecurityCode) return GE_INTERNALERROR;

	len = strlen(data->SecurityCode->Code);
	if (len < 0 || len >= 10) return GE_INTERNALERROR;

	pos = req + 4;
	*pos++ = data->SecurityCode->Type;
	memcpy(pos, data->SecurityCode->Code, len);
	pos += len;
	*pos++ = 0;
	*pos++ = 0;

	if (SM_SendMessage(state, pos - req, 0x08, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x08);
}

static GSM_Error GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07};

	if (!data->SecurityCode) return GE_INTERNALERROR;

	if (SM_SendMessage(state, 4, 0x08, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x08);
}

static GSM_Error ChangeSecurityCode(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[32] = {FBUS_FRAME_HEADER, 0x04};
	unsigned char *pos;
	int len1, len2;

	if (!data->SecurityCode) return GE_INTERNALERROR;

	len1 = strlen(data->SecurityCode->Code);
	len2 = strlen(data->SecurityCode->NewCode);
	if (len1 < 0 || len1 >= 10 || len2 < 0 || len2 >= 10) return GE_INTERNALERROR;

	pos = req + 4;
	*pos++ = data->SecurityCode->Type;
	memcpy(pos, data->SecurityCode->Code, len1);
	pos += len1;
	*pos++ = 0;
	memcpy(pos, data->SecurityCode->NewCode, len2);
	pos += len2;
	*pos++ = 0;

	if (SM_SendMessage(state, pos - req, 0x08, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x08);
}

static GSM_Error IncomingSecurityCode(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	/* change security code ok */
	case 0x05:
		break;

	/* change security code error */
	case 0x06:
		if (message[4] != 0x88) return GE_UNHANDLEDFRAME;
		dprintf("Message: Security code wrong.\n");
		return GE_INVALIDSECURITYCODE;
		
	/* security code status */
	case 0x08:
		dprintf("Message: Security Code status received: ");
		switch (message[4]) {
		case GSCT_SecurityCode: dprintf(_("waiting for Security Code.\n")); break;
		case GSCT_Pin: dprintf(_("waiting for PIN.\n")); break;
		case GSCT_Pin2: dprintf(_("waiting for PIN2.\n")); break;
		case GSCT_Puk: dprintf(_("waiting for PUK.\n")); break;
		case GSCT_Puk2: dprintf(_("waiting for PUK2.\n")); break;
		case GSCT_None: dprintf(_("nothing to enter.\n")); break;
		default: dprintf(_("Unknown!\n")); return GE_UNHANDLEDFRAME;
		}
		if (data->SecurityCode) data->SecurityCode->Type = message[4];
		break;
	
	/* security code OK */
	case 0x0b:
		dprintf("Message: Security code accepted.\n");
		break;
	
	/* security code wrong */
	case 0x0c:
		if (message[4] != 0x88) return GE_UNHANDLEDFRAME;
		dprintf("Message: Security code wrong.\n");
		return GE_INVALIDSECURITYCODE;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}
#endif


static GSM_Error PressOrReleaseKey(GSM_Data *data, GSM_Statemachine *state, bool press)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x42, 0x00, 0x00, 0x01};

	req[4] = press ? 0x01 : 0x02;
	req[5] = data->KeyCode;

	if (SM_SendMessage(state, 7, 0x0c, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0c);
}

static int ParseKey(GSM_Statemachine *state, GSM_KeyCode key, unsigned char **ppos)
{
	unsigned char ch;
	int n;

	ch = **ppos;
	(*ppos)++;

	if (key == GSM_KEY_NONE) return ch ? -1 : 0;

	for (n = 1; ch; n++) {
		Keytable[ch].Key = key;
		Keytable[ch].Repeat = n;
		ch = **ppos;
		(*ppos)++;
	}

	return 0;
}

static GSM_Error BuildKeytable(GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x40, 0x01};
	GSM_Error error;
	GSM_Data data;
	int i;

	/*
	 * This function builds a table, where the index is the character
	 * code to type, the value is the key code and the repeat count
	 * to press this key. The GSM_KEY_NONE means this character cannot
	 * be represented, the GSM_KEY_ASTERISK key means special symbols.
	 * We just initialise the table here, it will be filled by the
	 * IncomingKey() function which will be called as the result of
	 * this message which we send here.
	 */
	for (i = 0; i < 256; i++) {
		Keytable[i].Key = GSM_KEY_NONE;
		Keytable[i].Repeat = 0;
	}

	GSM_DataClear(&data);

	if (SM_SendMessage(state, 5, 0x0c, req) != GE_NONE) return GE_NOTREADY;
	if ((error = SM_Block(state, &data, 0x0c)) != GE_NONE) return error;

	return GE_NONE;
}

static GSM_Error PressKey(GSM_Statemachine *state, GSM_KeyCode key, int d)
{
	GSM_Data data;
	GSM_Error error;

	GSM_DataClear(&data);

	data.KeyCode = key;

	if ((error = PressOrReleaseKey(&data, state, true)) != GE_NONE) return error;
	if (d) usleep(d*1000);
	if ((error = PressOrReleaseKey(&data, state, false)) != GE_NONE) return error;

	return GE_NONE;
}

static GSM_Error EnterChar(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_KeyCode key;
	GSM_Error error;
	int i, r;

	/*
	 * Beware! Horrible kludge here. Try to write some sms manually with
	 * characters in different case and insert some symbols too. This
	 * function just "emulates" this process.
	 */

	if (isupper(data->Character)) {
		/*
		 * the table contains only lowercase characters and symbols,
		 * so we must translate into lowercase.
		 */
		i = tolower(data->Character);
		if (Keytable[i].Key == GSM_KEY_NONE) return GE_UNKNOWN;
	} else if (islower(data->Character)) {
		/*
		 * Ok, the character in proper case, but the phone in upper
		 * case mode (default). We must switch it into lowercase.
		 * We just have to press the hash key.
		 */
		i = data->Character;
		if (Keytable[i].Key == GSM_KEY_NONE) return GE_UNKNOWN;
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GE_NONE) return error;
	} else {
		/*
		 * This is a special character (number, space, symbol) which
		 * represents themself.
		 */
		i = data->Character;
		if (Keytable[i].Key == GSM_KEY_NONE) return GE_UNKNOWN;
	}
	if (Keytable[i].Key == GSM_KEY_ASTERISK) {
		/*
		 * This is a special symbol character, so we have to press
		 * the asterisk key, the down key the repeat count minus one
		 * times and finish with the ok (menu) key.
		 */
		if ((error = PressKey(state, GSM_KEY_ASTERISK, 0)) != GE_NONE) return error;
		key = GSM_KEY_DOWN;
		r = 1;
	}
	else {
		key = Keytable[i].Key;
		r = 0;
	}

	for (; r < Keytable[i].Repeat; r++) {
		if ((error = PressKey(state, key, 0)) != GE_NONE) return error;
	}

	if (islower(data->Character)) {
		/*
		 * This was a lowercase character. We must press the hash
		 * key again to go back to uppercase mode. We might store
		 * the lowercase/uppercase state, but it's much simpler.
		 */
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GE_NONE) return error;
	}
	else if (key == GSM_KEY_DOWN) {
		/* This was a symbol character, press the menu key to finish */
		if ((error = PressKey(state, GSM_KEY_MENU, 0)) != GE_NONE) return error;
	} else {
		/*
		 * Ok, the requested character is ready. But don't forget if
		 * we press the same key after it in a hurry it will toggle
		 * this char again! (Pressing key 2 will result "A", but if
		 * you press the key 2 again it will be "B".) Pressing the
		 * hash key is an easy workaround for it.
		 */
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GE_NONE) return error;
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GE_NONE) return error;
	}

	return GE_NONE;
}

static GSM_Error IncomingKey(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	unsigned char *pos;

	switch (message[3]) {
	/* Press key ack */
	case 0x43:
		if (message[4] != 0x01 && message[4] != 0x02)
			return GE_UNHANDLEDFRAME;
		break;

	/* Get key assignments */
	case 0x41:
		/*
		 * This message contains the string of characters for each
		 * phone key. We will simply parse it and build a reverse
		 * translation table. The last string is a special case. It
		 * contains the available symbols which isn't represented on
		 * the keyboard directly, you can choose from the menu when
		 * you press the asterisk key (think about sms writeing).
		 */
		pos = message + 4;
		if (ParseKey(State, GSM_KEY_1, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_2, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_3, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_4, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_5, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_6, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_7, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_8, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_9, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_0, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_NONE, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_NONE, &pos)) return GE_UNHANDLEDFRAME;
		if (ParseKey(State, GSM_KEY_ASTERISK, &pos)) return GE_UNHANDLEDFRAME;
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error IncomingMisc(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (messagetype == 0xda && message[0] == 0x00 && message[1] == 0x00)
	{
		/* seems like a keepalive */
		return GE_UNSOLICITED;
	}

	return GE_UNHANDLEDFRAME;
}
