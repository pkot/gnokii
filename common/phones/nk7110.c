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
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2001 Markus Plail, Pawe³ Kot

  This file provides functions specific to the 7110 series.
  See README for more details on supported mobile phones.

  The various routines are called P7110_(whatever).

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk7110.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

/* Functions prototypes */
static GSM_Error P7110_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_Initialise(GSM_Statemachine *state);
static GSM_Error P7110_GetModel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_SetBitmap(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetBitmap(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_WritePhonebookLocation(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetClock(char req_type, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSnoValidate(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_PollSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_SendSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_DeleteSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetPicture(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSFolders(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSFolderStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_NetMonitor(GSM_Data *data, GSM_Statemachine *state);

static GSM_Error P7110_IncomingIdentify(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingNetwork(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingBattLevel(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingStartup(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingFolder(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingClock(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error P7110_IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error P7110_IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data);

static int GetMemoryType(GSM_MemoryType memory_type);

/* Some globals */

static bool SMSLoop = false; /* Are we in infinite SMS reading loop? */
static bool NewSMS  = false; /* Do we have a new SMS? */

static const SMSMessage_Layout nk7110_deliver = {
	true,						/* Is the SMS type supported */
	 5, true, true,					/* SMSC */
	-1, 17, -1, -1,  3, -1, -1, -1, 20, 19, 17,
	-1, -1, -1,					/* Validity */
	21, true, true,					/* Remote Number */
	33, -1,						/* Time */
	 1,  0,						/* Nonstandard fields */
	40, true					/* User Data */
};

static const SMSMessage_Layout nk7110_submit = {
	true,
	 5, true, true,
	-1, 17, 17, 17, -1, 18, 19, -1, 21, 20, 17,
	17, -1, -1,
	22, true, true,
	-1, -1,
	-1, -1,
	41, true
};

static const SMSMessage_Layout nk7110_delivery_report = {
	true,
	 5, true, true,
	-1, -1, -1, -1,  3, -1, -1, -1, 19, 18, 17,
	-1, -1, -1,
	20, true, true,
	32, 39,
	 1,  0,
	19, true
};

static const SMSMessage_Layout nk7110_picture = {
	true,
	 5, true, true,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1,
	18, true, true,
	30, -1,
	-1, -1,
	43, true
};

static SMSMessage_PhoneLayout nk7110_layout;

static GSM_IncomingFunctionType P7110_IncomingFunctions[] = {
	{ P7110_MSG_FOLDER,	P7110_IncomingFolder },
	{ P7110_MSG_SMS,	P7110_IncomingSMS },
	{ P7110_MSG_PHONEBOOK,	P7110_IncomingPhonebook },
	{ P7110_MSG_NETSTATUS,	P7110_IncomingNetwork },
	{ P7110_MSG_CALENDAR,	P7110_IncomingCalendar },
	{ P7110_MSG_BATTERY,	P7110_IncomingBattLevel },
	{ P7110_MSG_CLOCK,	P7110_IncomingClock },
	{ P7110_MSG_IDENTITY,	P7110_IncomingIdentify },
	{ P7110_MSG_STLOGO,	P7110_IncomingStartup },
	{ P7110_MSG_DIVERT,	PNOK_IncomingCallDivert },
	{ P7110_MSG_SECURITY,	P7110_IncomingSecurity },
	{ 0, NULL }
};

GSM_Phone phone_nokia_7110 = {
	P7110_IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"7110|6210|6250",      /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Percentage,        /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Percentage,        /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		60, 96,                /* Startup logo size - 7110 is fixed at init */
		21, 78,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	P7110_Functions
};

/* FIXME - a little macro would help here... */

static GSM_Error P7110_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return P7110_Initialise(state);
	case GOP_Terminate:
		return PGEN_Terminate(data, state);
	case GOP_GetModel:
		return P7110_GetModel(data, state);
	case GOP_GetRevision:
		return P7110_GetRevision(data, state);
	case GOP_GetImei:
		return P7110_GetIMEI(data, state);
	case GOP_Identify:
		return P7110_Identify(data, state);
	case GOP_GetBatteryLevel:
		return P7110_GetBatteryLevel(data, state);
	case GOP_GetRFLevel:
		return P7110_GetRFLevel(data, state);
	case GOP_GetMemoryStatus:
		return P7110_GetMemoryStatus(data, state);
	case GOP_GetBitmap:
		return P7110_GetBitmap(data, state);
	case GOP_SetBitmap:
		return P7110_SetBitmap(data, state);
	case GOP_ReadPhonebook:
		return P7110_ReadPhonebook(data, state);
	case GOP_WritePhonebook:
		return P7110_WritePhonebookLocation(data, state);
	case GOP_GetNetworkInfo:
		return P7110_GetNetworkInfo(data, state);
	case GOP_GetSpeedDial:
		return P7110_GetSpeedDial(data, state);
	case GOP_GetSMSCenter:
		return P7110_GetSMSCenter(data, state);
	case GOP_GetDateTime:
		return P7110_GetClock(P7110_SUBCLO_GET_DATE, data, state);
	case GOP_GetAlarm:
		return P7110_GetClock(P7110_SUBCLO_GET_ALARM, data, state);
	case GOP_GetCalendarNote:
		return P7110_GetCalendarNote(data, state);
	case GOP_WriteCalendarNote:
		return P7110_WriteCalendarNote(data, state);
	case GOP_DeleteCalendarNote:
		return P7110_DeleteCalendarNote(data, state);
	case GOP_GetSMS:
		dprintf("Getting SMS (validating)...\n");
		return P7110_GetSMS(data, state);
	case GOP_GetSMSnoValidate:
		dprintf("Getting SMS (without validating)...\n");
		data->SMSFolder = NULL;
		return P7110_GetSMSnoValidate(data, state);
	case GOP_OnSMS:
		/* Register notify when running for the first time */
		if (data->OnSMS) {
			NewSMS = true;
			return GE_NONE; /* FIXME P7110_GetIncomingSMS(data, state); */
		}
		break;
	case GOP_PollSMS:
		if (NewSMS) return GE_NONE; /* FIXME P7110_GetIncomingSMS(data, state); */
		break;
	case GOP_SendSMS:
		return P7110_SendSMS(data, state);
	case GOP_DeleteSMS:
		return P7110_DeleteSMS(data, state);
	case GOP_GetSMSStatus:
		return P7110_GetSMSStatus(data, state);
	case GOP_CallDivert:
		return PNOK_CallDivert(data, state);
	case GOP_NetMonitor:
		return P7110_NetMonitor(data, state);
	case GOP_GetSMSFolders:
		return P7110_GetSMSFolders(data, state);
	case GOP_GetSMSFolderStatus:
		return P7110_GetSMSFolderStatus(data, state);
	case GOP7110_GetPicture:
		return P7110_GetPicture(data, state);
	default:
		return GE_NOTIMPLEMENTED;
	}
	return GE_NONE;
}

/* Initialise is the only function allowed to 'use' state */
static GSM_Error P7110_Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];
	GSM_Error err;
	int try = 0, connected = 0;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_7110, sizeof(GSM_Phone));

	/* SMS Layout */
	nk7110_layout.Type = 8; /* Locate the Type of the mesage field. */
	nk7110_layout.SendHeader = 6;
	nk7110_layout.ReadHeader = 4;
	nk7110_layout.Deliver = nk7110_deliver;
	nk7110_layout.Submit = nk7110_submit;
	nk7110_layout.DeliveryReport = nk7110_delivery_report;
	nk7110_layout.Picture = nk7110_picture;
	layout = nk7110_layout;

	dprintf("Connecting\n");
	while (!connected) {
		switch (state->Link.ConnectionType) {
		case GCT_DAU9P:
			if (try == 0) try = 1;
		case GCT_Serial:
			if (try > 1) return GE_NOTSUPPORTED;
			err = FBUS_Initialise(&(state->Link), state, 1 - try);
			break;
		case GCT_Infrared:
		case GCT_Irda:
			if (try > 0) return GE_NOTSUPPORTED;
			err = PHONET_Initialise(&(state->Link), state);
			break;
		default:
			return GE_NOTSUPPORTED;
			break;
		}

		if (err != GE_NONE) {
			dprintf("Error in link initialisation: %d\n", err);
			try++;
			continue;
		}

		SM_Initialise(state);

		/* Now test the link and get the model */
		GSM_DataClear(&data);
		data.Model = model;
		if (state->Phone.Functions(GOP_GetModel, &data, state) != GE_NONE) try++;
		else connected = 1;
	}
	/* Check for 7110 and alter the startup logo size */
	if (strcmp(model, "NSE-5") == 0) {
		state->Phone.Info.StartupLogoH = 65;
		dprintf("7110 detected - startup logo height set to 65\n");
	}
	return GE_NONE;
}

static GSM_Error P7110_GetModel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};

	dprintf("Getting model...\n");
	if (SM_SendMessage(state, 6, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static GSM_Error P7110_GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};

	dprintf("Getting revision...\n");
	if (SM_SendMessage(state, 6, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static GSM_Error P7110_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};

	dprintf("Getting imei...\n");
	if (SM_SendMessage(state, 4, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static GSM_Error P7110_GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02};

	dprintf("Getting battery level...\n");
	if (SM_SendMessage(state, 4, 0x17, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x17);
}

static GSM_Error P7110_IncomingBattLevel(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x03:
		if (data->BatteryLevel) {
			*(data->BatteryUnits) = GBU_Percentage;
			*(data->BatteryLevel) = message[5];
			dprintf("Battery level %f\n",*(data->BatteryLevel));
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x17 (%d)\n", message[3]);
		return GE_UNKNOWN;
	}
	return GE_NONE;
}

static GSM_Error P7110_GetRFLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x81};

	dprintf("Getting rf level...\n");
	if (SM_SendMessage(state, 4, 0x0a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static GSM_Error P7110_GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x70};

	dprintf("Getting Network Info...\n");
	if (SM_SendMessage(state, 4, 0x0a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static GSM_Error P7110_IncomingNetwork(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	unsigned char *blockstart;
	int i;

	switch (message[3]) {
	case 0x71:
		blockstart = message + 6;
		for (i = 0; i < message[4]; i++) {
			switch (blockstart[0]) {
			case 0x01:  /* Operator details */
				/* Network code is stored as 0xBA 0xXC 0xED ("ABC DE"). */
				if (data->NetworkInfo) {
					/* Is this correct? */
					data->NetworkInfo->CellID[0]=blockstart[4];
					data->NetworkInfo->CellID[1]=blockstart[5];
					data->NetworkInfo->LAC[0]=blockstart[6];
					data->NetworkInfo->LAC[1]=blockstart[7];
					data->NetworkInfo->NetworkCode[0] = '0' + (blockstart[8] & 0x0f);
					data->NetworkInfo->NetworkCode[1] = '0' + (blockstart[8] >> 4);
					data->NetworkInfo->NetworkCode[2] = '0' + (blockstart[9] & 0x0f);
					data->NetworkInfo->NetworkCode[3] = ' ';
					data->NetworkInfo->NetworkCode[4] = '0' + (blockstart[10] & 0x0f);
					data->NetworkInfo->NetworkCode[5] = '0' + (blockstart[10] >> 4);
					data->NetworkInfo->NetworkCode[6] = 0;
				}
				if (data->Bitmap) {
					data->Bitmap->netcode[0] = '0' + (blockstart[8] & 0x0f);
					data->Bitmap->netcode[1] = '0' + (blockstart[8] >> 4);
					data->Bitmap->netcode[2] = '0' + (blockstart[9] & 0x0f);
					data->Bitmap->netcode[3] = ' ';
					data->Bitmap->netcode[4] = '0' + (blockstart[10] & 0x0f);
					data->Bitmap->netcode[5] = '0' + (blockstart[10] >> 4);
					data->Bitmap->netcode[6] = 0;
					dprintf("Operator %s ",data->Bitmap->netcode);
				}
				break;
			case 0x04: /* Logo */
				if (data->Bitmap) {
					dprintf("Op logo received ok ");
					data->Bitmap->type = GSM_OperatorLogo;
					data->Bitmap->size = blockstart[5]; /* Probably + [4]<<8 */
					data->Bitmap->height = blockstart[3];
					data->Bitmap->width = blockstart[2];
					memcpy(data->Bitmap->bitmap, blockstart + 8, data->Bitmap->size);
					dprintf("Logo (%dx%d) ", data->Bitmap->height, data->Bitmap->width);
				}
				break;
			default:
				dprintf("Unknown operator block %d\n", blockstart[0]);
				break;
			}
			blockstart += blockstart[1];
		}
		break;
	case 0x82:
		if (data->RFLevel) {
			*(data->RFUnits) = GRF_Percentage;
			*(data->RFLevel) = message[4];
			dprintf("RF level %f\n",*(data->RFLevel));
		}
		break;
	case 0xa4:
		dprintf("Op Logo Set OK\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x0a (%d)\n", message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}

static GSM_Error P7110_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x00, 0x00};

	dprintf("Getting memory status...\n");
	req[5] = GetMemoryType(data->MemoryStatus->MemoryType);
	if (SM_SendMessage(state, 6, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error P7110_IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	unsigned char *blockstart;
	unsigned char blocks;
	unsigned char subblockcount;
	char *str;
	int i;
	GSM_SubPhonebookEntry* subEntry = NULL;

	switch (message[3]) {
	case 0x04:  /* Get status response */
		if (data->MemoryStatus) {
			if (message[5] != 0xff) {
				data->MemoryStatus->Used = (message[16] << 8) + message[17];
				data->MemoryStatus->Free = ((message[14] << 8) + message[15]) - data->MemoryStatus->Used;
				dprintf("Memory status - location = %d\n", (message[8] << 8) + message[9]);
			} else {
				dprintf("Unknown error getting mem status\n");
				return GE_NOTIMPLEMENTED;
			}
		}
		break;
	case 0x08:  /* Read Memory response */
		if (data->PhonebookEntry) {
			data->PhonebookEntry->Empty = true;
			data->PhonebookEntry->Group = 0;
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
		if (message[6] == 0x0f) { /* not found */
			switch (message[10]) {
			case 0x30:
				return GE_INVALIDMEMORYTYPE;
			case 0x33:
				return GE_EMPTYMEMORYLOCATION;
			case 0x34:
				return GE_INVALIDPHBOOKLOCATION;
			default:
				return GE_NOTIMPLEMENTED;
			}
		}
		dprintf("Received phonebook info\n");
		blocks     = message[17];
		blockstart = message + 18;
		subblockcount = 0;

		for (i = 0; i < blocks; i++) {
			if (data->PhonebookEntry)
				subEntry = &data->PhonebookEntry->SubEntries[subblockcount];
			switch(blockstart[0]) {
			case P7110_ENTRYTYPE_POINTER:	/* Pointer */
				switch (message[11]) {	/* Memory type */
				case P7110_MEMORY_SPEEDDIALS:	/* Speed dial numbers */
					if ((data != NULL) && (data->SpeedDial != NULL)) {
						data->SpeedDial->Location = (((unsigned int)blockstart[6]) << 8) + blockstart[7];
						switch(blockstart[8]) {
						case 0x05:
							data->SpeedDial->MemoryType = GMT_ME;
							str = "phone";
							break;
						case 0x06:
							str = "SIM";
							data->SpeedDial->MemoryType = GMT_SM;
							break;
						default:
							str = "unknown";
							data->SpeedDial->MemoryType = GMT_XX;
							break;
						}
					}

					dprintf("Speed dial pointer: %i in %s\n", data->SpeedDial->Location, str);

					break;
				default:
					/* FIXME: is it possible? */
					dprintf("Wrong memory type(?)\n");
					break;
				}

				break;
			case P7110_ENTRYTYPE_NAME:	/* Name */
				if (data->Bitmap) {
					DecodeUnicode(data->Bitmap->text, (blockstart + 6), blockstart[5] / 2);
					dprintf("Name: %s\n", data->Bitmap->text);
				} else if (data->PhonebookEntry) {
					DecodeUnicode(data->PhonebookEntry->Name, (blockstart + 6), blockstart[5] / 2);
					data->PhonebookEntry->Empty = false;
					dprintf("   Name: %s\n", data->PhonebookEntry->Name);
				}
				break;
			case P7110_ENTRYTYPE_EMAIL:
			case P7110_ENTRYTYPE_POSTAL:
			case P7110_ENTRYTYPE_NOTE:
				if (data->PhonebookEntry) {
					subEntry->EntryType   = blockstart[0];
					subEntry->NumberType  = 0;
					subEntry->BlockNumber = blockstart[4];
					DecodeUnicode(subEntry->data.Number, (blockstart + 6), blockstart[5] / 2);
					dprintf("   Type: %d (%02x)\n", subEntry->EntryType, subEntry->EntryType);
					dprintf("   Text: %s\n", subEntry->data.Number);
					subblockcount++;
					data->PhonebookEntry->SubEntriesCount++;
				}
				break;
			case P7110_ENTRYTYPE_NUMBER:
				if (data->PhonebookEntry) {
					subEntry->EntryType   = blockstart[0];
					subEntry->NumberType  = blockstart[5];
					subEntry->BlockNumber = blockstart[4];
					DecodeUnicode(subEntry->data.Number, (blockstart + 10), blockstart[9] / 2);
					if (!subblockcount) strcpy(data->PhonebookEntry->Number, subEntry->data.Number);
					dprintf("   Type: %d (%02x)\n", subEntry->NumberType, subEntry->NumberType);
					dprintf(" Number: %s\n", subEntry->data.Number);
					subblockcount++;
					data->PhonebookEntry->SubEntriesCount++;
				}
				break;
			case P7110_ENTRYTYPE_RINGTONE:	/* Ringtone */
				if (data->Bitmap) {
					data->Bitmap->ringtone = blockstart[5];
					dprintf("Ringtone no. %d\n", data->Bitmap->ringtone);
				}
				break;
			case P7110_ENTRYTYPE_DATE:
				if (data->PhonebookEntry) {
					subEntry->EntryType=blockstart[0];
					subEntry->NumberType=blockstart[5];
					subEntry->BlockNumber=blockstart[4];
					subEntry->data.Date.Year=(blockstart[6] << 8) + blockstart[7];
					subEntry->data.Date.Month  = blockstart[8];
					subEntry->data.Date.Day    = blockstart[9];
					subEntry->data.Date.Hour   = blockstart[10];
					subEntry->data.Date.Minute = blockstart[11];
					subEntry->data.Date.Second = blockstart[12];
					dprintf("   Date: %02u.%02u.%04u\n", subEntry->data.Date.Day,
						subEntry->data.Date.Month, subEntry->data.Date.Year);
					dprintf("   Time: %02u:%02u:%02u\n", subEntry->data.Date.Hour,
						subEntry->data.Date.Minute, subEntry->data.Date.Second);
					subblockcount++;
				}
				break;
			case P7110_ENTRYTYPE_LOGO:	/* Caller group logo */
				if (data->Bitmap) {
					data->Bitmap->width = blockstart[5];
					data->Bitmap->height = blockstart[6];
					data->Bitmap->size = (data->Bitmap->width * data->Bitmap->height) / 8;
					memcpy(data->Bitmap->bitmap, blockstart + 10, data->Bitmap->size);
					dprintf("Bitmap: width: %i, height: %i\n", blockstart[5], blockstart[6]);
				}
				break;
			case P7110_ENTRYTYPE_LOGOSWITCH:/* Logo on/off */
				break;
			case P7110_ENTRYTYPE_GROUP:	/* Caller group number */
				if (data->PhonebookEntry) {
					data->PhonebookEntry->Group = blockstart[5] - 1;
					dprintf("   Group: %d\n", data->PhonebookEntry->Group);
				}
				break;
			default:
				dprintf("Unknown phonebook block %02x\n", blockstart[0]);
				break;
			}
			blockstart += blockstart[3];
		}
		break;
	case 0x0c:
		if (message[6] == 0x0f) {
			switch (message[10]) {
			case 0x3d: return GE_PHBOOKWRITEFAILED;
			case 0x3e: return GE_PHBOOKWRITEFAILED;
			default:   return GE_UNHANDLEDFRAME;
			}
		}
		break;
	case 0x10:
		dprintf("Entry succesfully deleted!\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x03 (%d)\n", message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}

/* Just as an example.... */
/* But note that both requests are the same type which isn't very 'proper' */
static GSM_Error P7110_Identify(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};

	dprintf("Identifying...\n");
	PNOK_GetManufacturer(data->Manufacturer);
	if (SM_SendMessage(state, 4, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	if (SM_SendMessage(state, 6, 0x1b, req2) != GE_NONE) return GE_NOTREADY;
	SM_WaitFor(state, data, 0x1b);
	SM_Block(state, data, 0x1b); /* waits for all requests - returns req2 error */
	SM_GetError(state, 0x1b);

	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GE_UNKNOWN;
	return GE_NONE;
}

static GSM_Error P7110_IncomingIdentify(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x02:
		if (data->Imei) {
			int n;
			unsigned char *s = strchr(message + 4, '\n');

			if (s) n = s - message - 3;
			else n = GSM_MAX_IMEI_LENGTH;
			snprintf(data->Imei, GNOKII_MIN(n, GSM_MAX_IMEI_LENGTH), "%s", message + 4);
			dprintf("Received imei %s\n", data->Imei);
		}
		break;
	case 0x04:
		if (data->Model) {
			int n;
			unsigned char *s = strchr(message + 22, '\n');

			n = s ? s - message - 21 : GSM_MAX_MODEL_LENGTH;
			snprintf(data->Model, GNOKII_MIN(n, GSM_MAX_MODEL_LENGTH), "%s", message + 22);
			dprintf("Received model %s\n",data->Model);
		}
		if (data->Revision) {
			int n;
			unsigned char *s = strchr(message + 7, '\n');

			n = s ? s - message - 6 : GSM_MAX_REVISION_LENGTH;
			snprintf(data->Revision, GNOKII_MIN(n, GSM_MAX_REVISION_LENGTH), "%s", message + 7);
			dprintf("Received revision %s\n",data->Revision);
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x1b (%d)\n", message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}


/* handle messages of type 0x14 (SMS Handling, Folders, Logos.. */
static GSM_Error P7110_IncomingFolder(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int i, j;
	int nextfolder = 0x10;
	/* bool found; */

	switch (message[3]) {
	/* getsms */
	case 0x08:
		dprintf("Trying to get message # %i in folder # %i\n", message[7], message[5]);
		if (!data->SMSMessage) return GE_INTERNALERROR;

		memset(data->SMSMessage, 0, sizeof(GSM_SMSMessage));

		/* Number of SMS in folder */
		data->SMSMessage->Number = (message[6] << 8) | message[7];

		/* MemoryType/FolderID */
		data->SMSMessage->MemoryType = message[5];

		/* Short Message status */
		data->SMSMessage->Status = message[4];

		/* See if message# is given back by phone. If not and status is unread */
		/* we want it, if status is not unread it's a "random" message given back */
		/* by the phone because we want a message of which the # doesn't exist
		if (data->SMSFolder) {
			found = false;
			for (i = 0; i < data->SMSFolder->number; i++) {
				if (data->SMSMessage->Number == data->SMSFolder->locations[i])
					found = true;
			}
			if (!found && data->SMSMessage->Status != SMS_Unread) {
				if (data->SMSMessage->Number > MAX_SMS_MESSAGES)
					return GE_INVALIDSMSLOCATION;
				else
					return GE_EMPTYSMSLOCATION;
			}
		}
		*/
		if (!data->RawData) return GE_INTERNALERROR;
		memset(data->RawData, 0, sizeof(GSM_RawData));

		/* Skip the frame header */
		data->RawData->Length = length - nk7110_layout.ReadHeader;
		data->RawData->Data = calloc(data->RawData->Length, 1);
		memcpy(data->RawData->Data, message + nk7110_layout.ReadHeader, data->RawData->Length);

		break;

	/* error? the error codes are taken from 6100 sources */
	case 0x09:
		dprintf("SMS reading failed:\n");
		switch (message[4]) {
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

	/* delete sms */
	case 0x0b:
		dprintf("SMS deleted\n");
		break;

	/* delete sms failed */
	case 0x0c:
		switch (message[4]) {
		case 0x02:
			dprintf("Invalid location\n");
			return GE_INVALIDSMSLOCATION;
		default:
			dprintf("Unknown reason.\n");
			return GE_UNHANDLEDFRAME;
		}

	/* sms status */
	case 0x37:
		dprintf("SMS Status received\n");
		/* FIXME: Don't count messages in fixed locations together with other */
		data->SMSStatus->Number = ((message[10] << 8) | message[11]) +
					  ((message[14] << 8) | message[15]) +
					  (data->SMSFolder->number);
		data->SMSStatus->Unread = ((message[12] << 8) | message[13]) +
					  ((message[16] << 8) | message[17]);
		break;

	/* getfolderstatus */
	case 0x6C:
		dprintf("Message: SMS Folder status received\n" );
		if (!data->SMSFolder) return GE_INTERNALERROR;
		i = data->SMSFolder->FolderID;
		memset(data->SMSFolder, 0, sizeof(SMS_Folder));

		data->SMSFolder->FolderID = i;
		data->SMSFolder->number = (message[4] << 8) | message[5];
		
/*		if (data->SMSStatus) data->SMSStatus->Number = data->SMSFolder->number; */
		dprintf("Message: Number of Entries: %i\n" , data->SMSFolder->number);
		dprintf("Message: IDs of Entries : ");
		for (i = 0; i < data->SMSFolder->number; i++) {
			data->SMSFolder->locations[i] = (message[(i * 2) + 6] << 8) | message[(i * 2) + 7];
			dprintf("%d, ", data->SMSFolder->locations[i]);
		}
		dprintf("\n");
		break;

	/* getfolders */
	case 0x7B:
		if (!data->SMSFolderList) return GE_INTERNALERROR;
		i = 5;
		memset(data->SMSFolderList, 0, sizeof(SMS_FolderList));
		dprintf("Message: %d SMS Folders received:\n", message[4]);

		strcpy(data->SMSFolderList->Folder[1].Name, "               ");
		data->SMSFolderList->number = message[4];

		for (j = 0; j < message[4]; j++) {
			int len;
			strcpy(data->SMSFolderList->Folder[j].Name, "               ");
			data->SMSFolderList->FolderID[j] = message[i];
			dprintf("Folder Index: %d", data->SMSFolderList->FolderID[j]);
			i += 2;
			dprintf("   Folder name: ");
			len = 0;
			/* search for next folder's index number, i.e. length of folder name */
			while (message[i+1] != nextfolder && i < length) {
				i += 2;
				len++;
			}
			/* see nk7110.txt */
			nextfolder += 0x08;
			if (nextfolder == 0x28) nextfolder++;
			i -= 2 * len + 1;
			DecodeUnicode(data->SMSFolderList->Folder[j].Name, message + i, len);
			dprintf("%s\n", data->SMSFolderList->Folder[j].Name);
			i += 2 * len + 2;
		}
		break;

	/* get list of SMS pictures */
	case 0x97:
		dprintf("Getting list of SMS pictures...\n");
		break;

	/* Some errors */
	case 0xc9:
		dprintf("Unknown command???\n");
		return GE_UNHANDLEDFRAME;

	case 0xca:
		dprintf("Phone not ready???\n");
		return GE_UNHANDLEDFRAME;

	default:
		dprintf("Message: Unknown message of type 14 : %02x  length: %d\n", message[3], length);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}

static GSM_Error P7110_GetSMSnoValidate(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07,
				0x08, /* FolderID */
				0x00,
				0x01, /* Location */
				0x01, 0x65, 0x01};

	data->SMSFolder = NULL;
	req[4] = GetMemoryType(data->SMSMessage->MemoryType);
	req[5] = (data->SMSMessage->Number & 0xff00) >> 8;
	req[6] = data->SMSMessage->Number & 0x00ff;
	if (SM_SendMessage(state, 10, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error P7110_GetSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error;

	/* Handle MemoryType = 0 explicitely, because SMSFolder->FolderID = 0 by default */
	if (data->SMSMessage->MemoryType == 0) return GE_INVALIDMEMORYTYPE;

	/* see if the message we want is from the last read folder, i.e. */
	/* we don't have to get folder status again */
	if ((!data->SMSFolder) || 
	    ((data->SMSFolder) && (data->SMSMessage->MemoryType != data->SMSFolder->FolderID))) {
		if ((error = P7110_GetSMSFolders(data, state)) != GE_NONE) return error;
		if ( (GetMemoryType(data->SMSMessage->MemoryType) > 
		      data->SMSFolderList->FolderID[data->SMSFolderList->number-1]) ||
		     (data->SMSMessage->MemoryType < 12))
			return GE_INVALIDMEMORYTYPE;
		data->SMSFolder->FolderID = data->SMSMessage->MemoryType;
		if ((error = P7110_GetSMSFolderStatus(data, state)) != GE_NONE) return error;
	}

	if (data->SMSFolder->number + 2 < data->SMSMessage->Number) {
		if (data->SMSMessage->Number > MAX_SMS_MESSAGES)
			return GE_INVALIDSMSLOCATION;
		else
			return GE_EMPTYSMSLOCATION;
	} else {
		data->SMSMessage->Number = data->SMSFolder->locations[data->SMSMessage->Number - 1];
	}

	return P7110_GetSMSnoValidate(data, state);
}


static GSM_Error P7110_DeleteSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0a, 0x00, 0x00, 0x00, 0x01};

	if (!data->SMSMessage) return GE_INTERNALERROR;
	dprintf("Removing SMS %d\n", data->SMSMessage->Number);
	req[4] = GetMemoryType(data->SMSMessage->MemoryType);
	req[5] = (data->SMSMessage->Number & 0xff00) >> 8;
	req[6] = data->SMSMessage->Number & 0x00ff;
	if (SM_SendMessage(state, 8, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error P7110_ListPictures(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x96,
				0x09, /* Location */
				0x0f, 0x07};

	req[4] = GetMemoryType(data->SMSMessage->MemoryType);
	if (SM_SendMessage(state, 7, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}
static GSM_Error P7110_GetPicture(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x96,
				0x09, /* location */
				0x0f, 0x07};
	if (SM_SendMessage(state, 7, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error P7110_GetSMSFolders(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x7A, 0x00, 0x00};

	dprintf("Getting SMS Folders...\n");
	if (SM_SendMessage(state, 6, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error P7110_GetSMSStatus(GSM_Data *data, GSM_Statemachine *state)
{
	SMS_Folder fld;
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x36, 0x64};

	dprintf("Getting SMS Status...\n");

	/* Nokia 6210 and family does not show not "fixed" messages from the
	 * Templates folder, ie. when you save a message to the Templates folder,
	 * SMSStatus does not change! Workaround: get Templates folder status, which
	 * does show these messages.
	 */
	fld.FolderID = GMT_TE;
	data->SMSFolder = &fld;
	if (P7110_GetSMSFolderStatus(data, state) != GE_NONE) return GE_NOTREADY;
	if (SM_SendMessage(state, 5, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error P7110_PollSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};
	dprintf("Requesting for the notify of the incoming SMS\n");
	if (SM_SendMessage(state, 8, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x02);
}

static GSM_Error P7110_GetSMSFolderStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6B,
				0x08, // Folder ID
				0x0F, 0x01};

	req[4] = GetMemoryType(data->SMSFolder->FolderID);
	dprintf("Getting SMS Folder Status...\n");
	if (SM_SendMessage(state, 7, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error P7110_SendSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x01, 0x02, 0x00};
	int length;

	/* 6 is the frame header as above */
	length = data->RawData->Length + 1;
	if (length < 0) return GE_SMSWRONGFORMAT;
	memcpy(req + 6, data->RawData->Data + 5, data->RawData->Length);
	dprintf("Sending SMS...(%d)\n", length);
	if (SM_SendMessage(state, length, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_BlockNoRetryTimeout(state, data, 0x02, 100);
}

/* handle messages of type 0x02 (SMS Handling) */
static GSM_Error P7110_IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_Error	e = GE_NONE;
	int		digits, bytes;

	if (!data) return GE_INTERNALERROR;

	switch (message[3]) {
	case P7110_SUBSMS_SMSC_RCVD: /* 0x34 */
		dprintf("SMSC Received\n");
		/* FIXME: Implement all these in gsm-sms.c */
		data->MessageCenter->No = message[4];
		data->MessageCenter->Format = message[6];
		data->MessageCenter->Validity = message[8];  /* due to changes in format */
		digits = message[9];
		bytes = message[21] - 1;

		sprintf(data->MessageCenter->Name, "%s", message + 33);
		data->MessageCenter->DefaultName = -1;	/* FIXME */

		snprintf(data->MessageCenter->Recipient, sizeof(data->MessageCenter->Recipient), "%s", GetBCDNumber(message+9));
		snprintf(data->MessageCenter->Number, sizeof(data->MessageCenter->Number), "%s", GetBCDNumber(message+21));
		data->MessageCenter->Type = message[22];

		if (strlen(data->MessageCenter->Recipient) == 0) {
			sprintf(data->MessageCenter->Recipient, "(none)");
		}
		if (strlen(data->MessageCenter->Number) == 0) {
			sprintf(data->MessageCenter->Number, "(none)");
		}
		if(strlen(data->MessageCenter->Name) == 0) {
			sprintf(data->MessageCenter->Name, "(none)");
		}

		break;

	case P7110_SUBSMS_SMS_SENT: /* 0x02 */
		dprintf("SMS sent\n");
		e = GE_NONE;
		break;

	case P7110_SUBSMS_SEND_FAIL: /* 0x03 */
		dprintf("SMS sending failed\n");
		e = GE_SMSSENDFAILED;
		break;

	case 0x0e:
		dprintf("Ack for request on Incoming SMS\n");
		break;

	case 0x11:
		dprintf("SMS received\n");
		/* We got here the whole SMS */
		NewSMS = true;
		data->RawData->Length = length - nk7110_layout.ReadHeader;
		data->RawData->Data = calloc(data->RawData->Length, 1);
		ParseSMS(data, nk7110_layout.ReadHeader);
		free(data->RawData->Data);
		break;

	case P7110_SUBSMS_SMS_RCVD: /* 0x10 */
	case P7110_SUBSMS_CELLBRD_OK: /* 0x21 */
	case P7110_SUBSMS_CELLBRD_FAIL: /* 0x22 */
	case P7110_SUBSMS_READ_CELLBRD: /* 0x23 */
	case P7110_SUBSMS_SMSC_OK: /* 0x31 */
	case P7110_SUBSMS_SMSC_FAIL: /* 0x32 */
	case P7110_SUBSMS_SMSC_RCVFAIL: /* 0x35 */
		dprintf("Subtype 0x%02x of type 0x%02x (SMS handling) not implemented\n", message[3], P7110_MSG_SMS);
		return GE_NOTIMPLEMENTED;

	default:
		dprintf("Unknown subtype of type 0x%02x (SMS handling): 0x%02x\n", P7110_MSG_SMS, message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return e;
}

static GSM_Error P7110_IncomingClock(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_Error	e = GE_NONE;

	if (!data || !data->DateTime) return GE_INTERNALERROR;
	switch (message[3]) {
	case P7110_SUBCLO_DATE_RCVD:
		data->DateTime->Year = (((unsigned int)message[8]) << 8) + message[9];
		data->DateTime->Month = message[10];
		data->DateTime->Day = message[11];
		data->DateTime->Hour = message[12];
		data->DateTime->Minute = message[13];
		data->DateTime->Second = message[14];

		break;
	case P7110_SUBCLO_ALARM_RCVD:
		switch(message[8]) {
		case P7110_ALARM_ENABLED:
			data->DateTime->AlarmEnabled = 1;
			break;
		case P7110_ALARM_DISABLED:
			data->DateTime->AlarmEnabled = 0;
			break;
		default:
			data->DateTime->AlarmEnabled = -1;
			dprintf("Unknown value of alarm enable byte: 0x%02x\n", message[8]);
			e = GE_UNKNOWN;
			break;
		}

		data->DateTime->Hour = message[9];
		data->DateTime->Minute = message[10];

		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (clock handling): 0x%02x\n", P7110_MSG_CLOCK, message[3]);
		e = GE_UNHANDLEDFRAME;
		break;
	}
	return e;
}

static GSM_Error P7110_GetNoteAlarm(int alarmdiff, GSM_DateTime *time, GSM_DateTime *alarm)
{
	time_t				t_alarm;
	struct tm			tm_time;
	struct tm			*tm_alarm;
	GSM_Error			e = GE_NONE;

	if (!time || !alarm) return GE_INTERNALERROR;

	memset(&tm_time, 0, sizeof(tm_time));
	tm_time.tm_year = time->Year - 1900;
	tm_time.tm_mon = time->Month - 1;
	tm_time.tm_mday = time->Day;
	tm_time.tm_hour = time->Hour;
	tm_time.tm_min = time->Minute;

	tzset();
	t_alarm = mktime(&tm_time);
	t_alarm -= alarmdiff;
	t_alarm += timezone;

	tm_alarm = localtime(&t_alarm);

	alarm->Year = tm_alarm->tm_year + 1900;
	alarm->Month = tm_alarm->tm_mon + 1;
	alarm->Day = tm_alarm->tm_mday;
	alarm->Hour = tm_alarm->tm_hour;
	alarm->Minute = tm_alarm->tm_min;
	alarm->Second = tm_alarm->tm_sec;

	return e;
}


static GSM_Error P7110_GetNoteTimes(unsigned char *block, GSM_CalendarNote *c)
{
	time_t		alarmdiff;
	GSM_Error	e = GE_NONE;

	if (!c) return GE_INTERNALERROR;

	c->Time.Hour = block[0];
	c->Time.Minute = block[1];
	c->Recurrence = ((((unsigned int)block[4]) << 8) + block[5]) * 60;
	alarmdiff = (((unsigned int)block[2]) << 8) + block[3];

	if (alarmdiff != 0xffff) {
		e = P7110_GetNoteAlarm(alarmdiff * 60, &(c->Time), &(c->Alarm));
		c->Alarm.AlarmEnabled = 1;
	} else {
		c->Alarm.AlarmEnabled = 0;
	}

	return e;
}

static GSM_Error P7110_IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_Error			e = GE_NONE;
	unsigned char			*block;
	int				i, alarm, year;

	if (!data || !data->CalendarNote) return GE_INTERNALERROR;

	year = data->CalendarNote->Time.Year;
	dprintf("Year: %i\n", data->CalendarNote->Time.Year);
	switch (message[3]) {
	case P7110_SUBCAL_NOTE_RCVD:
		block = message + 12;

		/*		data->CalendarNote->Location = (((unsigned int)message[4]) << 8) + message[5];*/
		data->CalendarNote->Time.Year = (((unsigned int)message[8]) << 8) + message[9];
		data->CalendarNote->Time.Month = message[10];
		data->CalendarNote->Time.Day = message[11];
		data->CalendarNote->Time.Second = 0;

		dprintf("Year: %i\n", data->CalendarNote->Time.Year);

		switch (message[6]) {
		case P7110_NOTE_MEETING:
			data->CalendarNote->Type = GCN_MEETING;
			P7110_GetNoteTimes(block, data->CalendarNote);
			DecodeUnicode(data->CalendarNote->Text, (block + 8), block[6]);
			break;
		case P7110_NOTE_CALL:
			data->CalendarNote->Type = GCN_CALL;
			P7110_GetNoteTimes(block, data->CalendarNote);
			DecodeUnicode(data->CalendarNote->Text, (block + 8), block[6]);
			DecodeUnicode(data->CalendarNote->Phone, (block + 8 + block[6] * 2), block[7]);
			break;
		case P7110_NOTE_REMINDER:
			data->CalendarNote->Type = GCN_REMINDER;
			data->CalendarNote->Recurrence = ((((unsigned int)block[0]) << 8) + block[1]) * 60;
			DecodeUnicode(data->CalendarNote->Text, (block + 4), block[2]);
			break;
		case P7110_NOTE_BIRTHDAY:

			data->CalendarNote->Type = GCN_BIRTHDAY;
			data->CalendarNote->Time.Year = year;
			data->CalendarNote->Time.Hour = 23;
			data->CalendarNote->Time.Minute = 59;
			data->CalendarNote->Time.Second = 58;

			alarm = ((unsigned int)block[2]) << 24;
			alarm += ((unsigned int)block[3]) << 16;
			alarm += ((unsigned int)block[4]) << 8;
			alarm += block[5];

			dprintf("alarm: %i\n", alarm);

			if (alarm == 0xffff) {
				data->CalendarNote->Alarm.AlarmEnabled = 0;
			} else {
				data->CalendarNote->Alarm.AlarmEnabled = 1;
			}

			P7110_GetNoteAlarm(alarm, &(data->CalendarNote->Time), &(data->CalendarNote->Alarm));

			data->CalendarNote->Time.Hour = 0;
			data->CalendarNote->Time.Minute = 0;
			data->CalendarNote->Time.Second = 0;
			data->CalendarNote->Time.Year = (((unsigned int)block[6]) << 8) + block[7];

			DecodeUnicode(data->CalendarNote->Text, (block + 10), block[9]);

			break;
		default:
			data->CalendarNote->Type = -1;
			return GE_UNKNOWN;
		}

		break;
	case P7110_SUBCAL_INFO_RCVD:
		if (!data->CalendarNotesList) return GE_INTERNALERROR;
		dprintf("Calendar Notes Info received! %i\n", (message[4] << 8) | message[5]);
		data->CalendarNotesList->Number = (message[4] << 8) + message[5];
		dprintf("Location of Notes: ");
		for (i = 0; i < data->CalendarNotesList->Number; i++) {
			data->CalendarNotesList->Location[i] = (message[8 + 2 * i] << 8) | message[9 + 2 * i];
			dprintf("%i ", data->CalendarNotesList->Location[i]); 
		}
		dprintf("\n");
		break;
	case P7110_SUBCAL_FREEPOS_RCVD:
		dprintf("First free position received: %i!\n", (message[4] << 8) | message[5]);
		data->CalendarNote->Location = (((unsigned int)message[4]) << 8) + message[5];
		break;
	case P7110_SUBCAL_DEL_NOTE_RESP:
		dprintf("Succesfully deleted calendar note: %i!\n", (message[4] << 8) | message[5]);
		break;

	case P7110_SUBCAL_ADD_MEETING_RESP:
	case P7110_SUBCAL_ADD_CALL_RESP:
	case P7110_SUBCAL_ADD_BIRTHDAY_RESP:
	case P7110_SUBCAL_ADD_REMINDER_RESP:
		dprintf("Succesfully written calendar note: %i!\n", (message[4] << 8) | message[5]);
		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (calendar handling): 0x%02x\n", P7110_MSG_CALENDAR, message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return e;
}

long P7110_GetNoteAlarmDiff(GSM_DateTime *time, GSM_DateTime *alarm)
{
	time_t     t_alarm;
	time_t     t_time;
	struct tm  tm_alarm;
	struct tm  tm_time;

	tzset();

	tm_alarm.tm_year=alarm->Year-1900;
	tm_alarm.tm_mon=alarm->Month-1;
	tm_alarm.tm_mday=alarm->Day;
	tm_alarm.tm_hour=alarm->Hour;
	tm_alarm.tm_min=alarm->Minute;
	tm_alarm.tm_sec=alarm->Second;
	tm_alarm.tm_isdst=0;
	t_alarm = mktime(&tm_alarm);

	tm_time.tm_year=time->Year-1900;
	tm_time.tm_mon=time->Month-1;
	tm_time.tm_mday=time->Day;
	tm_time.tm_hour=time->Hour;
	tm_time.tm_min=time->Minute;
	tm_time.tm_sec=time->Second;
	tm_time.tm_isdst=0;
	t_time = mktime(&tm_time);

	dprintf("   Alarm: %02i-%02i-%04i %02i:%02i:%02i\n",
		alarm->Day,alarm->Month,alarm->Year,
		alarm->Hour,alarm->Minute,alarm->Second);
	dprintf("   Date: %02i-%02i-%04i %02i:%02i:%02i\n",
		time->Day,time->Month,time->Year,
		time->Hour,time->Minute,time->Second);
	dprintf("Difference in alarm time is %f\n", difftime( t_time, t_alarm )+3600);

	return difftime(t_time, t_alarm) + 3600;
}

static GSM_Error P7110_FirstCalendarFreePos(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x31 };

 	if (SM_SendMessage(state, 4, P7110_MSG_CALENDAR, req) != GE_NONE) return GE_NOTREADY;
	return SM_WaitFor(state, data, P7110_MSG_CALENDAR);
}


static GSM_Error P7110_WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[200] = { FBUS_FRAME_HEADER,
				   0x01,       /* note type ... */
				   0x00, 0x00, /* location */
				   0x00,       /* entry type */
				   0x00,       //fixed
				   0x00, 0x00, 0x00, 0x00, /* Year(2bytes), Month, Day */
				   /* here starts block */
				   0x00, 0x00, 0x00, 0x00,0x00, 0x00}; /* ... depends on note type ... */

	GSM_CalendarNote *CalendarNote;
	int count=0;
	long seconds, minutes;
	GSM_Error error;

	CalendarNote = data->CalendarNote;

	/* 6210/7110 needs to seek the first free pos to inhabit with next note */
	error = P7110_FirstCalendarFreePos(data, state);
	if (error != GE_NONE) return error;


	/* Location */
	req[4] = CalendarNote->Location >> 8;
	req[5] = CalendarNote->Location & 0xff;

	switch( CalendarNote->Type ) {
	case GCN_MEETING: 
		req[6]=0x01; 
		req[3]=0x01; 
		break;
	case GCN_CALL:
		req[6]=0x02; 
		req[3]=0x03; 
		break;
	case GCN_BIRTHDAY: 
		req[6]=0x04; 
		req[3]=0x05;
		break;
	case GCN_REMINDER:
		req[6]=0x08;
		req[3]=0x07;
		break;
	}

	req[8]=CalendarNote->Time.Year >> 8;
	req[9]=CalendarNote->Time.Year & 0xff;
	req[10]=CalendarNote->Time.Month;
	req[11]=CalendarNote->Time.Day;

	/* From here starts BLOCK */
	count=12;
	switch( CalendarNote->Type ) {

	case GCN_MEETING:
		req[count++] = CalendarNote->Time.Hour;   // 12
		req[count++] = CalendarNote->Time.Minute; // 13
		/* Alarm .. */
		req[count++] = 0xff; // 14
		req[count++] = 0xff; // 15
		if( CalendarNote->Alarm.Year ) {
			seconds = P7110_GetNoteAlarmDiff(&CalendarNote->Time, 
							&CalendarNote->Alarm);
			if( seconds >= 0L ) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes>>8;
				req[count++] = minutes&0xff;
			}
		}
		/* Recurrence */
		if( CalendarNote->Recurrence >= 8760 )
			CalendarNote->Recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = CalendarNote->Recurrence >> 8;   // 16
		req[count++] = CalendarNote->Recurrence & 0xff; // 17
		/* len of text */
		req[count++] = strlen(CalendarNote->Text);    // 18
		/* fixed 0x00 */
		req[count++] = 0x00; // 19
		/* Text */

		dprintf("Count before encode = %d\n", count );
		dprintf("Meeting Text is = \"%s\"\n", CalendarNote->Text );


		EncodeUnicode(req+count,CalendarNote->Text, strlen(CalendarNote->Text)); // 20->N
		count = count + 2 * strlen(CalendarNote->Text);
		break;

	case GCN_CALL:
		req[count++] = CalendarNote->Time.Hour;   // 12
		req[count++] = CalendarNote->Time.Minute; // 13
		/* Alarm .. */
		req[count++] = 0xff; // 14
		req[count++] = 0xff; // 15
		if( CalendarNote->Alarm.Year ) {
			seconds = P7110_GetNoteAlarmDiff(&CalendarNote->Time, 
							&CalendarNote->Alarm);
			if( seconds >= 0L ) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes >> 8;
				req[count++] = minutes & 0xff;
			}
		}
		/* Recurrence */
		if( CalendarNote->Recurrence >= 8760 )
			CalendarNote->Recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = CalendarNote->Recurrence >> 8;   // 16
		req[count++] = CalendarNote->Recurrence & 0xff; // 17
		/* len of text */
		req[count++] = strlen(CalendarNote->Text);    // 18
		/* fixed 0x00 */
		req[count++] = strlen(CalendarNote->Phone);   // 19
		/* Text */
		EncodeUnicode(req + count, CalendarNote->Text, strlen(CalendarNote->Text));// 20->N
		count += 2 * strlen(CalendarNote->Text);
		EncodeUnicode(req + count, CalendarNote->Phone, strlen(CalendarNote->Phone));// (N+1)->n
		count += 2 * strlen(CalendarNote->Phone);
		break;

	case GCN_BIRTHDAY:
		req[count++] = 0x00; // 12 Fixed
		req[count++] = 0x00; // 13 Fixed

		/* Alarm .. */
		req[count++] = 0x00; 
		req[count++] = 0x00; // 14, 15
		req[count++] = 0xff; // 16
		req[count++] = 0xff; // 17
		if( CalendarNote->Alarm.Year ) {
			/* I first try Time.Year = Alarm.Year. If negative, I increase year by one,
			   but only once! This is because I may have alarm period across
			   the year border, eg. birthday on 2001-01-10 and alarm on 2000-12-27 */

			CalendarNote->Time.Year = CalendarNote->Alarm.Year;
			if( (seconds= P7110_GetNoteAlarmDiff(&CalendarNote->Time, 
							     &CalendarNote->Alarm)) < 0L ) {
				CalendarNote->Time.Year++;
				seconds= P7110_GetNoteAlarmDiff(&CalendarNote->Time,
								&CalendarNote->Alarm);
			}
			if( seconds >= 0L ) { /* Otherwise it's an error condition.... */
				count -= 4;
				req[count++] = seconds>>24;              // 14
				req[count++] = (seconds>>16) & 0xff;     // 15
				req[count++] = (seconds>>8) & 0xff;      // 16
				req[count++] = seconds&0xff;             // 17
			}
		}	

		req[count++] = 0x00; /* FIXME: CalendarNote->AlarmType; 0x00 tone, 0x01 silent 18 */

		/* len of text */
		req[count++] = strlen(CalendarNote->Text); // 19

		/* Text */
		dprintf("Count before encode = %d\n", count );
		/*		dprintf("Meeting Text is = \"%s\" Altype is 0x%02x \n", CalendarNote->Text , CalendarNote->AlarmType );*/

		EncodeUnicode(req + count, CalendarNote->Text, strlen(CalendarNote->Text));// 22->N
		count = count + 2 * strlen(CalendarNote->Text);
		break;

	case GCN_REMINDER:
		/* Recurrence */
		if( CalendarNote->Recurrence >= 8760 )
			CalendarNote->Recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++]=CalendarNote->Recurrence >> 8;   // 12
		req[count++]=CalendarNote->Recurrence & 0xff; // 13
		/* len of text */
		req[count++]=strlen(CalendarNote->Text);    // 14
		/* fixed 0x00 */
		req[count++]=0x00; // 15
		/* Text */
		EncodeUnicode(req+count, CalendarNote->Text, strlen(CalendarNote->Text));// 16->N
		count = count + 2 * strlen(CalendarNote->Text);
		break;
	}

	/* padding */
	req[count] = 0x00;

	dprintf("Count after padding = %d\n", count);
  
	if (SM_SendMessage(state, count, P7110_MSG_CALENDAR, req) != GE_NONE) return GE_NOTREADY;
	return SM_WaitFor(state, data, P7110_MSG_CALENDAR);
}

static GSM_Error P7110_GetCalendarNotesInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, P7110_SUBCAL_GET_INFO, 0xFF, 0xFE};

	if (SM_SendMessage(state, 6, P7110_MSG_CALENDAR, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P7110_MSG_CALENDAR);
}

static GSM_Error P7110_GetCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error	error = GE_NOTREADY;
	unsigned char	req[] = {FBUS_FRAME_HEADER, P7110_SUBCAL_GET_NOTE, 0x00, 0x00};
	unsigned char	date[] = {FBUS_FRAME_HEADER, P7110_SUBCLO_GET_DATE};
	GSM_Data	tmpdata;
	GSM_DateTime	tmptime;
	GSM_CalendarNotesList list;

	data->CalendarNotesList = &list;
	tmpdata.DateTime = &tmptime;
	if ((error = P7110_GetCalendarNotesInfo(data, state)) == GE_NONE) {
		if (data->CalendarNote->Location < data->CalendarNotesList->Number + 1 &&
		    data->CalendarNote->Location > 0 ) {
			if (SM_SendMessage(state, 4, P7110_MSG_CLOCK, date) == GE_NONE) {
				SM_Block(state, &tmpdata, P7110_MSG_CLOCK);
				req[4] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] >> 8;
				req[5] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] & 0xff;
				data->CalendarNote->Time.Year = tmptime.Year;
				} else return GE_UNKNOWN; /* FIXME */
		} else return GE_INVALIDCALNOTELOCATION;
	} else return error;

	if (SM_SendMessage(state, 6, P7110_MSG_CALENDAR, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P7110_MSG_CALENDAR);
}

static GSM_Error P7110_DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER,
				0x0b,      /* delete calendar note */
				0x00, 0x00 //location
	};
	GSM_CalendarNotesList list;
	GSM_Error error;

	data->CalendarNotesList = &list;
	if (P7110_GetCalendarNotesInfo(data, state) == GE_NONE) {
		if (data->CalendarNote->Location < data->CalendarNotesList->Number + 1 &&
		    data->CalendarNote->Location > 0 ) {
			req[4] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] << 8;
			req[5] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] & 0xff;
		} else {
			return GE_INVALIDCALNOTELOCATION;
		}
	}
 
	if (SM_SendMessage(state, 6, P7110_MSG_CALENDAR, req) != GE_NONE) return GE_NOTREADY;
	return SM_WaitFor(state, data, P7110_MSG_CALENDAR);
	return error;
}


static int GetMemoryType(GSM_MemoryType memory_type)
{
	int result;

	switch (memory_type) {
	case GMT_MT:
		result = P7110_MEMORY_MT;
		break;
	case GMT_ME:
		result = P7110_MEMORY_ME;
		break;
	case GMT_SM:
		result = P7110_MEMORY_SM;
		break;
	case GMT_FD:
		result = P7110_MEMORY_FD;
		break;
	case GMT_ON:
		result = P7110_MEMORY_ON;
		break;
	case GMT_EN:
		result = P7110_MEMORY_EN;
		break;
	case GMT_DC:
		result = P7110_MEMORY_DC;
		break;
	case GMT_RC:
		result = P7110_MEMORY_RC;
		break;
	case GMT_MC:
		result = P7110_MEMORY_MC;
		break;
	case GMT_IN:
		result = P7110_MEMORY_IN;
		break;
	case GMT_OU:
		result = P7110_MEMORY_OU;
		break;
	case GMT_AR:
		result = P7110_MEMORY_AR;
		break;
	case GMT_TE:
		result = P7110_MEMORY_TE;
		break;
	case GMT_F1:
		result = P7110_MEMORY_F1;
		break;
	case GMT_F2:
		result = P7110_MEMORY_F2;
		break;
	case GMT_F3:
		result = P7110_MEMORY_F3;
		break;
	case GMT_F4:
		result = P7110_MEMORY_F4;
		break;
	case GMT_F5:
		result = P7110_MEMORY_F5;
		break;
	case GMT_F6:
		result = P7110_MEMORY_F6;
		break;
	case GMT_F7:
		result = P7110_MEMORY_F7;
		break;
	case GMT_F8:
		result = P7110_MEMORY_F8;
		break;
	case GMT_F9:
		result = P7110_MEMORY_F9;
		break;
	case GMT_F10:
		result = P7110_MEMORY_F10;
		break;
	case GMT_F11:
		result = P7110_MEMORY_F11;
		break;
	case GMT_F12:
		result = P7110_MEMORY_F12;
		break;
	case GMT_F13:
		result = P7110_MEMORY_F13;
		break;
	case GMT_F14:
		result = P7110_MEMORY_F14;
		break;
	case GMT_F15:
		result = P7110_MEMORY_F15;
		break;
	case GMT_F16:
		result = P7110_MEMORY_F16;
		break;
	case GMT_F17:
		result = P7110_MEMORY_F17;
		break;
	case GMT_F18:
		result = P7110_MEMORY_F18;
		break;
	case GMT_F19:
		result = P7110_MEMORY_F19;
		break;
	case GMT_F20:
		result = P7110_MEMORY_F20;
		break;
	default:
		result = P7110_MEMORY_XX;
		break;
	}
	return (result);
}

#if 0

static GSM_Error P7110_DialVoice(char *Number)
{

	/* Doesn't work (yet) */    /* 3 2 1 5 2 30 35 */

	// unsigned char req0[100] = { 0x00, 0x01, 0x64, 0x01 };

	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x01, 0x01, 0x01, 0x05, 0x00, 0x01, 0x03, 0x02, 0x91, 0x00, 0x031, 0x32, 0x00};
	// unsigned char req[100]={0x00, 0x01, 0x7c, 0x01, 0x31, 0x37, 0x30, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01};
	//  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x20, 0x01, 0x46};
	// unsigned char req_end[] = {0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00, 0x01};
	int len = 0/*, i*/;

	//req[4]=strlen(Number);

	//for(i=0; i < strlen(Number) ; i++)
	// req[5+i]=Number[i];

	//memcpy(req+5+strlen(Number), req_end, 10);

	//len=6+strlen(Number);

	//len = 4;


	//PGEN_CommandResponse(&link, req0, &len, 0x40, 0x40, 100);

	len = 17;

	if (PGEN_CommandResponse(&link, req, &len, 0x01, 0x01, 100)==GE_NONE) {
//		return GE_NONE;

	}
//	} else return GE_NOTIMPLEMENTED;


	while(1) link.Loop(NULL);

	return GE_NOTIMPLEMENTED;
}

#endif

static GSM_Error GetCallerBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
				  0x00, 0x10 , /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00};

	req[11] = GNOKII_MIN(data->Bitmap->number + 1, GSM_MAX_CALLER_GROUPS);
	dprintf("Getting caller(%d) logo...\n", req[11]);
	if (SM_SendMessage(state, 14, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static unsigned char PackBlock(u8 id, u8 size, u8 no, u8 *buf, u8 *block)
{
	*(block++) = id;
	*(block++) = 0;
	*(block++) = 0;
	*(block++) = size + 6;
	*(block++) = no;
	memcpy(block, buf, size);
	block += size;
	*(block++) = 0;
	return (size + 6);
}

static GSM_Error SetCallerBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0c,
				  0x00, 0x10,  /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00, 0x00};
	char string[500];
	int block, i;
	unsigned int count = 18;

	if ((data->Bitmap->width != state->Phone.Info.CallerLogoW) ||
	    (data->Bitmap->height != state->Phone.Info.CallerLogoH )) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n",state->Phone.Info.CallerLogoH, state->Phone.Info.CallerLogoW, data->Bitmap->height, data->Bitmap->width);
	    return GE_INVALIDIMAGESIZE;
	}

	req[13] = data->Bitmap->number + 1;
	dprintf("Setting caller(%d) bitmap...\n",data->Bitmap->number);
	block = 1;
	/* Name */
	i = strlen(data->Bitmap->text);
	EncodeUnicode((string + 1), data->Bitmap->text, i);
	string[0] = i * 2;
	count += PackBlock(0x07, i * 2 + 1, block++, string, req + count);
	/* Ringtone */
	string[0] = data->Bitmap->ringtone;
	string[1] = 0;
	count += PackBlock(0x0c, 2, block++, string, req + count);
	/* Number */
	string[0] = data->Bitmap->number+1;
	string[1] = 0;
	count += PackBlock(0x1e, 2, block++, string, req + count);
	/* Logo on/off - assume on for now */
	string[0] = 1;
	string[1] = 0;
	count += PackBlock(0x1c, 2, block++, string, req + count);
	/* Logo */
	string[0] = data->Bitmap->width;
	string[1] = data->Bitmap->height;
	string[2] = 0;
	string[3] = 0;
	string[4] = 0x7e; /* Size */
	memcpy(string + 5, data->Bitmap->bitmap, data->Bitmap->size);
	count += PackBlock(0x1b, data->Bitmap->size + 5, block++, string, req + count);
	req[17] = block - 1;

	if (SM_SendMessage(state, count, 0x03, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error GetStartupBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0xee, 0x15};

	dprintf("Getting startup logo...\n");
	if (SM_SendMessage(state, 5, 0x7a, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x7a);
}

static GSM_Error P7110_IncomingStartup(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0xeb:
		dprintf("Startup logo set ok\n");
		return GE_NONE;
		break;
	case 0xed:
		if (data->Bitmap) {
			/* I'm sure there are blocks here but never mind! */
			data->Bitmap->type = GSM_StartupLogo;
			data->Bitmap->height = message[13];
			data->Bitmap->width = message[17];
			data->Bitmap->size=((data->Bitmap->height/8)+(data->Bitmap->height%8>0))*data->Bitmap->width; /* Can't see this coded anywhere */
			memcpy(data->Bitmap->bitmap, message+22, data->Bitmap->size);
			dprintf("Startup logo got ok - height(%d) width(%d)\n", data->Bitmap->height, data->Bitmap->width);
		}
		return GE_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x7a (%d)\n", message[3]);
		return GE_UNHANDLEDFRAME;
		break;
	}
}

static GSM_Error GetOperatorBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x70};

	dprintf("Getting op logo...\n");
	if (SM_SendMessage(state, 4, 0x0a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static GSM_Error SetStartupBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[1000] = {FBUS_FRAME_HEADER, 0xec, 0x15, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x02, 0x00,
				   0x00,           /* Height */
				   0xc0, 0x03, 0x00,
				   0x00,           /* Width */
				   0xc0, 0x04, 0x03, 0x00 };
	int count = 21;


	if ((data->Bitmap->width != state->Phone.Info.StartupLogoW) ||
	    (data->Bitmap->height != state->Phone.Info.StartupLogoH )) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n",state->Phone.Info.StartupLogoH, state->Phone.Info.StartupLogoW, data->Bitmap->height, data->Bitmap->width);
	    return GE_INVALIDIMAGESIZE;
	}

	req[12] = data->Bitmap->height;
	req[16] = data->Bitmap->width;
	memcpy(req + count, data->Bitmap->bitmap, data->Bitmap->size);
	count += data->Bitmap->size;
	dprintf("Setting startup logo...\n");

	if (SM_SendMessage(state, count, 0x7a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x7a);
}

static GSM_Error SetOperatorBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[500] = { FBUS_FRAME_HEADER, 0xa3, 0x01,
				   0x00,              /* logo enabled */
				   0x00, 0xf0, 0x00,  /* network code (000 00) */
				   0x00 ,0x04,
				   0x08,              /* length of rest */
				   0x00, 0x00,        /* Bitmap width / height */
				   0x00,
				   0x00,              /* Bitmap size */
				   0x00, 0x00
	};
	int count = 18;

	if ((data->Bitmap->width != state->Phone.Info.OpLogoW) ||
	    (data->Bitmap->height != state->Phone.Info.OpLogoH )) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n",state->Phone.Info.OpLogoH, state->Phone.Info.OpLogoW, data->Bitmap->height, data->Bitmap->width);
	    return GE_INVALIDIMAGESIZE;
	}

	if (strcmp(data->Bitmap->netcode,"000 00")) {  /* set logo */
		req[5] = 0x01;      // Logo enabled
		req[6] = ((data->Bitmap->netcode[1] & 0x0f) << 4) | (data->Bitmap->netcode[0] & 0x0f);
		req[7] = 0xf0 | (data->Bitmap->netcode[2] & 0x0f);
		req[8] = ((data->Bitmap->netcode[5] & 0x0f) << 4) | (data->Bitmap->netcode[4] & 0x0f);
		req[11] = 8 + data->Bitmap->size;
		req[12] = data->Bitmap->width;
		req[13] = data->Bitmap->height;
		req[15] = data->Bitmap->size;
		memcpy(req + count, data->Bitmap->bitmap, data->Bitmap->size);
		count += data->Bitmap->size;
	}
	dprintf("Setting op logo...\n");
	if (SM_SendMessage(state, count, 0x0a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static GSM_Error P7110_GetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	switch(data->Bitmap->type) {
	case GSM_CallerLogo:
		return GetCallerBitmap(data, state);
	case GSM_StartupLogo:
		return GetStartupBitmap(data, state);
	case GSM_OperatorLogo:
		return GetOperatorBitmap(data, state);
	default:
		return GE_NOTIMPLEMENTED;
	}
}

static GSM_Error P7110_SetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	switch(data->Bitmap->type) {
	case GSM_CallerLogo:
		return SetCallerBitmap(data, state);
	case GSM_StartupLogo:
		return SetStartupBitmap(data, state);
	case GSM_OperatorLogo:
		return SetOperatorBitmap(data, state);
	default:
		return GE_NOTIMPLEMENTED;
	}
}

static GSM_Error P7110_DeletePhonebookLocation(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0f, 0x00, 0x01, 0x04, 0x00, 0x00, 0x0c, 0x01, 0xff,
				  0x00, 0x00,  /* location */
				  0x00,  /* memory type */
				  0x00, 0x00, 0x00};
	GSM_PhonebookEntry *entry;

	if (data->PhonebookEntry) entry = data->PhonebookEntry;
	else return GE_TRYAGAIN;
	/* Two octets for the memory location */
	req[12] = (entry->Location >> 8);
	req[13] = entry->Location & 0xff;
	req[14] = GetMemoryType(entry->MemoryType);

	if (SM_SendMessage(state, 18, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error P7110_WritePhonebookLocation(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0c,
				  0x00, 0x00,  /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00, 0x00, 0x00};

	char string[500];
	int block, i, j, defaultn;
	unsigned int count = 18;
	GSM_PhonebookEntry *entry;

	if (data->PhonebookEntry) entry = data->PhonebookEntry;
	else return GE_TRYAGAIN;

	req[11] = GetMemoryType(entry->MemoryType);
	/* Two octets for the memory location */
	req[12] = (entry->Location >> 8);
	req[13] = entry->Location & 0xff;
	block = 1;
	if ((*(entry->Name)) && (*(entry->Number))) { 
		/* Name */
		i = strlen(entry->Name);
		EncodeUnicode((string + 1), entry->Name, i);
		/* Terminating 0 */
		string[i * 2 + 1] = 0;
		/* Length ot the string + length field + terminating 0 */
		string[0] = (i + 1) * 2;
		count += PackBlock(0x07, (i + 1) * 2, block++, string, req + count);
		/* Group */
		string[0] = entry->Group + 1;
		string[1] = 0;
		count += PackBlock(0x1e, 2, block++, string, req + count);
		/* Default Number */
		defaultn = 999;
		for (i = 0; i < entry->SubEntriesCount; i++)
			if (entry->SubEntries[i].EntryType == GSM_Number)
				if (!strcmp(entry->Number, entry->SubEntries[i].data.Number))
					defaultn = i;
		if (defaultn < i) {
			string[0] = entry->SubEntries[defaultn].NumberType;
			string[1] = string[2] = string[3] = 0;
			i = strlen(entry->SubEntries[defaultn].data.Number);
			EncodeUnicode((string + 5), entry->SubEntries[defaultn].data.Number, i);
			string[i * 2 + 1] = 0;
			string[4] = i * 2;
			count += PackBlock(0x0b, i * 2 + 6, block++, string, req + count);
		}
		/* Rest of the numbers */
		for (i = 0; i < entry->SubEntriesCount; i++)
			if (entry->SubEntries[i].EntryType == GSM_Number)
				if (i != defaultn) {
					string[0] = entry->SubEntries[i].NumberType;
					string[1] = string[2] = string[3] = 0;
					j = strlen(entry->SubEntries[i].data.Number);
					EncodeUnicode((string + 5), entry->SubEntries[i].data.Number, j);
					string[i * 2 + 1] = 0;
					string[4] = j * 2;
					count += PackBlock(0x0b, j * 2 + 6, block++, string, req + count);
				}
		req[17] = block - 1;
		dprintf("Writing phonebook entry %s...\n",entry->Name);
	} else {
		return P7110_DeletePhonebookLocation(data, state);
	}
	if (SM_SendMessage(state, count, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error P7110_ReadPhonebookLL(GSM_Data *data, GSM_Statemachine *state, int memtype, int location)
{
	unsigned char req[2000] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
					0x00, 0x00, /* memory type */ //02,05
					0x00, 0x00, /* location */
					0x00, 0x00};

	dprintf("Reading phonebook location (%d)\n", location);

	req[9] = memtype;
	req[10] = location >> 8;
	req[11] = location & 0xff;

	if (SM_SendMessage(state, 14, P7110_MSG_PHONEBOOK, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P7110_MSG_PHONEBOOK);
}

static GSM_Error P7110_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	int memtype, location;

	memtype = GetMemoryType(data->PhonebookEntry->MemoryType);
	location = data->PhonebookEntry->Location;

	return P7110_ReadPhonebookLL(data, state, memtype, location);
}

static GSM_Error P7110_GetSpeedDial(GSM_Data *data, GSM_Statemachine *state)
{
	int memtype, location;

	memtype = P7110_MEMORY_SPEEDDIALS;
	location = data->SpeedDial->Number;

	return P7110_ReadPhonebookLL(data, state, memtype, location);
}

static GSM_Error P7110_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, P7110_SUBSMS_GET_SMSC, 0x64, 0x00};

	req[5] = data->MessageCenter->No;

	if (SM_SendMessage(state, 6, P7110_MSG_SMS, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P7110_MSG_SMS);
}

static GSM_Error P7110_GetClock(char req_type, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, req_type};

	if (SM_SendMessage(state, 4, P7110_MSG_CLOCK, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P7110_MSG_CLOCK);
}


static GSM_Error P7110_NetMonitor(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req1[2000] = {FBUS_FRAME_HEADER,
					P7110_SUBSEC_ENABLE_EXTENDED_CMDS, 0x01};
	unsigned char req2[2000] = {FBUS_FRAME_HEADER, P7110_SUBSEC_NETMONITOR};

	req2[4] = data->NetMonitor->Field;

	if (SM_SendMessage(state, 5, P7110_MSG_SECURITY, req1) != GE_NONE) return GE_NOTREADY;
	if (SM_SendMessage(state, 5, P7110_MSG_SECURITY, req2) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P7110_MSG_SECURITY);
}

static GSM_Error P7110_IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (!data || !data->NetMonitor) return GE_INTERNALERROR;
	switch (message[3]) {
	case P7110_SUBSEC_NETMONITOR:
		switch(message[4]) {
		case 0x00:
			dprintf("Message: Netmonitor correctly set.\n");
			break;
		default:
			dprintf("Message: Netmonitor menu %d received:\n", message[4]);
			dprintf("%s\n", message + 5);
			strcpy(data->NetMonitor->Screen, message + 5);
			break;
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (Security): 0x%02x\n", P7110_MSG_SECURITY, message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}
