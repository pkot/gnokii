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

  This file provides functions specific to the 6510 series.
  See README for more details on supported mobile phones.

  The various routines are called P6510_(whatever).

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk6510.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

/* Functions prototypes */
static GSM_Error P6510_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_Initialise(GSM_Statemachine *state);
static GSM_Error P6510_GetModel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
/*
static GSM_Error P6510_SetBitmap(GSM_Data *data, GSM_Statemachine *state);
*/
static GSM_Error P6510_GetBitmap(GSM_Data *data, GSM_Statemachine *state);
/*
static GSM_Error P6510_WritePhonebookLocation(GSM_Data *data, GSM_Statemachine *state);
*/
static GSM_Error P6510_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state);
/*
static GSM_Error P6510_GetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
*/
static GSM_Error P6510_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetClock(char req_type, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetCalendarNote(GSM_Data *data, GSM_Statemachine *state);
/*
static GSM_Error P6510_PollSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_SendSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_DeleteSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetPicture(GSM_Data *data, GSM_Statemachine *state);
*/
static GSM_Error P6510_GetSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetSMSFolders(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_GetSMSFolderStatus(GSM_Data *data, GSM_Statemachine *state);
/*
static GSM_Error P6510_GetSMSStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_CallDivert(GSM_Data *data, GSM_Statemachine *state);
*/

static GSM_Error P6510_GetRingtones(GSM_Data *data, GSM_Statemachine *state);

static GSM_Error P6510_NetMonitor(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P6510_IncomingIdentify(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P6510_IncomingPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P6510_IncomingNetwork(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P6510_IncomingBattLevel(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
/*
static GSM_Error P6510_IncomingStartup(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
*/
static GSM_Error P6510_IncomingSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P6510_IncomingFolder(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P6510_IncomingClock(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error P6510_IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data);
/*
static GSM_Error P6510_IncomingCallDivert(int messagetype, unsigned char *message, int length, GSM_Data *data);
*/
static GSM_Error P6510_IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error P6510_IncomingRingtone(int messagetype, unsigned char *message, int length, GSM_Data *data);


static int GetMemoryType(GSM_MemoryType memory_type);

/* Some globals */

static bool SMSLoop = false; /* Are we in infinite SMS reading loop? */
static bool NewSMS  = false; /* Do we have a new SMS? */

static const SMSMessage_Layout nk6510_deliver = {
	true,			/* Is the SMS type supported */
	33, true, true,		/* SMSC */
	-1, 
	3, 			/* Reply via same SMSC */
	-1, -1,  
	3, 			/* message freference number */
	-1, -1, -1,
	44, 41, 40,		/* UD length, DCS field, UDH indicator */
	-1, -1, -1,		/* Validity */
	21, true, true,		/* Remote Number */
	6, -1,			/* Time */
	1,  0,			/* Nonstandard fields */
	45, true			/* User Data */
};

static const SMSMessage_Layout nk6510_submit_stored = {
	true,
	-1, true, true,		/* SMSC */
	-1, 
	-1, -1, -1,		/* Reply via same SMSC, RejectDuplicates, Report */
	-1, 
	18, 19, 			/* Reference, PID */
	-1, 
	28, 20, 17,	       	/* UserDataLength Field, DCS Field, UDH bit */
	17, 			/* Valdity Type */
	-1, -1,			/* Validity Field, length */
	-1, true, true,		/* Remote Number, is coded?, fixed length? */
	-1, -1,			/* SMSC Response, delivery time */
	-1, -1,			/* MT, UserData */
	29, true			/* UserData field, UserData PDU (?) */
};

static const SMSMessage_Layout nk6510_submit_sent = {
	true,
	25, true, true,		/* SMSC */
	-1, 
	-1, -1, -1,		/* Reply via same SMSC, RejectDuplicates, Report */
	-1, 
	18, 19, 			/* Reference, PID */
	-1, 
	35, 20, 17,	       	/* UserDataLength Field, DCS Field, UDH bit */
	17, 			/* Valdity Type */
	-1, -1,			/* Validity Field, length */
	13, true, true,		/* Remote Number, is coded?, fixed length? */
	-1, -1,			/* SMSC Response, delivery time */
	-1, -1,			/* MT, UserData */
	37, true			/* UserData field, UserData PDU (?) */
};

static const SMSMessage_Layout nk6510_text_template = {
	true,
	-1, true, true,			/* SMSC */
	-1, 
	-1, -1, -1,			/* Reply via same SMSC, RejectDuplicates, Report */
	-1, 
	18, 19, 				/* Reference, PID */
	-1, 
	12, 8, 9,		       	/* UserDataLength Field, DCS Field, UDH bit */
	17, 				/* Valdity Type */
	-1, -1,				/* Validity Field, length */
	-1, true, true,			/* Remote Number, is coded?, fixed length? */
	-1, -1,				/* SMSC Response, delivery time */
	-1, -1,				/* MT, UserData */
	13, true				/* UserData field, UserData PDU (?) */
};

static const SMSMessage_Layout nk6510_picture_template = {
	true,
	-1, true, true,			/* SMSC */
	-1, 
	-1, -1, -1,			/* Reply via same SMSC, RejectDuplicates, Report */
	-1, 
	18, 19, 				/* Reference, PID */
	-1, 
	12, 8, 9,		       	/* UserDataLength Field, DCS Field, UDH bit */
	17, 				/* Valdity Type */
	-1, -1,				/* Validity Field, length */
	-1, true, true,			/* Remote Number, is coded?, fixed length? */
	-1, -1,				/* SMSC Response, delivery time */
	-1, -1,				/* MT, UserData */
	13, true				/* UserData field, UserData PDU (?) */
};

static const SMSMessage_Layout nk6510_delivery_report = {
	true,
	 5, true, true,
	-1, 
	-1, -1, -1,  
	3, 
	-1, -1, 
	-1, 
	19, 18, 17,
	-1, 
	-1, -1,
	20, true, true,
	32, 39,
	 1,  0,
	25, true
};

static const SMSMessage_Layout nk6510_picture = {
	true,
	37, true, true,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1,
	25, true, true,
	10, -1,
	-1, -1,
	52, true
};


static SMSMessage_PhoneLayout nk6510_layout;

static GSM_IncomingFunctionType P6510_IncomingFunctions[] = {
       	{ P6510_MSG_FOLDER,	P6510_IncomingFolder },
	{ P6510_MSG_SMS,	P6510_IncomingSMS },
	{ P6510_MSG_PHONEBOOK,	P6510_IncomingPhonebook },
	{ P6510_MSG_NETSTATUS,	P6510_IncomingNetwork },
	{ P6510_MSG_CALENDAR,	P6510_IncomingCalendar },
	{ P6510_MSG_BATTERY,	P6510_IncomingBattLevel },
	{ P6510_MSG_CLOCK,	P6510_IncomingClock },
	{ P6510_MSG_IDENTITY,	P6510_IncomingIdentify },
	/*	{ P6510_MSG_STLOGO,	P6510_IncomingStartup },
		{ P6510_MSG_DIVERT,	P6510_IncomingCallDivert },*/
	{ P6510_MSG_SECURITY,	P6510_IncomingSecurity },
	{ P6510_MSG_RINGTONE,	P6510_IncomingRingtone },
	{ 0, NULL }
};

GSM_Phone phone_nokia_6510 = {
	P6510_IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"6510|6310",		     /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Percentage,        /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Percentage,        /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		60, 96,                /* Startup logo size */
		21, 78,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	P6510_Functions
};

/* FIXME - a little macro would help here... */

static GSM_Error P6510_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return P6510_Initialise(state);
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
		/*
	case GOP_SetBitmap:
		return P6510_SetBitmap(data, state);
		*/
	case GOP_ReadPhonebook:
		return P6510_ReadPhonebook(data, state);
		/*
	case GOP_WritePhonebook:
		return P6510_WritePhonebookLocation(data, state);
		*/
	case GOP_GetNetworkInfo:
		return P6510_GetNetworkInfo(data, state);
		/*
	case GOP_GetSpeedDial:
		return P6510_GetSpeedDial(data, state);
		*/
	case GOP_GetSMSCenter:
		return P6510_GetSMSCenter(data, state);
	case GOP_GetDateTime:
		return P6510_GetClock(P6510_SUBCLO_GET_DATE, data, state);
	case GOP_GetAlarm:
		return P6510_GetClock(P6510_SUBCLO_GET_ALARM, data, state);
	case GOP_GetCalendarNote:
		return P6510_GetCalendarNote(data, state);
		/*
	case GOP_OnSMS:
		if (data->OnSMS) {
			NewSMS = true;
			return GE_NONE;
		}
		break;
	case GOP_PollSMS:
		if (NewSMS) return GE_NONE; 
		break;
	case GOP6510_GetPicture:
		return P6510_GetPicture(data, state);
	case GOP_SendSMS:
		return P6510_SendSMS(data, state);
	case GOP_DeleteSMS:
		return P6510_DeleteSMS(data, state);
	case GOP_GetSMSStatus:
		return P6510_GetSMSStatus(data, state);
	case GOP_CallDivert:
		return P6510_CallDivert(data, state);
		*/
	case GOP_NetMonitor:
		return P6510_NetMonitor(data, state);
	case GOP_GetSMSFolderStatus:
		return P6510_GetSMSFolderStatus(data, state);
	case GOP_GetSMS:
		return P6510_GetSMS(data, state);
	case GOP_GetSMSFolders:
		return P6510_GetSMSFolders(data, state);

	default:
		return GE_NOTIMPLEMENTED;
	}
	return GE_NONE;
}

/* Initialise is the only function allowed to 'use' state */
static GSM_Error P6510_Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];
	GSM_Error err;
	int try = 0, connected = 0;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_6510, sizeof(GSM_Phone));

	/* SMS Layout */
	nk6510_layout.Type = 14; /* Locate the Type of the message field. */
	nk6510_layout.SendHeader = 6;
	nk6510_layout.ReadHeader = 13;
	nk6510_layout.Deliver = nk6510_deliver;
	nk6510_layout.Submit = nk6510_submit_stored;
	nk6510_layout.DeliveryReport = nk6510_delivery_report;
	nk6510_layout.Picture = nk6510_picture;
	nk6510_layout.TextTemplate = nk6510_text_template;
	nk6510_layout.PictureTemplate = nk6510_picture_template;
	nk6510_layout.SubmitSent = nk6510_submit_sent;

	layout = nk6510_layout;

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
	return GE_NONE;
}


/**********************/
/* IDENTIFY FUNCTIONS */
/**********************/

static GSM_Error P6510_IncomingIdentify(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x01:
		if (data->Imei) {
			int n;
			unsigned char *s = strchr(message + 10, '\n');

			if (s) n = s - message - 9;
			else n = GSM_MAX_IMEI_LENGTH;
			snprintf(data->Imei, GNOKII_MIN(n, GSM_MAX_IMEI_LENGTH), "%s", message + 10);
			dprintf("Received imei %s\n", data->Imei);
		}
		break;
	case 0x08:
		if (data->Model) {
			int n;
			unsigned char *s = strchr(message + 27, '\n');

			n = s ? s - message - 26 : GSM_MAX_MODEL_LENGTH;
			snprintf(data->Model, GNOKII_MIN(n, GSM_MAX_MODEL_LENGTH), "%s", message + 27);
			dprintf("Received model %s\n",data->Model);
		}
		if (data->Revision) {
			int n;
			unsigned char *s = strchr(message + 10, '\n');

			n = s ? s - message - 9 : GSM_MAX_REVISION_LENGTH;
			snprintf(data->Revision, GNOKII_MIN(n, GSM_MAX_REVISION_LENGTH), "%s", message + 10);
			dprintf("Received revision %s\n",data->Revision);
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x2b (%d)\n", message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}

/* Just as an example.... */
/* But note that both requests are the same type which isn't very 'proper' */
static GSM_Error P6510_Identify(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x41};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Identifying...\n");
	PNOK_GetManufacturer(data->Manufacturer);
	if (SM_SendMessage(state, 5, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	if (SM_SendMessage(state, 6, 0x1b, req2) != GE_NONE) return GE_NOTREADY;
	SM_WaitFor(state, data, 0x1b);
	SM_Block(state, data, 0x1b); /* waits for all requests - returns req2 error */
	SM_GetError(state, 0x1b);

	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GE_UNKNOWN;
	return GE_NONE;
}

static GSM_Error P6510_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x41};

	dprintf("Getting imei...\n");
	if (SM_SendMessage(state, 5, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static GSM_Error P6510_GetModel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Getting model...\n");
	if (SM_SendMessage(state, 6, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static GSM_Error P6510_GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Getting revision...\n");
	if (SM_SendMessage(state, 6, 0x1b, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

/*******************************/
/* SMS/PICTURE/FOLDER HANDLING */
/*******************************/

/* handle messages of type 0x14 (SMS Handling, Folders, Logos.. */
static GSM_Error P6510_IncomingFolder(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int i, j, status;

	switch (message[3]) {
	/* getsms */
	case 0x03:
		dprintf("Trying to get message # %i in folder # %i\n", message[9], message[7]);
		if (!data->SMSMessage) return GE_INTERNALERROR;
		status = data->SMSMessage->Status;
		memset(data->SMSMessage, 0, sizeof(GSM_SMSMessage));
		data->SMSMessage->Status = status;

		/* Number of SMS in folder */
		data->SMSMessage->Number = (message[8] << 8) | message[9];

		/* MessageType/FolderID */
		data->SMSMessage->MemoryType = message[7];

		if (!data->RawData) return GE_INTERNALERROR;
		memset(data->RawData, 0, sizeof(GSM_RawData));

		if (data->SMSMessage->MemoryType == 5) {	/* Ugly hack to handle different headers */
			if (message[14] == 0xa0) {
				if (data->SMSMessage->Number < 21) message[14] = 0x09;	/* picture template */
			} else
				message[14] = 0x08;	/* text template */
		}
		if (message[14] == 0xa0) message[14] = 0x07;
		if (data->SMSMessage->Status == SMS_Sent) message[14] = 0x0A;
		/* Skip the frame header */
		data->RawData->Length = length - nk6510_layout.ReadHeader;
		data->RawData->Data = calloc(data->RawData->Length, 1);
		memcpy(data->RawData->Data, message + nk6510_layout.ReadHeader, data->RawData->Length);

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
	case 0x0d:
		dprintf("Message: SMS Folder status received\n" );
		if (!data->SMSFolder) return GE_INTERNALERROR;
		memset(data->SMSFolder, 0, sizeof(SMS_Folder));
		data->SMSFolder->number = (message[6] << 8) | message[7];
		dprintf("Message: Number of Entries: %i\n" , data->SMSFolder->number);
		dprintf("Message: IDs of Entries : ");
		for (i = 0; i < data->SMSFolder->number; i++) {
			data->SMSFolder->locations[i] = (message[(i * 2) + 8] << 8) | message[(i * 2) + 9];
			dprintf("%d, ", data->SMSFolder->locations[i]);
		}
		dprintf("\n");
		break;
	/* get message status ? */
	case 0x0f:
		dprintf("Message: SMS message(%i in folder %i) status received!\n", 
			(message[10] << 8) | message[11],  message[12]);

		if (!data->SMSMessage) return GE_INTERNALERROR;

		/* Short Message status */
		data->SMSMessage->Status = message[13];

		break;

	/* getfolders */
	case 0x13:
		if (!data->SMSFolderList) return GE_INTERNALERROR;
		memset(data->SMSFolderList, 0, sizeof(SMS_FolderList));

		data->SMSFolderList->number = message[5];
		dprintf("Message: %d SMS Folders received:\n", data->SMSFolderList->number);

		for (j = 0; j < data->SMSFolderList->number; j++) {
			int len;
			strcpy(data->SMSFolderList->Folder[j].Name, "               ");

			i = 10 + (j * 40);
			data->SMSFolderList->FolderID[j] = message[i - 2];
			dprintf("Folder(%i) name: ", message[i - 2]);
			len = message[i - 1];
			DecodeUnicode(data->SMSFolderList->Folder[j].Name, message + i, len);
			dprintf("%s\n", data->SMSFolderList->Folder[j].Name);
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


static GSM_Error P6510_GetSMSFolders(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x12, 0x00, 0x00};

	dprintf("Getting SMS Folders...\n");
	if (SM_SendMessage(state, 6, P6510_MSG_FOLDER, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static GSM_Error P6510_GetSMSFolderStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0C, 
			       0x01, /* 0x01 Inbox, 0x02 others*/
			       0x00, /* Folder ID */
			       0x0F, 0x55, 0x55, 0x55};

	req[5] = GetMemoryType(data->SMSFolder->FolderID);

	if (req[5] == P6510_MEMORY_IN) 
		req[4] = 0x01;
	else
		req[4] = 0x02;

	dprintf("Getting SMS Folder (%i) status (%i)...\n", req[5], req[4]);

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static GSM_Error P6510_GetSMSMessageStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0E, 
			       0x01, /* 0x01 Inbox, 0x02 others*/
			       0x00, /* Folder ID */
			       0x00, 
			       0x00, /* Location */
			       0x55, 0x55};

	dprintf("Getting SMS message (%i) status (%i)...\n", data->SMSMessage->Number, data->SMSMessage->MemoryType);

	req[5] = GetMemoryType(data->SMSMessage->MemoryType);
	if (req[5] == P6510_MEMORY_IN) 
		req[4] = 0x01;
	else
		req[4] = 0x02;
	req[7] = data->SMSMessage->Number;

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}

static GSM_Error P6510_GetSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02,
				   0x01, /* 0x01 for INBOX, 0x02 for others */
				   0x00, /* FolderID */
				   0x00,
				   0x02, /* Location */
				   0x01, 0x00};
	GSM_Error error;

	/* see if the message we want is from the last read folder, i.e. */
	/* we don't have to get folder status again */

       	if ((data->SMSFolder) && (data->SMSMessage->MemoryType != data->SMSFolder->FolderID)) {
		dprintf("Getting list of SMS folders...\n");
		if ((error = P6510_GetSMSFolders(data, state)) != GE_NONE) return error;
		
		if (GetMemoryType(data->SMSMessage->MemoryType) > 
		    data->SMSFolderList->FolderID[data->SMSFolderList->number-1])
			return GE_INVALIDMEMORYTYPE;

		data->SMSFolder->FolderID = data->SMSMessage->MemoryType;

		dprintf("Getting entries for SMS folder %i...\n", data->SMSMessage->MemoryType);
		if ((error = P6510_GetSMSFolderStatus(data, state)) != GE_NONE) return error;
	}

	error = P6510_GetSMSMessageStatus(data, state);

	dprintf("Getting SMS...\n");

	req[5] = GetMemoryType(data->SMSMessage->MemoryType);

	if (req[5] == P6510_MEMORY_IN) 
		req[4] = 0x01;
	else
		req[4] = 0x02;

	req[7] = data->SMSMessage->Number;

	if (SM_SendMessage(state, 10, P6510_MSG_FOLDER, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_FOLDER);
}


/****************/
/* SMS HANDLING */
/****************/

static GSM_Error P6510_IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_Error	e = GE_NONE;
	int		digits, bytes;
	
	dprintf("Frame of type 0x02 (SMS handling) received!\n");

	if (!data) return GE_INTERNALERROR;

	switch (message[3]) {
	case P6510_SUBSMS_SMSC_RCVD: /* 0x34 */
		dprintf("SMSC Received\n");
		/* FIXME: Implement all these in gsm-sms.c */
		data->MessageCenter->No = message[8];
		data->MessageCenter->Format = message[4];
		data->MessageCenter->Validity = message[12];  /* due to changes in format */
		digits = message[15];
		bytes = message[18] - 1;

		sprintf(data->MessageCenter->Name, "%s", message + 33);
		data->MessageCenter->DefaultName = -1;	/* FIXME */

		snprintf(data->MessageCenter->Recipient, sizeof(data->MessageCenter->Recipient), "%s", GetBCDNumber(message + 30));
		snprintf(data->MessageCenter->Number, sizeof(data->MessageCenter->Number), "%s", GetBCDNumber(message + 18));
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

	case P6510_SUBSMS_SMS_SENT: /* 0x02 */
		dprintf("SMS sent\n");
		e = GE_SMSSENDOK;
		break;

	case P6510_SUBSMS_SEND_FAIL: /* 0x03 */
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
		data->RawData->Length = length - nk6510_layout.ReadHeader;
		data->RawData->Data = calloc(data->RawData->Length, 1);
		ParseSMS(data, nk6510_layout.ReadHeader);
		free(data->RawData->Data);
		break;

	case P6510_SUBSMS_SMS_RCVD: /* 0x10 */
	case P6510_SUBSMS_CELLBRD_OK: /* 0x21 */
	case P6510_SUBSMS_CELLBRD_FAIL: /* 0x22 */
	case P6510_SUBSMS_READ_CELLBRD: /* 0x23 */
	case P6510_SUBSMS_SMSC_OK: /* 0x31 */
	case P6510_SUBSMS_SMSC_FAIL: /* 0x32 */
	case P6510_SUBSMS_SMSC_RCVFAIL: /* 0x35 */
		dprintf("Subtype 0x%02x of type 0x%02x (SMS handling) not implemented\n", message[3], P6510_MSG_SMS);
		return GE_NOTIMPLEMENTED;

	default:
		dprintf("Unknown subtype of type 0x%02x (SMS handling): 0x%02x\n", P6510_MSG_SMS, message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return e;
}

static GSM_Error P6510_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, P6510_SUBSMS_GET_SMSC, 0x01, 0x00};

	req[4] = data->MessageCenter->No;
	if (SM_SendMessage(state, 6, P6510_MSG_SMS, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_SMS);
}


/**********************/
/* PHONEBOOK HANDLING */
/**********************/

static GSM_Error P6510_IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data)
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
				data->MemoryStatus->Used = (message[20] << 8) + message[21];
				data->MemoryStatus->Free = ((message[18] << 8) + message[19]) - data->MemoryStatus->Used;
				dprintf("Memory status - location = %d, Capacity: %d \n",
					(message[4] << 8) + message[5], (message[18] << 8) + message[19]);
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
				return GE_UNKNOWN;
			}
		}
		dprintf("Received phonebook info\n");
		blocks     = message[21];
		blockstart = message + 22;
		subblockcount = 0;

		for (i = 0; i < blocks; i++) {
			if (data->PhonebookEntry)
				subEntry = &data->PhonebookEntry->SubEntries[subblockcount];
			dprintf("Blockstart: %i\n", blockstart[0]);
			switch(blockstart[0]) {
			case P6510_ENTRYTYPE_POINTER:	/* Pointer */
				switch (message[11]) {	/* Memory type */
				case P6510_MEMORY_SPEEDDIALS:	/* Speed dial numbers */
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
			case P6510_ENTRYTYPE_NAME:	/* Name */
				if (data->Bitmap) {
					DecodeUnicode(data->Bitmap->text, (blockstart + 6), blockstart[5] / 2);
					dprintf("Name: %s\n", data->Bitmap->text);
				} else if (data->PhonebookEntry) {
					DecodeUnicode(data->PhonebookEntry->Name, (blockstart + 6), blockstart[5] / 2);
					data->PhonebookEntry->Empty = false;
					dprintf("   Name: %s\n", data->PhonebookEntry->Name);
				}
				break;
			case P6510_ENTRYTYPE_EMAIL:
			case P6510_ENTRYTYPE_POSTAL:
			case P6510_ENTRYTYPE_NOTE:
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
			case P6510_ENTRYTYPE_NUMBER:
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
			case P6510_ENTRYTYPE_RINGTONE:	/* Ringtone */
				if (data->Bitmap) {
					data->Bitmap->ringtone = blockstart[5];
					dprintf("Ringtone no. %d\n", data->Bitmap->ringtone);
				}
				break;
			case P6510_ENTRYTYPE_DATE:
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
			case P6510_ENTRYTYPE_LOGO:	 /* Caller group logo */
				if (data->Bitmap) {
					dprintf("Caller logo received (h: %i, w: %i)!\n", blockstart[5], blockstart[6]);
					data->Bitmap->width = blockstart[5];
					data->Bitmap->height = blockstart[6];
					data->Bitmap->size = (data->Bitmap->width * data->Bitmap->height) / 8;
					memcpy(data->Bitmap->bitmap, blockstart + 10, data->Bitmap->size);
					dprintf("Bitmap: width: %i, height: %i\n", blockstart[5], blockstart[6]);
				}
				break;
			case P6510_ENTRYTYPE_LOGOSWITCH:/* Logo on/off */
				break;
			case P6510_ENTRYTYPE_GROUP:	/* Caller group number */
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
	default:
		dprintf("Unknown subtype of type 0x03 (%d)\n", message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}

static GSM_Error P6510_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x02, 0x00, 0x55, 0x55, 0x55, 0x00};
	/* 00 01 00 07 00 00 FE 00 */
	/* #$03#$02+chr(abook)+#$55#$55#$55#$00,1,false) */

	dprintf("Getting memory status...\n");

	req[5] = GetMemoryType(data->MemoryStatus->MemoryType);
	if (SM_SendMessage(state, 10, P6510_MSG_PHONEBOOK, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}


static GSM_Error P6510_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x04, 0x01, 0x00, 0x01,
					0x02, 0x05, /* memory type */ //02,05
					0x00, 0x00, 0x00, 0x00, 
				         0x00, 0x01, /*location */
				         0x00, 0x00};

	/*       00 01 00 07 01 01 00 01 
		FE 10 
		00 00 
		00 00 00 04 00 00    get caller group number

		00 01 00 07 04 01 00 01 
		02 05 
		00 00 
		00 00 00 01 00 00   get phonebook
	*/

	dprintf("Reading phonebook location (%d)\n", data->PhonebookEntry->Location);

	req[9] = GetMemoryType(data->PhonebookEntry->MemoryType);
	req[14] = data->PhonebookEntry->Location >> 8;
	req[15] = data->PhonebookEntry->Location & 0xff;

	if (SM_SendMessage(state, 20, P6510_MSG_PHONEBOOK, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}

static GSM_Error GetCallerBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07,
			       0x01, 0x01, 0x00, 0x01,
			       0xFE, 0x10,
			       0x00, 0x00, 0x00, 0x00, 0x00, 
			       0x03,  /* location */
			       0x00, 0x00};

	/* You can only get logos which have been altered, the standard */
	/* logos can't be read!! */

	req[15] = GNOKII_MIN(data->Bitmap->number + 1, GSM_MAX_CALLER_GROUPS);
	dprintf("Getting caller(%d) logo...\n", req[15]);

	if (SM_SendMessage(state, 18, P6510_MSG_PHONEBOOK, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_PHONEBOOK);
}


/******************/
/* CLOCK HANDLING */
/******************/

static GSM_Error P6510_IncomingClock(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_Error error = GE_NONE;

	dprintf("Incoming clock!\n");
	if (!data || !data->DateTime) return GE_INTERNALERROR;
	switch (message[3]) {
	case P6510_SUBCLO_DATE_RCVD:
		data->DateTime->Year = (((unsigned int)message[8]) << 8) + message[9];
		data->DateTime->Month = message[10];
		data->DateTime->Day = message[11];
		data->DateTime->Hour = message[12];
		data->DateTime->Minute = message[13];
		data->DateTime->Second = message[14];

		break;
	case P6510_SUBCLO_ALARM_RCVD:
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
			error = GE_UNKNOWN;
			break;
		}

		data->DateTime->Hour = message[9];
		data->DateTime->Minute = message[10];

		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (clock handling): 0x%02x\n", P6510_MSG_CLOCK, message[3]);
		error = GE_UNHANDLEDFRAME;
		break;
	}
	return error;
}

static GSM_Error P6510_GetClock(char req_type, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, req_type};

	if (SM_SendMessage(state, 4, P6510_MSG_CLOCK, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_CLOCK);
}

/*********************/
/* CALENDAR HANDLING */
/********************/

static GSM_Error P6510_GetNoteAlarm(int alarmdiff, GSM_DateTime *time, GSM_DateTime *alarm)
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


static GSM_Error P6510_GetNoteTimes(unsigned char *block, GSM_CalendarNote *c)
{
	time_t		alarmdiff;
	GSM_Error	e = GE_NONE;

	if (!c) return GE_INTERNALERROR;

	c->Time.Hour = block[0];
	c->Time.Minute = block[1];
	c->Recurrence = ((((unsigned int)block[4]) << 8) + block[5]) * 60;
	alarmdiff = (((unsigned int)block[2]) << 8) + block[3];

	if (alarmdiff != 0xffff) {
		e = P6510_GetNoteAlarm(alarmdiff * 60, &(c->Time), &(c->Alarm));
		c->Alarm.AlarmEnabled = 1;
	} else {
		c->Alarm.AlarmEnabled = 0;
	}

	return e;
}

static GSM_Error P6510_IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_Error			e = GE_NONE;
	unsigned char			*block;
	int				i, alarm, year;

	if (!data || !data->CalendarNote) return GE_INTERNALERROR;

	year = 	data->CalendarNote->Time.Year;
	dprintf("Year: %i\n", data->CalendarNote->Time.Year);
	switch (message[3]) {
	case P6510_SUBCAL_NOTE_RCVD:
		block = message + 12;

		data->CalendarNote->Location = (((unsigned int)message[4]) << 8) + message[5];
		data->CalendarNote->Time.Year = (((unsigned int)message[8]) << 8) + message[9];
		data->CalendarNote->Time.Month = message[10];
		data->CalendarNote->Time.Day = message[11];
		data->CalendarNote->Time.Second = 0;

		dprintf("Year: %i\n", data->CalendarNote->Time.Year);

		switch (message[6]) {
		case P6510_NOTE_MEETING:
			data->CalendarNote->Type = GCN_MEETING;
			P6510_GetNoteTimes(block, data->CalendarNote);
			DecodeUnicode(data->CalendarNote->Text, (block + 8), block[6]);
			break;
		case P6510_NOTE_CALL:
			data->CalendarNote->Type = GCN_CALL;
			P6510_GetNoteTimes(block, data->CalendarNote);
			DecodeUnicode(data->CalendarNote->Text, (block + 8), block[6]);
			DecodeUnicode(data->CalendarNote->Phone, (block + 8 + block[6] * 2), block[7]);
			break;
		case P6510_NOTE_REMINDER:
			data->CalendarNote->Type = GCN_REMINDER;
			data->CalendarNote->Recurrence = ((((unsigned int)block[0]) << 8) + block[1]) * 60;
			DecodeUnicode(data->CalendarNote->Text, (block + 4), block[2]);
			break;
		case P6510_NOTE_BIRTHDAY:

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

			P6510_GetNoteAlarm(alarm, &(data->CalendarNote->Time), &(data->CalendarNote->Alarm));

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

	case P6510_SUBCAL_ADD_MEETING_RESP:
	case P6510_SUBCAL_ADD_CALL_RESP:
	case P6510_SUBCAL_ADD_BIRTHDAY_RESP:
	case P6510_SUBCAL_ADD_REMINDER_RESP:
	case P6510_SUBCAL_DEL_NOTE_RESP:
	case P6510_SUBCAL_FREEPOS_RCVD:
		dprintf("Subtype 0x%02x of type 0x%02x (calendar handling) not implemented\n", message[3], P6510_MSG_CALENDAR);
		return GE_NOTIMPLEMENTED;
	default:
		dprintf("Unknown subtype of type 0x%02x (calendar handling): 0x%02x\n", P6510_MSG_CALENDAR, message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return e;
}


static GSM_Error P6510_GetCalendarNotesInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, P6510_SUBCAL_GET_INFO, 0xFF, 0xFE};
	dprintf("Getting calendar notes info...\n");
	if (SM_SendMessage(state, 6, P6510_MSG_CALENDAR, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_CALENDAR);
}

static GSM_Error P6510_GetCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error	error = GE_NOTREADY;
	unsigned char	req[] = {FBUS_FRAME_HEADER, P6510_SUBCAL_GET_NOTE, 0x00, 0x00};
	unsigned char	date[] = {FBUS_FRAME_HEADER, P6510_SUBCLO_GET_DATE};
	GSM_Data	tmpdata;
	GSM_DateTime	tmptime;

	dprintf("Getting calendar note...\n");
	tmpdata.DateTime = &tmptime;
	if (P6510_GetCalendarNotesInfo(data, state) == GE_NONE) {
		if (data->CalendarNote->Location < data->CalendarNotesList->Number + 1 &&
		    data->CalendarNote->Location > 0 ) {
			if (SM_SendMessage(state, 4, P6510_MSG_CLOCK, date) == GE_NONE) {
				SM_Block(state, &tmpdata, P6510_MSG_CLOCK);
				req[4] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] >> 8;
				req[5] = data->CalendarNotesList->Location[data->CalendarNote->Location - 1] & 0xff;
				data->CalendarNote->Time.Year = tmptime.Year;

				if (SM_SendMessage(state, 6, P6510_MSG_CALENDAR, req) == GE_NONE) {
					error = SM_Block(state, data, P6510_MSG_CALENDAR);
				}
			}
		}
	}

	return error;
}

/*********************/
/* SECURITY COMMANDS */
/*********************/

static GSM_Error P6510_IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (!data || !data->NetMonitor) return GE_INTERNALERROR;
	switch (message[3]) {
	case P6510_SUBSEC_NETMONITOR:
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
		dprintf("Unknown subtype of type 0x%02x (Security): 0x%02x\n", P6510_MSG_SECURITY, message[3]);
		return GE_UNHANDLEDFRAME;
	}
	return GE_NONE;
}

static GSM_Error P6510_EnableExtendedCmds(GSM_Data *data, GSM_Statemachine *state, unsigned char type)
{
	unsigned char req[] = {0x00, 0x01, 0x64, 0x00};

	dprintf("Enabling extended commands...\n");

	if (type == 0x06) {
		dump(_("Tried to activate CONTACT SERVICE\n"));
		return GE_INTERNALERROR;
	}

       	req[3] = type;

	if (SM_SendMessage(state, 4, 0x42, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x42);
}

static GSM_Error P6510_NetMonitor(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0xc8, 0x01}; /* P6510_SUBSEC_NETMONITOR};*/
	GSM_Error error;

	//	req2[4] = data->NetMonitor->Field;
	error = P6510_EnableExtendedCmds(data, state, 0x01);
       	if (SM_SendMessage(state, 4, 0x42, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x42);
}

/********************/
/* INCOMING NETWORK */
/********************/

static GSM_Error P6510_IncomingNetwork(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	unsigned char *blockstart;
	int i;

	switch (message[3]) {
	case 0x01:
		blockstart = message + 6;
		for (i = 0; i < message[5]; i++) {
			dprintf("Blockstart: %i\n", blockstart[0]);
			switch (blockstart[0]) {
			case 0x09:  /* Operator details */
				/* Network code is stored as 0xBA 0xXC 0xED ("ABC DE"). */
				if (data->NetworkInfo) {
					/* Is this correct? */
					/* 01 56 00 01 00 02 || 00 0C 00 02 00 02 00 44 00 32 00 00 || 
					   09 14 02 05 00 00 50 53 62 F2 20 00 00 01 02 01 00 00 00 00
					   FIXME: What is the first block about? */
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
	case 0x1E:
		if (data->RFLevel) {
			*(data->RFUnits) = GRF_Percentage;
			*(data->RFLevel) = message[5];
			dprintf("RF level %f\n\n\n",*(data->RFLevel));
		}
		break;
	case 0x24:
		if (data->Bitmap) {
			int x;
			data->Bitmap->netcode[0] = '0' + (message[12] & 0x0f);
			data->Bitmap->netcode[1] = '0' + (message[12] >> 4);
			data->Bitmap->netcode[2] = '0' + (message[13] & 0x0f);
			data->Bitmap->netcode[3] = ' ';
			data->Bitmap->netcode[4] = '0' + (message[14] & 0x0f);
			data->Bitmap->netcode[5] = '0' + (message[14] >> 4);
			data->Bitmap->netcode[6] = 0;
			dprintf("Operator %s ",data->Bitmap->netcode);
			data->Bitmap->type = GSM_NewOperatorLogo;
			data->Bitmap->height = message[21];
			data->Bitmap->width = message[20];
			x = message[20] * message[21];
			data->Bitmap->size = (x / 8) + (x % 8 > 0);
			dprintf("size: %i\n", data->Bitmap->size);
			memcpy(data->Bitmap->bitmap, message + 26, data->Bitmap->size);
			dprintf("Logo (%dx%d) ", data->Bitmap->height, data->Bitmap->width);
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

static GSM_Error P6510_GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x00};

	dprintf("Getting network info ...\n");
	if (SM_SendMessage(state, 5, P6510_MSG_NETSTATUS, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_NETSTATUS);
}

static GSM_Error P6510_GetRFLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0B, 0x00, 0x01, 0x00, 0x00, 0x00};
	/* 00 01 00 0B 00 01 00 00 00 00 
	   00 01 00 02 01 00 00 04 01 00
	*/
	dprintf("Getting network info ...\n");
	if (SM_SendMessage(state, 9, P6510_MSG_NETSTATUS, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_NETSTATUS);
}

static GSM_Error GetOperatorBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x23, 0x00, 0x00, 0x55, 0x55, 0x55};
	/*  00 01 00 23 00 00 55 55 55 */


	dprintf("Getting op logo...\n");
	if (SM_SendMessage(state, 9, P6510_MSG_NETSTATUS, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_NETSTATUS);
}

/*********************/
/* INCOMING BATTERY */
/*********************/

static GSM_Error P6510_IncomingBattLevel(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x0B:
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

static GSM_Error P6510_GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0A, 0x02, 0x00};

	dprintf("Getting battery level...\n");
	if (SM_SendMessage(state, 6, P6510_MSG_BATTERY, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_BATTERY);
}


/*************/
/* RINGTONES */
/*************/

static GSM_Error P6510_IncomingRingtone(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int i, j, index;

	switch (message[3]) {
	case 0x08:
		dprintf("List of ringtones received!\n");
		index = 13;
		for (j = 0; j < message[7]; j++) {

			dprintf("Ringtone (#%i) name (length: %i): ", message[index - 4], message[index]);
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
		return GE_UNKNOWN;
	}
	return GE_NONE;
}

static GSM_Error P6510_GetRingtones(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00, 0x00, 0xFE, 0x00, 0x7D};

	dprintf("Getting list of ringtones...\n");
	if (SM_SendMessage(state, 9, P6510_MSG_RINGTONE, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, P6510_MSG_RINGTONE);
}

/********************************/
/* NOT FRAME SPECIFIC FUNCTIONS */
/********************************/

static GSM_Error P6510_GetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	switch(data->Bitmap->type) {
	case GSM_CallerLogo:
		return GetCallerBitmap(data, state);
	case GSM_OperatorLogo:
		return GetOperatorBitmap(data, state);
		/*	case GSM_StartupLogo:
		return GetStartupBitmap(data, state);
		*/
	default:
		return GE_NOTIMPLEMENTED;
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
