/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>
  Copyright (C) 2002 BORBELY Zoltan

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 6100 series. 
  See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk6100.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

/* Some globals */

static const SMSMessage_Layout nk6100_deliver = {
	true,
	 4, true, true,
	-1,  7, -1, -1,  2, -1, -1, -1, 19, 18, 16,
	-1, -1, -1,
	20, true, true,
	32, -1,
	 1,  0,
	39, true
};

static const SMSMessage_Layout nk6100_submit = {
	true,
	 4, true, true,
	-1, 16, 16, 16,  2, 17, 18, -1, 20, 19, 16,
	16, -1, -1,
	21, true, true,
	33, -1,
	-1, -1,
	40, true
};

static const SMSMessage_Layout nk6100_delivery_report = {
	true,
	 4, true, true,
	-1, -1, -1, -1,  2, -1, -1, -1, 18, 17, 16,
	-1, -1, -1,
	20, true, true,
	31, 38,
	 1,  0,
	18, true
};

static const SMSMessage_Layout nk6100_picture = {
	false,
	-1, true, true,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1,
	-1, true, true,
	-1, -1,
	-1, -1,
	-1, true
};

static SMSMessage_PhoneLayout nk6100_layout;

static unsigned char MagicBytes[4] = { 0x00, 0x00, 0x00, 0x00 };

/* static functions prototypes */
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Initialise(GSM_Statemachine *state);
static GSM_Error GetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error SetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetModelName(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state);
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
static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingModelInfo(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingProfile(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error Incoming0x17(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingPhoneClockAndAlarm(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error IncomingDisplay(int messagetype, unsigned char *message, int length, GSM_Data *data);

static int GetMemoryType(GSM_MemoryType memory_type);

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 0x04, IncomingPhoneStatus },
	{ 0x0a, IncomingNetworkInfo },
	{ 0x03, IncomingPhonebook },
	{ 0x05, IncomingProfile },
	{ 0x11, IncomingPhoneClockAndAlarm },
	{ 0x13, IncomingCalendar },
	{ 0x0d, IncomingDisplay },

	{ 0x64, IncomingPhoneInfo },
	{ 0xd2, IncomingModelInfo },
	{ 0x14, IncomingSMS },
	{ 0x17, Incoming0x17 },
	{ 0, NULL}
};

GSM_Phone phone_nokia_6100 = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"6110|6130|6150|6190|5110|5130|5190|3210|3310|3330|8210", /* Supported models */
		4,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Arbitrary,         /* RF level units */
		4,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Arbitrary,         /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		48, 84,                /* Startup logo size - 7110 is fixed at init*/
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
	case GOP_GetSpeedDial:
		return GetSpeedDial(data, state);
	case GOP_SetSpeedDial:
		return SetSpeedDial(data, state);
	case GOP_GetModel:
		return GetModelName(data, state);
	case GOP_GetRevision:
		return GetRevision(data, state);
	case GOP_GetImei:
		return GetIMEI(data, state);
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
	case GOP_GetSMS:
		return GetSMSMessage(data, state);
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
	default:
		return GE_NOTIMPLEMENTED;
	}
}

/* static bool LinkOK = true; */

/* Initialise is the only function allowed to 'use' state */
static GSM_Error Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];
	GSM_Error err;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_6100, sizeof(GSM_Phone));

	/* SMS Layout */
	nk6100_layout.Type = 7;
	nk6100_layout.SendHeader = 0;
	nk6100_layout.ReadHeader = 4;
	nk6100_layout.Deliver = nk6100_deliver;
	nk6100_layout.Submit = nk6100_submit;
	nk6100_layout.DeliveryReport = nk6100_delivery_report;
	nk6100_layout.Picture = nk6100_picture;
	layout = nk6100_layout;

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
	case GCT_Infrared:
		err = FBUS_Initialise(&(state->Link), state, 0);
		break;
	default:
		return GE_NOTSUPPORTED;
	}

	if (err != GE_NONE) {
		dprintf("Error in link initialisation\n");
		return GE_NOTSUPPORTED;
	}
	
	SM_Initialise(state);

	/* Now test the link and get the model */
	GSM_DataClear(&data);
	data.Model = model;
	if (state->Phone.Functions(GOP_GetModel, &data, state) != GE_NONE) return GE_NOTSUPPORTED;
  	return GE_NONE;
}

static GSM_Error GetPhoneInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x00};

	dprintf("Getting phone info...\n");
	if (SM_SendMessage(state, 5, 0xd1, req) != GE_NONE) return GE_NOTREADY;
	return (SM_Block(state, data, 0xd2));
}

static GSM_Error GetModelName(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting model...\n");
	return GetPhoneInfo(data, state);
}

static GSM_Error GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting revision...\n");
	return GetPhoneInfo(data, state);
}

static GSM_Error GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};
  
	dprintf("Getting imei...\n");
	if (SM_SendMessage(state, 4, 0x1b, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
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
	switch (message[3]) {
	/* Phone status */
	case 0x02:
		dprintf("\tRFLevel: %d\n", message[5]);
		if (data->RFLevel) {
			*(data->RFUnits) = GRF_Arbitrary;
			*(data->RFLevel) = message[5];
		}
		dprintf("\tPowerSource: %d\n", message[7]);
		if (data->PowerSource) {
			*(data->PowerSource) = message[7];
		}
		dprintf("\tBatteryLevel: %d\n", message[8]);
		if (data->BatteryLevel) {
			*(data->BatteryUnits) = GBU_Arbitrary;
			*(data->BatteryLevel) = message[8];
		}
		break;
	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}


static GSM_Error Incoming0x17(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x03:
		if (data->BatteryLevel) { 
			*(data->BatteryUnits) = GBU_Percentage;
			*(data->BatteryLevel) = message[5];
			dprintf("Battery level %f\n", *(data->BatteryLevel));
		}
		return GE_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x17 (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}
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
	case GSM_PictureImage:
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

static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x10 };

	dprintf("Identifying...\n");
	PNOK_GetManufacturer(data->Manufacturer);
	if (SM_SendMessage(state, 4, 0x64, req) != GE_NONE) return GE_NOTREADY;
	SM_WaitFor(state, data, 0x64);
       	SM_Block(state, data, 0x64); /* waits for all requests */
	SM_GetError(state, 0x64);
	
	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GE_UNKNOWN;
	return GE_NONE;
}

static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (data->Imei) { 
		snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 4);
		dprintf("Received imei %s\n", data->Imei);
	}
	if (data->Model) { 
		snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 22);
		dprintf("Received model %s\n", data->Model);
	}
	if (data->Revision) { 
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "%s", message + 7);
		dprintf("Received revision %s\n", data->Revision);
	}

	dprintf("Message: Mobile phone identification received:\n");
	dprintf("\tIMEI: %s\n", data->Imei);
	dprintf("\tModel: %s\n", data->Model);
	dprintf("\tProduction Code: %s\n", message + 31);
	dprintf("\tHW: %s\n", message + 39);
	dprintf("\tFirmware: %s\n", message + 44);
	
	/* These bytes are probably the source of the "Accessory not connected"
	   messages on the phone when trying to emulate NCDS... I hope....
	   UPDATE: of course, now we have the authentication algorithm. */
	dprintf("\tMagic bytes: %02x %02x %02x %02x\n", message[50], message[51], message[52], message[53]);
	
	MagicBytes[0] = message[50];
	MagicBytes[1] = message[51];
	MagicBytes[2] = message[52];
	MagicBytes[3] = message[53];

	return GE_NONE;
}

static GSM_Error IncomingModelInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("%p %p %p\n", data, data->Model, data->Revision);
	if (data->Model) {
		snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 21);
		data->Model[GSM_MAX_MODEL_LENGTH - 1] = 0;
	}
	if (data->Revision) {
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW%s", message + 5);
		data->Revision[GSM_MAX_REVISION_LENGTH - 1] = 0;
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

static GSM_Error GetSMSStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x36, 0x64 };

	if (SM_SendMessage(state, 5, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x07, 0x02 /* Unknown */, 0x00 /* Location */, 0x01, 0x64};

	req[5] = data->SMSMessage->Number;

	if (SM_SendMessage(state, 8, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int i;

	switch (message[3]) {
	/* save sms succeeded */
	case 0x05:
		dprintf("Message stored at %d\n", message[5]);
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
			return GE_UNKNOWN;
		}
        /* read sms */
        case 0x08:
		for (i = 0; i < length; i++)
			dprintf("[%02x(%d)]", message[i], i);
		dprintf("\n");

		memset(data->SMSMessage, 0, sizeof(GSM_SMSMessage));

		/* Short Message status */
		data->SMSMessage->Status = message[4];
		dprintf("\tStatus: ");
		switch (data->SMSMessage->Status) {
		case SMS_Read:
			dprintf("READ\n");
			break;
		case SMS_Unread:
			dprintf("UNREAD\n");
			break;
		case SMS_Sent:
			dprintf("SENT\n");
			break;
		case SMS_Unsent:
			dprintf("UNSENT\n");
			break;
		default:
			dprintf("UNKNOWN\n");
			break;
		}

		/* Short Message status */
		if (!data->RawData) {
			data->RawData = (GSM_RawData *)malloc(sizeof(GSM_RawData));
		}

		/* Skip the frame header */
//		data->RawData->Data = message + nk6100_layout.ReadHeader;
		data->RawData->Length = length - nk6100_layout.ReadHeader;
		data->RawData->Data = malloc(data->RawData->Length);
		memcpy(data->RawData->Data, message + nk6100_layout.ReadHeader, data->RawData->Length);
		dprintf("Everything set. Length: %d\n", data->RawData->Length);

                break;
	/* read sms failed */
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
			return GE_UNKNOWN;
		}
	/* delete sms succeeded */
	case 0x0b:
		dprintf("Message: SMS deleted successfully.\n");
		break;
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
		return GE_UNKNOWN;
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
	case GSM_PictureImage:
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
		if ((error = GetModelName(&d, state)) != GE_NONE) {
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

static GSM_Error DisplayOutput(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x53, 0x00};

	req[4] = data->OutputFn ? 0x01 : 0x02;

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
	int n;

	switch (message[3]) {
	/* Display output */
	case 0x50:
		if (data->OutputFn) {
			switch (message[4]) {
			case 0x01:
				break;
			default:
				return GE_UNHANDLEDFRAME;
			}
			pos = message + 5;
			dprintf("(x,y): %d/%d, len: %d, data: ", pos[1], pos[0], pos[2]);
			n = pos[2];
			pos += 3;
			for ( ; n > 0; n--, pos += 2) dprintf("%c", pos[1]);
			dprintf("\n");
		}
		break;

	/* Display status */
	case 0x52:
		if (data->DisplayStatus) {
			*data->DisplayStatus = 0;
			pos = message + 4;
			for ( n = *pos++; n > 0; n--, pos += 2) {
				if ((pos[0] < 1) || (pos[0] > 8))
					return GE_UNHANDLEDFRAME;
				if (pos[1] == 0x02)
					*data->DisplayStatus |= state_table[pos[0] - 1];
			}
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
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}
