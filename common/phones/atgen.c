/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.7  2001-11-26 18:06:08  pkot
  Checking for *printf functions, N_(x) for localization, generic ARRAY_LEN, SAFE_STRNCPY, G_GNUC_PRINTF (Jan Kratochvil)

  Revision 1.6  2001/11/19 13:03:18  pkot
  nk3110.c cleanup

  Revision 1.5  2001/11/08 16:49:19  pkot
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
#include "phones/atgen.h"
#include "phones/ateric.h"
#include "phones/atsie.h"
#include "phones/atnok.h"
#include "links/atbus.h"
#include "links/cbus.h"


static GSM_Error Initialise(GSM_Data *setupdata, GSM_Statemachine *state);
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyIdentify(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetRFLevel(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetBattery(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, GSM_Data *data);

static GSM_Error AT_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetModel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetManufacturer(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetBattery(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_GetRFLevel(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_GetMemoryStatus(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_ReadPhonebook(GSM_Data *data,  GSM_Statemachine *state);

typedef struct {
	int gop;
	AT_SendFunctionType sfunc;
	GSM_RecvFunctionType rfunc;
} AT_FunctionInitType;


/* Mobile phone information */
static AT_SendFunctionType AT_Functions[GOP_Max];
static GSM_IncomingFunctionType IncomingFunctions[GOP_Max];
static AT_FunctionInitType AT_FunctionInit[] = {
	{ GOP_Init, NULL, Reply },
	{ GOP_GetModel, AT_GetModel, ReplyIdentify },
	{ GOP_GetRevision, AT_GetRevision, ReplyIdentify },
	{ GOP_GetImei, AT_GetIMEI, ReplyIdentify },
	{ GOP_GetManufacturer, AT_GetManufacturer, ReplyIdentify },
	{ GOP_Identify, AT_Identify, ReplyIdentify },
	{ GOP_GetBatteryLevel, AT_GetBattery, ReplyGetBattery },
	{ GOP_GetPowersource, AT_GetBattery, ReplyGetBattery },
	{ GOP_GetRFLevel, AT_GetRFLevel, ReplyGetRFLevel },
	{ GOP_GetMemoryStatus, AT_GetMemoryStatus, ReplyMemoryStatus },
	{ GOP_ReadPhonebook, AT_ReadPhonebook, ReplyReadPhonebook },
};


#define REPLY_SIMPLETEXT(l1, l2, c, t) \
	if ((0 == strcmp(l1, c)) && (NULL != t)) strcpy(t, l2)


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


GSM_RecvFunctionType AT_InsertRecvFunction(int type, GSM_RecvFunctionType func)
{
	static int pos = 0;
	int i;
	GSM_RecvFunctionType oldfunc;

	if (type >= GOP_Max) {
		return (GSM_RecvFunctionType) -1;
	}
	if (pos == 0) {
		IncomingFunctions[pos].MessageType = type;
		IncomingFunctions[pos].Functions = func;
		pos++;
		return NULL;
	}
	for (i=0; i < pos; i++) {
		if (IncomingFunctions[i].MessageType == type) {
			oldfunc = IncomingFunctions[i].Functions;
			IncomingFunctions[i].Functions = func;
			return oldfunc;
		}
	}
	if (pos < GOP_Max-1) {
		IncomingFunctions[pos].MessageType = type;
		IncomingFunctions[pos].Functions = func;
		pos++;
	}
	return NULL;
}


AT_SendFunctionType AT_InsertSendFunction(int type, AT_SendFunctionType func)
{
	AT_SendFunctionType f;

	f = AT_Functions[type];
	AT_Functions[type] = func;
	return f;
}


static GSM_Error SetEcho(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "ATE1\r\n");
	if (SM_SendMessage(state, 6, GOP_Init, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, GOP_Init);
}


GSM_Error AT_SetMemoryType(GSM_MemoryType mt, GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret = GE_NONE;
	GSM_Data data;

	if (mt != memorytype) {
		sprintf(req, "AT+CPBS=\"%s\"\r\n", memorynames[mt]);
		ret = SM_SendMessage(state, 14, GOP_Init, req);
		if (ret != GE_NONE)
			return GE_NOTREADY;
		GSM_DataClear(&data);
		ret = SM_Block(state, &data, GOP_Init);
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
		ret = SM_SendMessage(state, 15, GOP_Init, req);
		if (ret != GE_NONE)
			return GE_NOTREADY;
		GSM_DataClear(&data);
		ret = SM_Block(state, &data, GOP_Init);
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
	if (SM_SendMessage(state, 9, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


static GSM_Error AT_GetManufacturer(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMI\r\n");
	if (SM_SendMessage(state, 9, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


static GSM_Error AT_GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMR\r\n");
	if (SM_SendMessage(state, 9, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


static GSM_Error AT_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGSN\r\n");
	if (SM_SendMessage(state, 9, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


/* gets battery level and power source */

static GSM_Error AT_GetBattery(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CBC\r\n");
	if (SM_SendMessage(state, 8, GOP_GetBatteryLevel, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_GetBatteryLevel);
}


static GSM_Error AT_GetRFLevel(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];
 
	sprintf(req, "AT+CSQ\r\n");
 	if (SM_SendMessage(state, 8, GOP_GetRFLevel, req) != GE_NONE)
		return GE_NOTREADY;
 	return SM_Block(state, data, GOP_GetRFLevel);
}
 
 
static GSM_Error AT_GetMemoryStatus(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret;

	ret = AT_SetMemoryType(data->MemoryStatus->MemoryType,  state);
	if (ret != GE_NONE)
		return ret;
	sprintf(req, "AT+CPBS?\r\n");
	if (SM_SendMessage(state, 10, GOP_GetMemoryStatus, req) != GE_NONE)
		return GE_NOTREADY;
	ret = SM_Block(state, data, GOP_GetMemoryStatus);
	if (ret != GE_UNKNOWN)
		return ret;
	sprintf(req, "AT+CPBR=?\r\n");
	if (SM_SendMessage(state, 11, GOP_GetMemoryStatus, req) != GE_NONE)
		return GE_NOTREADY;
	ret = SM_Block(state, data, GOP_GetMemoryStatus);
	return ret;
}


static GSM_Error AT_ReadPhonebook(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret;

	ret = SetCharset(state);
	if (ret != GE_NONE)
		return ret;
	ret = AT_SetMemoryType(data->PhonebookEntry->MemoryType,  state);
	if (ret != GE_NONE)
		return ret;
	sprintf(req, "AT+CPBR=%d\r\n", data->PhonebookEntry->Location);
	if (SM_SendMessage(state, strlen(req), GOP_ReadPhonebook, req) != GE_NONE)
		return GE_NOTREADY;
	ret = SM_Block(state, data, GOP_ReadPhonebook);
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


static GSM_Error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos, *endpos;
	int l;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);
	if (buf.line1 == NULL)
		return GE_INVALIDPHBOOKLOCATION;

	if (strncmp(buffer, "AT+CPBR", 7)) {
		return GE_NONE; /*FIXME*/
 	}

	if (!strncmp(buf.line2, "OK", 2)) {
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
		pos = strchr(buf.line2, '\"');
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
		if (pos) {
			pos++;
			l = pos - (char *)buffer;
			endpos = memchr(pos, '\"', length - l);
		}
		if (endpos) {
			l = endpos - pos;
			DecodeAscii(data->PhonebookEntry->Name, pos, l);
			*(data->PhonebookEntry->Name + l) = '\0';
		}
	}
	return GE_NONE;
}


static GSM_Error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);
	if (buf.line1 == NULL)
		return GE_INVALIDMEMORYTYPE;

	if (data->MemoryStatus) {
		if (strstr(buf.line2,"+CPBS")) {
			pos = strchr(buf.line2, ',');
			if (pos) {
				data->MemoryStatus->Used = atoi(++pos);
			} else {
				data->MemoryStatus->Used = 100;
				data->MemoryStatus->Free = 0;
				return GE_UNKNOWN;
			}
			pos = strchr(pos, ',');
			if (pos) {
				data->MemoryStatus->Free = atoi(++pos) - data->MemoryStatus->Used;
			} else {
				return GE_UNKNOWN;
			}
		}
	}
	return GE_NONE;
}


static GSM_Error ReplyGetBattery(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);
	if ((buf.line1 == NULL) || (buf.line2 == NULL))
		return GE_NONE;

	if (!strncmp(buffer, "AT+CBC", 6)) {
		if (data->BatteryLevel) {
			*(data->BatteryUnits) = GBU_Percentage;
			pos = strchr(buf.line2, ',');
			if (pos) {
				pos++;
				*(data->BatteryLevel) = atoi(pos);
			} else {
				*(data->BatteryLevel) = 1;
			}
		}
		if (data->PowerSource) {
			*(data->PowerSource) = 0;
			if (*buf.line2 == '1') *(data->PowerSource) = GPS_ACDC;
			if (*buf.line2 == '0') *(data->PowerSource) = GPS_BATTERY;
		}
	}
	return GE_NONE;
}


static GSM_Error ReplyGetRFLevel(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos1, *pos2;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);
	if (buf.line1 == NULL)
		return GE_NONE;

	if ((!strncmp(buffer, "AT+CSQ", 6)) && (data->RFUnits)) {
		*(data->RFUnits) = GRF_CSQ;
		pos1 = buf.line2 + 6;
		pos2 = strchr(buf.line2, ',');
		if (pos1 < pos2) {
			*(data->RFLevel) = atoi(pos1);
		} else {
			*(data->RFLevel) = 1;
		}
	}
	return GE_NONE;
}


static GSM_Error ReplyIdentify(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);
	if (buf.line1 == NULL)
		return GE_NONE;		/* Fixme */
	if (!strncmp(buffer, "AT+CG", 5)) {
		REPLY_SIMPLETEXT(buffer+5, buf.line2, "SN", data->Imei);
		REPLY_SIMPLETEXT(buffer+5, buf.line2, "MM", data->Model);
		REPLY_SIMPLETEXT(buffer+5, buf.line2, "MI", data->Manufacturer);
		REPLY_SIMPLETEXT(buffer+5, buf.line2, "MR", data->Revision);
 	}
	return GE_NONE;
}


static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	int error = 0;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);
	if (buf.line1 == NULL)
		error = 1;

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

	for (i=0; i<GOP_Max; i++) {
		AT_Functions[i] = NULL;
		IncomingFunctions[i].MessageType = 0;
		IncomingFunctions[i].Functions = NULL;
	}
	for (i=0; i<ARRAY_LEN(AT_FunctionInit); i++) {
		AT_InsertSendFunction(AT_FunctionInit[i].gop, AT_FunctionInit[i].sfunc);
		AT_InsertRecvFunction(AT_FunctionInit[i].gop, AT_FunctionInit[i].rfunc);
	}

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
	data.Manufacturer = manufacturer;
	ret = state->Phone.Functions(GOP_GetManufacturer, &data, state);
	if (ret != GE_NONE) return ret;

	if (!strncasecmp(manufacturer, "ericsson", 8))
		AT_InitEricsson(state, model, setupdata->Model);
	if (!strncasecmp(manufacturer, "siemens", 7))
		AT_InitSiemens(state, model, setupdata->Model);
	if (!strncasecmp(manufacturer, "nokia", 5))
		AT_InitNokia(state, model, setupdata->Model);

	return GE_NONE;
}

 
void splitlines(AT_LineBuffer *buf)
{
	char *pos;

	if ((buf->length > 7) && (!strncmp(buf->line1+buf->length-7, "ERROR", 5))) {
		buf->line1 = NULL;
		return;
	}
	pos = findcrlf(buf->line1, 0, buf->length);
	if (pos) {
		*pos = 0;
		buf->line2 = skipcrlf(++pos);
	} else {
		buf->line2 = buf->line1;
	}
	pos = findcrlf(buf->line2, 1, buf->length);
	if (pos) {
		*pos = 0;
		buf->line3 = skipcrlf(++pos);
	} else {
		buf->line3 = buf->line2;
	}
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
 * return NULL if no <cr> or <lf> was found in the
 * range of max bytes.
 */
 
char *findcrlf(char *str, int test, int max)
{
        if (str == NULL)
                return str;
        while ((*str != '\n') && (*str != '\r') && ((*str != '\0') || test) && (max > 0)) {
                str++;
		max--;
	}
        if ((*str == '\0') || ((max == 0) && (*str != '\n') && (*str != '\r')))
                return NULL;
        return str;
}
