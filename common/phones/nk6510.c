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

  Copyright (C) 2002 Markus Plail
  Copyright (C) 2002 Pawe³ Kot

  This file provides functions specific to the 6510 series.
  See README for more details on supported mobile phones.

  The various routines are called P6510_(whatever).

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "nokia-decoding.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk6510.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

#define SEND_MESSAGE_BLOCK(type, length) \
do { \
	if (SM_SendMessage(state, length, type, req) != GN_ERR_NONE) return GN_ERR_NOTREADY; \
	return SM_Block(state, data, type); \
} while (0)

#define SEND_MESSAGE_WAITFOR(type, length) \
do { \
	if (SM_SendMessage(state, length, type, req) != GN_ERR_NONE) return GN_ERR_NOTREADY; \
	return SM_WaitFor(state, data, type); \
} while (0)

/* Functions prototypes */
static gn_error P6510_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_Initialise(GSM_Statemachine *state);

static gn_error P6510_GetModel(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetRevision(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_Identify(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetRFLevel(GSM_Data *data, GSM_Statemachine *state);

static gn_error P6510_SetBitmap(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetBitmap(GSM_Data *data, GSM_Statemachine *state);

static gn_error P6510_WritePhonebookLocation(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_SetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);

static gn_error P6510_GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state);

static gn_error P6510_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state);

static gn_error P6510_GetDateTime(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetAlarm(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_SetDateTime(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_SetAlarm(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state);

static gn_error P6510_DeleteWAPBookmark(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetWAPBookmark(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_WriteWAPBookmark(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetWAPSetting(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_ActivateWAPSetting(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_WriteWAPSetting(GSM_Data *data, GSM_Statemachine *state);

/*
static gn_error P6510_PollSMS(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetPicture(GSM_Data *data, GSM_Statemachine *state);
*/
static gn_error P6510_SendSMS(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_SaveSMS(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetSMS(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetSMSnoValidate(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_CreateSMSFolder(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_DeleteSMSFolder(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetSMSFolders(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetSMSFolderStatus(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetSMSStatus(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_DeleteSMS(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_DeleteSMSnoValidate(GSM_Data *data, GSM_Statemachine *state);
/*
static gn_error P6510_CallDivert(GSM_Data *data, GSM_Statemachine *state);
*/
static gn_error P6510_GetRingtoneList(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_GetProfile(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_SetProfile(GSM_Data *data, GSM_Statemachine *state);

static gn_error P6510_GetAnykeyAnswer(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_PressOrReleaseKey(GSM_Data *data, GSM_Statemachine *state, bool press);
static gn_error P6510_Subscribe(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_MakeCall(GSM_Data *data, GSM_Statemachine *state);

#ifdef  SECURITY
static gn_error P6510_GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state);
#endif
static gn_error P6510_GetToDo(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_DeleteAllToDoLocations(GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_WriteToDo(GSM_Data *data, GSM_Statemachine *state);


static gn_error P6510_IncomingIdentify(int messagetype, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingNetwork(int messagetype, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingBattLevel(int messagetype, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingStartup(int messagetype, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingFolder(int messagetype, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingClock(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
/*
static gn_error P6510_IncomingCallDivert(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
*/
static gn_error P6510_IncomingRingtone(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingProfile(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingKeypress(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingSubscribe(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingCommStatus(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingWAP(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error P6510_IncomingToDo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);

#ifdef  SECURITY
static gn_error P6510_IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
#endif


static int EncodeSMS(GSM_Data *data, GSM_Statemachine *state, unsigned char *req);
static int GetMemoryType(GSM_MemoryType memory_type);

/* Some globals */

static bool SMSLoop = false; /* Are we in infinite SMS reading loop? */
static bool NewSMS  = false; /* Do we have a new SMS? */

static GSM_IncomingFunctionType P6510_IncomingFunctions[] = {
       	{ P6510_MSG_FOLDER,	P6510_IncomingFolder },
	{ P6510_MSG_SMS,	P6510_IncomingSMS },
	{ P6510_MSG_PHONEBOOK,	P6510_IncomingPhonebook },
	{ P6510_MSG_NETSTATUS,	P6510_IncomingNetwork },
	{ P6510_MSG_CALENDAR,	P6510_IncomingCalendar },
	{ P6510_MSG_BATTERY,	P6510_IncomingBattLevel },
	{ P6510_MSG_CLOCK,	P6510_IncomingClock },
	{ P6510_MSG_IDENTITY,	P6510_IncomingIdentify },
	{ P6510_MSG_STLOGO,	P6510_IncomingStartup },
	/*
	{ P6510_MSG_DIVERT,	P6510_IncomingCallDivert },
	*/
	{ P6510_MSG_PROFILE,    P6510_IncomingProfile },
	{ P6510_MSG_RINGTONE,	P6510_IncomingRingtone },
	{ P6510_MSG_KEYPRESS,	P6510_IncomingKeypress },
#ifdef  SECURITY
	{ P6510_MSG_SECURITY,	P6510_IncomingSecurity },
#endif
	{ P6510_MSG_SUBSCRIBE,	P6510_IncomingSubscribe },
	{ P6510_MSG_COMMSTATUS,	P6510_IncomingCommStatus },
	{ P6510_MSG_WAP,	P6510_IncomingWAP },
	{ P6510_MSG_TODO,	P6510_IncomingToDo },
	{ 0, NULL }
};

GSM_Phone phone_nokia_6510 = {
	P6510_IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"6510|6310|8310|6310i|7650",      /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Percentage,        /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Percentage,        /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		65, 96,                /* Startup logo size */
		21, 78,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	P6510_Functions,
	NULL
};

/* FIXME - a little macro would help here... */

static gn_error P6510_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return P6510_Initialise(state);
	case GOP_Terminate:
		return PGEN_Terminate(data, state);
	case GOP_GetModel:
		return P6510_GetModel(data, state);
      	case GOP_GetRevision:
		return P6510_GetRevision(data, state);
	case GOP_GetImei:
		return P6510_GetIMEI(data, state);
	case GOP_Identify:
		return P6510_Identify(data, state);
	case GOP_GetBatteryLevel:
		return P6510_GetBatteryLevel(data, state);
	case GOP_GetRFLevel:
		return P6510_GetRFLevel(data, state);
	case GOP_GetMemoryStatus:
		return P6510_GetMemoryStatus(data, state);
	case GOP_GetBitmap:
		return P6510_GetBitmap(data, state);
	case GOP_SetBitmap:
		return P6510_SetBitmap(data, state);
	case GOP_ReadPhonebook:
		return P6510_ReadPhonebook(data, state);
	case GOP_WritePhonebook:
		return P6510_WritePhonebookLocation(data, state);
	case GOP_GetNetworkInfo:
		return P6510_GetNetworkInfo(data, state);
	case GOP_GetSpeedDial:
		return P6510_GetSpeedDial(data, state);
	case GOP_SetSpeedDial:
		return P6510_SetSpeedDial(data, state);
	case GOP_GetSMSCenter:
		return P6510_GetSMSCenter(data, state);
	case GOP_GetDateTime:
		return P6510_GetDateTime(data, state);
	case GOP_GetAlarm:
		return P6510_GetAlarm(data, state);
	case GOP_SetDateTime:
		return P6510_SetDateTime(data, state);
	case GOP_SetAlarm:
		return P6510_SetAlarm(data, state);
	case GOP_GetToDo:
		return P6510_GetToDo(data, state);
	case GOP_WriteToDo:
		return P6510_WriteToDo(data, state);
	case GOP_DeleteAllToDos:
		return P6510_DeleteAllToDoLocations(data, state);
	case GOP_GetCalendarNote:
		return P6510_GetCalendarNote(data, state);
	case GOP_WriteCalendarNote:
		return P6510_WriteCalendarNote(data, state);
	case GOP_DeleteCalendarNote:
		return P6510_DeleteCalendarNote(data, state);
	case GOP_DeleteWAPBookmark:
		return P6510_DeleteWAPBookmark(data, state);
	case GOP_GetWAPBookmark:
		return P6510_GetWAPBookmark(data, state);
	case GOP_WriteWAPBookmark:
		return P6510_WriteWAPBookmark(data, state);
	case GOP_GetWAPSetting:
		return P6510_GetWAPSetting(data, state);
	case GOP_ActivateWAPSetting:
		return P6510_ActivateWAPSetting(data, state);
	case GOP_WriteWAPSetting:
		return P6510_WriteWAPSetting(data, state);
		/*

	case GOP_OnSMS:
		if (data->OnSMS) {
			NewSMS = true;
			return GN_ERR_NONE;
		}
		break;
	case GOP_PollSMS:
		if (NewSMS) return GN_ERR_NONE; 
		break;
	case GOP6510_GetPicture:
		return P6510_GetPicture(data, state);
		*/
	case GOP_DeleteSMS:
		return P6510_DeleteSMS(data, state);
	case GOP_DeleteSMSnoValidate:
		return P6510_DeleteSMSnoValidate(data, state);
	case GOP_GetSMSStatus:
		return P6510_GetSMSStatus(data, state);
		/*
	case GOP_CallDivert:
		return P6510_CallDivert(data, state);
		*/
	case GOP_SaveSMS:
		return P6510_SaveSMS(data, state);
	case GOP_SendSMS:
		return P6510_SendSMS(data, state);
	case GOP_GetSMSFolderStatus:
		return P6510_GetSMSFolderStatus(data, state);
	case GOP_GetSMS:
		return P6510_GetSMS(data, state);
	case GOP_GetSMSnoValidate:
		return P6510_GetSMSnoValidate(data, state);
	case GOP_GetUnreadMessages:
		return GN_ERR_NONE;
	case GOP_GetSMSFolders:
		return P6510_GetSMSFolders(data, state);
	case GOP_CreateSMSFolder:
		return P6510_CreateSMSFolder(data, state);
	case GOP_DeleteSMSFolder:
		return P6510_DeleteSMSFolder(data, state);
	case GOP_GetProfile:
		return P6510_GetProfile(data, state);
	case GOP_SetProfile:
		return P6510_SetProfile(data, state);
	case GOP_PressPhoneKey:
		return P6510_PressOrReleaseKey(data, state, true);
	case GOP_ReleasePhoneKey:
		return P6510_PressOrReleaseKey(data, state, false);
	case GOP_MakeCall:
		return P6510_MakeCall(data, state);
#ifdef  SECURITY
	case GOP_GetSecurityCodeStatus:
		return P6510_GetSecurityCodeStatus(data, state);
	case GOP_EnterSecurityCode:
		return P6510_EnterSecurityCode(data, state);
#endif
	case GOP_Subscribe:
		return P6510_Subscribe(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_NONE;
}

/* Initialise is the only function allowed to 'use' state */
static gn_error P6510_Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];
	gn_error err;
	bool connected = false;
	int try = 0;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_6510, sizeof(GSM_Phone));

	dprintf("Connecting\n");
	while (!connected) {
		if (try > 2) break;
		switch (state->Link.ConnectionType) {
		case GCT_DAU9P:
			try++;
		case GCT_DLR3P:
			if (try > 1) {
				try = 3;
				break;
			}
		case GCT_Serial:
			err = FBUS_Initialise(&(state->Link), state, try++);
			break;
		case GCT_Infrared:
#ifdef HAVE_IRDA
		case GCT_Irda:
			err = PHONET_Initialise(&(state->Link), state);
			break;
#endif
		default:
			return GN_ERR_NOTSUPPORTED;
		}
	
		if (err != GN_ERR_NONE) {
			dprintf("Error in link initialisation: %d\n", err);
			continue;
		}
	
		SM_Initialise(state);
	
		/* Now test the link and get the model */
		GSM_DataClear(&data);
		data.Model = model;
		if (state->Phone.Functions(GOP_GetModel, &data, state) == GN_ERR_NONE)
			connected = true;
	}
	if (!connected) return err;
	return GN_ERR_NONE;
}


/**********************/
/* IDENTIFY FUNCTIONS */
/**********************/

static gn_error P6510_IncomingIdentify(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int pos, string_length;

	switch (message[3]) {
	case 0x01:
		if (data->Imei) {
			snprintf(data->Imei, GNOKII_MIN(message[9], GSM_MAX_IMEI_LENGTH), "%s", message + 10);
			dprintf("Received imei %s\n", data->Imei);
		}
		break;
	case 0x08:
		if (data->Revision) {
			pos = 10;
			string_length = 0;
			while (message[pos + string_length] != 0x0a) string_length++;
			snprintf(data->Revision, GNOKII_MIN(string_length + 1, GSM_MAX_REVISION_LENGTH), "%s", message + pos);
			dprintf("Received revision %s\n",data->Revision);
		}
		if (data->Model) {
			pos = 10;
			string_length = 0;
			while (message[pos++] != 0x0a) ;  /* skip revision */
			pos++;
			while (message[pos++] != 0x0a) ;  /* skip date */

			string_length = 0;
			while (message[pos + string_length] != 0x0a) string_length++;  /* find model length */
			dprintf("model length: %i\n", string_length);

			snprintf(data->Model, GNOKII_MIN(string_length + 1, GSM_MAX_MODEL_LENGTH), "%s", message + pos);
			dprintf("Received model %s\n", data->Model);
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x2b (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

/* Just as an example.... */
/* But note that both requests are the same type which isn't very 'proper' */
static gn_error P6510_Identify(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x41};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Identifying...\n");
	PNOK_GetManufacturer(data->Manufacturer);
	if (SM_SendMessage(state, 5, 0x1b, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	if (SM_SendMessage(state, 6, 0x1b, req2) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	SM_WaitFor(state, data, 0x1b);
	SM_Block(state, data, 0x1b); /* waits for all requests - returns req2 error */
	SM_GetError(state, 0x1b);

	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GN_ERR_UNKNOWN;
	return GN_ERR_NONE;
}

static gn_error P6510_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x41};

	dprintf("Getting imei...\n");
	if (SM_SendMessage(state, 5, 0x1b, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static gn_error P6510_GetModel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Getting model...\n");
	if (SM_SendMessage(state, 6, 0x1b, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static gn_error P6510_GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Getting revision...\n");
	if (SM_SendMessage(state, 6, 0x1b, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

/*******************************/
/* SMS/PICTURE/FOLDER HANDLING */
/*******************************/
static void ResetLayout(unsigned char *message, GSM_Data *data)
{
	/* reset everything */
	data->RawSMS->MoreMessages     = 0;
	data->RawSMS->ReplyViaSameSMSC = 0;
	data->RawSMS->RejectDuplicates = 0;
	data->RawSMS->Report           = 0;
	data->RawSMS->Reference        = 0;
	data->RawSMS->PID              = 0;
	data->RawSMS->ReportStatus     = 0;
	memcpy(data->RawSMS->SMSCTime, message, 0);
	memcpy(data->RawSMS->Time, message, 0);
	memcpy(data->RawSMS->RemoteNumber, message, 0);
	memcpy(data->RawSMS->MessageCenter, message, 0);
	data->RawSMS->UDHIndicator     = 0;
	data->RawSMS->DCS              = 0;
	data->RawSMS->Length           = 0;
	memcpy(data->RawSMS->UserData, message, 0);
	data->RawSMS->ValidityIndicator = 0;
	memcpy(data->RawSMS->Validity, message, 0);
}

static void ParseLayout(unsigned char *message, GSM_Data *data)
{
	int i, subblocks;
	unsigned char *block = message;

	ResetLayout(message, data);
	/*
	dprintf("Trying to parse message....\n");
	dprintf("Blocks: %i\n", message[0]);
	dprintf("Type: %i\n", message[1]);
	dprintf("Length: %i\n", message[2]);
	*/
	data->RawSMS->UDHIndicator = message[3];
	data->RawSMS->DCS = message[5];

	switch (message[1]) {
	case 0x00: /* deliver */
		dprintf("Type: Deliver\n");
		data->RawSMS->Type = SMS_Deliver;
		block = message + 16;
		memcpy(data->RawSMS->SMSCTime, message + 6, 7);
		break;
	case 0x01: /* delivery report */
		dprintf("Type: Delivery Report\n");
		data->RawSMS->Type = SMS_Delivery_Report;
		block = message + 20; 
		memcpy(data->RawSMS->SMSCTime, message + 6, 7);
		memcpy(data->RawSMS->Time, message + 13, 7);
		break;
	case 0x02: /* submit, templates */
		if (data->RawSMS->MemoryType == 5) {
			dprintf("Type: TextTemplate\n");
			data->RawSMS->Type = SMS_TextTemplate;
			break;
		}
		switch (data->RawSMS->Status) {
		case SMS_Sent:
			dprintf("Type: SubmitSent\n");
			data->RawSMS->Type = SMS_SubmitSent;
			break;
		case SMS_Unsent:
			dprintf("Type: Submit\n");
			data->RawSMS->Type = SMS_Submit;
			break;
		default:
			dprintf("Wrong type\n");
			break;
		}
		block = message + 8; 
		break;
	case 0x80:
		dprintf("Type: Picture\n");
		data->RawSMS->Type = SMS_Picture;
		break;
	case 0xa0: /* pictures, still ugly */
		switch (message[2]) {
		case 0x01:
			dprintf("Type: PictureTemplate\n");
			data->RawSMS->Type = SMS_PictureTemplate;
			data->RawSMS->Length = 256;
			memcpy(data->RawSMS->UserData, message + 13, data->RawSMS->Length);
			return;
		case 0x02:
			dprintf("|Type: Picture\n");
			data->RawSMS->Type = SMS_Picture;
			block = message + 20;
			memcpy(data->RawSMS->SMSCTime, message + 10, 7);
			data->RawSMS->Length = 256; 
			memcpy(data->RawSMS->UserData, message + 50, data->RawSMS->Length);
			break;
		default:
			dprintf("Unknown picture message!\n");
			break;
		}
		break;
	default:
		dprintf("Type %02x not yet handled!\n", message[1]);
		break;
	}
	subblocks = block[0];
	/*
	dprintf("Subblocks: %i\n", subblocks);
	*/
	block = block + 1;
	for (i = 0; i < subblocks; i++) {
		/*
		dprintf("  Type of subblock %i: %02x\n", i,  block[0]);
		dprintf("  Length of subblock %i: %i\n", i, block[1]);
		*/
		switch (block[0]) {
		case 0x82: /* number */
			switch (block[2]) {
			case 0x01:
				memcpy(data->RawSMS->RemoteNumber,  block + 4, block[3]);
				/*
				dprintf("   Setting remote number: length: %i first: %02x\n", block[3], block[4]);
				*/
				break;
			case 0x02:
				memcpy(data->RawSMS->MessageCenter,  block + 4, block[3]);
				/*
				dprintf("   Setting MC number: length: %i first: %02x\n", block[3], block[4]);
				*/
				break;
			default:
				/*
				dprintf("Error while parsing numbers!\n");
				*/
				break;
			}
			break;
		case 0x80: /* User Data */
			if ((data->RawSMS->Type != SMS_Picture) && (data->RawSMS->Type != SMS_PictureTemplate)) {  
					/* Ignore the found UserData block for pictures */
				data->RawSMS->Length = block[3];
				memcpy(data->RawSMS->UserData, block + 4, data->RawSMS->Length);
			}
			break;
		case 0x08: /* Time blocks (mainly at the end of submit sent messages */
			/*
			dprintf("   Setting time...\n");
			*/
			memcpy(data->RawSMS->SMSCTime, block + 3, block[2]);
			break;
		default:
			dprintf("Unknown block of type: %02x!\n", block[0]);
			break;
		}
		/*
		dprintf("\n");
		*/
		block = block + block[1];
	}
	return;
}

/* handle messages of type 0x14 (SMS Handling, Folders, Logos.. */
static gn_error P6510_IncomingFolder(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int i, j, status;

	switch (message[3]) {
	/* savesms */
	case 0x01:
		switch (message[4]) {
		case 0x00:
			dprintf("SMS successfully saved\n");
			dprintf("Saved in folder %i at location %i\n", message[8], (message[6] << 8) | message[7]);
			data->RawSMS->Number = (message[6] << 8) | message[7];
			break;
		case 0x02:
			printf("SMS saving failed: Invalid location\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x05:
			printf("SMS saving failed: Incorrect folder\n");
			return GN_ERR_INVALIDMEMORYTYPE;
		default:
			dprintf("ERROR: unknown (%02x)\n",message[4]);
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	/* getsms */
	case 0x03:
		dprintf("Trying to get message # %i in folder # %i\n", message[9], message[7]);
		if (!data->RawSMS) return GN_ERR_INTERNALERROR;
		status = data->RawSMS->Status;
		memset(data->RawSMS, 0, sizeof(GSM_SMSMessage));
		data->RawSMS->Status = status;
		ParseLayout(message + 13, data);

		/* Number of SMS in folder */
		data->RawSMS->Number = (message[8] << 8) | message[9];

		/* MessageType/FolderID */
		data->RawSMS->MemoryType = message[7];

		break;
	/* delete sms */
	case 0x05:
		switch (message[4]) {
		case 0x00:
			dprintf("SMS successfully deleted\n");
			break;
		case 0x02:
			printf("SMS deleting failed: Invalid location?\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x05:
			printf("SMS saving failed: Incorrect folder\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("ERROR: unknown %i\n",message[4]);
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	/* delete sms failed */
	case 0x06:
		switch (message[4]) {
		case 0x02:
			dprintf("Invalid location\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("Unknown reason.\n");
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* sms status */
	case 0x09:
		dprintf("SMS Status received\n");

		data->SMSStatus->Number = ((message[12] << 8) | message[13]) + 
			((message[24] << 8) | message[25]) + data->SMSFolder->Number;
		data->SMSStatus->Unread = ((message[14] << 8) | message[15]) +
			((message[26] << 8) | message[27]);
		break;

	/* getfolderstatus */
	case 0x0d:
		dprintf("Message: SMS Folder status received\n" );
		if (!data->SMSFolder) return GN_ERR_INTERNALERROR;
		status = data->SMSFolder->FolderID;
		memset(data->SMSFolder, 0, sizeof(SMS_Folder));
		data->SMSFolder->FolderID = status;

		data->SMSFolder->Number = (message[6] << 8) | message[7];
		dprintf("Message: Number of Entries: %i\n" , data->SMSFolder->Number);
		dprintf("Message: IDs of Entries : ");
		for (i = 0; i < data->SMSFolder->Number; i++) {
			data->SMSFolder->Locations[i] = (message[(i * 2) + 8] << 8) | message[(i * 2) + 9];
			dprintf("%d, ", data->SMSFolder->Locations[i]);
		}
		dprintf("\n");
		break;
	/* get message status */
	case 0x0f:
		dprintf("Message: SMS message(%i in folder %i) status received: %i\n", 
			(message[10] << 8) | message[11],  message[12], message[13]);

		if (!data->RawSMS) return GN_ERR_INTERNALERROR;

		/* Short Message status */
		data->RawSMS->Status = message[13];

		break;
	/* create folder */
	case 0x11:
		dprintf("Create SMS folder status received..\n");
		if (!data->SMSFolder) return GN_ERR_INTERNALERROR;
		memset(data->SMSFolder, 0, sizeof(SMS_Folder));

		switch (message[4]) {
		case 0x00:
			dprintf("SMS Folder successfully created!\n");
			data->SMSFolder->FolderID = message[8];
			char_decode_unicode(data->SMSFolder->Name, message + 10, length - 11);
			dprintf("   Folder ID: %i\n", data->SMSFolder->FolderID);
			dprintf("   Name: %s\n", data->SMSFolder->Name);
			break;
		default:
			dprintf("Failed to create SMS Folder! Reason unknown (%02x)!\n", message[4]);
			return GN_ERR_UNKNOWN;
			break;
		}
		break;
	/* getfolders */
	case 0x13:
		if (!data->SMSFolderList) return GN_ERR_INTERNALERROR;
		memset(data->SMSFolderList, 0, sizeof(SMS_FolderList));

		data->SMSFolderList->Number = message[5];
		dprintf("Message: %d SMS Folders received:\n", data->SMSFolderList->Number);

		for (j = 0; j < data->SMSFolderList->Number; j++) {
			int len;
			strcpy(data->SMSFolderList->Folder[j].Name, "               ");

			i = 10 + (j * 40);
			data->SMSFolderList->FolderID[j] = message[i - 2];
			dprintf("Folder(%i) name: ", message[i - 2]);
			len = message[i - 1] << 1;
			char_decode_unicode(data->SMSFolderList->Folder[j].Name, message + i, len);
			dprintf("%s\n", data->SMSFolderList->Folder[j].Name);
		}
		break;
	/* delete folder */
	case 0x15:
		switch (message[4]) {
		case 0x00:
			dprintf("SMS Folder successfully deleted!\n");
			break;
		case 0x68:
			dprintf("SMS Folder could not be deleted! Not existant?\n");
			return GN_ERR_INVALIDLOCATION;
			break;
		case 0x6b:
			dprintf("SMS Folder could not be deleted! Not empty?\n");
			return GN_ERR_FAILED;
			break;
		default:
			dprintf("SMS Folder could not be deleted! Reason unknown (%02x)\n", message[4]);
			return GN_ERR_FAILED;
			break;
		}
		break;
	case 0x17:
		break;
	/* get list of SMS pictures */
	case 0x97:
		dprintf("Getting list of SMS pictures...\n");
		break;

	/* Some errors */
	case 0xc9:
		dprintf("Unknown command???\n");
		return GN_ERR_UNHANDLEDFRAME;

	case 0xca:
		dprintf("Phone not ready???\n");
		return GN_ERR_UNHANDLEDFRAME;

	default:
		dprintf("Message: Unknown message of type 14 : %02x  length: %d\n", message[3], length);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_GetSMSStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x08, 0x00, 0x00, 0x01};
	SMS_Folder status_fld, *old_fld;
	gn_error error;

	dprintf("Getting SMS Status...\n");

	/* Nokia 6510 and family does not show not "fixed" messages from the
	 * Templates folder, ie. when you save a message to the Templates folder,
	 * SMSStatus does not change! Workaround: get Templates folder status, which
	 * does show these messages.
	 */

	old_fld = data->SMSFolder;

	data->SMSFolder = &status_fld;
	data->SMSFolder->FolderID = GMT_TE;

	error = P6510_GetSMSFolderStatus(data, state);
	if (error != GN_ERR_NONE) goto out;

	error = SM_SendMessage(state, 7, P6510_MSG_FOLDER, req);
	if (error != GN_ERR_NONE) goto out;

	error = SM_Block(state, data, P6510_MSG_FOLDER);
 out:
	data->SMSFolder = old_fld;
	return error;
}

static gn_error P6510_GetSMSFolders(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x12, 0x00, 0x00};

	dprintf("Getting SMS Folders...\n");
	if (SM_SendMessage(state, 6, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_DeleteSMSFolder(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x14, 
			       0x06, /* Folder ID */
			       0x00};

	dprintf("Deleting SMS Folder...\n");
	req[4] = data->SMSFolder->FolderID + 5;
	if (req[4] < 6) return GN_ERR_INVALIDMEMORYTYPE;

	if (SM_SendMessage(state, 6, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_CreateSMSFolder(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[50] = {FBUS_FRAME_HEADER, 0x10, 
				 0x01, 0x00, 0x01,
				 0x00, /* length */
				 0x00, 0x00 };

	dprintf("Creating SMS Folder...\n");
	
	char_encode_unicode(req + 10, data->SMSFolder->Name, strlen(data->SMSFolder->Name));
	req[7] = strlen(data->SMSFolder->Name) * 2 + 6;

	if (SM_SendMessage(state, req[7] + 6, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_GetSMSFolderStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0C, 
			       0x02, /* 0x01 SIM, 0x02 ME*/
			       0x00, /* Folder ID */
			       0x0F, 0x55, 0x55, 0x55};
	gn_error error;
	int i;
	SMS_Folder phone;


	req[5] = GetMemoryType(data->SMSFolder->FolderID);

	dprintf("Getting SMS Folder (%i) status (%i)...\n", req[5], req[4]);

       	if ((req[5] == P6510_MEMORY_IN) || (req[5] == P6510_MEMORY_OU)) { /* special case IN/OUTBOX */
		dprintf("Special case IN/OUTBOX in GetSMSFolderStatus!\n");

		if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		error = SM_Block(state, data, P6510_MSG_FOLDER);
		if (error != 0) return error;

		memcpy(&phone, data->SMSFolder, sizeof(SMS_Folder));

		req[4] = 0x01;
		if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		error = SM_Block(state, data, P6510_MSG_FOLDER);

		for (i = 0; i < phone.Number; i++) {
			data->SMSFolder->Locations[data->SMSFolder->Number] = phone.Locations[i] + 1024;
			data->SMSFolder->Number++;
		}
		return GN_ERR_NONE;
	} 

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_GetSMSMessageStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0E, 
			       0x02, /* 0x01 SIM, 0x02 phone*/
			       0x00, /* Folder ID */
			       0x00, 
			       0x00, /* Location */
			       0x55, 0x55};

	if ((data->RawSMS->MemoryType == GMT_IN) || (data->RawSMS->MemoryType == GMT_OU)) {
		if (data->RawSMS->Number > 1024) {
			req[7] = data->RawSMS->Number - 1024;
		} else {
			req[4] = 0x01;
			req[7] = data->RawSMS->Number;
		}
	}

	dprintf("Getting SMS message (%i in folder %i) status...\n", req[7], data->RawSMS->MemoryType);

       	req[5] = GetMemoryType(data->RawSMS->MemoryType);

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_GetSMSnoValidate(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02,
				   0x02, /* 0x01 for INBOX, 0x02 for others */
				   0x00, /* FolderID */
				   0x00,
				   0x02, /* Location */
				   0x01, 0x00};
	gn_error error;

	dprintf("Getting SMS (no validate) ...\n");

	error = P6510_GetSMSMessageStatus(data, state);

	if ((data->RawSMS->MemoryType == GMT_IN) || (data->RawSMS->MemoryType == GMT_OU)) {
		if (data->RawSMS->Number > 1024) {
			data->RawSMS->Number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}

	req[5] = GetMemoryType(data->RawSMS->MemoryType);
	req[7] = data->RawSMS->Number;

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error ValidateSMS(GSM_Data *data, GSM_Statemachine *state)
{
	gn_error error;

	/* Handle MemoryType = 0 explicitely, because SMSFolder->FolderID = 0 by default */
	if (data->RawSMS->MemoryType == 0) return GN_ERR_INVALIDMEMORYTYPE;

	/* see if the message we want is from the last read folder, i.e. */
	/* we don't have to get folder status again */
	if ((!data->SMSFolder) || (!data->SMSFolderList))
		return GN_ERR_INTERNALERROR;
	if (data->RawSMS->MemoryType != data->SMSFolder->FolderID) {
		if ((error = P6510_GetSMSFolders(data, state)) != GN_ERR_NONE) return error;

		if ((GetMemoryType(data->RawSMS->MemoryType) > 
		     data->SMSFolderList->FolderID[data->SMSFolderList->Number - 1]) ||
		    (data->RawSMS->MemoryType < 12))
			return GN_ERR_INVALIDMEMORYTYPE;
		data->SMSFolder->FolderID = data->RawSMS->MemoryType;
		if ((error = P6510_GetSMSFolderStatus(data, state)) != GN_ERR_NONE) return error;
	}

	if (data->SMSFolder->Number < data->RawSMS->Number) {
		if (data->RawSMS->Number < MAX_SMS_MESSAGES)
			return GN_ERR_EMPTYLOCATION;
		else
			return GN_ERR_INVALIDLOCATION;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_DeleteSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x04,
				   0x02, /* 0x01 for SM, 0x02 for ME */
				   0x00, /* FolderID */
				   0x00,
				   0x02, /* Location */
				   0x0F, 0x55};
	gn_error error;

	dprintf("Deleting SMS...\n");

	error = ValidateSMS(data, state);
	if (error != GN_ERR_NONE) return error;

	data->RawSMS->Number = data->SMSFolder->Locations[data->RawSMS->Number - 1];

	if ((data->RawSMS->MemoryType == GMT_IN) || (data->RawSMS->MemoryType == GMT_OU)) {
		if (data->RawSMS->Number > 1024) {
			data->RawSMS->Number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}

	req[5] = GetMemoryType(data->RawSMS->MemoryType);
	req[7] = data->SMSFolder->Locations[data->RawSMS->Number - 1];

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_DeleteSMSnoValidate(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x04,
				   0x02, /* 0x01 for SM, 0x02 for ME */
				   0x00, /* FolderID */
				   0x00,
				   0x02, /* Location */
				   0x0F, 0x55};

	dprintf("Deleting SMS (noValidate)...\n");

	if ((data->RawSMS->MemoryType == GMT_IN) || (data->RawSMS->MemoryType == GMT_OU)) {
		if (data->RawSMS->Number > 1024) {
			data->RawSMS->Number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}

	req[5] = GetMemoryType(data->RawSMS->MemoryType);
	req[7] = data->RawSMS->Number;

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_GetSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02,
				   0x02, /* 0x01 for SM, 0x02 for ME */
				   0x00, /* FolderID */
				   0x00,
				   0x02, /* Location */
				   0x01, 0x00};
	gn_error error;

	dprintf("Getting SMS...\n");

	error = ValidateSMS(data, state);
	if (error != GN_ERR_NONE) return error;

	data->RawSMS->Number = data->SMSFolder->Locations[data->RawSMS->Number - 1];

	error = P6510_GetSMSMessageStatus(data, state);

	if ((data->RawSMS->MemoryType == GMT_IN) || (data->RawSMS->MemoryType == GMT_OU)) {
		if (data->RawSMS->Number > 1024) {
			data->RawSMS->Number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}

	req[5] = GetMemoryType(data->RawSMS->MemoryType);
	req[7] = data->RawSMS->Number;

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static gn_error P6510_SaveSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = { FBUS_FRAME_HEADER, 0x00,
				   0x02,			/* 1 = SIM, 2 = ME 	*/
				   0x02,			/* Folder   		*/
				   0x00, 0x00,			/* Location 		*/
				   0x03 };			/* SMS state 		*/
#if 0
	unsigned char req2[200] = { FBUS_FRAME_HEADER, 0x16,
				    0x02,			/* 1 = SIM, 2 = ME 	*/
				    0x02,			/* Folder   		*/
				    0x00, 0x01};		/* Location 		*/
	unsigned char desc[10];
#endif
	gn_error error = GN_ERR_NONE;
	int len = 9;
	unsigned char ans[5];
	/* int i; */

	dprintf("Saving sms\n");
	if (data->RawSMS->MemoryType == GMT_IN && data->RawSMS->Type == SMS_Submit)
		return GN_ERR_INVALIDMEMORYTYPE;
	if (data->RawSMS->MemoryType != GMT_IN && 
	    data->RawSMS->Type == SMS_Deliver && 
	    data->RawSMS->Status != SMS_Sent)
		return GN_ERR_INVALIDMEMORYTYPE;
	if (data->RawSMS->MemoryType == GMT_TE ||
	    data->RawSMS->MemoryType == GMT_SM ||
	    data->RawSMS->MemoryType == GMT_ME)
		return GN_ERR_INVALIDMEMORYTYPE;

	req[5] = GetMemoryType(data->RawSMS->MemoryType);

	req[6] = data->RawSMS->Number / 256;
	req[7] = data->RawSMS->Number % 256;

	if (data->RawSMS->Type == SMS_Submit) req[8] = 0x07;
	if (data->RawSMS->Status == SMS_Sent) req[8] -= 2;

	memset(req + 15, 0x00, sizeof(req) - 15);

	len += EncodeSMS(data, state, req + 9);
	/*
	for (i = 0; i < len; i++) dprintf("%02x ", req[i]);
	dprintf("\n");
	*/
	fprintf(stdout, "6510 series phones seem to be quite sensitive to malformed SMS messages\n"
			 "It may have to be sent to Nokia Service if something fails!\n"
			 "Do you really want to continue? (yes/no) ");
	GetLine(stdin, ans, 4);
	if (strcmp(ans, "yes")) return GN_ERR_USERCANCELED;

	if (SM_SendMessage(state, len, P6510_MSG_FOLDER, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = SM_Block(state, data, P6510_MSG_FOLDER);
#if 0
	if (error == GN_ERR_NONE) {
		req2[5] = GetMemoryType(data->RawSMS->MemoryType);
		if (req2[5] == 0x05) req2[5] = 0x03;
		req2[6] = data->RawSMS->Number / 256;
		req2[7] = data->RawSMS->Number % 256;

		len = 8;
		memcpy(req2 + len, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
		len += 16;

		req2[len++] = 0;
		req2[len++] = 0;
		if (SM_SendMessage(state, len, P6510_MSG_FOLDER, req2) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		return SM_Block(state, data, P6510_MSG_FOLDER);
	}
#endif
	return error;
}


/****************/
/* SMS HANDLING */
/****************/

static gn_error P6510_IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	gn_error	e = GN_ERR_NONE;
	unsigned int parts_no, offset, i;

	dprintf("Frame of type 0x02 (SMS handling) received!\n");

	if (!data) return GN_ERR_INTERNALERROR;

	switch (message[3]) {
	case P6510_SUBSMS_SMSC_RCV: /* 0x15 */
		switch (message[4]) {
		case 0x00:
			dprintf("SMSC Received\n");
			break;
		case 0x02:
			dprintf("SMSC reception failed\n");
			e = GN_ERR_EMPTYLOCATION;
			break;
		default:
			dprintf("Unknown response subtype: %02x\n", message[4]);
			e = GN_ERR_UNHANDLEDFRAME;
			break;
		}

		if (e != GN_ERR_NONE) break;

		data->MessageCenter->No = message[8];
		data->MessageCenter->Format = message[4];
		data->MessageCenter->Validity = message[12];  /* due to changes in format */

		parts_no = message[13];
		offset = 14;

		for (i = 0; i < parts_no; i++) {
			switch (message[offset]) {
			case 0x82: /* Number */
				switch (message[offset + 2]) {
				case 0x01: /* Default number */
					if (message[offset + 4] % 2) message[offset + 4]++;
					message[offset + 4] = message[offset + 4] / 2 + 1;
					snprintf(data->MessageCenter->Recipient.Number,
						 MAX_NUMBER_LEN + 1, "%s", char_get_bcd_number(message + offset + 4));
					data->MessageCenter->Recipient.Type = message[offset + 5];
					break;
				case 0x02: /* SMSC number */
					snprintf(data->MessageCenter->SMSC.Number,
						 MAX_NUMBER_LEN + 1, "%s", char_get_bcd_number(message + offset + 4));
					data->MessageCenter->SMSC.Type = message[offset + 5];
					break;
				default:
					dprintf("Unknown subtype %02x. Ignoring\n", message[offset + 1]);
					break;
				}
				break;
			case 0x81: /* SMSC name */
				char_decode_unicode(data->MessageCenter->Name,
					      message + offset + 4,
					      message[offset + 2] << 1);
				break;
			default:
				dprintf("Unknown subtype %02x. Ignoring\n", message[offset]);
				break;
			}
			offset += message[offset + 1];
		}

		data->MessageCenter->DefaultName = -1;	/* FIXME */

		if (strlen(data->MessageCenter->Recipient.Number) == 0) {
			sprintf(data->MessageCenter->Recipient.Number, "(none)");
		}
		if (strlen(data->MessageCenter->SMSC.Number) == 0) {
			sprintf(data->MessageCenter->SMSC.Number, "(none)");
		}
		if(strlen(data->MessageCenter->Name) == 0) {
			data->MessageCenter->Name[0] = '\0';
		}

		break;

	case P6510_SUBSMS_SMS_SEND_STATUS: /* 0x03 */
		switch (message[8]) {
		case P6510_SUBSMS_SMS_SEND_OK: /* 0x00 */
			dprintf("SMS sent\n");
			e = GN_ERR_NONE;
			break;

		case P6510_SUBSMS_SMS_SEND_FAIL: /* 0x01 */
			dprintf("SMS sending failed\n");
			e = GN_ERR_FAILED;
			break;

		default:
			dprintf("Unknown status of the SMS sending -- assuming failure\n");
			e = GN_ERR_FAILED;
			break;
		}
		break;

	case 0x0e:
		dprintf("Ack for request on Incoming SMS\n");
		break;

	case 0x11:
		dprintf("SMS received\n");
		/* We got here the whole SMS */
		NewSMS = true;
		break;

	case P6510_SUBSMS_SMS_RCVD: /* 0x10 */
	case P6510_SUBSMS_CELLBRD_OK: /* 0x21 */
	case P6510_SUBSMS_CELLBRD_FAIL: /* 0x22 */
	case P6510_SUBSMS_READ_CELLBRD: /* 0x23 */
	case P6510_SUBSMS_SMSC_OK: /* 0x31 */
	case P6510_SUBSMS_SMSC_FAIL: /* 0x32 */
		dprintf("Subtype 0x%02x of type 0x%02x (SMS handling) not implemented\n", message[3], P6510_MSG_SMS);
		return GN_ERR_NOTIMPLEMENTED;

	default:
		dprintf("Unknown subtype of type 0x%02x (SMS handling): 0x%02x\n", P6510_MSG_SMS, message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return e;
}

static gn_error P6510_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, P6510_SUBSMS_GET_SMSC, 0x01, 0x00};

	req[4] = data->MessageCenter->No;
	if (SM_SendMessage(state, 6, P6510_MSG_SMS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_SMS);
}

/**
 * P6510_SendSMS - low level SMS sending function for 6310/6510 phones
 * @data: gsm data
 * @state: statemachine
 *
 * Nokia changed the format of the SMS frame completly. Currently there are
 * here some magic numbers (well, many) but hopefully we'll get their meaning
 * soon.
 * 10.07.2002: Almost all frames should be known know :-) (Markus)
 */

static gn_error P6510_SendSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x02,
				  0x00, 0x00, 0x00, 0x55, 0x55}; /* What's this? */
	gn_error error;
	unsigned int pos;

	memset(req + 9, 0, 244);
	pos = EncodeSMS(data, state, req + 9);

	dprintf("Sending SMS...(%d)\n", pos + 9);
	if (SM_SendMessage(state, pos + 9, P6510_MSG_SMS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	do {
		error = SM_BlockNoRetryTimeout(state, data, P6510_MSG_SMS, state->Link.SMSTimeout);
	} while (!state->Link.SMSTimeout && error == GN_ERR_TIMEOUT);
	return error;
}

/**********************/
/* PHONEBOOK HANDLING */
/**********************/

static gn_error P6510_IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char blocks;

	switch (message[3]) {
	case 0x04:  /* Get status response */
		if (data->MemoryStatus) {
			if (message[5] != 0xff) {
				data->MemoryStatus->Used = (message[20] << 8) + message[21];
				data->MemoryStatus->Free = ((message[18] << 8) + message[19]) - data->MemoryStatus->Used;
				dprintf("Memory status - location = %d, Capacity: %d \n",
					(message[4] << 8) + message[5], (message[18] << 8) + message[19]);
			} else {
				dprintf("Unknown error getting mem status\n");
				return GN_ERR_NOTIMPLEMENTED;
			}
		}
		break;
	case 0x08:  /* Read Memory response */
		if (data->PhonebookEntry) {
			data->PhonebookEntry->Empty = true;
			data->PhonebookEntry->Group = 5; /* no group */
			data->PhonebookEntry->Name[0] = '\0';
			data->PhonebookEntry->Number[0] = '\0';
			data->PhonebookEntry->SubEntriesCount = 0;
			data->PhonebookEntry->Date.Year = 0;
			data->PhonebookEntry->Date.Month = 0;
			data->PhonebookEntry->Date.Day = 0;
			data->PhonebookEntry->Date.Hour = 0;
			data->PhonebookEntry->Date.Minute = 0;
			data->PhonebookEntry->Date.Second = 0;
		}
		if (data->Bitmap) {
			data->Bitmap->text[0] = '\0';
		}
		if (message[6] == 0x0f) { /* not found */
			switch (message[10]) {
			case 0x30:
				return GN_ERR_INVALIDMEMORYTYPE;
			case 0x33:
				return GN_ERR_EMPTYLOCATION;
			case 0x34:
				return GN_ERR_INVALIDLOCATION;
			default:
				return GN_ERR_UNKNOWN;
			}
		}
		dprintf("Received phonebook info\n");
		blocks     = message[21];
		return DecodePhonebook(message + 22, length - 21, data, blocks, message[11], 12);
	case 0x0c:
		if (message[6] == 0x0f) {
			switch (message[10]) {
			case 0x3d: return GN_ERR_FAILED;
			case 0x3e: return GN_ERR_FAILED;
			default:   return GN_ERR_UNHANDLEDFRAME;
			}
		}
		break;
	case 0x10:
		if (message[6] == 0x0f) {
			switch (message[10]) {
			case 0x34: return GN_ERR_INVALIDLOCATION;
			default:   return GN_ERR_UNHANDLEDFRAME;
			}
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x03 (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x00, 0x55, 0x55, 0x55, 0x00};

	dprintf("Getting memory status...\n");

	req[5] = GetMemoryType(data->MemoryStatus->MemoryType);
	if (SM_SendMessage(state, 10, P6510_MSG_PHONEBOOK, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}

static gn_error P6510_ReadPhonebookLocation(GSM_Data *data, GSM_Statemachine *state, int memtype, int location)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
			       0x02, 0x05, /* memory type */
			       0x00, 0x00, 0x00, 0x00, 
			       0x00, 0x01, /*location */
			       0x00, 0x00};

	dprintf("Reading phonebook location (%d)\n", location);

	req[9] = memtype;
	req[14] = location >> 8;
	req[15] = location & 0xff;

	if (SM_SendMessage(state, 18, P6510_MSG_PHONEBOOK, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}

static gn_error P6510_GetSpeedDial(GSM_Data *data, GSM_Statemachine *state)
{
	return P6510_ReadPhonebookLocation(data, state, P6510_MEMORY_SPEEDDIALS, data->SpeedDial->Number);
}

static unsigned char PackBlock(u8 id, u8 size, u8 no, u8 *buf, u8 *block)
{
	*(block++) = id;
	*(block++) = 0;
	*(block++) = 0;
	*(block++) = size + 5;
	*(block++) = 0xff; /* no; */
	memcpy(block, buf, size);
	return (size + 5);
}

static gn_error P6510_SetSpeedDial(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[40] = {FBUS_FRAME_HEADER, 0x0B, 0x00, 0x01, 0x01, 0x00, 0x00, 0x10, 
				 0xFF, 0x0E, 
				 0x00, 0x06, /* number */
				 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
				 0x01}; /* blocks */
	char string[40];

	dprintf("Setting speeddial...\n");
	req[13] = (data->SpeedDial->Number >> 8);
	req[13] = data->SpeedDial->Number & 0xff;

	string[0] = 0xff;
	string[1] = 0x00;
	string[2] = data->SpeedDial->Location;
	memcpy(string + 3, "\x00\x00\x00\x00", 4);

	if (data->SpeedDial->MemoryType == GMT_SM) 
		string[7] = 0x06;
	else
		string[7] = 0x05;
	memcpy(string + 8, "\x0B\x02", 2);

	PackBlock(0x1a, 10, 1, string, req + 22);

	if (SM_SendMessage(state, 38, P6510_MSG_PHONEBOOK, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}

static gn_error SetCallerBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[200] = {FBUS_FRAME_HEADER, 0x0B, 0x00, 0x01, 0x01, 0x00, 0x00, 0x10, 
				 0xFF, 0x10, 
				 0x00, 0x06, /* number */
				 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
				 0x01}; /* blocks */
	unsigned int count = 22, block = 0;
	char string[150];

/*
  00 01 00 0B 00 01 01 00 00 10 
  FE 10 
  00 02 
  00 00 00 00 00 00 00 
  04 
  1C 00 00 08 01 01 00 00 
  1E 00 00 08 02 02 00 00 
  1B 00 00 88 03 48 0E 00 00 7E CE A4 01 7D E0
  .....
  07 00 00 0E 04 08 00 56 00 49 00 50 00 00 
*/
	if (!data->Bitmap) return GN_ERR_INTERNALERROR;

	req[13] = data->Bitmap->number + 1;

	/* Group */
	string[0] = data->Bitmap->number + 1;
	string[1] = 0;
	string[2] = 0x55;
	count += PackBlock(0x1e, 3, block++, string, req + count);

	/* Logo */
	string[0] = data->Bitmap->width;
	string[1] = data->Bitmap->height;
	string[2] = string[3] = 0x00;
	string[4] = 0x7e;
	memcpy(string + 5, data->Bitmap->bitmap, data->Bitmap->size);
	count += PackBlock(0x1b, data->Bitmap->size + 5, block++, string, req + count);

#if 0
	/* Name */
	switch (data->Bitmap->number) {
	case 0:
		string[0] = 0x0d;
		char_encode_unicode(string + 1, "Family", 6);
		break;
	case 1:
		string[0] = 0x07;
		char_encode_unicode(string + 1, "VIP", 3);
		break;
	case 2:
		string[0] = 0x0f;
		char_encode_unicode(string + 1, "Friends", 7);
		break;
	case 3:
		string[0] = 0x15;
		char_encode_unicode(string + 1, "Colleagues", 10);
		break;
	case 4:
		string[0] = 0x0d;
		char_encode_unicode(string + 1, "Others", 6);
		break;
	default:
		string[0] = 0x00;
		break;
	}
	if (string[0]) count += PackBlock(0x07, string[0], block++, string, req + count);
#endif

	req[21] = block;
	if (SM_SendMessage(state, count, P6510_MSG_PHONEBOOK, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}

static gn_error P6510_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	int memtype;

	memtype = GetMemoryType(data->PhonebookEntry->MemoryType);

	return P6510_ReadPhonebookLocation(data, state, memtype, data->PhonebookEntry->Location);
}

static gn_error GetCallerBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07,
			       0x01, 0x01, 0x00, 0x01,
			       0xFE, 0x10,
			       0x00, 0x00, 0x00, 0x00, 0x00, 
			       0x03,  /* location */
			       0x00, 0x00};

	/* You can only get logos which have been altered, */
	/* the standard logos can't be read!! */

	req[15] = GNOKII_MIN(data->Bitmap->number + 1, GSM_MAX_CALLER_GROUPS);
	req[15] = data->Bitmap->number + 1;
	dprintf("Getting caller(%d) logo...\n", req[15]);

	if (SM_SendMessage(state, 18, P6510_MSG_PHONEBOOK, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}

static gn_error P6510_DeletePhonebookLocation(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0f, 0x55, 0x01, 0x04, 0x55, 0x00, 0x10, 0xFF, 0x02,
			       0x00, 0x08,  /* location */ 
			       0x00, 0x00, 0x00, 0x00,
			       0x05,  /* memory type */
			       0x55, 0x55, 0x55};
	GSM_PhonebookEntry *entry;

	if (data->PhonebookEntry) 
		entry = data->PhonebookEntry;
	else 
		return GN_ERR_TRYAGAIN;

	req[12] = (entry->Location >> 8);
	req[13] = entry->Location & 0xff;
	req[18] = GetMemoryType(entry->MemoryType);

	SEND_MESSAGE_BLOCK(P6510_MSG_PHONEBOOK, 22);
}

static gn_error P6510_WritePhonebookLocation(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x10,
				  0x02, 0x00,  /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* blocks */
	char string[500];
	int block, i, j, defaultn;
	unsigned int count = 22;
	GSM_PhonebookEntry *entry;

	if (data->PhonebookEntry) 
		entry = data->PhonebookEntry;
	else 
		return GN_ERR_TRYAGAIN;

	req[11] = GetMemoryType(entry->MemoryType);
	req[12] = (entry->Location >> 8);
	req[13] = entry->Location & 0xff;

	block = 1;
	if ((*(entry->Name)) && (*(entry->Number))) {
		/* Name */
		j = strlen(entry->Name);
		char_encode_unicode((string + 1), entry->Name, j);
		/* Length ot the string + length field + terminating 0 */
		string[0] = j * 2;
		count += PackBlock(0x07, j * 2 + 1, block++, string, req + count);

		/* Group */
		string[0] = entry->Group + 1;
		string[1] = 0;
		string[2] = 0x55;
		count += PackBlock(0x1e, 3, block++, string, req + count);

		/* Default Number */
		defaultn = 999;
		for (i = 0; i < entry->SubEntriesCount; i++)
			if (entry->SubEntries[i].EntryType == GSM_Number)
				if (!strcmp(entry->Number, entry->SubEntries[i].data.Number))
					defaultn = i;
		if (defaultn < i) {
			string[0] = entry->SubEntries[defaultn].NumberType;
			string[1] = string[2] = string[3] = 0;
			j = strlen(entry->SubEntries[defaultn].data.Number);
			char_encode_unicode((string + 5), entry->SubEntries[defaultn].data.Number, j);
			string[4] = j * 2;
			count += PackBlock(0x0b, j * 2 + 5, block++, string, req + count);
		}

		/* Rest of the numbers */
		for (i = 0; i < entry->SubEntriesCount; i++)
			if (entry->SubEntries[i].EntryType == GSM_Number) {
				if (i != defaultn) {
					string[0] = entry->SubEntries[i].NumberType;
					string[1] = string[2] = string[3] = 0;
					j = strlen(entry->SubEntries[i].data.Number);
					char_encode_unicode((string + 5), entry->SubEntries[i].data.Number, j);
					string[4] = j * 2;
					count += PackBlock(0x0b, j * 2 + 5, block++, string, req + count);
				}
			} else {
				j = strlen(entry->SubEntries[i].data.Number);
				string[0] = j * 2;
				char_encode_unicode((string + 1), entry->SubEntries[i].data.Number, j);
				count += PackBlock(entry->SubEntries[i].EntryType, j * 2 + 1, block++, string, req + count);
			}

		req[21] = block - 1;
		dprintf("Writing phonebook entry %s...\n",entry->Name);
	} else {
		return P6510_DeletePhonebookLocation(data, state);
	}
	SEND_MESSAGE_BLOCK(P6510_MSG_PHONEBOOK, count);
}


/******************/
/* CLOCK HANDLING */
/******************/

static gn_error P6510_IncomingClock(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	/*
Unknown subtype of type 0x19 (clock handling): 0x1c
UNHANDLED FRAME RECEIVED
request: 0x19 / 0x0004
00 01 00 1b                                     |                 
reply: 0x19 / 0x0012
01 23 00 1c 06 01 ff ff ff ff 00 00 49 4c 01 3e |  #    ÿÿÿÿ  IL >
01 56
	*/

	dprintf("Incoming clock!\n");
	if (!data || !data->DateTime) return GN_ERR_INTERNALERROR;
	switch (message[3]) {
	case P6510_SUBCLO_SET_DATE_RCVD:
		dprintf("Date/Time succesfully set!\n");
		break;
	case P6510_SUBCLO_SET_ALARM_RCVD:
		dprintf("Alarm succesfully set!\n");
		break;
	case P6510_SUBCLO_DATE_RCVD:
		dprintf("Date/Time received!\n");
		data->DateTime->Year = (((unsigned int)message[10]) << 8) + message[11];
		data->DateTime->Month = message[12];
		data->DateTime->Day = message[13];
		data->DateTime->Hour = message[14];
		data->DateTime->Minute = message[15];
		data->DateTime->Second = message[16];

		break;
	case P6510_SUBCLO_ALARM_RCVD:
		/*
		switch(message[8]) {
		case P6510_ALARM_ENABLED:
			data->DateTime->AlarmEnabled = 1;
			break;
		case P6510_ALARM_DISABLED:
			data->DateTime->AlarmEnabled = 0;
			break;
		default:
			data->DateTime->AlarmEnabled = -1;
			dprintf("Unknown value of alarm enable byte: 0x%02x\n", message[8]);
			error = GN_ERR_UNKNOWN;
			break;
		}
		*/
		data->DateTime->Hour = message[14];
		data->DateTime->Minute = message[15];

		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (clock handling): 0x%02x\n", P6510_MSG_CLOCK, message[3]);
		error = GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return error;
}

static gn_error P6510_GetDateTime(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, P6510_SUBCLO_GET_DATE, 0x00, 0x00};

	SEND_MESSAGE_BLOCK(P6510_MSG_CLOCK, 6);
}

static gn_error P6510_SetDateTime(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x01, 
			       0x01, 0x0c, 0x01, 0x03,
			       0x07, 0xd2,	/* Year */
			       0x08, 0x01,     /* Month & Day */
			       0x15, 0x1f,	/* Hours & Minutes */
			       0x00,		/* Seconds */
			       0x00};

	
	req[10] = data->DateTime->Year / 256;
	req[11] = data->DateTime->Year % 256;
	req[12] = data->DateTime->Month;
	req[13] = data->DateTime->Day;
	req[14] = data->DateTime->Hour;
	req[15] = data->DateTime->Minute;
	req[16] = data->DateTime->Second;

	SEND_MESSAGE_BLOCK(P6510_MSG_CLOCK, 18);
}

static gn_error P6510_GetAlarm(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x19, 0x00, 0x02};

	SEND_MESSAGE_BLOCK(P6510_MSG_CLOCK, 6);
}

static gn_error P6510_SetAlarm(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x11, 0x00, 0x01, 
			       0x01, 0x0c, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00,		/* Hours, Minutes */
			       0x00, 0x00, 0x00 };

	dprintf("Setting alarm\n");
	if (data->DateTime->AlarmEnabled != 1) return GN_ERR_NOTSUPPORTED;

	req[14] = data->DateTime->Hour;
	req[15] = data->DateTime->Minute;

	SEND_MESSAGE_BLOCK(P6510_MSG_CLOCK, 19);
}


/*********************/
/* CALENDAR HANDLING */
/********************/

static gn_error P6510_IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	gn_error			e = GN_ERR_NONE;
	int				i, year;

	if (!data || !data->CalendarNote) return GN_ERR_INTERNALERROR;

	year = 	data->CalendarNote->Time.Year;
	dprintf("Year: %i\n", data->CalendarNote->Time.Year);
	switch (message[3]) {
	case P6510_SUBCAL_NOTE_RCVD:
		return DecodeCalendar(message, length, data);
		break;
	case P6510_SUBCAL_INFO_RCVD:
		dprintf("Calendar Notes Info received! %i\n", (message[4] << 8) | message[5]);
		data->CalendarNotesList->Number = (message[4] << 8) + message[5];
		dprintf("Location of Notes: ");
		for (i = 0; i < data->CalendarNotesList->Number; i++) {
			data->CalendarNotesList->Location[i] = (message[8 + 2 * i] << 8) | message[9 + 2 * i];
			dprintf("%i ", data->CalendarNotesList->Location[i]); 
		}
		dprintf("\n");
		break;
	case P6510_SUBCAL_FREEPOS_RCVD:
		dprintf("First free position received: %i!\n", (message[4] << 8) | message[5]);
		data->CalendarNote->Location = (((unsigned int)message[4]) << 8) + message[5];
		break;
	case P6510_SUBCAL_DEL_NOTE_RESP:
		dprintf("Succesfully deleted calendar note: %i!\n", (message[4] << 8) | message[5]);
		break;
	case P6510_SUBCAL_ADD_MEETING_RESP:
	case P6510_SUBCAL_ADD_CALL_RESP:
	case P6510_SUBCAL_ADD_BIRTHDAY_RESP:
	case P6510_SUBCAL_ADD_REMINDER_RESP:
		dprintf("Succesfully written calendar note: %i!\n", (message[4] << 8) | message[5]);
		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (calendar handling): 0x%02x\n", P6510_MSG_CALENDAR, message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return e;
}


static gn_error P6510_GetCalendarNotesInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, P6510_SUBCAL_GET_INFO, 0xff, 0xfe};
	dprintf("Getting calendar notes info...\n");
	if (SM_SendMessage(state, 6, P6510_MSG_CALENDAR, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_CALENDAR);
}

static gn_error P6510_GetCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	gn_error	error = GN_ERR_NOTREADY;
	unsigned char	req[] = { FBUS_FRAME_HEADER, P6510_SUBCAL_GET_NOTE, 0x00, 0x00 };
	unsigned char	date[] = { FBUS_FRAME_HEADER, P6510_SUBCLO_GET_DATE };
	GSM_Data	tmpdata;
	GSM_DateTime	tmptime;

	dprintf("Getting calendar note...\n");
	tmpdata.DateTime = &tmptime;
	if (P6510_GetCalendarNotesInfo(data, state) == GN_ERR_NONE) {
		if (data->CalendarNote->Location < data->CalendarNotesList->Number + 1 &&
		    data->CalendarNote->Location > 0 ) {
			if (SM_SendMessage(state, 4, P6510_MSG_CLOCK, date) == GN_ERR_NONE) {
				SM_Block(state, &tmpdata, P6510_MSG_CLOCK);
				req[4] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] >> 8;
				req[5] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] & 0xff;
				data->CalendarNote->Time.Year = tmptime.Year;

				if (SM_SendMessage(state, 6, P6510_MSG_CALENDAR, req) == GN_ERR_NONE) {
					error = SM_Block(state, data, P6510_MSG_CALENDAR);
				}
			}
		}
	}

	return error;
}

long P6510_GetNoteAlarmDiff(GSM_DateTime *time, GSM_DateTime *alarm)
{
	time_t     t_alarm;
	time_t     t_time;
	struct tm  tm_alarm;
	struct tm  tm_time;

	tzset();

	tm_alarm.tm_year  = alarm->Year-1900;
	tm_alarm.tm_mon   = alarm->Month-1;
	tm_alarm.tm_mday  = alarm->Day;
	tm_alarm.tm_hour  = alarm->Hour;
	tm_alarm.tm_min   = alarm->Minute;
	tm_alarm.tm_sec   = alarm->Second;
	tm_alarm.tm_isdst = 0;
	t_alarm = mktime(&tm_alarm);

	tm_time.tm_year  = time->Year-1900;
	tm_time.tm_mon   = time->Month-1;
	tm_time.tm_mday  = time->Day;
	tm_time.tm_hour  = time->Hour;
	tm_time.tm_min   = time->Minute;
	tm_time.tm_sec   = time->Second;
	tm_time.tm_isdst = 0;
	t_time = mktime(&tm_time);

	dprintf("\tAlarm: %02i-%02i-%04i %02i:%02i:%02i\n",
		alarm->Day, alarm->Month, alarm->Year,
		alarm->Hour, alarm->Minute, alarm->Second);
	dprintf("\tDate: %02i-%02i-%04i %02i:%02i:%02i\n",
		time->Day, time->Month, time->Year,
		time->Hour, time->Minute, time->Second);
	dprintf("Difference in alarm time is %f\n", difftime(t_time, t_alarm));

	return difftime(t_time, t_alarm);
}

static gn_error P6510_FirstCalendarFreePos(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x31 };

	if (SM_SendMessage(state, 4, P6510_MSG_CALENDAR, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_CALENDAR);
}


static gn_error P6510_WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[350] = { FBUS_FRAME_HEADER,
				   0x01,       /* note type ... */
				   0x00, 0x00, /* location */
				   0x00,       /* entry type */
				   0x00,       /* fixed */
				   0x00, 0x00, 0x00, 0x00, /* Year(2bytes), Month, Day */
				   /* here starts block */
				   0x00, 0x00, 0x00, 0x00,0x00, 0x00}; /* ... depends on note type ... */

	GSM_CalendarNote *CalendarNote;
	int count = 0;
	long seconds, minutes;
	gn_error error;

	CalendarNote = data->CalendarNote;

	/* 6510 needs to seek the first free pos to inhabit with next note */
	error = P6510_FirstCalendarFreePos(data, state);
	if (error != GN_ERR_NONE) return error;

	/* Location */
	req[4] = CalendarNote->Location >> 8;
	req[5] = CalendarNote->Location & 0xff;

	switch (CalendarNote->Type) {
	case GCN_MEETING:
		req[6] = 0x01;
		req[3] = 0x01;
		break;
	case GCN_CALL:
		req[6] = 0x02;
		req[3] = 0x03;
		break;
	case GCN_BIRTHDAY:
		req[6] = 0x04;
		req[3] = 0x05;
		break;
	case GCN_REMINDER:
		req[6] = 0x08;
		req[3] = 0x07;
		break;
	}

	req[8]  = CalendarNote->Time.Year >> 8;
	req[9]  = CalendarNote->Time.Year & 0xff;
	req[10] = CalendarNote->Time.Month;
	req[11] = CalendarNote->Time.Day;

	/* From here starts BLOCK */
	count = 12;
	switch (CalendarNote->Type) {

	case GCN_MEETING:
		req[count++] = CalendarNote->Time.Hour;   /* Field 12 */
		req[count++] = CalendarNote->Time.Minute; /* Field 13 */
		/* Alarm .. */
		req[count++] = 0xff; /* Field 14 */
		req[count++] = 0xff; /* Field 15 */
		if (CalendarNote->Alarm.Year) {
			seconds = P6510_GetNoteAlarmDiff(&CalendarNote->Time,
							 &CalendarNote->Alarm);
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes >> 8;
				req[count++] = minutes & 0xff;
			}
		}
		/* Recurrence */
		if (CalendarNote->Recurrence >= 8760)
			CalendarNote->Recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = CalendarNote->Recurrence >> 8;   /* Field 16 */
		req[count++] = CalendarNote->Recurrence & 0xff; /* Field 17 */
		/* len of the text */
		req[count++] = strlen(CalendarNote->Text);    /* Field 18 */
		/* fixed 0x00 */
		req[count++] = 0x00; /* Field 19 */

		/* Text */
		dprintf("Count before encode = %d\n", count);
		dprintf("Meeting Text is = \"%s\"\n", CalendarNote->Text);

		char_encode_unicode(req + count, CalendarNote->Text, strlen(CalendarNote->Text)); /* Fields 20->N */
		count = count + 2 * strlen(CalendarNote->Text);
		break;

	case GCN_CALL:
		req[count++] = CalendarNote->Time.Hour;   /* Field 12 */
		req[count++] = CalendarNote->Time.Minute; /* Field 13 */
		/* Alarm .. */
		req[count++] = 0xff; /* Field 14 */
		req[count++] = 0xff; /* Field 15 */
		if (CalendarNote->Alarm.Year) {
			seconds = P6510_GetNoteAlarmDiff(&CalendarNote->Time,
							&CalendarNote->Alarm);
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes >> 8;
				req[count++] = minutes & 0xff;
			}
		}
		/* Recurrence */
		if (CalendarNote->Recurrence >= 8760)
			CalendarNote->Recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = CalendarNote->Recurrence >> 8;   /* Field 16 */
		req[count++] = CalendarNote->Recurrence & 0xff; /* Field 17 */
		/* len of text */
		req[count++] = strlen(CalendarNote->Text);    /* Field 18 */
		/* fixed 0x00 */
		req[count++] = strlen(CalendarNote->Phone);   /* Field 19 */
		/* Text */
		char_encode_unicode(req + count, CalendarNote->Text, strlen(CalendarNote->Text)); /* Fields 20->N */
		count += 2 * strlen(CalendarNote->Text);
		char_encode_unicode(req + count, CalendarNote->Phone, strlen(CalendarNote->Phone)); /* Fields (N+1)->n */
		count += 2 * strlen(CalendarNote->Phone);
		break;
	case GCN_BIRTHDAY:
		req[count++] = 0x00; /* Field 12 Fixed */
		req[count++] = 0x00; /* Field 13 Fixed */

		/* Alarm .. */
		req[count++] = 0x00;
		req[count++] = 0x00; /* Fields 14, 15 */
		req[count++] = 0xff; /* Field 16 */
		req[count++] = 0xff; /* Field 17 */
		if (CalendarNote->Alarm.Year) {
			/* First I try Time.Year = Alarm.Year. If negative, I increase year by one,
			   but only once! This is because I may have alarm period across
			   the year border, eg. birthday on 2001-01-10 and alarm on 2000-12-27 */
			CalendarNote->Time.Year = CalendarNote->Alarm.Year;
			if ((seconds= P6510_GetNoteAlarmDiff(&CalendarNote->Time,
							     &CalendarNote->Alarm)) < 0L) {
				CalendarNote->Time.Year++;
				seconds = P6510_GetNoteAlarmDiff(&CalendarNote->Time,
								 &CalendarNote->Alarm);
			}
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				count -= 4;
				req[count++] = seconds >> 24;              /* Field 14 */
				req[count++] = (seconds >> 16) & 0xff;     /* Field 15 */
				req[count++] = (seconds >> 8) & 0xff;      /* Field 16 */
				req[count++] = seconds & 0xff;             /* Field 17 */
			}
		}	

		req[count++] = 0x00; /* FIXME: CalendarNote->AlarmType; 0x00 tone, 0x01 silent 18 */

		/* len of text */
		req[count++] = strlen(CalendarNote->Text); /* Field 19 */

		/* Text */
		dprintf("Count before encode = %d\n", count);

		char_encode_unicode(req + count, CalendarNote->Text, strlen(CalendarNote->Text)); /* Fields 22->N */
		count = count + 2 * strlen(CalendarNote->Text);
		break;

	case GCN_REMINDER:
		/* Recurrence */
		if (CalendarNote->Recurrence >= 8760)
			CalendarNote->Recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = CalendarNote->Recurrence >> 8;   /* Field 12 */
		req[count++] = CalendarNote->Recurrence & 0xff; /* Field 13 */
		/* len of text */
		req[count++] = strlen(CalendarNote->Text);    /* Field 14 */
		/* fixed 0x00 */
		req[count++] = 0x00; /* Field 15 */
		/* Text */
		char_encode_unicode(req + count, CalendarNote->Text, strlen(CalendarNote->Text)); /* Fields 16->N */
		count += 2 * strlen(CalendarNote->Text);
		break;
	}

	/* padding */
	req[count] = 0x00;
	count++;
	dprintf("Count after padding = %d\n", count);

	SEND_MESSAGE_WAITFOR(P6510_MSG_CALENDAR, count);
}

static gn_error P6510_DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER,
				0x0b,      /* delete calendar note */
				0x00, 0x00}; /*location */

	GSM_CalendarNotesList list;

	data->CalendarNotesList = &list;
	if (P6510_GetCalendarNotesInfo(data, state) == GN_ERR_NONE) {
		if (data->CalendarNote->Location < data->CalendarNotesList->Number + 1 &&
		    data->CalendarNote->Location > 0) {
			req[4] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] << 8;
			req[5] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] & 0xff;
		} else {
			return GN_ERR_INVALIDLOCATION;
		}
	}

	SEND_MESSAGE_WAITFOR(P6510_MSG_CALENDAR, 6);
}

/********************/
/* INCOMING NETWORK */
/********************/

static gn_error P6510_IncomingNetwork(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char *blockstart, *operatorname;
	int i;

	switch (message[3]) {
	case 0x01:
	case 0x02:
		blockstart = message + 6;
		for (i = 0; i < message[5]; i++) {
			dprintf("Blockstart: %i\n", blockstart[0]);
			switch (blockstart[0]) {
			case 0x00:
				switch (blockstart[2]) {
				case 0x00:
					dprintf("Logged into home network.\n");
					break;
				case 0x01:
					dprintf("Logged into a roaming network.\n");
					break;
				case 0x04:
				case 0x09:
					dprintf("Not logged in any network!");
					break;
				default:
					dprintf("Unknown network status!\n");
					break;
				}
				operatorname = malloc(blockstart[5] + 1);
				char_decode_unicode(operatorname, blockstart + 6, blockstart[5] << 1);
				dprintf("Operator Name: %s\n", operatorname);
				break;
			case 0x09:  /* Operator details */
				/* Network code is stored as 0xBA 0xXC 0xED ("ABC DE"). */
				if (data->NetworkInfo) {
					data->NetworkInfo->CellID[0]=blockstart[6];
					data->NetworkInfo->CellID[1]=blockstart[7];
					data->NetworkInfo->LAC[0]=blockstart[2];
					data->NetworkInfo->LAC[1]=blockstart[3];
					data->NetworkInfo->NetworkCode[0] = '0' + (blockstart[8] & 0x0f);
					data->NetworkInfo->NetworkCode[1] = '0' + (blockstart[8] >> 4);
					data->NetworkInfo->NetworkCode[2] = '0' + (blockstart[9] & 0x0f);
					data->NetworkInfo->NetworkCode[3] = ' ';
					data->NetworkInfo->NetworkCode[4] = '0' + (blockstart[10] & 0x0f);
					data->NetworkInfo->NetworkCode[5] = '0' + (blockstart[10] >> 4);
					data->NetworkInfo->NetworkCode[6] = 0;
				}
				break;
			default:
				dprintf("Unknown operator block %d\n", blockstart[0]);
				break;
			}
			blockstart += blockstart[1];
		}
		break;
	case 0x0b:
		/*
		  no idea what this is about
		  00 01 00 0b 00 02 00 00 00
		*/
		break;
	case 0x0c: /* RF Level */
		if (data->RFLevel) {
			*(data->RFUnits) = GRF_Percentage;
			*(data->RFLevel) = message[8];
			dprintf("RF level %f\n",*(data->RFLevel));
		}
		break;
	case 0x1e: /* RF Level change notify */
		/*
		  01 56 00 1E 0D 65
		  01 56 00 1E 0C 65
		  01 56 00 1E 0B 65
		  01 56 00 1E 09 66
		  01 56 00 1E 08 66
		  01 56 00 1E 0A 66
		*/
		if (data->RFLevel) {
			*(data->RFUnits) = GRF_Percentage;
			*(data->RFLevel) = message[4];
			dprintf("RF level %f\n", *(data->RFLevel));
		}
		break;
	case 0x20:
		/*
		  no idea what this is about
		  01 56 00 20 02 00 55
		  01 56 00 20 01 00 55
		*/
		break;
	case 0x24:
		if (length == 18) return GN_ERR_EMPTYLOCATION;
		if (data->Bitmap) {
			data->Bitmap->netcode[0] = '0' + (message[12] & 0x0f);
			data->Bitmap->netcode[1] = '0' + (message[12] >> 4);
			data->Bitmap->netcode[2] = '0' + (message[13] & 0x0f);
			data->Bitmap->netcode[3] = ' ';
			data->Bitmap->netcode[4] = '0' + (message[14] & 0x0f);
			data->Bitmap->netcode[5] = '0' + (message[14] >> 4);
			data->Bitmap->netcode[6] = 0;
			dprintf("Operator %s \n",data->Bitmap->netcode);
			data->Bitmap->type = GN_BMP_NewOperatorLogo;
			data->Bitmap->height = message[21];
			data->Bitmap->width = message[20];
			data->Bitmap->size = message[25];
			dprintf("size: %i\n", data->Bitmap->size);
			memcpy(data->Bitmap->bitmap, message + 26, data->Bitmap->size);
			dprintf("Logo (%dx%d) \n", data->Bitmap->height, data->Bitmap->width);
		} else 
			return GN_ERR_INTERNALERROR;
		break;
	case 0x26:
		dprintf("Op Logo Set OK\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x0a (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x00};

	dprintf("Getting network info ...\n");
	if (SM_SendMessage(state, 5, P6510_MSG_NETSTATUS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_NETSTATUS);
}

static gn_error P6510_GetRFLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0B, 0x00, 0x02, 0x00, 0x00, 0x00};

	dprintf("Getting network info ...\n");
	if (SM_SendMessage(state, 9, P6510_MSG_NETSTATUS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_NETSTATUS);
}

static gn_error GetOperatorBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x23, 0x00, 0x00, 0x55, 0x55, 0x55};

	dprintf("Getting op logo...\n");
	if (SM_SendMessage(state, 9, P6510_MSG_NETSTATUS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_NETSTATUS);
}

static gn_error SetOperatorBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[300] = {FBUS_FRAME_HEADER, 0x25, 0x01, 0x55, 0x00, 0x00, 0x55,
				  0x02, /* Blocks */
				  0x0c, 0x08, /* type, length */
				  0x62, 0xf2, 0x20, 0x03, 0x55, 0x55,
				  0x1a};

	memset(req + 19, 0, 281);
	/*
	  00 01 00 25 01 55 00 00 55 02 
	  0C 08 62 F2 20 03 55 55 
	  1A F4 4E 15 00 EA 00 EA 5F 91 4E 80 5F 95 51 80 DF C2 DF 7D E0 4E 11 0E 80 55 20 E0 E0 F0 EA 7D E0 E0 7D E0 E
	*/
	if ((data->Bitmap->width != state->Phone.Info.OpLogoW) ||
	    (data->Bitmap->height != state->Phone.Info.OpLogoH)) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n", 
			state->Phone.Info.OpLogoH, state->Phone.Info.OpLogoW, data->Bitmap->height, data->Bitmap->width);
		return GN_ERR_INVALIDSIZE;
	}

	if (strcmp(data->Bitmap->netcode, "000 00")) {  /* set logo */
		req[12] = ((data->Bitmap->netcode[1] & 0x0f) << 4) | (data->Bitmap->netcode[0] & 0x0f);
		req[13] = 0xf0 | (data->Bitmap->netcode[2] & 0x0f);
		req[14] = ((data->Bitmap->netcode[5] & 0x0f) << 4) | (data->Bitmap->netcode[4] & 0x0f);

		req[19] = data->Bitmap->size + 8;
		req[20] = data->Bitmap->width;
		req[21] = data->Bitmap->height;
		req[23] = req[25] = data->Bitmap->size;
		req[19] = req[23] + 8;
		memcpy(req + 26, data->Bitmap->bitmap, data->Bitmap->size);
	}
	dprintf("Setting op logo...\n");

	if (SM_SendMessage(state, req[19] + req[11] + 10, P6510_MSG_NETSTATUS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_NETSTATUS);
}

/*********************/
/* INCOMING BATTERY */
/*********************/

static gn_error P6510_IncomingBattLevel(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	switch (message[3]) {
	case 0x0B:
		if (!data->BatteryLevel) return GN_ERR_INTERNALERROR;
		*(data->BatteryUnits) = GBU_Percentage;
		*(data->BatteryLevel) = message[9] * 100 / 7;
		dprintf("Battery level %f\n\n",*(data->BatteryLevel));
		break;
	default:
		dprintf("Unknown subtype of type 0x17 (%d)\n", message[3]);
		return GN_ERR_UNKNOWN;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0A, 0x02, 0x00};

	dprintf("Getting battery level...\n");
	if (SM_SendMessage(state, 6, P6510_MSG_BATTERY, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_BATTERY);
}


/*************/
/* RINGTONES */
/*************/

static gn_error P6510_IncomingRingtone(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int i, j, index;

	switch (message[3]) {
	case 0x08:
		dprintf("List of ringtones received!\n");
		index = 13;
		for (j = 0; j < message[7]; j++) {

			dprintf("Ringtone (#%03i) name: ", message[index - 4]);
			for (i = 0; i < message[index]; i++) {
				dprintf("%c", message[index + (2 * (i + 1))]);
			}
			dprintf("\n");
			index += message[index] * 2;
			while (message[index] != 0x01 || message[index + 1] != 0x01) index++;
			index += 3;
		}		
		break;
	default:
		dprintf("Unknown subtype of type 0x1f (%d)\n", message[3]);
		return GN_ERR_UNKNOWN;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_GetRingtoneList(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00, 0x00, 0xFE, 0x00, 0x7D};

	dprintf("Getting list of ringtones...\n");
	if (SM_SendMessage(state, 9, P6510_MSG_RINGTONE, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_RINGTONE);
}

static gn_error P6510_GetRingtone(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00, 0x00, 0xFE, 0x00, 0x7D};

	dprintf("Getting list of ringtones...\n");
	if (SM_SendMessage(state, 9, P6510_MSG_RINGTONE, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_RINGTONE);
}


/*************/
/* START UP  */
/*************/

static gn_error P6510_IncomingStartup(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	/*
UNHANDLED FRAME RECEIVED
request: 0x7a / 0x0005
00 01 00 02 0b                                  |                 
reply: 0x7a / 0x0036
01 44 00 03 0b 00 00 00 01 38 00 00 00 00 00 00 |  D       8      
35 5f 00 00 00 00 00 00 00 3e 00 00 00 00 00 00 | 5_       >      
36 97 00 00 00 00 01 55 55 55 55 55 55 55 55 55 | 6      UUUUUUUUU
55 55 55 55 55 55                               | UUUUUU          
	*/
	switch (message[3]) {
	case 0x03:
		switch (message[4]) {
		case 0x01:
			dprintf("Greeting text received\n");
			char_decode_unicode(data->Bitmap->text, message + 6, length - 7);
			return GN_ERR_NONE;
			break;
		case 0x05:
			if (message[6] == 0)
				dprintf("Anykey answer not set!\n");
			else
				dprintf("Anykey answer set!\n");
			return GN_ERR_NONE;
			break;
		case 0x0f:
			if (!data->Bitmap) return GN_ERR_INTERNALERROR;
				/* I'm sure there are blocks here but never mind! */
				/* yes there are:
				   c0 02 00 41   height
				   c0 03 00 60   width
				   c0 04 03 60   size
				*/
			data->Bitmap->type = GN_BMP_StartupLogo;
			data->Bitmap->height = message[13];
			data->Bitmap->width = message[17];
			data->Bitmap->size = (message[20] << 8) | message[21];
				
			memcpy(data->Bitmap->bitmap, message + 22, data->Bitmap->size);
			dprintf("Startup logo got ok - height(%d) width(%d)\n", data->Bitmap->height, data->Bitmap->width);
			return GN_ERR_NONE;
			break;
		default:
			dprintf("Unknown sub-subtype of type 0x7a subtype 0x03(%d)\n", message[4]);
			return GN_ERR_UNHANDLEDFRAME;
			break;
		}
	case 0x05:
		switch (message[4]) {
		case 0x0f:
			if (message[5] == 0)
				dprintf("Operator logo succesfully set!\n");
			else
				dprintf("Setting operator logo failed!\n");
			return GN_ERR_NONE;
			break;
		default:
			dprintf("Unknown sub-subtype of type 0x7a subtype 0x05 (%d)\n", message[4]);
			return GN_ERR_UNHANDLEDFRAME;
			break;
		}

	default:	
		dprintf("Unknown subtype of type 0x7a (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
}

static gn_error SetStartupBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[1000] = {FBUS_FRAME_HEADER, 0x04, 0x0f, 0x00, 0x00, 0x00, 0x04, 
				   0xc0, 0x02, 0x00, 0x00,           /* Height */
				   0xc0, 0x03, 0x00, 0x00,           /* Width */
				   0xc0, 0x04, 0x03, 0x60 };         /* size */
	int count = 21;


	if ((data->Bitmap->width != state->Phone.Info.StartupLogoW) ||
	    (data->Bitmap->height != state->Phone.Info.StartupLogoH)) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n", 
			state->Phone.Info.StartupLogoH, state->Phone.Info.StartupLogoW, 
			data->Bitmap->height, data->Bitmap->width);

	    return GN_ERR_INVALIDSIZE;
	}

	req[12] = data->Bitmap->height;
	req[16] = data->Bitmap->width;
	memcpy(req + count, data->Bitmap->bitmap, data->Bitmap->size);
	count += data->Bitmap->size;
	dprintf("Setting startup logo...\n");

	SEND_MESSAGE_BLOCK(P6510_MSG_STLOGO, count);
}

static gn_error GetWelcomeNoteText(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02, 0x01, 0x00};

	dprintf("Getting startup greeting...\n");
	SEND_MESSAGE_BLOCK(P6510_MSG_STLOGO, 6);
}

static gn_error P6510_GetAnykeyAnswer(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02, 0x05, 0x00, 0x7d};

	dprintf("See if anykey answer is set...\n");
	SEND_MESSAGE_BLOCK(P6510_MSG_STLOGO, 7);
}

static gn_error GetStartupBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02, 0x0f};
	
	dprintf("Getting startup logo...\n");
	SEND_MESSAGE_BLOCK(P6510_MSG_STLOGO, 5);
}


/***************/
/*   PROFILES **/
/***************/
static gn_error P6510_IncomingProfile(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char *blockstart;
	int i;

	switch (message[3]) {
	case 0x02:
		if (!data->Profile) return GN_ERR_INTERNALERROR;
		blockstart = message + 7;
		for (i = 0; i < 11; i++) {
			switch (blockstart[1]) {
			case 0x00:
				dprintf("type: %02x, keypad tone level: %i\n", blockstart[1], blockstart[7]);
				switch (blockstart[7]) {
				case 0x00:
					data->Profile->KeypadTone = 0xff;
					break;
				case 0x01:
					data->Profile->KeypadTone = 0x00;
					break;
				case 0x02:
					data->Profile->KeypadTone = 0x01;
					break;
				case 0x03:
					data->Profile->KeypadTone = 0x02;
					break;
				default:
					dprintf("Unknown keypad tone volume!\n");
					break;
				}
				break;
			case 0x02:
				dprintf("type: %02x, call alert: %i\n", blockstart[1], blockstart[7]);
				data->Profile->CallAlert = blockstart[7];
				break;
			case 0x03:
				dprintf("type: %02x, ringtone: %i\n", blockstart[1], blockstart[7]);
				data->Profile->Ringtone = blockstart[7];
				break;
			case 0x04:
				dprintf("type: %02x, ringtone volume: %i\n", blockstart[1], blockstart[7]);
				data->Profile->Volume = blockstart[7] + 6;
				break;
			case 0x05:
				dprintf("type: %02x, message tone: %i\n", blockstart[1], blockstart[7]);
				data->Profile->MessageTone = blockstart[7];
				break;
			case 0x06:
				dprintf("type: %02x, vibration: %i\n", blockstart[1], blockstart[7]);
				data->Profile->Vibration = blockstart[7];
				break;
			case 0x07:
				dprintf("type: %02x, warning tone: %i\n", blockstart[1], blockstart[7]);
				data->Profile->WarningTone = blockstart[7];
				break;
			case 0x08:
				dprintf("type: %02x, caller groups: %i\n", blockstart[1], blockstart[7]);
				data->Profile->CallerGroups = blockstart[7];
				break;
			case 0x0c:
				char_decode_unicode(data->Profile->Name, blockstart + 7, blockstart[6] << 1);
				dprintf("Profile Name: %s\n", data->Profile->Name);
				break;
			default:
				dprintf("Unknown profile subblock type %02x!\n", blockstart[1]);
				break;
			}
			blockstart = blockstart + blockstart[0];
		}
		return GN_ERR_NONE;
		break;
	case 0x04:
		dprintf("Response to profile writing received!\n");

		blockstart = message + 6;
		for (i = 0; i < message[5]; i++) {
			switch (blockstart[2]) {
			case 0x00:
				if (message[4] == 0x00) 
					dprintf("keypad tone level successfully set!\n");
				else
					dprintf("failed to set keypad tone level! error: %i\n", message[4]);
				break;
			case 0x02:
				if (message[4] == 0x00) 
					dprintf("call alert successfully set!\n");
				else
					dprintf("failed to set call alert! error: %i\n", message[4]);
				break;
			case 0x03:
				if (message[4] == 0x00) 
					dprintf("ringtone successfully set!\n");
				else
					dprintf("failed to set ringtone! error: %i\n", message[4]);
				break;
			case 0x04:
				if (message[4] == 0x00) 
					dprintf("ringtone volume successfully set!\n");
				else
					dprintf("failed to set ringtone volume! error: %i\n", message[4]);
				break;
			case 0x05:
				if (message[4] == 0x00) 
					dprintf("message tone successfully set!\n");
				else
					dprintf("failed to set message tone! error: %i\n", message[4]);
				break;
			case 0x06:
				if (message[4] == 0x00) 
					dprintf("vibration successfully set!\n");
				else
					dprintf("failed to set vibration! error: %i\n", message[4]);
				break;
			case 0x07:
				if (message[4] == 0x00) 
					dprintf("warning tone level successfully set!\n");
				else
					dprintf("failed to set warning tone level! error: %i\n", message[4]);
				break;
			case 0x08:
				if (message[4] == 0x00) 
					dprintf("caller groups successfully set!\n");
				else
					dprintf("failed to set caller groups! error: %i\n", message[4]);
				break;
			case 0x0c:
				if (message[4] == 0x00) 
					dprintf("name successfully set!\n");
				else
					dprintf("failed to set name! error: %i\n", message[4]);
				break;
			default:
				dprintf("Unknown profile subblock type %02x!\n", blockstart[1]);
				break;
			}
			blockstart = blockstart + blockstart[1];
		}
		return GN_ERR_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x39 (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
}

static gn_error P6510_GetProfile(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[150] = {FBUS_FRAME_HEADER, 0x01, 0x01, 0x0C, 0x01};
	int i, length = 7;

	for (i = 0; i < 0x0a; i++) {
		req[length] = 0x04;
		req[length + 1] = data->Profile->Number;
		req[length + 2] = i;
		req[length + 3] = 0x01;
		length += 4;
	}

	req[length] = 0x04;
	req[length + 1] = data->Profile->Number;
	req[length + 2] = 0x0c;
	req[length + 3] = 0x01;
	length += 4;

	req[length] = 0x04;

	dprintf("Getting profile #%i...\n", data->Profile->Number);
	P6510_GetRingtoneList(data, state);
	SEND_MESSAGE_BLOCK(P6510_MSG_PROFILE, length + 1);
}

static gn_error P6510_SetProfile(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[150] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x06, 0x03};
	int i, j, blocks = 0, length = 7;

	for (i = 0; i < 0x07; i++) {
		switch (i) {
		case 0x00:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->Profile->Number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			switch (data->Profile->KeypadTone) {
			case 0xff:
				req[length + 3] = req[length + 7] = 0x00;
				break;
			case 0x00:
				req[length + 3] = req[length + 7] = 0x01;
				break;
			case 0x01:
				req[length + 3] = req[length + 7] = 0x02;
				break;
			case 0x02:
				req[length + 3] = req[length + 7] = 0x03;
				break;
			default:
				dprintf("Unknown keypad tone volume!\n");
				break;
			}
			blocks++;
			length += 9;
			break;
		case 0x02:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->Profile->Number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->Profile->CallAlert;
			blocks++;
			length += 9;
			break;
		case 0x03:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->Profile->Number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->Profile->Ringtone;
			blocks++;
			length += 9;
			break;
		case 0x04:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->Profile->Number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->Profile->Volume - 6;
			blocks++;
			length += 9;
			break;
		case 0x05:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->Profile->Number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->Profile->MessageTone;
			blocks++;
			length += 9;
			break;
		case 0x06:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->Profile->Number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->Profile->Vibration;
			blocks++;
			length += 9;
			break;
		}
	}

	req[length + 1] = 0x0c;
	req[length + 2] = data->Profile->Number;
	memcpy(req + length + 3, "\xcc\xad\xff", 3);
	/* Name */
	j = strlen(data->Profile->Name);
	char_encode_unicode((req + length + 7), data->Profile->Name, j);
	/* Terminating 0 */
	req[j * 2 + 1] = 0;
	/* Length of the string + length field + terminating 0 */
	req[length + 6] = (j + 1) * 2;
	req[length] = (j + 1) * 2 + 8;
	length += (j + 1) * 2 + 8;
	blocks++;

	req[length] = 0x09;
	req[length + 1] = 0x07;
	req[length + 2] = data->Profile->Number;
	memcpy(req + length + 4, "\x00\x00\x01", 3);
	req[length + 3] = req[length + 7] = data->Profile->WarningTone;
	length += 8;
	blocks++;

	req[5] = blocks;
	dprintf("length: %i\n", length);

	if (SM_SendMessage(state, length + 1, P6510_MSG_PROFILE, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PROFILE);
}


/*************/
/*** RADIO  **/
/*************/

static gn_error P6510_IncomingRadio(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	/*
00 01 00 0D 00 00
00 0E 00 03 01 00 00 1C 00 14 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 01 00 00 08 00 00 01 00 01 00 00 0C 00 01 02 00 FF 30 30 30

00 01 00 05 00 00 00 2C 00 02 00 00 00 01 00 00
01 1F 00 06 00 00 00 2C 00 02 00 00 00 01 00 00 00 00 01

00 01 00 05 00 00 00 2C 00 01 00 00 00 2A 00 A8
01 1F 00 06 00 00 00 2C 00 01 00 00 00 2A 00 A8 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

00 01 00 05 00 00 00 2C 00 00 00 00 00 CA 03 28
01 1F 00 06 00 00 00 2C 00 00 00 00 00 CA 03 28 00 00 00 many more 00

00 01 00 05 00 00 00 2C 00 02 00 00 00 01 00 00

	 */
	return GN_ERR_NOTIMPLEMENTED;
}

/*****************/
/*** KEYPRESS  ***/
/*****************/

static gn_error P6510_IncomingKeypress(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	switch (message[4]) {
	default:
		dprintf("Unknown subtype of type 0x3c (%d)\n", message[4]);
		return GN_ERR_NONE;
		break;
	}
}

static gn_error P6510_PressOrReleaseKey(GSM_Data *data, GSM_Statemachine *state, bool press)
{


	unsigned char req[] = {FBUS_FRAME_HEADER, 0x11, 
			       0x00, 0x01, 
			       0x00, 0x00, 
			       0x00, 
			       0x01, 0x01, 0x43, 0x12, 0x53};
	/*
	  req[5] = data->KeyCode;

	  not functional yet

	*/
	req[5] = press ? 0x01 : 0x02;

	SEND_MESSAGE_BLOCK(P6510_MSG_KEYPRESS, 11);
}

/*****************/
/*** SECURITY  ***/
/*****************/

static gn_error P6510_IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	/*
	  01 4e 00 12 05 12 02 00 00 00
	  01 4e 00 09 06 00
	 */
	switch (message[3]) {
	case 0x08:
		dprintf("Security Code OK!\n");
		break;
	case 0x09:
		switch (message[4]) {
		case 0x06:
			dprintf("PIN wrong!\n");
			break;
		case 0x09:
			dprintf("PUK wrong!\n");
			break;
		default:
			dprintf(" unknown security Code wrong!\n");
			break;
		}
		break;
	case 0x12:
		dprintf("Security Code status received: ");
		if (!data->SecurityCode) return GN_ERR_INTERNALERROR;
		switch (message[4]) {
		case 0x01:
			dprintf("waiting for Security Code.\n");
			data->SecurityCode->Type = GSCT_SecurityCode;
			break;
		case 0x07:
		case 0x02:
			dprintf("waiting for PIN.\n");
			data->SecurityCode->Type = GSCT_Pin;
			break;
		case 0x03:
			dprintf("waiting for PUK.\n");
			data->SecurityCode->Type = GSCT_Puk;
			break;
		case 0x05:
			dprintf("PIN ok, SIM ok\n");
			data->SecurityCode->Type = GSCT_None;
			break;
		case 0x06:
			dprintf("No input status\n");
			data->SecurityCode->Type = GSCT_None;
			break;
		case 0x16:
			dprintf("No SIM!\n");
			data->SecurityCode->Type = GSCT_None;
			break;
		case 0x1A:
			dprintf("SIM rejected!\n");
			data->SecurityCode->Type = GSCT_None;
			break;
		default: 
			dprintf(_("Unknown!\n")); 
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x08 (%d)\n", message[3]);
		return GN_ERR_NONE;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x11, 0x00};

	if (!data->SecurityCode) return GN_ERR_INTERNALERROR;

	SEND_MESSAGE_BLOCK(P6510_MSG_SECURITY, 5);
}

static gn_error P6510_EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[35] = {FBUS_FRAME_HEADER, 0x07, 
				 0x02 }; /* 0x02 PIN, 0x03 PUK */
	int len ;
	
	/* Only PIN for the moment */

	if (!data->SecurityCode) return GN_ERR_INTERNALERROR;

	len = strlen(data->SecurityCode->Code);
	memcpy(req + 5, data->SecurityCode->Code, len);
	req[5 + len] = '\0';

	SEND_MESSAGE_BLOCK(P6510_MSG_SECURITY, 6 + len);
}

/*****************/
/*** SUBSCRIBE ***/
/*****************/

static gn_error P6510_IncomingSubscribe(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	switch (message[3]) {
	default:
		dprintf("Unknown subtype of type 0x3c (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_Subscribe(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[100] = {FBUS_FRAME_HEADER, 0x10,
			       0x34, /* number of groups */
			       0x01, 0x0a, 0x02, 0x14, 0x15};
	int i;

	dprintf("Subscribing to various channels!\n");
	for (i = 1; i < 35; i++) req[4 + i] = i;

	if (SM_SendMessage(state, 39, P6510_MSG_SUBSCRIBE, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_SUBSCRIBE);
}


/*****************/
/*** COMMSTATUS ***/
/*****************/

static gn_error P6510_IncomingCommStatus(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char *dummy;

	switch (message[3]) {
	case 0x02:
		dprintf("Call established, remote phone is ringing.\n");
		dprintf("Call ID: %i\n", message[4]);
		break;
	case 0x03:
		dprintf("Call complete.\n");
		dprintf("Call ID: %i\n", message[4]);
		dprintf("Call Mode: %i\n", message[5]);
		dummy = malloc(message[6] + 1);
		char_decode_unicode(dummy, message + 7, message[6] << 1);
		dprintf("Number: %s\n", dummy);
		break;		
	case 0x04:
		dprintf("Hangup!\n");
		dprintf("Call ID: %i\n", message[4]);
		dprintf("Cause Type: %i\n", message[5]);
		dprintf("Cause ID: %i\n", message[6]);
		break;
	case 0x05:
		dprintf("Incoming call:\n");
		dprintf("Call ID: %i\n", message[4]);
		dprintf("Call Mode: %i\n", message[5]);
		dummy = malloc(message[6] + 1);
		char_decode_unicode(dummy, message + 7, message[6] << 1);
		dprintf("From: %s\n", dummy);
		break;
	case 0x07:
		dprintf("Call answer initiated.\n");
		dprintf("Call ID: %i\n", message[4]);
		break;
	case 0x09:
		dprintf("Call released.\n");
		dprintf("Call ID: %i\n", message[4]);
		break;
	case 0x0a:
		dprintf("Call is being released.\n");
		dprintf("Call ID: %i\n", message[4]);
		break;
	case 0x0b:
		/* No idea what this is about! */
		break;
	case 0x0c:
		if (message[4] == 0x01)
			dprintf("Audio enabled\n");
		else
			dprintf("Audio disabled\n");
		break;
	case 0x53:
		dprintf("Outgoing call:\n");
		dprintf("Call ID: %i\n", message[4]);
		dprintf("Call Mode: %i\n", message[5]);
		dummy = malloc(message[6] + 1);
		char_decode_unicode(dummy, message + 7, message[6] << 1);
		dprintf("To: %s\n", dummy);
		break;
	default:
		dprintf("Unknown subtype of type 0x01 (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error P6510_MakeCall(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01,
				  0x0C }; /* number length in chars */

	int pos = 4, len;


	switch (data->CallInfo->Type) {
	case GSM_CT_VoiceCall:
		break;

	case GSM_CT_NonDigitalDataCall:
	case GSM_CT_DigitalDataCall:
		dprintf("Unsupported call type %d\n", data->CallInfo->Type);
		return GN_ERR_NOTSUPPORTED;

	default:
		dprintf("Invalid call type %d\n", data->CallInfo->Type);
		return GN_ERR_INTERNALERROR;
	}

	len = strlen(data->CallInfo->Number);
	if (len > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	req[pos++] = len;
	char_encode_unicode(req + pos, data->CallInfo->Number, len);
	pos += len << 1;
	req[pos++] = 0x01; /* one block */
	req[pos++] = 0x01; /* type */
	req[pos++] = 0x05; /* length */
	req[pos++] = 0x00; /* adress type */
	req[pos++] = 0x02; /* CLIR ON */
	req[pos++] = 0x00; /* CUG index */
	req[pos++] = 0x00; /* CUG index */
	req[pos++] = 0x01; /* CUG_OA true */

	if (SM_SendMessage(state, pos, P6510_MSG_COMMSTATUS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, P6510_MSG_COMMSTATUS);
}

/*****************/
/****** WAP ******/
/*****************/

static gn_error P6510_IncomingWAP(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int string_length, pos, pad = 0;

	switch (message[3]) {
	case 0x02:
	case 0x05:
	case 0x08:
	case 0x0b:
	case 0x0e:
	case 0x11:
	case 0x14:
	case 0x17:
	case 0x1a:
		switch (message[4]) {
		case 0x00:
			dprintf("WAP not activated?\n");
			return GN_ERR_UNKNOWN;
		case 0x01:
			dprintf("Security error. Inside WAP bookmarks menu\n");
			return GN_ERR_UNKNOWN;
		case 0x02:
			dprintf("Invalid or empty\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("ERROR: unknown %i\n",message[4]);
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	case 0x01:
	case 0x04:
	case 0x10:
		break;
	case 0x07:
		if (!data->WAPBookmark) return GN_ERR_INTERNALERROR;
		dprintf("WAP bookmark received\n");
		string_length = (message[6] << 8 | message[7]) << 1;

		char_decode_unicode(data->WAPBookmark->Name, message + 8, string_length);
		dprintf("Name: %s\n", data->WAPBookmark->Name);
		pos = string_length + 8;

		string_length = message[pos++] << 8;
		string_length |= message[pos++];
		char_decode_unicode(data->WAPBookmark->URL, message + pos, string_length << 1);
		dprintf("URL: %s\n", data->WAPBookmark->URL);
		break;
	case 0x0a:
		dprintf("WAP bookmark succesfully set!\n");
		break;
	case 0x0d:
		dprintf("WAP bookmark succesfully deleted!\n");
		break;
	case 0x13:
		dprintf("WAP setting succesfully activated!\n");
		break;
	case 0x16:
		if (!data->WAPSetting) return GN_ERR_INTERNALERROR;
		dprintf("WAP setting received\n");

		string_length = (message[4] << 8 | message[5]) << 1;
		char_decode_unicode(data->WAPSetting->Name, message + 6, string_length);
		dprintf("Name: %s\n", data->WAPSetting->Name);
		pos = string_length + 6;
		if (!(string_length % 2)) pad = 1;

		string_length = message[pos++] << 8; 
		string_length = (string_length | message[pos++]) << 1;
		char_decode_unicode(data->WAPSetting->Home, message + pos, string_length);
		dprintf("Home: %s\n", data->WAPSetting->Home);
		pos += string_length;

		if (!((string_length + pad) % 2)) 
			pad = 2; 
		else 
			pad = 0;
		
		data->WAPSetting->Session = message[pos++];
		if (message[pos++] == 0x01) 	
			data->WAPSetting->Security = true;
		else
			data->WAPSetting->Security = false;
		data->WAPSetting->Bearer = message[pos++];
		
		while ((message[pos] != 0x01) || (message[pos + 1] != 0x00)) pos++;
		pos += 4;

		data->WAPSetting->GSMdataAuthentication = message[pos++];
		data->WAPSetting->CallType = message[pos++];
		pos++;
		data->WAPSetting->CallSpeed = message[pos++];
		data->WAPSetting->GSMdataLogin = message[pos++];

		dprintf("GSM data:\n");

		string_length = message[pos++] << 1;
		char_decode_unicode(data->WAPSetting->GSMdataIP, message + pos, string_length);
		dprintf("   IP: %s\n", data->WAPSetting->GSMdataIP);
		pos += string_length;

  		string_length = message[pos++] << 8; 
		string_length = (string_length | message[pos++]) << 1;
		char_decode_unicode(data->WAPSetting->Number, message + pos, string_length);
		dprintf("   Number: %s\n", data->WAPSetting->Number);
		pos += string_length;

  		string_length = message[pos++] << 8; 
		string_length = (string_length | message[pos++]) << 1;
		char_decode_unicode(data->WAPSetting->GSMdataUsername, message + pos, string_length);
		dprintf("   Username: %s\n", data->WAPSetting->GSMdataUsername);
		pos += string_length;

  		string_length = message[pos++] << 8; 
		string_length = (string_length | message[pos++]) << 1;
		char_decode_unicode(data->WAPSetting->GSMdataPassword, message + pos, string_length);
		dprintf("   Password: %s\n", data->WAPSetting->GSMdataPassword);
		pos += string_length;

		while (message[pos] != 0x03) pos++;
		pos += 4;

		dprintf("GPRS:\n");

		data->WAPSetting->GPRSAuthentication = message[pos++];
		data->WAPSetting->GPRSConnection = message[pos++];
		data->WAPSetting->GPRSLogin = message[pos++];

		string_length = message[pos++] << 1;
		char_decode_unicode(data->WAPSetting->AccessPoint, message + pos, string_length);
		dprintf("   Access point: %s\n", data->WAPSetting->AccessPoint);
		pos += string_length;

  		string_length = message[pos++] << 8; 
		string_length = (string_length | message[pos++]) << 1;
		char_decode_unicode(data->WAPSetting->GPRSIP, message + pos, string_length);
		dprintf("   IP: %s\n", data->WAPSetting->GPRSIP);
		pos += string_length;

  		string_length = message[pos++] << 8; 
		string_length = (string_length | message[pos++]) << 1;
		char_decode_unicode(data->WAPSetting->GPRSUsername, message + pos, string_length);
		dprintf("   Username: %s\n", data->WAPSetting->GPRSUsername);
		pos += string_length;

		if (message[pos] != 0x80) {
			string_length = message[pos++] << 8; 
			string_length |= message[pos++];
			char_decode_unicode(data->WAPSetting->GPRSPassword, message + pos, string_length);
			dprintf("   Password: %s\n", data->WAPSetting->GPRSPassword);
			pos += string_length;
		} else
			pos += 2;

		break;
	case 0x19:
		dprintf("WAP setting succesfully written!\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x3f (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error SendWAPFrame(GSM_Data *data, GSM_Statemachine *state, int frame)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x00 };

	dprintf("Sending WAP frame\n");
	req[3] = frame;
	SEND_MESSAGE_BLOCK(P6510_MSG_WAP, 4);
}

static gn_error PrepareWAP(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Preparing WAP\n");
	return SendWAPFrame(data, state, 0x00);
}

static gn_error FinishWAP(GSM_Data *data, GSM_Statemachine *state)
{
	gn_error error;

	dprintf("Finishing WAP\n");

	error = SendWAPFrame(data, state, 0x03);
	if (error != GN_ERR_NONE) return error;

	error = SendWAPFrame(data, state, 0x00);
	if (error != GN_ERR_NONE) return error;

	error = SendWAPFrame(data, state, 0x0f);
	if (error != GN_ERR_NONE) return error;

	return SendWAPFrame(data, state, 0x03);
}

static int PackWAPString(unsigned char *dest, unsigned char *string, int length_size)
{
	int length;
	
	length = strlen(string);
	if (length_size == 2) {
		dest[0] = length / 256;
		dest[1] = length % 256;
	} else {
		dest[0] = length % 256;
	}

	char_encode_unicode(dest + length_size, string, length);
	return ((length << 1) + length_size);
}

static gn_error P6510_WriteWAPSetting(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[1000] = { FBUS_FRAME_HEADER, 0x18,
				    0x00 }; 		/* Location */
	gn_error error;
	int pad = 0, length, pos = 5;

	dprintf("Writing WAP setting\n");
	memset(req + pos, 0, 1000 - pos);
	req[4] = data->WAPSetting->Location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		SendWAPFrame(data, state, 0x03);
		if ((error = PrepareWAP(data, state)) != GN_ERR_NONE) return error;
	}

	/* Name */
	length = strlen(data->WAPSetting->Name);
	if (!(length % 2)) pad = 1;
	pos += PackWAPString(req + pos, data->WAPSetting->Name, 1);

	/* Home */
	length = strlen(data->WAPSetting->Home);
	if (((length + pad) % 2)) 
		pad = 2; 
	else 
		pad = 0;
	pos += PackWAPString(req + pos, data->WAPSetting->Home, 2);

	req[pos++] = data->WAPSetting->Session;
	pos++;
	if (data->WAPSetting->Security)	req[pos] = 0x01;
	req[pos++] = data->WAPSetting->Bearer;

	/* How many parts do we send? */
	req[pos++] = 0x02;
	pos += pad;

	/* GSM data */
	memcpy(req + pos, "\x01\x00", 2);
	pos += 2;
	length = ((strlen(data->WAPSetting->GSMdataIP) + strlen(data->WAPSetting->GSMdataUsername)
		   + strlen(data->WAPSetting->GSMdataPassword) + strlen(data->WAPSetting->Number)) << 1) + 18;
	req[pos++] = length / 256;
	req[pos++] = length % 256;

	req[pos++] = data->WAPSetting->GSMdataAuthentication;
	req[pos++] = data->WAPSetting->CallType;
	pos++;
	req[pos++] = data->WAPSetting->CallSpeed;
	req[pos++] = data->WAPSetting->GSMdataLogin;
	
	/* IP */
	pos += PackWAPString(req + pos, data->WAPSetting->GSMdataIP, 1);
	/* Number */
	pos += PackWAPString(req + pos, data->WAPSetting->Number, 2);
	/* Username */
	pos += PackWAPString(req + pos, data->WAPSetting->GSMdataUsername, 2);
	/* Password */
	pos += PackWAPString(req + pos, data->WAPSetting->GSMdataPassword, 2);

	/* Padding */
	if (length % 2) pos++;

	/* GPRS block */
	memcpy(req + pos, "\x03\x00", 2);
	pos += 2;
	length = ((strlen(data->WAPSetting->GPRSIP) + strlen(data->WAPSetting->GPRSUsername)
		   + strlen(data->WAPSetting->GPRSPassword) + strlen(data->WAPSetting->AccessPoint)) << 1) + 14;
	req[pos++] = length / 256;
	req[pos++] = length % 256;

	req[pos++] = 0x00; // data->WAPSetting->GPRSAuthentication;
	req[pos++] = data->WAPSetting->GPRSConnection;
	req[pos++] = data->WAPSetting->GPRSLogin;

	/* Access point */
	pos += PackWAPString(req + pos, data->WAPSetting->AccessPoint, 1);
	/* IP */
	pos += PackWAPString(req + pos, data->WAPSetting->GPRSIP, 2);
	/* Username */
	pos += PackWAPString(req + pos, data->WAPSetting->GPRSUsername, 2);
	/* Password */
	pos += PackWAPString(req + pos, data->WAPSetting->GPRSPassword, 2);

	/* end of block ? */
	memcpy(req + pos, "\x80\x00", 2);
	pos += 2;

	/* no idea what this is about, seems to be some kind of 'end of message' */
	memcpy(req + pos, "\x00\x0c\x07\x6B\x0C\x1E\x00\x00\x00\x55", 10);
	pos += 10;

	if (SM_SendMessage(state, pos, P6510_MSG_WAP, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = SM_Block(state, data, P6510_MSG_WAP);
	if (error != GN_ERR_NONE) return error;

	return FinishWAP(data, state);
}

static gn_error P6510_DeleteWAPBookmark(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x0C,
				0x00, 0x00};		/* Location */
	gn_error error;

	dprintf("Deleting WAP bookmark\n");
	req[5] = data->WAPBookmark->Location + 1;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state)) != GN_ERR_NONE) return error;
	}

	if (SM_SendMessage(state, 6, P6510_MSG_WAP, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = SM_Block(state, data, P6510_MSG_WAP);
	if (error != GN_ERR_NONE) return error;

	return FinishWAP(data, state);
}

static gn_error P6510_GetWAPBookmark(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x06,
				0x00, 0x00};		/* Location */
	gn_error error;

	dprintf("Getting WAP bookmark\n");
	req[5] = data->WAPBookmark->Location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state)) != GN_ERR_NONE) return error;
	}

	if (SM_SendMessage(state, 6, P6510_MSG_WAP, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = SM_Block(state, data, P6510_MSG_WAP);
	if (error != GN_ERR_NONE) return error;

	return FinishWAP(data, state);
}

static gn_error P6510_WriteWAPBookmark(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[350] = { FBUS_FRAME_HEADER, 0x09,
				0xFF, 0xFF};		/* Location */
	gn_error error;
	int pos = 6;

	dprintf("Writing WAP bookmark\n");

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state)) != GN_ERR_NONE) return error;
	}

	pos += PackWAPString(req + pos, data->WAPBookmark->Name, 2);
	pos += PackWAPString(req + pos, data->WAPBookmark->URL, 2);

	if (SM_SendMessage(state, pos, P6510_MSG_WAP, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = SM_Block(state, data, P6510_MSG_WAP);
	if (error != GN_ERR_NONE) return error;

	return FinishWAP(data, state);
}

static gn_error P6510_GetWAPSetting(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x15,
				0x00 };		/* Location */
	gn_error error;

	dprintf("Getting WAP setting\n");
	req[4] = data->WAPSetting->Location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state)) != GN_ERR_NONE) return error;
	}


	if (SM_SendMessage(state, 5, P6510_MSG_WAP, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = SM_Block(state, data, P6510_MSG_WAP);
	if (error != GN_ERR_NONE) return error;

	return FinishWAP(data, state);
}

static gn_error P6510_ActivateWAPSetting(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x12,
				0x00 };		/* Location */
	gn_error error;

	dprintf("Activating WAP setting\n");
	req[4] = data->WAPSetting->Location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state)) != GN_ERR_NONE) return error;
	}

	if (SM_SendMessage(state, 5, P6510_MSG_WAP, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = SM_Block(state, data, P6510_MSG_WAP);
	if (error != GN_ERR_NONE) return error;

	return FinishWAP(data, state);
}

/********************/
/***** ToDo *********/
/********************/

static gn_error P6510_IncomingToDo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int i;

	switch (message[3]) {
	case 0x02:
		dprintf("ToDo set!\n");
		break;
	case 0x04:
		dprintf("ToDo received!\n");
		if (!data->ToDo) return GN_ERR_INTERNALERROR;
		if (message[5] == 0x08) return GN_ERR_INVALIDLOCATION;
		if ((message[4] > 0) && (message[4] < 4)) {
			data->ToDo->Priority = message[4];
		}
		dprintf("Priority: %i\n", message[4]);
		char_decode_unicode(data->ToDo->Text, message + 14, length - 16);
		dprintf("Text: \"%s\"\n", data->ToDo->Text);
		break;
	case 0x10:
		dprintf("Next free ToDo location received!\n");
		if (!data->ToDo) return GN_ERR_INTERNALERROR;
		data->ToDo->Location = (message[8] << 8) | message[9];
		dprintf("   location: %i\n", data->ToDo->Location);
		break;
	case 0x12:
		dprintf("All ToDo locations deleted!\n");
		break;
	case 0x16:
		dprintf("ToDo locations received!\n");
		if (!data->ToDo) return GN_ERR_INTERNALERROR;
		data->ToDoList->Number = (message[6] << 8) | message[7];
		dprintf("Number of Entries: %i\n", data->ToDoList->Number);

		dprintf("Locations: ");
		for (i = 0; i < data->ToDoList->Number; i++) {
			data->ToDoList->Location[i] = (message[12 + (i * 4)] << 8) | message[(i * 4) + 13];
			dprintf("%i ", data->ToDoList->Location[i]);
		}
		dprintf("\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x01 (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}


static gn_error P6510_GetToDoLocations(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x15, 
				0x01, 0x00, 0x00,
				0x00, 0x00, 0x00};

	SEND_MESSAGE_BLOCK(P6510_MSG_TODO, 10);
}

static gn_error P6510_DeleteAllToDoLocations(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x11 };

	SEND_MESSAGE_BLOCK(P6510_MSG_TODO, 4);
}

static gn_error GetNextFreeToDoLocation(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x0f };

	SEND_MESSAGE_BLOCK(P6510_MSG_TODO, 4);
}

static gn_error P6510_GetToDo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER,0x03, 
			       0x00, 0x00, 0x80, 0x00,
			       0x00, 0x01};		/* Location */
	gn_error error;


	error = P6510_GetToDoLocations(data, state);
	if (error != GN_ERR_NONE) return error;

	req[8] = data->ToDoList->Location[data->ToDo->Location - 1] / 256;
	req[9] = data->ToDoList->Location[data->ToDo->Location - 1] % 256;

	dprintf("Getting ToDo\n");
	SEND_MESSAGE_BLOCK(P6510_MSG_TODO, 10);
}

static gn_error P6510_WriteToDo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[300] = {FBUS_FRAME_HEADER, 0x01, 
				  0x02, /* prority */
				  0x0D, 0x80, 0x00, 0x00, 0x01};	/* Location */
	unsigned char text[257];
	gn_error error;

	error = GetNextFreeToDoLocation(data, state);
	if (error != GN_ERR_NONE) return error;

	req[4] = data->ToDo->Priority;
	req[8] = data->ToDo->Location / 256;
	req[9] = data->ToDo->Location % 256;

	char_encode_unicode(text, data->ToDo->Text, strlen(data->ToDo->Text));
	memcpy(req + 10, text, strlen(data->ToDo->Text) * 2);

	dprintf("Setting ToDo\n");
	SEND_MESSAGE_BLOCK(P6510_MSG_TODO, 10 + strlen(data->ToDo->Text) * 2);
}

/********************************/
/* NOT FRAME SPECIFIC FUNCTIONS */
/********************************/

static gn_error P6510_GetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	switch(data->Bitmap->type) {
	case GN_BMP_WelcomeNoteText:
		return GetWelcomeNoteText(data, state);
	case GN_BMP_CallerLogo:
		return GetCallerBitmap(data, state);
	case GN_BMP_NewOperatorLogo:
	case GN_BMP_OperatorLogo:
		return GetOperatorBitmap(data, state);
	case GN_BMP_StartupLogo:
		return GetStartupBitmap(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static gn_error P6510_SetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	switch(data->Bitmap->type) {
	case GN_BMP_StartupLogo:
		return SetStartupBitmap(data, state);
	case GN_BMP_CallerLogo:
		return SetCallerBitmap(data, state);
	case GN_BMP_OperatorLogo:
	case GN_BMP_NewOperatorLogo:
		return SetOperatorBitmap(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
}


static int GetMemoryType(GSM_MemoryType memory_type)
{
	int result;

	switch (memory_type) {
	case GMT_MT:
		result = P6510_MEMORY_MT;
		break;
	case GMT_ME:
		result = P6510_MEMORY_ME;
		break;
	case GMT_SM:
		result = P6510_MEMORY_SM;
		break;
	case GMT_FD:
		result = P6510_MEMORY_FD;
		break;
	case GMT_ON:
		result = P6510_MEMORY_ON;
		break;
	case GMT_EN:
		result = P6510_MEMORY_EN;
		break;
	case GMT_DC:
		result = P6510_MEMORY_DC;
		break;
	case GMT_RC:
		result = P6510_MEMORY_RC;
		break;
	case GMT_MC:
		result = P6510_MEMORY_MC;
		break;
	case GMT_IN:
		result = P6510_MEMORY_IN;
		break;
	case GMT_OU:
		result = P6510_MEMORY_OU;
		break;
	case GMT_AR:
		result = P6510_MEMORY_AR;
		break;
	case GMT_TE:
		result = P6510_MEMORY_TE;
		break;
	case GMT_F1:
		result = P6510_MEMORY_F1;
		break;
	case GMT_F2:
		result = P6510_MEMORY_F2;
		break;
	case GMT_F3:
		result = P6510_MEMORY_F3;
		break;
	case GMT_F4:
		result = P6510_MEMORY_F4;
		break;
	case GMT_F5:
		result = P6510_MEMORY_F5;
		break;
	case GMT_F6:
		result = P6510_MEMORY_F6;
		break;
	case GMT_F7:
		result = P6510_MEMORY_F7;
		break;
	case GMT_F8:
		result = P6510_MEMORY_F8;
		break;
	case GMT_F9:
		result = P6510_MEMORY_F9;
		break;
	case GMT_F10:
		result = P6510_MEMORY_F10;
		break;
	case GMT_F11:
		result = P6510_MEMORY_F11;
		break;
	case GMT_F12:
		result = P6510_MEMORY_F12;
		break;
	case GMT_F13:
		result = P6510_MEMORY_F13;
		break;
	case GMT_F14:
		result = P6510_MEMORY_F14;
		break;
	case GMT_F15:
		result = P6510_MEMORY_F15;
		break;
	case GMT_F16:
		result = P6510_MEMORY_F16;
		break;
	case GMT_F17:
		result = P6510_MEMORY_F17;
		break;
	case GMT_F18:
		result = P6510_MEMORY_F18;
		break;
	case GMT_F19:
		result = P6510_MEMORY_F19;
		break;
	case GMT_F20:
		result = P6510_MEMORY_F20;
		break;
	default:
		result = P6510_MEMORY_XX;
		break;
	}
	return (result);
}
/*
01 33 00 17 05 01 05 08 04 00 01 00 00 00
01 33 00 17 05 01 05 08 04 00 01 00 00 00

0x0b / 0x000e
01 33 00 17 05 01 05 08 05 00 01 00 00 00
0x0b / 0x000e
01 33 00 17 05 01 05 08 04 00 01 00 00 00
0x0b / 0x000e
01 33 00 17 05 01 05 08 04 00 01 00 00 00
0x0b / 0x000e
01 33 00 17 05 01 05 08 05 00 01 00 00 00

0x0e / 0x000e
01 42 00 68 55 01 01 08 00 32 01 55 55 55
0x0e / 0x000e
01 42 00 68 55 01 01 08 00 32 01 55 55 55
*/
static int EncodeSMS(GSM_Data *data, GSM_Statemachine *state, unsigned char *req)
{
	int pos = 0, udh_length_pos, len;


	req[pos++] = 0x01; /* one big block */
	if (data->RawSMS->Type == SMS_Deliver)
		req[pos++] = 0x00; /* message type: deliver */
	else
		req[pos++] = 0x02; /* message type: submit */
	
	req[pos++] = 0x00; /* will be set at the end */
	/*        is supposed to be the length of the whole message 
		  starting from req[10], which is the message type */

	if (data->RawSMS->Type == SMS_Deliver) {
		req[pos++] = 0x04; /* SMS Deliver */
	} else {
		req[pos] = 0x01; /* SMS Submit */

		if (data->RawSMS->ReplyViaSameSMSC)  req[pos] |= 0x80;
		if (data->RawSMS->RejectDuplicates)  req[pos] |= 0x04;
		if (data->RawSMS->Report)            req[pos] |= 0x20;
		if (data->RawSMS->UDHIndicator)      req[pos] |= 0x40;
		if (data->RawSMS->ValidityIndicator) req[pos] |= 0x10;
		pos++;
		req[pos++] = data->RawSMS->Reference;
		req[pos++] = data->RawSMS->PID;
	}

	req[pos++] = data->RawSMS->DCS;
	req[pos++] = 0x00;
	
	/* FIXME: real date/time */
	if (data->RawSMS->Type == SMS_Deliver) {
		memcpy(req + pos, data->RawSMS->SMSCTime, 7);
		pos += 7;
		memcpy(req + pos, "\x55\x55\x55", 3);
		pos += 3;
		/* Magic. Nokia new ideas: coding SMS in the sequent blocks */
		req[pos++] = 0x03; /* total blocks */
	} else {
		req[pos++] = 0x04; /* total blocks */
	}

	/* FIXME. Do it in the loop */

	/* Block 1. Remote Number */
	if (data->RawSMS->Type == SMS_Submit && data->RawSMS->Status != SMS_Sent) {
		memcpy(req + pos, "\x82\x10\x01\x0C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
		pos += 16;
	} else {
		len = data->RawSMS->RemoteNumber[0] + 4;
		if (len % 2) len++;
		len /= 2;
		req[pos] = 0x82; /* type: number */
		req[pos + 1] = 0x0c; /* offset to next block starting from start of block (req[18]) */
		req[pos + 2] = 0x01; /* first number field => RemoteNumber */
		req[pos + 3] = len; /* actual data length in this block */
		memcpy(req + pos + 4, data->RawSMS->RemoteNumber, len);
		pos += 12;
	}

	/* Block 2. SMSC Number */
	if (data->RawSMS->Type == SMS_Submit && data->RawSMS->Status != SMS_Sent) {
		memcpy(req + pos, "\x82\x10\x02\x0C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
		pos += 16;
	} else {
		len = data->RawSMS->MessageCenter[0] + 1;
		memcpy(req + pos, "\x82\x0c\x02", 3); /* as above 0x02 => MessageCenterNumber */
		req[pos + 3] = len;
		memcpy(req + pos + 4, data->RawSMS->MessageCenter, len);
		pos += 12;
	}

	/* Block 3. User Data */
	req[pos++] = 0x80; /* type: User Data */
	req[pos++] = data->RawSMS->UserDataLength + 4; /* same as req[11] but starting from req[42] */

	req[pos++] = data->RawSMS->UserDataLength;
	req[pos++] = data->RawSMS->Length;

	memcpy(req + pos, data->RawSMS->UserData, data->RawSMS->UserDataLength);
	pos += data->RawSMS->UserDataLength;

	/* padding */
	udh_length_pos = pos - data->RawSMS->UserDataLength - 3;
	if (req[udh_length_pos] % 8 != 0) {
		memcpy(req + pos, "\x55\x55\x55\x55\x55\x55\x55\x55", 8 - req[udh_length_pos] % 8);
		pos += 8 - req[udh_length_pos] % 8;
		req[udh_length_pos] += 8 - req[udh_length_pos] % 8;
	}

	
	if (data->RawSMS->Type == SMS_Submit) {
		/* Block 4. Validity Period */
		req[pos++] = 0x08; /* type: validity */
		req[pos++] = 0x04; /* block length */
		req[pos++] = 0x01; /* data length */
		req[pos++] = data->RawSMS->Validity[0];
	}
	req[2] = pos - 1;
	return pos;
}
