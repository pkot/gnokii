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
  Revision 1.9  2001-06-10 23:49:49  pkot
  Small fixes to hide compilation warnings and allow gnokii.c to compile

  Revision 1.8  2001/05/24 20:47:30  chris
  More updating of 7110 code and some of xgnokii_lowlevel changed over.

  Revision 1.7  2001/05/07 16:24:04  pkot
  DLR-3P temporary fix. How should I do it better?

  Revision 1.6  2001/03/23 13:40:24  chris
  Pavel's patch and a few fixes.

  Revision 1.5  2001/03/22 16:17:06  chris
  Tidy-ups and fixed gnokii/Makefile and gnokii/ChangeLog which I somehow corrupted.

  Revision 1.4  2001/03/21 23:36:06  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.3  2001/03/13 01:24:03  pkot
  Code cleanup - no warnings during compilation

  Revision 1.2  2001/03/13 01:23:18  pkot
  Windows updates (Manfred Jonsson)

  Revision 1.1  2001/02/21 19:57:07  chris
  More fiddling with the directory layout

  Revision 1.1  2001/02/16 14:29:53  chris
  Restructure of common/.  Fixed a problem in fbus-phonet.c
  Lots of dprintfs for Marcin
  Any size xpm can now be loaded (eg for 7110 startup logos)
  nk7110 code detects 7110/6210 and alters startup logo size to suit
  Moved Marcin's extended phonebook code into gnokii.c

  Revision 1.7  2001/02/06 21:15:35  chris
  Preliminary irda support for 7110 etc.  Not well tested!

  Revision 1.6  2001/02/03 23:56:15  chris
  Start of work on irda support (now we just need fbus-irda.c!)
  Proper unicode support in 7110 code (from pkot)

  Revision 1.5  2001/02/01 15:17:31  pkot
  Fixed --identify and added Manfred's manufacturer patch

  Revision 1.4  2001/01/29 17:14:42  chris
  dprintf now in misc.h (and fiddling with 7110 code)

  Revision 1.3  2001/01/23 15:32:41  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.2  2001/01/17 02:54:54  chris
  More 7110 work.  Use with care! (eg it is not possible to delete phonebook entries)
  I can now edit my phonebook in xgnokii but it is 'work in progress'.

  Revision 1.1  2001/01/14 22:46:59  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define __phones_nk7110_c  /* Turn on prototypes in phones/nk7110.h */
#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk7110.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

/* Some globals */

static GSM_IncomingFunctionType P7110_IncomingFunctions[] = {
	{ 0x1b, P7110_Incoming0x1b },
	{ 0x03, P7110_IncomingPhonebook },
	{ 0x0a, P7110_IncomingNetwork },
	{ 0x17, P7110_IncomingBattLevel },
	{ 0x7a, P7110_IncomingStartup },
	{ 0, NULL}
};

GSM_Phone phone_nokia_7110 = {
	P7110_IncomingFunctions,
	PGEN_IncomingDefault,
        /* Mobile phone information */
	{
		"7110|6210", /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Percentage,        /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Percentage,        /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	         /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		60, 96,                /* Startup logo size - 7110 is fixed at init*/
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
		break;
	case GOP_GetModel:
		return P7110_GetModel(data, state);
		break;
	case GOP_GetRevision:
		return P7110_GetRevision(data, state);
		break;
	case GOP_GetImei:
		return P7110_GetIMEI(data, state);
		break;
	case GOP_Identify:
		return P7110_Identify(data, state);
		break;
	case GOP_GetBatteryLevel:
		return P7110_GetBatteryLevel(data, state);
		break;
	case GOP_GetRFLevel:
		return P7110_GetRFLevel(data, state);
		break;
	case GOP_GetMemoryStatus:
		return P7110_GetMemoryStatus(data, state);
		break;
	case GOP_GetBitmap:
		return P7110_GetBitmap(data, state);
		break;
	case GOP_SetBitmap:
		return P7110_SetBitmap(data, state);
		break;
	case GOP_ReadPhonebook:
		return P7110_ReadPhonebook(data, state);
		break;
	case GOP_WritePhonebook:
		return P7110_WritePhonebookLocation(data, state);
		break;
	case GOP_GetNetworkInfo:
		return P7110_GetNetworkInfo(data, state);
		break;
	default:
		return GE_NOTIMPLEMENTED;
		break;
	}
}

/* LinkOK is always true for now... */
bool P7110_LinkOK = true;


/* Initialise is the only function allowed to 'use' state */

static GSM_Error P7110_Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];
	GSM_Error err;
	int try = 0, connected = 0;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_7110, sizeof(GSM_Phone));

	while (!connected) {
		switch (state->Link.ConnectionType) {
		case GCT_Serial:
			if (try > 1) return GE_NOTSUPPORTED;
			err = FBUS_Initialise(&(state->Link), state);
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
			dprintf("Error in link initialisation\n");
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
	if (SM_SendMessage(state, 6, 0x1b, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static GSM_Error P7110_GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};
  
	dprintf("Getting revision...\n");
	if (SM_SendMessage(state, 6, 0x1b, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}

static GSM_Error P7110_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};
  
	dprintf("Getting imei...\n");
	if (SM_SendMessage(state, 4, 0x1b, req)!=GE_NONE) return GE_NOTREADY;
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
		return GE_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x17 (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}
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
				dprintf(_("Unknown operator block %d\n"), blockstart[0]);
				break;
			}
			blockstart += blockstart[1];
		}
		return GE_NONE;
		break;
	case 0x82:
		if (data->RFLevel) { 
			*(data->RFUnits) = GRF_Percentage;
			*(data->RFLevel) = message[4];
			dprintf("RF level %f\n",*(data->RFLevel));
		}
		return GE_NONE;
		break;
	case 0xa4:
		dprintf("Op Logo Set OK\n");
		return GE_NONE;
	default:
		dprintf("Unknown subtype of type 0x0a (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}
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
	int i;
	GSM_SubPhonebookEntry* subEntry = NULL;

	PGEN_DebugMessage(messagetype, message, length);

	switch (message[3]) {
	case 0x04:  /* Get status response */
		if (data->MemoryStatus) {
			if (message[5] != 0xff) {
				data->MemoryStatus->Used = (message[16] << 8) + message[17];
				data->MemoryStatus->Free = ((message[14] << 8) + message[15]) - data->MemoryStatus->Used;
				dprintf(_("Memory status - location = %d\n"), (message[8] << 8) + message[9]);
				return GE_NONE;
			} else {
				dprintf("Unknown error getting mem status\n");
				return GE_NOTIMPLEMENTED;
				
			}
		}
		return GE_NONE;
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
		if (message[6] == 0x0f) { // not found
			if (message[10] == 0x34 || message[10] == 0x33 || message[10] == 0x30) {
				dprintf("Invalid caller location\n");
				return GE_INVALIDPHBOOKLOCATION;
			}
			else {
				dprintf("Unknown error getting phonebook\n");
				return GE_NOTIMPLEMENTED;
			}
		}
		dprintf("Received phonebook info\n");
		blocks     = message[17];        
		blockstart = message + 18;
		subblockcount = 0;
           
		for (i = 0; i < blocks; i++) {
			if (data->PhonebookEntry) subEntry = &data->PhonebookEntry->SubEntries[subblockcount];
			switch (blockstart[0]) {
			case 0x07:                   /* Name */
				if (data->Bitmap) {
					DecodeUnicode(data->Bitmap->text, (blockstart + 6), blockstart[5] / 2);
					dprintf("Name: %s\n",data->Bitmap->text);
				} else if (data->PhonebookEntry) {
					DecodeUnicode(data->PhonebookEntry->Name, (blockstart + 6), blockstart[5] / 2);
					data->PhonebookEntry->Empty = false;
					dprintf(_("   Name: %s\n"), data->PhonebookEntry->Name);
				}
				break;
			case 0x08:
			case 0x09:
			case 0x0a:
				if (data->PhonebookEntry) {
					subEntry->EntryType   = blockstart[0];
					subEntry->NumberType  = 0;
					subEntry->BlockNumber = blockstart[4];
					DecodeUnicode(subEntry->data.Number, (blockstart + 6), blockstart[5] / 2);
					dprintf(_("   Type: %d (%02x)\n"), subEntry->EntryType, subEntry->EntryType);
					dprintf(_("   Text: %s\n"), subEntry->data.Number);
					subblockcount++;
					data->PhonebookEntry->SubEntriesCount++;
				}
				break;
			case 0x0b:
				if (data->PhonebookEntry) {
					subEntry->EntryType   = blockstart[0];
					subEntry->NumberType  = blockstart[5];
					subEntry->BlockNumber = blockstart[4];
					DecodeUnicode(subEntry->data.Number, (blockstart + 10), blockstart[9] / 2);
					if (!subblockcount) strcpy(data->PhonebookEntry->Number, subEntry->data.Number);
					dprintf(_("   Type: %d (%02x)\n"), subEntry->NumberType, subEntry->NumberType);
					dprintf(_(" Number: %s\n"), subEntry->data.Number);					
					subblockcount++;
					data->PhonebookEntry->SubEntriesCount++;
				}
				break;
			case 0x0c:                   /* Ringtone */
				if (data->Bitmap) {
					data->Bitmap->ringtone = blockstart[5];
					dprintf("Ringtone no. %d\n", data->Bitmap->ringtone);
				}
				break;
			case 0x13:
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
					dprintf(_("   Date: %02u.%02u.%04u\n"), subEntry->data.Date.Day,
						subEntry->data.Date.Month, subEntry->data.Date.Year);
					dprintf(_("   Time: %02u:%02u:%02u\n"), subEntry->data.Date.Hour,
						subEntry->data.Date.Minute, subEntry->data.Date.Second);
					subblockcount++;
				}
				break;
			case 0x1b:                   /* Bitmap */
				if (data->Bitmap) {
					data->Bitmap->width = blockstart[5];
					data->Bitmap->height = blockstart[6];
					data->Bitmap->size = (data->Bitmap->width * data->Bitmap->height) / 8;
					memcpy(data->Bitmap->bitmap, blockstart + 10, data->Bitmap->size);
					dprintf("Bitmap\n");
				}
				break;
			case 0x1c:                   /* Graphic on/off */
				break;
			case 0x1e:                   /* Number */
				if (data->PhonebookEntry) {
					data->PhonebookEntry->Group = blockstart[5] - 1;
					dprintf(_("   Group: %d\n"), data->PhonebookEntry->Group);
				}
				break;
			default:
				dprintf(_("Unknown phonebook block %02x\n"), blockstart[0]);
				break;
			}
			blockstart += blockstart[3];
		}
		return GE_NONE;
		break;
	case 0x0c:
		switch (message[6]) {
		case 0x3d: return GE_PHBOOKWRITEFAILED; break;
		case 0x3e: return GE_PHBOOKWRITEFAILED; break;
		default: return GE_NONE; break;	
		}
		break;	
	default:
		dprintf("Unknown subtype of type 0x03 (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}
}


/* Just as an example.... */
/* But note that both requests are the same type which isn't very 'proper' */ 
static GSM_Error P7110_Identify(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};
  
	dprintf("Identifying...\n");
	if (SM_SendMessage(state, 4, 0x1b, req)!=GE_NONE) return GE_NOTREADY;
	if (SM_SendMessage(state, 6, 0x1b, req2)!=GE_NONE) return GE_NOTREADY;
	SM_WaitFor(state, data, 0x1b);
       	SM_Block(state, data, 0x1b); /* waits for all requests - returns req2 error */
	SM_GetError(state, 0x1b);
	
	/* Check that we are back at state Initialised */
	if (SM_Loop(state,0)!=Initialised) return GE_UNKNOWN;
	return GE_NONE;
}


static GSM_Error P7110_Incoming0x1b(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 02:
		if (data->Imei) { 
			snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 4);
			dprintf("Received imei %s\n",data->Imei);
		}
		return GE_NONE;
		break;
	case 04:
		if (data->Model) { 
			snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 22);
			dprintf("Received model %s\n",data->Model);
		}
		if (data->Revision) { 
			snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "%s", message + 7);
			dprintf("Received revision %s\n",data->Revision);
		}
		return GE_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x1b (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}

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
	
	len=17;

	if (PGEN_CommandResponse(&link, req, &len, 0x01, 0x01, 100)==GE_NONE) {
		PGEN_DebugMessage(1, req, len);
//		return GE_NONE;

	}
//	} else return GE_NOTIMPLEMENTED;

 
	while(1){
		link.Loop(NULL);
	}

	return GE_NOTIMPLEMENTED;
}
#endif

static GSM_Error GetCallerBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
				  0x00, 0x10 , /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00};

	req[11] = data->Bitmap->number + 1;
	dprintf("Getting caller(%d) logo...\n",data->Bitmap->number);
	if (SM_SendMessage(state, 14, 0x03, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}


inline unsigned char PackBlock(u8 id, u8 size, u8 no, u8 *buf, u8 *block)
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

	if ((data->Bitmap->width!=state->Phone.Info.CallerLogoW) ||
	    (data->Bitmap->height!=state->Phone.Info.CallerLogoH )) {
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
		return GE_UNKNOWN;
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


	if ((data->Bitmap->width!=state->Phone.Info.StartupLogoW) ||
	    (data->Bitmap->height!=state->Phone.Info.StartupLogoH )) {
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

	if ((data->Bitmap->width!=state->Phone.Info.OpLogoW) ||
	    (data->Bitmap->height!=state->Phone.Info.OpLogoH )) {
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


static GSM_Error P7110_WritePhonebookLocation(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0c,
				  0x00, 0x00,  /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00, 0x00};
	char string[500];
	int block, i, j, defaultn;
	unsigned int count = 18;
	GSM_PhonebookEntry *entry;
	
	if (data->PhonebookEntry) entry=data->PhonebookEntry;
	else return GE_TRYAGAIN;

	req[11] = GetMemoryType(entry->MemoryType);
	req[12] = (entry->Location >> 8);
	req[13] = entry->Location & 0xff;
	block = 1;
	if (*(entry->Name)) {
		/* Name */
		i = strlen(entry->Name);
		EncodeUnicode((string + 1), entry->Name, i);
		string[0] = i * 2;
		count += PackBlock(0x07, i * 2 + 1, block++, string, req + count);
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
			string[1] = 0;
			string[2] = 0;
			string[3] = 0;
			i = strlen(entry->SubEntries[defaultn].data.Number);
			EncodeUnicode((string + 5), entry->SubEntries[defaultn].data.Number, i);
			string[4] = i * 2;
			count += PackBlock(0x0b, i * 2 + 5, block++, string, req + count);
		}
		/* Rest of the numbers */
		for (i = 0; i < entry->SubEntriesCount; i++)
			if (entry->SubEntries[i].EntryType == GSM_Number)
				if (i != defaultn) {
					string[0] = entry->SubEntries[i].NumberType;
					string[1] = 0;
					string[2] = 0;
					string[3] = 0;
					j = strlen(entry->SubEntries[i].data.Number);
					EncodeUnicode((string + 5), entry->SubEntries[i].data.Number, j);
					string[4] = j + 2;
					count += PackBlock(0x0b, j * 2 + 5, block++, string, req + count);
				} 
		req[17] = block - 1;
		dprintf("Writing phonebook entry %s...\n",entry->Name);
	}
	if (SM_SendMessage(state, count, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}



static GSM_Error P7110_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[2000] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
				   0x00, 0x00 , /* memory type */ //02,05
				   0x00, 0x00,  /* location */
				   0x00, 0x00};
     
	req[9] = GetMemoryType(data->PhonebookEntry->MemoryType);
	req[10] = (data->PhonebookEntry->Location >> 8);
	req[11] = data->PhonebookEntry->Location & 0xff;
	dprintf("Reading phonebook location (%d)\n",data->PhonebookEntry->Location);
	if (SM_SendMessage(state, 14, 0x03, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

	
	



