/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.2  2001-08-09 11:51:39  pkot
  Generic AT support updates and cleanup (Manfred Jonsson)

  Revision 1.1  2001/07/27 00:02:21  pkot
  Generic AT support for the new structure (Manfred Jonsson)

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-statemachine.h"
#include "phones/generic.h"
#include "links/atbus.h"
#include "links/cbus.h"


static GSM_Error Initialise(GSM_Data *setupdata, GSM_Statemachine *state);
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data);


#define REPLY_SIMPLETEXT(l1, l2, c, t) \
	if ((0 == strcmp(l1, c)) && (NULL != t)) strcpy(t, l2)


/* Mobile phone information */

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 1, Reply },
	{ 0, NULL }
};


GSM_Phone phone_at = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	{
		"AT|dancall",			/* Supported models */
		99,			/* Max RF Level */
		0,			/* Min RF Level */
		GRF_CSQ,		/* RF level units */
		100,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GBU_Percentage,		/* Battery level units */
		0,			/* Have date/time support */
		0,			/* Alarm supports time only */
		0,			/* Alarms available - FIXME */
		0, 0,			/* Startup logo size - FIXME */
		0, 0,			/* Op logo size */
		0, 0			/* Caller logo size */
	},
	Functions
};


static GSM_MemoryType memorytype = GMT_XX;
static char *memorynames[] = {
	"ME", /* Internal memory of the mobile equipment */
	"SM", /* SIM card memory */
	"FD", /* Fixed dial numbers */
	"ON", /* Own numbers */
	"EN", /* Emergency numbers */
	"DC", /* Dialled numbers */
	"RC", /* Received numbers */
	"MC", /* Missed numbers */
	"LD", /* Last dialed */
	"MT", /* combined ME and SIM phonebook */
	"TA", /* for compatibility only: TA=computer memory */
	"CB", /* Currently selected memory */
};

/* LinkOK is always true for now... */
bool ATGEN_LinkOK = true;


static GSM_Error SetEcho(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "ATE1\r\n");
	if (SM_SendMessage(state, 6, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error SetMemoryType(GSM_MemoryType mt, GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret = GE_NONE;
	GSM_Data data;

	if (mt != memorytype) {
		sprintf(req, "AT+CPBS=%s\r\n", memorynames[mt]);
		ret = SM_SendMessage(state, 12, 1, req);
		if (ret != GE_NONE)
			return GE_NOTREADY;
		GSM_DataClear(&data);
		ret = SM_Block(state, &data, 1);
		if (ret == GE_NONE)
			memorytype = mt;
	}
	return ret;
}


static GSM_Error AT_Identify(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret;

	sprintf(req, "AT+CGMM\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	if (ret != GE_NONE) return ret;
	sprintf(req, "AT+CGSN\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	if (ret != GE_NONE) return ret;
	sprintf(req, "AT+CGMI\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	if (ret != GE_NONE) return ret;
	sprintf(req, "AT+CGMR\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	return ret;
}


static GSM_Error AT_GetModel(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMM\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error AT_GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMR\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error AT_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGSN\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


/* gets battery level and power source */

static GSM_Error AT_GetBattery(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CBC\r\n");
	if (SM_SendMessage(state, 8, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error AT_GetRFLevel(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];
 
	sprintf(req, "AT+CSQ\r\n");
 	if (SM_SendMessage(state, 8, 1, req) != GE_NONE) return GE_NOTREADY;
 	return SM_Block(state, data, 1);
}
 
 
static GSM_Error AT_GetMemoryStatus(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret;

	ret = SetMemoryType(data->MemoryStatus->MemoryType,  state);
	if (ret != GE_NONE)
		return ret;
	sprintf(req, "AT+CPBS?\r\n");
	if (SM_SendMessage(state, 10, 1, req) != GE_NONE)
		return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	return ret;
}


static GSM_Error AT_ReadPhonebook(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret;

	ret = SetMemoryType(data->PhonebookEntry->MemoryType,  state);
	if (ret != GE_NONE)
		return ret;
	sprintf(req, "AT+CPBR=%d\r\n", data->PhonebookEntry->Location);
	if (SM_SendMessage(state, strlen(req), 1, req) != GE_NONE)
		return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	return ret;
}


static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return Initialise(data, state);
		break;
	case GOP_GetModel:
		return AT_GetModel(data, state);
		break;
	case GOP_GetRevision:
		return AT_GetRevision(data, state);
		break;
	case GOP_GetImei:
		return AT_GetIMEI(data, state);
		break;
	case GOP_Identify:
		return AT_Identify(data, state);
		break;
	case GOP_GetBatteryLevel:
	case GOP_GetPowersource:
		return AT_GetBattery(data, state);
		break;
	case GOP_GetRFLevel:
		return AT_GetRFLevel(data, state);
		break;
	case GOP_GetMemoryStatus:
		return AT_GetMemoryStatus(data, state);
		break;
	case GOP_ReadPhonebook:
		return AT_ReadPhonebook(data, state);
		break;
	default:
		return GE_NOTIMPLEMENTED;
		break;
	}
}


static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	char *line2, *line3, *pos, *endpos;
	int error = 0;

	if ((length > 7) && (!strncmp(buffer+length-7, "ERROR", 5)))
		error = 1;
	line2 = strchr(buffer, '\r');
	if (line2) {
		*line2 = 0;
		line2++;
		line2++;
		line2++;
	} else {
		line2 = buffer;
	}
	line3 = strchr(line2, '\r');
	if (line3) {
		*line3 = 0;
		line3++;
		line3++;
		line3++;
	} else {
		line3 = line2;
	}

	if (!strncmp(buffer, "AT+C", 4)) {
		if (*(buffer+4) =='G') {
			REPLY_SIMPLETEXT(buffer+5, line2, "SN", data->Imei);
			REPLY_SIMPLETEXT(buffer+5, line2, "MM", data->Model);
			REPLY_SIMPLETEXT(buffer+5, line2, "MI", data->Manufacturer);
			REPLY_SIMPLETEXT(buffer+5, line2, "MR", data->Revision);
		} else if ((!strncmp(buffer+4, "SQ", 2)) && (data->RFUnits)) {
			*(data->RFUnits) = GRF_CSQ;
			pos = line2 + 6;
			line2 = strchr(line2, ',');
			if (pos < line2) {
				*(data->RFLevel) = atoi(pos);
			} else {
				*(data->RFLevel) = 1;
			}
		} else if (!strncmp(buffer+4, "BC", 2)) {
			if (data->BatteryLevel) {
				*(data->BatteryUnits) = GBU_Percentage;
				pos = strchr(line2, ',');
				if (pos) {
					pos++;
					*(data->BatteryLevel) = atoi(pos);
				} else {
					*(data->BatteryLevel) = 1;
				}
			}
			if (data->PowerSource) {
				*(data->PowerSource) = 0;
				if (*line2 == '1') *(data->PowerSource) = GPS_ACDC;
				if (*line2 == '0') *(data->PowerSource) = GPS_BATTERY;
			}
		} else if (!strncmp(buffer+4, "PB", 2)) {
			if (*(buffer+6) == 'S') {
				if (error)
					return GE_INVALIDMEMORYTYPE;
				if (data->MemoryStatus) {
					pos = strchr(line2, ',');
					if (pos)
						data->MemoryStatus->Used = atoi(++pos);
					pos = strchr(pos, ',');
					if (pos)
						data->MemoryStatus->Free = atoi(++pos) - data->MemoryStatus->Used;
				}
			}
			if (*(buffer+6) == 'R') {
				if (error) {
					return GE_INVALIDPHBOOKLOCATION;
				}
				if (!strncmp(line2, "OK", 2)) {
					if (data->PhonebookEntry) {
						*(data->PhonebookEntry->Number) = '\0';
						*(data->PhonebookEntry->Name) = '\0';
						data->PhonebookEntry->Group = 0;
						data->PhonebookEntry->SubEntriesCount = 0;
					}
					return GE_NONE;
				}
				if (data->PhonebookEntry) {
					data->PhonebookEntry->Group = 0;
					data->PhonebookEntry->SubEntriesCount = 0;
					pos = strchr(line2, '\"');
					endpos = NULL;
					if (pos)
						endpos = strchr(++pos, '\"');
					if (endpos) {
						*endpos = '\0';
						strcpy(data->PhonebookEntry->Number, pos);
					}
					pos = NULL;
					if (endpos)
						pos = strchr(++endpos, '\"');
					endpos = NULL;
					if (pos)
						endpos = strchr(++pos, '\"');
					if (pos) {
						*endpos = '\0';
						DecodeAscii(data->PhonebookEntry->Name, pos, strlen(pos));
						*(data->PhonebookEntry->Name+strlen(pos)) = '\0';
					}
				}
			}
		}
 	}
	return GE_NONE;
}


static GSM_Error Initialise(GSM_Data *setupdata, GSM_Statemachine *state)
{
	GSM_Data data;
	GSM_Error ret;
	char model[10];

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_at, sizeof(GSM_Phone));

	dprintf("Initializing AT capable mobile phone ...\n");
	switch (state->Link.ConnectionType) {
	case GCT_Serial:
		if (!strcmp(setupdata->Model, "dancall"))
			CBUS_Initialise(state);
		else
			ATBUS_Initialise(state);
		break;
	default:
		return GE_NOTSUPPORTED;
		break;
	}
	SM_Initialise(state);

	SetEcho(&data, state);

	GSM_DataClear(&data);
	data.Model = model;

	ret = state->Phone.Functions(GOP_GetModel, &data, state);
	return ret;
}

