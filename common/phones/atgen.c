/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.5  2001-11-08 16:49:19  pkot
  Cleanups

  Revision 1.4  2001/08/20 23:36:27  pkot
  More cleanup in AT code (Manfred Jonsson)

  Revision 1.3  2001/08/20 23:27:37  pkot
  Add hardware shakehand to the link layer (Manfred Jonsson)

  Revision 1.2  2001/08/09 11:51:39  pkot
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
#include "gsm-encoding.h"
#include "phones/generic.h"
#include "links/atbus.h"
#include "links/cbus.h"


#define ARRAY_LEN(x) (sizeof((x))/sizeof((x)[0]))

static GSM_Error Initialise(GSM_Data *setupdata, GSM_Statemachine *state);
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data);

static GSM_Error AT_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetModel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetManufacturer(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetBattery(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_GetRFLevel(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_GetMemoryStatus(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_ReadPhonebook(GSM_Data *data,  GSM_Statemachine *state);


typedef GSM_Error (*AT_FunctionType)(GSM_Data *d, GSM_Statemachine *s);
typedef struct {
	int gop;
	AT_FunctionType func;
} AT_FunctionInitType;

static AT_FunctionType AT_Functions[GOP_Max];
static AT_FunctionInitType AT_FunctionInit[] = {
	{ GOP_GetModel, AT_GetModel },
	{ GOP_GetRevision, AT_GetRevision },
	{ GOP_GetImei, AT_GetIMEI },
	{ GOP_GetManufacturer, AT_GetManufacturer },
	{ GOP_Identify, AT_Identify },
	{ GOP_GetBatteryLevel, AT_GetBattery },
	{ GOP_GetPowersource, AT_GetBattery },
	{ GOP_GetRFLevel, AT_GetRFLevel },
	{ GOP_GetMemoryStatus, AT_GetMemoryStatus },
	{ GOP_ReadPhonebook, AT_ReadPhonebook },
};


char *skipcrlf(char *str);
char *findcrlf(char *str, int test);


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
		"AT|AT-HW|dancall",			/* Supported models */
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
static int atcharset = 0;

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


static GSM_Error SetCharset(GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret = GE_NONE;
	GSM_Data data;

	if (atcharset == 0) {
		sprintf(req, "AT+CSCS=\"GSM\"\r\n");
		ret = SM_SendMessage(state, 15, 1, req);
		if (ret != GE_NONE)
			return GE_NOTREADY;
		GSM_DataClear(&data);
		ret = SM_Block(state, &data, 1);
		if (ret == GE_NONE)
			atcharset = 1;
	}
	return ret;
}


static GSM_Error AT_Identify(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error ret;

	if ((ret = Functions(GOP_GetModel, data, state)) != GE_NONE)
		return ret;
	if ((ret = Functions(GOP_GetManufacturer, data, state)) != GE_NONE)
		return ret;
	if ((ret = Functions(GOP_GetRevision, data, state)) != GE_NONE)
		return ret;
	return Functions(GOP_GetImei, data, state);
}


static GSM_Error AT_GetModel(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMM\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error AT_GetManufacturer(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMI\r\n");
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

	ret = SetCharset(state);
	if (ret != GE_NONE)
		return ret;
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
	if (op == GOP_Init)
		return Initialise(data, state);
	if ((op > GOP_Init) && (op < GOP_Max))
		if (AT_Functions[op])
			return (*AT_Functions[op])(data, state);
	return GE_NOTIMPLEMENTED;
}


static GSM_Error ReplyReadPhonebook(GSM_Data *data, char *line, int error, int len)
{
	char *pos, *endpos;
	int l;

	if (error) {
		return GE_INVALIDPHBOOKLOCATION;
	}
	if (!strncmp(line, "OK", 2)) {
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
		pos = strchr(line, '\"');
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
			endpos = memchr(++pos, '\"', len);
		if (endpos) {
			l= endpos - pos;
			DecodeAscii(data->PhonebookEntry->Name, pos, l);
			*(data->PhonebookEntry->Name + l) = '\0';
		}
	}
	return GE_NONE;
}


static GSM_Error ReplyMemoryStatus(GSM_Data *data, char *line, int error)
{
	char *pos;

	if (error)
		return GE_INVALIDMEMORYTYPE;
	if (data->MemoryStatus) {
		pos = strchr(line, ',');
		if (pos)
			data->MemoryStatus->Used = atoi(++pos);
		pos = strchr(pos, ',');
		if (pos)
			data->MemoryStatus->Free = atoi(++pos) - data->MemoryStatus->Used;
	}
	return GE_NONE;
}


static GSM_Error ReplyBattery(GSM_Data *data, char *line, int error)
{
	char *pos;

	if (data->BatteryLevel) {
		*(data->BatteryUnits) = GBU_Percentage;
		pos = strchr(line, ',');
		if (pos) {
			pos++;
			*(data->BatteryLevel) = atoi(pos);
		} else {
			*(data->BatteryLevel) = 1;
		}
	}
	if (data->PowerSource) {
		*(data->PowerSource) = 0;
		if (*line == '1') *(data->PowerSource) = GPS_ACDC;
		if (*line == '0') *(data->PowerSource) = GPS_BATTERY;
	}
	return GE_NONE;
}


static GSM_Error ReplyRFLevel(GSM_Data *data, char *line, int error)
{
	char *pos, *buf;

	if (data->RFUnits) {
		*(data->RFUnits) = GRF_CSQ;
		pos = line + 6;
		buf = strchr(line, ',');
		if (pos < buf) {
			*(data->RFLevel) = atoi(pos);
		} else {
			*(data->RFLevel) = 1;
		}
	}
	return GE_NONE;
}


static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	char *line2, *line3, *pos;
	int error = 0;

	if ((length > 7) && (!strncmp(buffer+length-7, "ERROR", 5)))
		error = 1;
	pos = findcrlf(buffer, 0);
	if (pos) {
		*pos = 0;
		line2 = skipcrlf(++pos);
	} else {
		line2 = buffer;
	}
	pos = findcrlf(line2, 1);
	if (pos) {
		*pos = 0;
		line3 = skipcrlf(++pos);
	} else {
		line3 = line2;
	}

	if (!strncmp(buffer, "AT+C", 4)) {
		if (*(buffer+4) =='G') {
			REPLY_SIMPLETEXT(buffer+5, line2, "SN", data->Imei);
			REPLY_SIMPLETEXT(buffer+5, line2, "MM", data->Model);
			REPLY_SIMPLETEXT(buffer+5, line2, "MI", data->Manufacturer);
			REPLY_SIMPLETEXT(buffer+5, line2, "MR", data->Revision);
		} else if (!strncmp(buffer+4, "SQ", 2)) {
			ReplyRFLevel(data, line2, error);
		} else if (!strncmp(buffer+4, "BC", 2)) {
			ReplyBattery(data, line2, error);
		} else if (!strncmp(buffer+4, "PB", 2)) {
			if (*(buffer+6) == 'S') {
				ReplyMemoryStatus(data, line2, error);
			} else if (*(buffer+6) == 'R') {
				ReplyReadPhonebook(data, line2, error, length);
			}
		}
 	}
	return GE_NONE;
}


static GSM_Error Initialise(GSM_Data *setupdata, GSM_Statemachine *state)
{
	GSM_Data data;
	GSM_Error ret;
	char model[20];
	char manufacturer[20];
	int i;

	fprintf(stderr, "Initializing AT capable mobile phone ...\n");

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_at, sizeof(GSM_Phone));

	for (i=0; i<GOP_Max; i++)
		AT_Functions[i] = NULL;
	for (i=0; i<ARRAY_LEN(AT_FunctionInit); i++)
		AT_Functions[AT_FunctionInit[i].gop] = AT_FunctionInit[i].func;

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
		if (!strcmp(setupdata->Model, "dancall"))
			CBUS_Initialise(state);
		else if (!strcmp(setupdata->Model, "AT-HW"))
			ATBUS_Initialise(state, true);
		else
			ATBUS_Initialise(state, false);
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
	if (ret != GE_NONE) return ret;
	GSM_DataClear(&data);
	data.Model = manufacturer;
	ret = state->Phone.Functions(GOP_GetManufacturer, &data, state);
	if (ret != GE_NONE) return ret;

	/*
	if (!strcasecmp(manufacturer, "siemens"))
		AT_InitSiemens(state, model, setupdata->Model, AT_Functions);
	if (!strcasecmp(manufacturer, "ericsson"))
		AT_InitEricsson(state, model, setupdata->Model, AT_Functions);
	*/

	return GE_NONE;
}

 
/*
 * increments the argument until a char unequal to
 * <cr> or <lf> is found. returns the new position.
 */
 
char *skipcrlf(char *str)
{
        if (str == NULL)
                return str;
        while ((*str == '\n') || (*str == '\r'))
                str++;
        return str;
}
 
 
/*
 * searches for <cr> or <lf> and returns the first
 * occurrence. if test is set, the gsm char @ which
 * is 0x00 is not considered as end of string.
 * return NULL if test is not set and no <cr> or
 * <lf> was found.
 * TODO should ask for maximum length.
 */
 
char *findcrlf(char *str, int test)
{
        if (str == NULL)
                return str;
        while ((*str != '\n') && (*str != '\r') && ((*str != '\0') || test))
                str++;
        if (*str == '\0')
                return NULL;
        return str;
}
