/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

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

  Copyright (C) 2001 Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2002 Pawel Kot <pkot@linuxnews.pl>

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

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
#ifndef WIN32
#  include "links/cbus.h"
#endif

static GSM_Error Initialise(GSM_Data *setupdata, GSM_Statemachine *state);
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyIdentify(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetRFLevel(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetBattery(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyCallDivert(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetPrompt(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplySendSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
/* static GSM_Error ReplyDeleteSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data); */
static GSM_Error ReplyGetCharset(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetSMSCenter(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error ReplyGetSecurityCodeStatus(int messagetype, unsigned char *buffer, int length, GSM_Data *data);


static GSM_Error AT_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetModel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetManufacturer(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetBattery(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_GetRFLevel(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_GetMemoryStatus(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_ReadPhonebook(GSM_Data *data,  GSM_Statemachine *state);
static GSM_Error AT_CallDivert(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_SetPDUMode(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_SendSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_SaveSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_WriteSMS(GSM_Data *data, GSM_Statemachine *state, unsigned char *cmd);
static GSM_Error AT_GetSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_DeleteSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetCharset(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error AT_GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state);

typedef struct {
	int gop;
	AT_SendFunctionType sfunc;
	GSM_RecvFunctionType rfunc;
} AT_FunctionInitType;


/* Mobile phone information */
static AT_SendFunctionType AT_Functions[GOPAT_Max];
static GSM_IncomingFunctionType IncomingFunctions[GOPAT_Max];
static AT_FunctionInitType AT_FunctionInit[] = {
	{ GOP_Init, NULL, Reply },
	{ GOP_Terminate, PGEN_Terminate, Reply }, /* Replyfunction must not be NULL */
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
	{ GOP_CallDivert, AT_CallDivert, ReplyCallDivert },
	{ GOPAT_SetPDUMode, AT_SetPDUMode, Reply },
	{ GOPAT_Prompt, NULL, ReplyGetPrompt },
	{ GOP_SendSMS, AT_SendSMS, ReplySendSMS },
	{ GOP_SaveSMS, AT_SaveSMS, ReplySendSMS },
	{ GOP_GetSMS, AT_GetSMS, ReplyGetSMS },
	{ GOP_DeleteSMS, AT_DeleteSMS, Reply },
	{ GOPAT_GetCharset, AT_GetCharset, ReplyGetCharset },
	{ GOP_GetSMSCenter, AT_GetSMSCenter, ReplyGetSMSCenter },
	{ GOP_GetSecurityCodeStatus, AT_GetSecurityCodeStatus, ReplyGetSecurityCodeStatus },
	{ GOP_EnterSecurityCode, AT_EnterSecurityCode, Reply }
};


#define REPLY_SIMPLETEXT(l1, l2, c, t) \
	if ((strcmp(l1, c) == 0) && (t != NULL)) strcpy(t, l2)


GSM_Phone phone_at = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	{
		"AT|AT-HW|dancall",	/* Supported models */
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
static GSMAT_Charset atdefaultcharset = CHARNONE;
static GSMAT_Charset atcharset = CHARNONE;

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


GSM_RecvFunctionType AT_InsertRecvFunction(int type, GSM_RecvFunctionType func)
{
	static int pos = 0;
	int i;
	GSM_RecvFunctionType oldfunc;

	if (type >= GOPAT_Max) {
		return (GSM_RecvFunctionType) -1;
	}
	if (pos == 0) {
		IncomingFunctions[pos].MessageType = type;
		IncomingFunctions[pos].Functions = func;
		pos++;
		return NULL;
	}
	for (i = 0; i < pos; i++) {
		if (IncomingFunctions[i].MessageType == type) {
			oldfunc = IncomingFunctions[i].Functions;
			IncomingFunctions[i].Functions = func;
			return oldfunc;
		}
	}
	if (pos < GOPAT_Max-1) {
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


static GSM_Error SoftReset(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "ATZ\r");
	if (SM_SendMessage(state, 4, GOP_Init, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, GOP_Init);
}


static GSM_Error SetEcho(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "ATE1\r");
	if (SM_SendMessage(state, 5, GOP_Init, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, GOP_Init);
}


/* StoreDefaultCharset
 *
 * for a correct communication with the phone for phonebook entries or
 * SMS text mode, we need to set a suited charset. a suited charset
 * doesn't contain characters which are also used by the serial line for
 * software handshake. so the GSM charset (or PC437, latin-1, etc) are
 * a bad choice.
 * so the GSM specification defines the HEX charset which is a hexidecimal
 * representation of the "original" charset. this is a good choice for the
 * above problem. but the GSM specification defines the default charset and
 * no "original" charset.
 * so what we do is to ask the phone (after a reset) for its original
 * charset and store the result for future referece. we don't do a full
 * initialization for speed reason. at further processing we can chose
 * a working charset if needed.
 *
 * see also AT_SetCharset, AT_GetCharset
 *
 * GSM_Data has no field for charset so i misuse Model.
 */
static void StoreDefaultCharset(GSM_Statemachine *state)
{
	GSM_Data data;
	GSM_Error ret = GE_NONE;
	char buf[256];

	GSM_DataClear(&data);
	data.Model = buf;
	ret = state->Phone.Functions(GOPAT_GetCharset, &data, state);
	if (ret != GE_NONE) return;
	if (!strncmp(buf, "GSM", 3)) atdefaultcharset = CHARGSM;
	if (strstr(buf, "437")) atdefaultcharset = CHARCP437;
	return;
}


GSM_Error AT_SetMemoryType(GSM_MemoryType mt, GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret = GE_NONE;
	GSM_Data data;

	if (mt != memorytype) {
		sprintf(req, "AT+CPBS=\"%s\"\r", memorynames[mt]);
		ret = SM_SendMessage(state, 13, GOP_Init, req);
		if (ret != GE_NONE)
			return GE_NOTREADY;
		GSM_DataClear(&data);
		ret = SM_Block(state, &data, GOP_Init);
		if (ret == GE_NONE)
			memorytype = mt;
	}
	return ret;
}


/* SetCharset
 *
 * before we start sending or receiving phonebook entries from the phone,
 * we should set a charset. this is done once before the first read or write.
 *
 * we try to chose a charset with hexadecimal representation. first ucs2
 * (which is a hexencoded unicode charset) is tested and set if available.
 * if this fails for any reason, it is checked if the original charset is
 * GSM. if this is true, we try to set HEX (a hexencoded GSM charset). if
 * this again fails or is impossible, we try to use the GSM charset. if
 * the original charset was GSM nothing is done (we rely on not changing
 * anything by the failing tries before). if the original charset was
 * something else, we set the GSM charset. if this too fails, the user is
 * on his own, characters will be copied from or to the phone without
 * conversion.
 *
 * the whole bunch is needed to get a reasonable support for different
 * phones. eg a siemens s25 has GSM as original charset and aditional
 * supports only UCS2, a nokia 7110 has PCCP437 as original charset which
 * renders HEX unusable for us (in this case HEX will give a hexadecimal
 * encoding of the PCCP437 charset) and no UCS2. a ericsson t39 uses
 * GSM as original charset but has never heard of any hex encoded charset.
 * but this doesn't matter with IRDA and i haven't found a serial cable
 * in a shop yet, so this is no problem
 *
 * FIXME because errorreporting is broken, one may not end up with the
 * desired charset.
 *
 * see AT_GetCharset, StoreDefaultCharset
 *
 * GSM_Data has no field for charset so i misuse Model.
 */
static GSM_Error SetCharset(GSM_Statemachine *state)
{
	char req[128];
	char charsets[256];
	GSM_Error ret = GE_NONE;
	GSM_Data data;

	if (atcharset == CHARNONE) {
		/* check if ucs2 is available */
		sprintf(req, "AT+CSCS=?\r");
		ret = SM_SendMessage(state, 10, GOPAT_GetCharset, req);
		if (ret != GE_NONE)
			return GE_NOTREADY;
		GSM_DataClear(&data);
		*charsets = '\0';
		data.Model = charsets;
		ret = SM_Block(state, &data, GOPAT_GetCharset);
		if (ret != GE_NONE) {
			*charsets = '\0';
		}
		else if (strstr(charsets, "UCS2")) {
			/* ucs2 charset found. try to set it */
			sprintf(req, "AT+CSCS=\"UCS2\"\r");
			ret = SM_SendMessage(state, 15, GOP_Init, req);
			if (ret != GE_NONE)
				return GE_NOTREADY;
			GSM_DataClear(&data);
			ret = SM_Block(state, &data, GOP_Init);
			if (ret == GE_NONE)
				atcharset = CHARUCS2;
		}
		if (atcharset == CHARNONE) {
			/* no ucs2 charset found or error occured */
			if (atdefaultcharset == CHARGSM) {
				atcharset = CHARGSM;
				if (!strstr(charsets, "HEX")) {
					/* no hex charset found! */
					return GE_NONE;
				}
				/* try to set hex charset */
				sprintf(req, "AT+CSCS=\"HEX\"\r");
				ret = SM_SendMessage(state, 14, GOP_Init, req);
				if (ret != GE_NONE)
					return GE_NOTREADY;
				GSM_DataClear(&data);
				ret = SM_Block(state, &data, GOP_Init);
				if (ret == GE_NONE)
					atcharset = CHARHEXGSM;
			} else {
				sprintf(req, "AT+CSCS=\"GSM\"\r");
				ret = SM_SendMessage(state, 14, GOP_Init, req);
				if (ret != GE_NONE)
					return GE_NOTREADY;
				GSM_DataClear(&data);
				ret = SM_Block(state, &data, GOP_Init);
				if (ret == GE_NONE)
					atcharset = CHARGSM;
			}
		}
	}
	return ret;
}


/* AT_GetCharset
 *
 * this function detects the current charset used by the phone. if it is
 * called immediatedly after a reset of the phone, this is also the
 * original charset of the phone.
 *
 * GSM_Data has no field for charset so you must misuse Model. the Model
 * string must be initialized with a length of 256 bytes.
 */
static GSM_Error AT_GetCharset(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];
	GSM_Error ret = GE_NONE;

	sprintf(req, "AT+CSCS?\r");
	ret = SM_SendMessage(state, 9, GOPAT_GetCharset, req);
	if (ret != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOPAT_GetCharset);
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

	sprintf(req, "AT+CGMM\r");
	if (SM_SendMessage(state, 8, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


static GSM_Error AT_GetManufacturer(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMI\r");
	if (SM_SendMessage(state, 8, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


static GSM_Error AT_GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGMR\r");
	if (SM_SendMessage(state, 8, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


static GSM_Error AT_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CGSN\r");
	if (SM_SendMessage(state, 8, GOP_Identify, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_Identify);
}


/* gets battery level and power source */

static GSM_Error AT_GetBattery(GSM_Data *data, GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CBC\r");
	if (SM_SendMessage(state, 7, GOP_GetBatteryLevel, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_GetBatteryLevel);
}


static GSM_Error AT_GetRFLevel(GSM_Data *data,  GSM_Statemachine *state)
{
	char req[128];

	sprintf(req, "AT+CSQ\r");
	if (SM_SendMessage(state, 7, GOP_GetRFLevel, req) != GE_NONE)
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
	sprintf(req, "AT+CPBS?\r");
	if (SM_SendMessage(state, 9, GOP_GetMemoryStatus, req) != GE_NONE)
		return GE_NOTREADY;
	ret = SM_Block(state, data, GOP_GetMemoryStatus);
	if (ret != GE_UNKNOWN)
		return ret;
	sprintf(req, "AT+CPBR=?\r");
	if (SM_SendMessage(state, 10, GOP_GetMemoryStatus, req) != GE_NONE)
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
	sprintf(req, "AT+CPBR=%d\r", data->PhonebookEntry->Location);
	if (SM_SendMessage(state, strlen(req), GOP_ReadPhonebook, req) != GE_NONE)
		return GE_NOTREADY;
	ret = SM_Block(state, data, GOP_ReadPhonebook);
	return ret;
}


static GSM_Error AT_CallDivert(GSM_Data *data, GSM_Statemachine *state)
{
	char req[64];

	if (!data->CallDivert) return GE_UNKNOWN;

	sprintf(req, "AT+CCFC=");

	switch (data->CallDivert->DType) {
	case GSM_CDV_AllTypes:
		strcat(req, "4");
		break;
	case GSM_CDV_Busy:
		strcat(req, "1");
		break;
	case GSM_CDV_NoAnswer:
		strcat(req, "2");
		break;
	case GSM_CDV_OutOfReach:
		strcat(req, "3");
		break;
	default:
		dprintf("3. %d\n", data->CallDivert->DType);
		return GE_NOTIMPLEMENTED;
	}
	if (data->CallDivert->Operation == GSM_CDV_Register)
		sprintf(req, "%s,%d,\"%s\",%d,,,%d", req,
			data->CallDivert->Operation,
			data->CallDivert->Number.Number,
			data->CallDivert->Number.Type,
			data->CallDivert->Timeout);
	else
		sprintf(req, "%s,%d", req, data->CallDivert->Operation);

	strcat(req, "\r");

	dprintf("%s", req);
	if (SM_SendMessage(state, strlen(req), GOP_CallDivert, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_WaitFor(state, data, GOP_CallDivert);
}

static GSM_Error AT_SetPDUMode(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[12];

	sprintf(req, "AT+CMGF=0\r");
	if (SM_SendMessage(state, 10, GOPAT_SetPDUMode, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOPAT_SetPDUMode);
}

static GSM_Error AT_SendSMS(GSM_Data *data, GSM_Statemachine *state)
{
	return AT_WriteSMS(data, state, "CMGS");
}

static GSM_Error AT_SaveSMS(GSM_Data *data, GSM_Statemachine *state)
{
	return GE_NOTSUPPORTED;
	return AT_WriteSMS(data, state, "CMGW");
}

static GSM_Error AT_WriteSMS(GSM_Data *data, GSM_Statemachine *state, unsigned char *cmd)
{
	unsigned char req[10240], req2[5120];
	GSM_Error error;
	unsigned int length, tmp, offset = 0;

	if (!data->RawSMS) return GE_INTERNALERROR;

	/* Select PDU mode */
	error = Functions(GOPAT_SetPDUMode, data, state);
	if (error != GE_NONE) {
		dprintf("PDU mode not supported\n");
		return error;
	}
	dprintf("AT mode set\n");

	/* Prepare the message and count the size */
	memcpy(req2, data->RawSMS->MessageCenter, data->RawSMS->MessageCenter[0] + 1);
	offset += data->RawSMS->MessageCenter[0];

	req2[offset + 1] = 0x01 | 0x10; /* Validity period in relative format */
	if (data->RawSMS->RejectDuplicates) req2[offset + 1] |= 0x04;
	if (data->RawSMS->Report) req2[offset + 1] |= 0x20;
	if (data->RawSMS->UDHIndicator) req2[offset + 1] |= 0x40;
	if (data->RawSMS->ReplyViaSameSMSC) req2[offset + 1] |= 0x80;
	req2[offset + 2] = 0x00; /* Message Reference */

	tmp = data->RawSMS->RemoteNumber[0];
	if (tmp % 2) tmp++;
	tmp /= 2;
	memcpy(req2 + offset + 3, data->RawSMS->RemoteNumber, tmp + 2);
	offset += tmp + 1;

	req2[offset + 4] = data->RawSMS->PID;
	req2[offset + 5] = data->RawSMS->DCS;
	req2[offset + 6] = 0xaa; /* Validity period */
	req2[offset + 7] = data->RawSMS->Length;
	memcpy(req2 + offset + 8, data->RawSMS->UserData, data->RawSMS->Length);

	length = data->RawSMS->Length + offset + 8;

	/* Length in AT mode is the length of the full message minus SMSC field length */
	sprintf(req, "AT+%s=%d\r", cmd, length - data->RawSMS->MessageCenter[0] - 1);
	dprintf("Sending initial sequence\n");
	if (SM_SendMessage(state, strlen(req), GOPAT_Prompt, req) != GE_NONE) return GE_NOTREADY;
	error = SM_BlockNoRetry(state, data, GOPAT_Prompt);
	dprintf("Got response %d\n", error);

	bin2hex(req, req2, length);
	req[length * 2] = 0x1a;
	req[length * 2 + 1] = 0;
	dprintf("Sending frame: %s\n", req);
	if (SM_SendMessage(state, strlen(req), GOP_SendSMS, req) != GE_NONE) return GE_NOTREADY;
	return SM_BlockNoRetry(state, data, GOP_SendSMS);
}

static GSM_Error AT_GetSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[16];
	sprintf(req, "AT+CMGR=%d\r", data->SMS->Number);
	dprintf("%s", req);
	data->RawSMS->Number = data->SMS->Number;
	data->RawSMS->MemoryType = data->SMS->MemoryType;
	if (SM_SendMessage(state, strlen(req), GOP_GetSMS, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_GetSMS);
}

/* FIXME
 * This function doesn't set memory type. this need to be fixed when 
 * SMS for AT works */
static GSM_Error AT_DeleteSMS(GSM_Data *data, GSM_Statemachine *state)
{
	return GE_NOTSUPPORTED;
/*
	unsigned char req[16];
	sprintf(req, "AT+CMGD=%d\r", data->SMS->Number);
	dprintf("%s", req);

	if (SM_SendMessage(state, strlen(req), GOP_DeleteSMS, req) != GE_NONE)
 		return GE_NOTREADY;
	return SM_Block(state, data, GOP_DeleteSMS);
*/
}

/* Hey nokia users. don't expect this to return anything useful */
/* You can't read the number set by the phone menu with this */
/* command, nor can you change this number by AT commands. Worse, */
/* an ATZ will clear a SMS Center Number set by AT commands. This */
/* doesn't affect the number set by the phone menu */
static GSM_Error AT_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[16];
	sprintf(req, "AT+CSCA?\r");
 	if (SM_SendMessage(state, 9, GOP_GetSMSCenter, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_GetSMSCenter);
}


static GSM_Error AT_GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[16];
	sprintf(req, "AT+CPIN?\r");
 	if (SM_SendMessage(state, 9, GOP_GetSecurityCodeStatus, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_GetSecurityCodeStatus);
}


static GSM_Error AT_EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state)
{
	/* fixme bufferoverflow */
	unsigned char req[256];

	if ((data->SecurityCode->Type != GSCT_Pin))
		return (GE_NOTIMPLEMENTED);

	sprintf(req, "AT+CPIN=\"%s\"\r", data->SecurityCode->Code);
 	if (SM_SendMessage(state, strlen(req), GOP_EnterSecurityCode, req) != GE_NONE)
		return GE_NOTREADY;
	return SM_Block(state, data, GOP_EnterSecurityCode);
}


static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	if (op == GOP_Init)
		return Initialise(data, state);
	if ((op > GOP_Init) && (op < GOPAT_Max))
		if (AT_Functions[op])
			return (*AT_Functions[op])(data, state);
	return GE_NOTIMPLEMENTED;
}


static GSM_Error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos, *endpos;
	int l;

	if (buffer[0] != GEAT_OK)
		return GE_INVALIDPHBOOKLOCATION;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	if (strncmp(buf.line1, "AT+CPBR", 7)) {
		return GE_UNKNOWN;
	}

	if (!strncmp(buf.line2, "OK", 2)) {
		/* Empty phonebook location found */
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

		/* store number */
		pos = strchr(buf.line2, '\"');
		endpos = NULL;
		if (pos)
			endpos = strchr(++pos, '\"');
		if (endpos) {
			*endpos = '\0';
			strcpy(data->PhonebookEntry->Number, pos);
		}

		/* store name */
		pos = NULL;
		if (endpos)
			pos = strchr(++endpos, '\"');
		endpos = NULL;
		if (pos) {
			pos++;
			/* parse the string form behind for quotation.
			 * this will allways succede because quotation
			 * was found at pos.
			 */
			endpos = buf.line1 + length - 1;
			for (;;) {
				if (*endpos == '\"') break;
				endpos--;
			}
			l = endpos - pos;
			switch (atcharset) {
			case CHARGSM:
				DecodeAscii(data->PhonebookEntry->Name, pos, l);
				*(data->PhonebookEntry->Name + l) = '\0';
				break;
			case CHARHEXGSM:
				DecodeHex(data->PhonebookEntry->Name, pos, l);
				*(data->PhonebookEntry->Name + (l / 2)) = '\0';
				break;
			case CHARUCS2:
				DecodeUCS2(data->PhonebookEntry->Name, pos, l);
				*(data->PhonebookEntry->Name + (l / 4)) = '\0';
				break;
			default:
				memcpy(data->PhonebookEntry->Name, pos, l);
				*(data->PhonebookEntry->Name + l) = '\0';
				break;
			}
		}
	}
	return GE_NONE;
}

static GSM_Error ReplyGetSMSCenter(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos;

	if (buffer[0] != GEAT_OK)
		return GE_UNKNOWN; /* FIXME */

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);
	
	if (data->MessageCenter) {
		if (strstr(buf.line2,"+CSCA")) {
			pos = strchr(buf.line2 + 8, '\"');
			if (pos) {
				*pos = '\0';
				data->MessageCenter->No = 1;
				strncpy(data->MessageCenter->SMSC.Number, buf.line2 + 8, MAX_BCD_STRING_LENGTH);
		                data->MessageCenter->SMSC.Number[MAX_BCD_STRING_LENGTH - 1] = '\0';
				if (data->MessageCenter->SMSC.Number[0] == '+')
					data->MessageCenter->SMSC.Type = SMS_International;
				else
					data->MessageCenter->SMSC.Type = SMS_Unknown;
			} else {
				data->MessageCenter->No = 0;
				strncpy(data->MessageCenter->Name, "SMS Center", GSM_MAX_SMS_CENTER_NAME_LENGTH);
				data->MessageCenter->SMSC.Type = SMS_Unknown;
			}
			data->MessageCenter->DefaultName = 1; /* use default name */
			data->MessageCenter->Format = SMS_FText; /* whatever */
			data->MessageCenter->Validity = SMS_VMax;
			strcpy(data->MessageCenter->Recipient.Number, "") ;
		}
	}
	return GE_NONE;
}

static GSM_Error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos;

	if (buffer[0] != GEAT_OK)
		return GE_INVALIDMEMORYTYPE;

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);

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

	if (buffer[0] != GEAT_OK)
		return GE_UNKNOWN;

	buf.line1 = buffer + 1;
	buf.length= length;
	
	splitlines(&buf);

	if (!strncmp(buf.line1, "AT+CBC", 6)) { /* FIXME realy needed? */
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

	if (buffer[0] != GEAT_OK)
		return GE_UNKNOWN;
		
	buf.line1 = buffer + 1;
	buf.length= length;
	
	splitlines(&buf);

	if ((!strncmp(buf.line1, "AT+CSQ", 6)) && (data->RFUnits)) { /*FIXME realy needed? */
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

	if (buffer[0] != GEAT_OK)
		return GE_UNKNOWN;		/* Fixme */
	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);
	if (!strncmp(buf.line1, "AT+CG", 5)) {
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "SN", data->Imei);
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "MM", data->Model);
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "MI", data->Manufacturer);
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "MR", data->Revision);
	}
	return GE_NONE;
}

static GSM_Error ReplyCallDivert(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	int i;
	for (i = 0; i < length; i++) {
		dprintf("%02x ", buffer[i + 1]);
	}
	dprintf("\n");
	return GE_NONE;
}

static GSM_Error ReplyGetPrompt(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	if (buffer[0] == GEAT_PROMPT) return GE_NONE;
	return GE_INTERNALERROR;
}

static GSM_Error ReplySendSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;

	if (buffer[0] != GEAT_OK)
		return GE_SMSSENDFAILED;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	/* SendSMS or SaveSMS */
	if (!strncmp("+CMGW:", buf.line2, 6) ||
	    !strncmp("+CMGS:", buf.line2, 6))
		data->SMS->Number = atoi(buf.line2 + 6);
	else
		data->SMS->Number = -1;
	dprintf("Message sent okay\n");
	return GE_NONE;
}

static GSM_Error ReplyGetSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	unsigned int sms_len, l, offset = 0;
	char *tmp = NULL;

	if (buffer[0] != GEAT_OK)
		return GE_INTERNALERROR;

	buf.line1 = buffer + 1;
	buf.length = length;

	splitlines(&buf);

	if (!data->RawSMS) return GE_INTERNALERROR;
	
	sms_len = strlen(buf.line3) / 2;
	tmp = calloc(sms_len, 1);
	dprintf("%s\n", buf.line3);
	hex2bin(tmp, buf.line3, sms_len);
	memcpy(data->RawSMS->MessageCenter, tmp, tmp[offset] + 1);
	offset += tmp[offset] + 1;
	data->RawSMS->Type             = tmp[offset] & 0x03;
	data->RawSMS->UDHIndicator     = tmp[offset];
	data->RawSMS->MoreMessages     = tmp[offset];
	data->RawSMS->ReportStatus     = tmp[offset];
	l = (tmp[offset + 1] % 2) ? tmp[offset + 1] + 1 : tmp[offset + 1] ;
	l = l / 2 + 2;
	memcpy(data->RawSMS->RemoteNumber, tmp + offset + 1, l);
	offset += l;
	data->RawSMS->ReplyViaSameSMSC = 0;
	data->RawSMS->RejectDuplicates = 0;
	data->RawSMS->Report           = 0;
	data->RawSMS->Reference        = 0;
	data->RawSMS->PID              = tmp[offset + 1];
	data->RawSMS->DCS              = tmp[offset + 2];
	memcpy(data->RawSMS->SMSCTime, tmp + offset + 3, 7);
	data->RawSMS->Length           = tmp[offset + 10] & 0x00ff;
	memcpy(data->RawSMS->UserData, tmp + offset + 11, sms_len - offset - 11);
	free(tmp);
	return GE_NONE;
}


/* ReplyGetCharset
 *
 * parses the reponse from a check for the actual charset or the
 * available charsets. a bracket in the response is taken as a request
 * for available charsets.
 *
 * GSM_Data has no field for charset so you must misuse Model. the Model
 * string must be initialized with a length of 256 bytes.
 */
static GSM_Error ReplyGetCharset(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos;

	if (buffer[0] != GEAT_OK)
		return GE_UNKNOWN;
		
	buf.line1 = buffer + 1;
	buf.length= length;
	splitlines(&buf);

	if ((!strncmp(buf.line1, "AT+CSCS", 7)) && (data->Model)) {
		/* if a opening bracket is in the string don't skip anything */
		pos = strchr(buf.line2, '(');
		if (pos) {
			strncpy(data->Model, buf.line2, 255);
			return GE_NONE;
		}
		/* skip leading +CSCS: and quotation */
		pos = strchr(buf.line2 + 8, '\"');
		if (pos) *pos = '\0';
		strncpy(data->Model, buf.line2 + 8, 255);
		(data->Model)[255] = '\0';
	}
	return GE_NONE;
}


static GSM_Error ReplyGetSecurityCodeStatus(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
	char *pos;

	if (buffer[0] != GEAT_OK)
		return GE_UNKNOWN;
		
	buf.line1 = buffer + 1;
	buf.length= length;
	splitlines(&buf);

	if ((!strncmp(buf.line1, "AT+CPIN", 7)) && (data->SecurityCode)) {
		if (strncmp(buf.line2, "+CPIN: ", 7)) {
			data->SecurityCode->Type = 0;
			return GE_INTERNALERROR;
		}

		pos = 7 + buf.line2;

		if (!strncmp(pos, "READY", 5)) {
			data->SecurityCode->Type = GSCT_None;
			return GE_NONE;
		}

		if (!strncmp(pos, "SIM ", 4)) {
			pos += 4;
			if (!strncmp(pos, "PIN2", 4)) {
				data->SecurityCode->Type = GSCT_Pin2;
			}
			if (!strncmp(pos, "PUK2", 4)) {
				data->SecurityCode->Type = GSCT_Puk2;
			}
			if (!strncmp(pos, "PIN", 3)) {
				data->SecurityCode->Type = GSCT_Pin;
			}
			if (!strncmp(pos, "PUK", 3)) {
				data->SecurityCode->Type = GSCT_Puk;
			}
		}
	}
	return GE_NONE;
}


/* General reply function for phone responses. buffer[0] holds the compiled
 * success of the result (OK, ERROR, ... ). see links/atbus.h and links/atbus.c 
 * for reference */
static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	if (buffer[0] != GEAT_OK)
		return GE_UNKNOWN;

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

	for (i = 0; i < GOPAT_Max; i++) {
		AT_Functions[i] = NULL;
		IncomingFunctions[i].MessageType = 0;
		IncomingFunctions[i].Functions = NULL;
	}
	for (i = 0; i < ARRAY_LEN(AT_FunctionInit); i++) {
		AT_InsertSendFunction(AT_FunctionInit[i].gop, AT_FunctionInit[i].sfunc);
		AT_InsertRecvFunction(AT_FunctionInit[i].gop, AT_FunctionInit[i].rfunc);
	}

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
	case GCT_Irda:
		if (!strcmp(setupdata->Model, "dancall"))
			ret = CBUS_Initialise(state);
		else if (!strcmp(setupdata->Model, "AT-HW"))
			ret = ATBUS_Initialise(state, true);
		else
			ret = ATBUS_Initialise(state, false);
		break;
	default:
		return GE_NOTSUPPORTED;
		break;
	}
	if (ret != GE_NONE) return ret;

	SM_Initialise(state);

	SoftReset(&data, state);
	SetEcho(&data, state);
	StoreDefaultCharset(state);

	/*
	 * detect manufacturer and model for further initialization
	 */
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

	dprintf("Initialisation completed\n");

	return GE_NONE;
}


void splitlines(AT_LineBuffer *buf)
{
	char *pos;

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
	pos = findcrlf(buf->line3, 1, buf->length);
	if (pos) {
		*pos = 0;
		buf->line4 = skipcrlf(++pos);
	} else {
		buf->line4 = buf->line3;
	}
}


/*
 * increments the argument until a char unequal to
 * <cr> or <lf> is found. returns the new position.
 */

char *skipcrlf(unsigned char *str)
{
	if (str == NULL)
		return str;
	while ((*str == '\n') || (*str == '\r') || (*str > 127))
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

char *findcrlf(unsigned char *str, int test, int max)
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
