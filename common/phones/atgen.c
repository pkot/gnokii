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

#include "gnokii-internal.h"
#include "gsm-api.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atbosch.h"
#include "phones/ateric.h"
#include "phones/atnok.h"
#include "phones/atsie.h"
#include "links/atbus.h"
#ifndef WIN32
#  include "links/cbus.h"
#endif

static gn_error Initialise(gn_data *setupdata, struct gn_statemachine *state);
static gn_error Functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error Reply(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyIdentify(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetRFLevel(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetBattery(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyCallDivert(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetPrompt(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplySendSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
/* static gn_error ReplyDeleteSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state); */
static gn_error ReplyGetCharset(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetSMSCenter(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetSecurityCodeStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);

static gn_error AT_Identify(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetModel(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetRevision(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetIMEI(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetManufacturer(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetBattery(gn_data *data,  struct gn_statemachine *state);
static gn_error AT_GetRFLevel(gn_data *data,  struct gn_statemachine *state);
static gn_error AT_GetMemoryStatus(gn_data *data,  struct gn_statemachine *state);
static gn_error AT_ReadPhonebook(gn_data *data,  struct gn_statemachine *state);
static gn_error AT_WritePhonebook(gn_data *data,  struct gn_statemachine *state);
static gn_error AT_CallDivert(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SetPDUMode(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SendSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SaveSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_WriteSMS(gn_data *data, struct gn_statemachine *state, unsigned char *cmd);
static gn_error AT_GetSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_DeleteSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetCharset(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SetCharset(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetSMSCenter(gn_data *data, struct gn_statemachine *state);
static gn_error AT_EnterSecurityCode(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state);

typedef struct {
	int gop;
	at_send_function_type sfunc;
	at_recv_function_type rfunc;
} at_function_init_type;

/* Mobile phone information */
static at_function_init_type at_function_init[] = {
	{ GN_OP_Init,                  NULL,                     Reply },
	{ GN_OP_Terminate,             pgen_terminate,           Reply },
	{ GN_OP_GetModel,              AT_GetModel,              ReplyIdentify },
	{ GN_OP_GetRevision,           AT_GetRevision,           ReplyIdentify },
	{ GN_OP_GetImei,               AT_GetIMEI,               ReplyIdentify },
	{ GN_OP_GetManufacturer,       AT_GetManufacturer,       ReplyIdentify },
	{ GN_OP_Identify,              AT_Identify,              ReplyIdentify },
	{ GN_OP_GetBatteryLevel,       AT_GetBattery,            ReplyGetBattery },
	{ GN_OP_GetPowersource,        AT_GetBattery,            ReplyGetBattery },
	{ GN_OP_GetRFLevel,            AT_GetRFLevel,            ReplyGetRFLevel },
	{ GN_OP_GetMemoryStatus,       AT_GetMemoryStatus,       ReplyMemoryStatus },
	{ GN_OP_ReadPhonebook,         AT_ReadPhonebook,         ReplyReadPhonebook },
	{ GN_OP_WritePhonebook,        AT_WritePhonebook,        Reply },
	{ GN_OP_CallDivert,            AT_CallDivert,            ReplyCallDivert },
	{ GN_OP_AT_SetPDUMode,         AT_SetPDUMode,            Reply },
	{ GN_OP_AT_Prompt,             NULL,                     ReplyGetPrompt },
	{ GN_OP_SendSMS,               AT_SendSMS,               ReplySendSMS },
	{ GN_OP_SaveSMS,               AT_SaveSMS,               ReplySendSMS },
	{ GN_OP_GetSMS,                AT_GetSMS,                ReplyGetSMS },
	{ GN_OP_DeleteSMS,             AT_DeleteSMS,             Reply },
	{ GN_OP_AT_GetCharset,         AT_GetCharset,            ReplyGetCharset },
	{ GN_OP_AT_SetCharset,         AT_SetCharset,            Reply },
	{ GN_OP_GetSMSCenter,          AT_GetSMSCenter,          ReplyGetSMSCenter },
	{ GN_OP_GetSecurityCodeStatus, AT_GetSecurityCodeStatus, ReplyGetSecurityCodeStatus },
	{ GN_OP_EnterSecurityCode,     AT_EnterSecurityCode,     Reply },
};

#define REPLY_SIMPLETEXT(l1, l2, c, t) \
	if ((strcmp(l1, c) == 0) && (t != NULL)) strcpy(t, l2)

gn_driver driver_at = {
	NULL,
	pgen_incoming_default,
	{
		"AT|AT-HW|dancall",	/* Supported models */
		99,			/* Max RF Level */
		0,			/* Min RF Level */
		GN_RF_CSQ,		/* RF level units */
		100,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GN_BU_Percentage,	/* Battery level units */
		0,			/* Have date/time support */
		0,			/* Alarm supports time only */
		0,			/* Alarms available - FIXME */
		0, 0,			/* Startup logo size - FIXME */
		0, 0,			/* Op logo size */
		0, 0			/* Caller logo size */
	},
	Functions,
	NULL
};

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


static gn_error Functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	if (op == GN_OP_Init)
		return Initialise(data, state);
	if ((op > GN_OP_Init) && (op < GN_OP_AT_Max))
		if (drvinst->functions[op])
			return (*(drvinst->functions[op]))(data, state);
	return GN_ERR_NOTIMPLEMENTED;
}

at_recv_function_type at_insert_recv_function(int type, at_recv_function_type func, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_recv_function_type oldfunc;
	int i;

	if (type >= GN_OP_AT_Max) {
		return (at_recv_function_type) -1;
	}
	if (drvinst->if_pos == 0) {
		drvinst->incoming_functions[0].message_type = type;
		drvinst->incoming_functions[0].functions = func;
		drvinst->if_pos++;
		return NULL;
	}
	for (i = 0; i < drvinst->if_pos; i++) {
		if (drvinst->incoming_functions[i].message_type == type) {
			oldfunc = drvinst->incoming_functions[i].functions;
			drvinst->incoming_functions[i].functions = func;
			return oldfunc;
		}
	}
	if (drvinst->if_pos < GN_OP_AT_Max-1) {
		drvinst->incoming_functions[drvinst->if_pos].message_type = type;
		drvinst->incoming_functions[drvinst->if_pos].functions = func;
		drvinst->if_pos++;
	}
	return NULL;
}

at_send_function_type at_insert_send_function(int type, at_send_function_type func, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_send_function_type f;

	f = drvinst->functions[type];
	drvinst->functions[type] = func;
	return f;
}

static gn_error SoftReset(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(4, GN_OP_Init, "ATZ\r", state)) return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Init, data, state);
}

static gn_error SetEcho(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(5, GN_OP_Init, "ATE1\r", state)) return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Init, data, state);
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
 * gn_data has no field for charset so i misuse model.
 */
static void StoreDefaultCharset(struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	char buf[256];
	gn_error ret;

	gn_data_clear(&data);
	data.model = buf;
	ret = state->driver.functions(GN_OP_AT_GetCharset, &data, state);
	if (ret) return;
	if (!strncmp(buf, "GSM", 3)) drvinst->defaultcharset = AT_CHAR_GSM;
	if (strstr(buf, "437")) drvinst->defaultcharset = AT_CHAR_CP437;
	return;
}

gn_error at_memory_type_set(gn_memory_type mt, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	char req[32];
	gn_error ret = GN_ERR_NONE;

	if (mt != drvinst->memorytype) {
		sprintf(req, "AT+CPBS=\"%s\"\r", memorynames[mt]);
		ret = sm_message_send(13, GN_OP_Init, req, state);
		if (ret)
			return GN_ERR_NOTREADY;
		gn_data_clear(&data);
		ret = sm_block_no_retry(GN_OP_Init, &data, state);
		if (ret == GN_ERR_NONE)
			drvinst->memorytype = mt;
	}
	return ret;
}

gn_error AT_SetSMSMemoryType(gn_memory_type mt, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	char req[32];
	gn_error ret = GN_ERR_NONE;

	if (mt != drvinst->smsmemorytype) {
		sprintf(req, "AT+CPMS=\"%s\"\r", memorynames[mt]);
		ret = sm_message_send(13, GN_OP_Init, req, state);
		if (ret != GN_ERR_NONE)
			return GN_ERR_NOTREADY;
		gn_data_clear(&data);
		ret = sm_block_no_retry(GN_OP_Init, &data, state);
		if (ret == GN_ERR_NONE)
			drvinst->smsmemorytype = mt;
	}
	return ret;
}

/* AT_SetCharset
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
 * gn_data has no field for charset so i misuse model.
 */
static gn_error AT_SetCharset(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data tmpdata;
	char charsets[256];
	gn_error ret;

	if (drvinst->charset != AT_CHAR_NONE)
		return GN_ERR_NONE;
	/* check if ucs2 is available */
	ret = sm_message_send(10, GN_OP_AT_GetCharset, "AT+CSCS=?\r", state);
	if (ret)
		return GN_ERR_NOTREADY;
	gn_data_clear(&tmpdata);
	*charsets = '\0';
	tmpdata.model = charsets;
	ret = sm_block_no_retry(GN_OP_AT_GetCharset, &tmpdata, state);
	if (ret) {
		*charsets = '\0';
	}
	else if (strstr(charsets, "UCS2")) {
		/* ucs2 charset found. try to set it */
		ret = sm_message_send(15, GN_OP_Init, "AT+CSCS=\"UCS2\"\r", state);
		if (ret)
			return GN_ERR_NOTREADY;
		gn_data_clear(&tmpdata);
		ret = sm_block_no_retry(GN_OP_Init, &tmpdata, state);
		if (ret == GN_ERR_NONE)
			drvinst->charset = AT_CHAR_UCS2;
	}
	if (drvinst->charset != AT_CHAR_NONE)
		return GN_ERR_NONE;
	/* no ucs2 charset found or error occured */
	if (drvinst->defaultcharset == AT_CHAR_GSM) {
		drvinst->charset = AT_CHAR_GSM;
		if (!strstr(charsets, "HEX")) {
			/* no hex charset found! */
			return GN_ERR_NONE;
		}
		/* try to set hex charset */
		ret = sm_message_send(14, GN_OP_Init, "AT+CSCS=\"HEX\"\r", state);
		if (ret)
			return GN_ERR_NOTREADY;
		gn_data_clear(&tmpdata);
		ret = sm_block_no_retry(GN_OP_Init, &tmpdata, state);
		if (ret == GN_ERR_NONE)
			drvinst->charset = AT_CHAR_HEXGSM;
	} else {
		ret = sm_message_send(14, GN_OP_Init, "AT+CSCS=\"GSM\"\r", state);
		if (ret)
			return GN_ERR_NOTREADY;
		gn_data_clear(&tmpdata);
		ret = sm_block_no_retry(GN_OP_Init, &tmpdata, state);
		if (ret == GN_ERR_NONE)
			drvinst->charset = AT_CHAR_GSM;
	}
	return ret;
}

/* AT_GetCharset
 *
 * this function detects the current charset used by the phone. if it is
 * called immediatedly after a reset of the phone, this is also the
 * original charset of the phone.
 *
 * gn_data has no field for charset so you must misuse model. the Model
 * string must be initialized with a length of 256 bytes.
 */
static gn_error AT_GetCharset(gn_data *data, struct gn_statemachine *state)
{
	gn_error ret;

	ret = sm_message_send(9, GN_OP_AT_GetCharset, "AT+CSCS?\r", state);
	if (ret)
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_AT_GetCharset, data, state);
}

static gn_error AT_Identify(gn_data *data, struct gn_statemachine *state)
{
	gn_error ret;

	if ((ret = state->driver.functions(GN_OP_GetModel, data, state)))
		return ret;
	if ((ret = state->driver.functions(GN_OP_GetManufacturer, data, state)))
		return ret;
	if ((ret = state->driver.functions(GN_OP_GetRevision, data, state)))
		return ret;
	return state->driver.functions(GN_OP_GetImei, data, state);
}

static gn_error AT_GetModel(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(8, GN_OP_Identify, "AT+CGMM\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Identify, data, state);
}

static gn_error AT_GetManufacturer(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(8, GN_OP_Identify, "AT+CGMI\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Identify, data, state);
}

static gn_error AT_GetRevision(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(8, GN_OP_Identify, "AT+CGMR\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Identify, data, state);
}

static gn_error AT_GetIMEI(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(8, GN_OP_Identify, "AT+CGSN\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Identify, data, state);
}

/* gets battery level and power source */
static gn_error AT_GetBattery(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(7, GN_OP_GetBatteryLevel, "AT+CBC\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetBatteryLevel, data, state);
}

static gn_error AT_GetRFLevel(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(7, GN_OP_GetRFLevel, "AT+CSQ\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetRFLevel, data, state);
}

static gn_error AT_GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	gn_error ret;

	ret = at_memory_type_set(data->memory_status->memory_type,  state);
	if (ret)
		return ret;
	if (sm_message_send(9, GN_OP_GetMemoryStatus, "AT+CPBS?\r", state))
		return GN_ERR_NOTREADY;
	ret = sm_block_no_retry(GN_OP_GetMemoryStatus, data, state);
	if (ret != GN_ERR_UNKNOWN)
		return ret;
	if (sm_message_send(10, GN_OP_GetMemoryStatus, "AT+CPBR=?\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetMemoryStatus, data, state);
}

static gn_error AT_ReadPhonebook(gn_data *data, struct gn_statemachine *state)
{
	char req[32];
	gn_error ret;

	ret = state->driver.functions(GN_OP_AT_SetCharset, data, state);
	if (ret)
		return ret;
	ret = at_memory_type_set(data->phonebook_entry->memory_type, state);
	if (ret)
		return ret;
	sprintf(req, "AT+CPBR=%d\r", data->phonebook_entry->location);
	if (sm_message_send(strlen(req), GN_OP_ReadPhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_ReadPhonebook, data, state);
}

static gn_error AT_WritePhonebook(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	int len, ofs;
	char req[256], *tmp;
	gn_error ret;
	
	ret = at_memory_type_set(data->phonebook_entry->memory_type, state);
	if (ret)
		return ret;
	if (data->phonebook_entry->empty)
		len = sprintf(req, "AT+CPBW=%d\r", data->phonebook_entry->location);
	else {
		ret = state->driver.functions(GN_OP_AT_SetCharset, data, state);
		if (ret)
			return ret;
		ofs = sprintf(req, "AT+CPBW=%d,\"%s\",%s,\"",
			      data->phonebook_entry->location,
			      data->phonebook_entry->number,
			      data->phonebook_entry->number[0] == '+' ? "145" : "129");
		len = strlen(data->phonebook_entry->name);
		tmp = req + ofs;
		switch (drvinst->charset) {
		case AT_CHAR_GSM:
			len = char_encode_ascii(tmp, data->phonebook_entry->name, len);
			break;
		case AT_CHAR_HEXGSM:
			char_encode_hex(tmp, data->phonebook_entry->name, len);
			len *= 2;
			break;
		case AT_CHAR_UCS2:
			char_encode_ucs2(tmp, data->phonebook_entry->name, len);
			len *= 4;
			break; 
		default:
			memcpy(tmp, data->phonebook_entry->name, len);
			break;
		}
		tmp[len++] = '"'; tmp[len++] = '\r';
		len += ofs;
	} 
	if (sm_message_send(len, GN_OP_WritePhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_WritePhonebook, data, state);
}

static gn_error AT_CallDivert(gn_data *data, struct gn_statemachine *state)
{
	char req[64];

	if (!data->call_divert) return GN_ERR_UNKNOWN;

	sprintf(req, "AT+CCFC=");

	switch (data->call_divert->type) {
	case GN_CDV_AllTypes:
		strcat(req, "4");
		break;
	case GN_CDV_Busy:
		strcat(req, "1");
		break;
	case GN_CDV_NoAnswer:
		strcat(req, "2");
		break;
	case GN_CDV_OutOfReach:
		strcat(req, "3");
		break;
	default:
		dprintf("3. %d\n", data->call_divert->type);
		return GN_ERR_NOTIMPLEMENTED;
	}
	if (data->call_divert->operation == GN_CDV_Register)
		sprintf(req, "%s,%d,\"%s\",%d,,,%d", req,
			data->call_divert->operation,
			data->call_divert->number.number,
			data->call_divert->number.type,
			data->call_divert->timeout);
	else
		sprintf(req, "%s,%d", req, data->call_divert->operation);

	strcat(req, "\r");

	dprintf("%s", req);
	if (sm_message_send(strlen(req), GN_OP_CallDivert, req, state))
		return GN_ERR_NOTREADY;
	return sm_wait_for(GN_OP_CallDivert, data, state);
}

static gn_error AT_SetPDUMode(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(10, GN_OP_AT_SetPDUMode, "AT+CMGF=0\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_AT_SetPDUMode, data, state);
}

static gn_error AT_SendSMS(gn_data *data, struct gn_statemachine *state)
{
	return AT_WriteSMS(data, state, "CMGS");
}

static gn_error AT_SaveSMS(gn_data *data, struct gn_statemachine *state)
{
	gn_error ret = AT_SetSMSMemoryType(data->raw_sms->memory_type,  state);
	if (ret)
		return ret;
	return AT_WriteSMS(data, state, "CMGW");
}

static gn_error AT_WriteSMS(gn_data *data, struct gn_statemachine *state,
			    unsigned char *cmd)
{
	unsigned char req[10240], req2[5120];
	gn_error error;
	unsigned int length, tmp, offset = 0;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	/* Select PDU mode */
	error = state->driver.functions(GN_OP_AT_SetPDUMode, data, state);
	if (error) {
		dprintf("PDU mode not supported\n");
		return error;
	}
	dprintf("AT mode set\n");

	/* Prepare the message and count the size */
	memcpy(req2, data->raw_sms->message_center,
	       data->raw_sms->message_center[0] + 1);
	offset += data->raw_sms->message_center[0];

	/* Validity period in relative format */
	req2[offset + 1] = 0x01 | 0x10;
	if (data->raw_sms->reject_duplicates) req2[offset + 1] |= 0x04;
	if (data->raw_sms->report) req2[offset + 1] |= 0x20;
	if (data->raw_sms->udh_indicator) req2[offset + 1] |= 0x40;
	if (data->raw_sms->reply_via_same_smsc) req2[offset + 1] |= 0x80;
	req2[offset + 2] = 0x00; /* Message Reference */

	tmp = data->raw_sms->remote_number[0];
	if (tmp % 2) tmp++;
	tmp /= 2;
	memcpy(req2 + offset + 3, data->raw_sms->remote_number, tmp + 2);
	offset += tmp + 1;

	req2[offset + 4] = data->raw_sms->pid;
	req2[offset + 5] = data->raw_sms->dcs;
	req2[offset + 6] = 0xaa; /* Validity period */
	req2[offset + 7] = data->raw_sms->length;
	memcpy(req2 + offset + 8, data->raw_sms->user_data,
	       data->raw_sms->user_data_length);

	length = data->raw_sms->user_data_length + offset + 8;

	/* Length in AT mode is the length of the full message minus
	 * SMSC field length */
	sprintf(req, "AT+%s=%d\r", cmd, length - 1);
	dprintf("Sending initial sequence\n");
	if (sm_message_send(strlen(req), GN_OP_AT_Prompt, req, state))
		return GN_ERR_NOTREADY;
	error = sm_block_no_retry(GN_OP_AT_Prompt, data, state);
	dprintf("Got response %d\n", error);
	if (error)
		return error;

	bin2hex(req, req2, length);
	req[length * 2] = 0x1a;
	req[length * 2 + 1] = 0;
	dprintf("Sending frame: %s\n", req);
	if (sm_message_send(strlen(req), GN_OP_SendSMS, req, state))
		return GN_ERR_NOTREADY;
	do {
		error = sm_block_no_retry_timeout(GN_OP_SendSMS, state->link.sms_timeout, data, state);
	} while (!state->link.sms_timeout && error == GN_ERR_TIMEOUT);
	return error;
}

static gn_error AT_GetSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[16];
	gn_error err = AT_SetSMSMemoryType(data->raw_sms->memory_type,  state);
	if (err)
		return err;
	sprintf(req, "AT+CMGR=%d\r", data->raw_sms->number);
	dprintf("%s", req);
	if (sm_message_send(strlen(req), GN_OP_GetSMS, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetSMS, data, state);
}

static gn_error AT_DeleteSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[16];
	gn_error err = AT_SetSMSMemoryType(data->raw_sms->memory_type,  state);
	if (err)
		return err;
	sprintf(req, "AT+CMGD=%d\r", data->sms->number);
	dprintf("%s", req);

	if (sm_message_send(strlen(req), GN_OP_DeleteSMS, req, state))
 		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_DeleteSMS, data, state);
}

/* 
 * Hey nokia users. don't expect this to return anything useful.
 * You can't read the number set by the phone menu with this command,
 * nor can you change this number by AT commands. Worse, an ATZ will
 * clear a SMS Center Number set by AT commands. This doesn't affect
 * the number set by the phone menu 
 */
static gn_error AT_GetSMSCenter(gn_data *data, struct gn_statemachine *state)
{
 	if (sm_message_send(9, GN_OP_GetSMSCenter, "AT+CSCA?\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetSMSCenter, data, state);
}

static gn_error AT_GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state)
{
 	if (sm_message_send(9, GN_OP_GetSecurityCodeStatus, "AT+CPIN?\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetSecurityCodeStatus, data, state);
}

static gn_error AT_EnterSecurityCode(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[32];

	if (data->security_code->type != GN_SCT_Pin)
		return GN_ERR_NOTIMPLEMENTED;

	sprintf(req, "AT+CPIN=\"%s\"\r", data->security_code->code);
 	if (sm_message_send(strlen(req), GN_OP_EnterSecurityCode, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_EnterSecurityCode, data, state);
}

static gn_error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos, *endpos;
	int l;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_INVALIDLOCATION;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	if (strncmp(buf.line1, "AT+CPBR", 7)) {
		return GN_ERR_UNKNOWN;
	}

	if (!strncmp(buf.line2, "OK", 2)) {
		/* Empty phonebook location found */
		if (data->phonebook_entry) {
			*(data->phonebook_entry->number) = '\0';
			*(data->phonebook_entry->name) = '\0';
			data->phonebook_entry->caller_group = 0;
			data->phonebook_entry->subentries_count = 0;
		}
		return GN_ERR_NONE;
	}
	if (data->phonebook_entry) {
		data->phonebook_entry->caller_group = 0;
		data->phonebook_entry->subentries_count = 0;

		/* store number */
		pos = strchr(buf.line2, '\"');
		endpos = NULL;
		if (pos)
			endpos = strchr(++pos, '\"');
		if (endpos) {
			*endpos = '\0';
			strcpy(data->phonebook_entry->number, pos);
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
			switch (drvinst->charset) {
			case AT_CHAR_GSM:
				char_decode_ascii(data->phonebook_entry->name, pos, l);
				*(data->phonebook_entry->name + l) = '\0';
				break;
			case AT_CHAR_HEXGSM:
				char_decode_hex(data->phonebook_entry->name, pos, l);
				*(data->phonebook_entry->name + (l / 2)) = '\0';
				break;
			case AT_CHAR_UCS2:
				char_decode_ucs2(data->phonebook_entry->name, pos, l);
				*(data->phonebook_entry->name + (l / 4)) = '\0';
				break;
			default:
				memcpy(data->phonebook_entry->name, pos, l);
				*(data->phonebook_entry->name + l) = '\0';
				break;
			}
		}
	}
	return GN_ERR_NONE;
}

static gn_error ReplyGetSMSCenter(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	unsigned char *pos, *aux;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_UNKNOWN; /* FIXME */

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);
	
	if (data->message_center && strstr(buf.line2,"+CSCA")) {
		pos = strchr(buf.line2 + 8, '\"');
		if (pos) {
			*pos++ = '\0';
			data->message_center->id = 1;
			strncpy(data->message_center->smsc.number, buf.line2 + 8, GN_BCD_STRING_MAX_LENGTH);
			data->message_center->smsc.number[GN_BCD_STRING_MAX_LENGTH - 1] = '\0';
			/* Now we look for the number type */
			data->message_center->smsc.type = 0;
			aux = strchr(pos, ',');
			if (aux)
				data->message_center->smsc.type = atoi(++aux);
			else if (data->message_center->smsc.number[0] == '+')
				data->message_center->smsc.type = GN_GSM_NUMBER_International;
			if (!data->message_center->smsc.type)
				data->message_center->smsc.type = GN_GSM_NUMBER_Unknown;
		} else {
			data->message_center->id = 0;
			strncpy(data->message_center->name, "SMS Center", GN_SMS_CENTER_NAME_MAX_LENGTH);
			data->message_center->smsc.type = GN_GSM_NUMBER_Unknown;
		}
		data->message_center->default_name = 1; /* use default name */
		data->message_center->format = GN_SMS_MF_Text; /* whatever */
		data->message_center->validity = GN_SMS_VP_Max;
		strcpy(data->message_center->recipient.number, "") ;
	}
	return GN_ERR_NONE;
}

static gn_error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_INVALIDMEMORYTYPE;

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);

	if (data->memory_status && strstr(buf.line2,"+CPBS")) {
		pos = strchr(buf.line2, ',');
		if (pos) {
			data->memory_status->used = atoi(++pos);
		} else {
			data->memory_status->used = 100;
			data->memory_status->free = 0;
			return GN_ERR_UNKNOWN;
		}
		pos = strchr(pos, ',');
		if (pos) {
			data->memory_status->free = atoi(++pos) - data->memory_status->used;
		} else {
			return GN_ERR_UNKNOWN;
		}
	}
	return GN_ERR_NONE;
}

static gn_error ReplyGetBattery(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_UNKNOWN;

	buf.line1 = buffer + 1;
	buf.length= length;
	
	splitlines(&buf);

	if (!strncmp(buf.line1, "AT+CBC", 6)) { /* FIXME realy needed? */
		if (data->battery_level) {
			*(data->battery_unit) = GN_BU_Percentage;
			pos = strchr(buf.line2, ',');
			if (pos) {
				pos++;
				*(data->battery_level) = atoi(pos);
			} else {
				*(data->battery_level) = 1;
			}
		}
		if (data->power_source) {
			*(data->power_source) = 0;
			if (*buf.line2 == '1') *(data->power_source) = GN_PS_ACDC;
			if (*buf.line2 == '0') *(data->power_source) = GN_PS_BATTERY;
		}
	}
	return GN_ERR_NONE;
}

static gn_error ReplyGetRFLevel(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos1, *pos2;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_UNKNOWN;
		
	buf.line1 = buffer + 1;
	buf.length= length;
	
	splitlines(&buf);

	if (data->rf_unit && !strncmp(buf.line1, "AT+CSQ", 6)) { /*FIXME realy needed? */
		*(data->rf_unit) = GN_RF_CSQ;
		pos1 = buf.line2 + 6;
		pos2 = strchr(buf.line2, ',');
		if (pos1 < pos2) {
			*(data->rf_level) = atoi(pos1);
		} else {
			*(data->rf_level) = 1;
		}
	}
	return GN_ERR_NONE;
}

static gn_error ReplyIdentify(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_UNKNOWN;		/* FIXME */
	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);
	if (!strncmp(buf.line1, "AT+CG", 5)) {
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "SN", data->imei);
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "MM", data->model);
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "MI", data->manufacturer);
		REPLY_SIMPLETEXT(buf.line1+5, buf.line2, "MR", data->revision);
	}
	return GN_ERR_NONE;
}

static gn_error ReplyCallDivert(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	int i;
	for (i = 0; i < length; i++) {
		dprintf("%02x ", buffer[i + 1]);
	}
	dprintf("\n");
	return GN_ERR_NONE;
}

static gn_error ReplyGetPrompt(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	return (buffer[0] == GN_AT_PROMPT) ? GN_ERR_NONE : GN_ERR_INTERNALERROR;
}

static gn_error ReplySendSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_FAILED;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	/* SendSMS or SaveSMS */
	if (!strncmp("+CMGW:", buf.line2, 6) ||
	    !strncmp("+CMGS:", buf.line2, 6))
		data->raw_sms->number = atoi(buf.line2 + 6);
	else
		data->raw_sms->number = -1;
	dprintf("Message sent okay\n");
	return GN_ERR_NONE;
}

static gn_error ReplyGetSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	gn_error ret = GN_ERR_NONE;
	unsigned int sms_len, l, offset = 0;
	char *tmp;
	
	if (buffer[0] != GN_AT_OK)
		return GN_ERR_INTERNALERROR;

	buf.line1 = buffer + 1;
	buf.length = length;

	splitlines(&buf);

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;
	
	sms_len = strlen(buf.line3) / 2;
	tmp = calloc(sms_len, 1);
	if (!tmp) {
		dprintf("Not enough memory for buffer.\n");
		return GN_ERR_INTERNALERROR;
	}
	dprintf("%s\n", buf.line3);
	hex2bin(tmp, buf.line3, sms_len);
	l = tmp[offset] + 1;
	if (l > sms_len || l > GN_SMS_SMSC_NUMBER_MAX_LENGTH) {
		dprintf("Invalid message center length (%d)\n", l);
		ret = GN_ERR_INTERNALERROR;
		goto out;
	}
	memcpy(data->raw_sms->message_center, tmp, l);
	offset += l;
	data->raw_sms->type                = (tmp[offset] & 0x03) << 1;
	data->raw_sms->udh_indicator       = tmp[offset];
	data->raw_sms->more_messages       = tmp[offset];
	data->raw_sms->report_status       = tmp[offset];
	l = (tmp[offset + 1] % 2) ? tmp[offset + 1] + 1 : tmp[offset + 1] ;
	l = l / 2 + 2;
	if (l + offset + 11 > sms_len || l > GN_SMS_NUMBER_MAX_LENGTH) {
		dprintf("Invalid remote number length (%d)\n", l);
		ret = GN_ERR_INTERNALERROR;
		goto out;
	}
	memcpy(data->raw_sms->remote_number, tmp + offset + 1, l);
	offset += l;
	data->raw_sms->reply_via_same_smsc = 0;
	data->raw_sms->reject_duplicates   = 0;
	data->raw_sms->report              = 0;
	data->raw_sms->reference           = 0;
	data->raw_sms->pid                 = tmp[offset + 1];
	data->raw_sms->dcs                 = tmp[offset + 2];
	memcpy(data->raw_sms->time, tmp + offset + 3, 7);
	data->raw_sms->length              = tmp[offset + 10] & 0x00ff;
	if (sms_len - offset - 11 > 1000) {
		dprintf("Phone gave as poisonous (too short?) reply %s, either phone went crazy or communication went out of sync\n", buf.line3);
		ret = GN_ERR_INTERNALERROR;
		goto out;
	}
	memcpy(data->raw_sms->user_data, tmp + offset + 11, sms_len - offset - 11);
out:
	free(tmp);
	return ret;
}

/* ReplyGetCharset
 *
 * parses the reponse from a check for the actual charset or the
 * available charsets. a bracket in the response is taken as a request
 * for available charsets.
 *
 * gn_data has no field for charset so you must misuse model. the Model
 * string must be initialized with a length of 256 bytes.
 */
static gn_error ReplyGetCharset(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_UNKNOWN;
		
	buf.line1 = buffer + 1;
	buf.length= length;
	splitlines(&buf);

	if (data->model && !strncmp(buf.line1, "AT+CSCS", 7)) {
		/* if a opening bracket is in the string don't skip anything */
		pos = strchr(buf.line2, '(');
		if (pos) {
			strncpy(data->model, buf.line2, 255);
			return GN_ERR_NONE;
		}
		/* skip leading +CSCS: and quotation */
		pos = strchr(buf.line2 + 8, '\"');
		if (pos) *pos = '\0';
		strncpy(data->model, buf.line2 + 8, 255);
		(data->model)[255] = '\0';
	}
	return GN_ERR_NONE;
}

static gn_error ReplyGetSecurityCodeStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_UNKNOWN;
		
	buf.line1 = buffer + 1;
	buf.length= length;
	splitlines(&buf);

	if (data->security_code && !strncmp(buf.line1, "AT+CPIN", 7)) {
		if (strncmp(buf.line2, "+CPIN: ", 7)) {
			data->security_code->type = 0;
			return GN_ERR_INTERNALERROR;
		}

		pos = 7 + buf.line2;

		if (!strncmp(pos, "READY", 5)) {
			data->security_code->type = GN_SCT_None;
			return GN_ERR_NONE;
		}

		if (!strncmp(pos, "SIM ", 4)) {
			pos += 4;
			if (!strncmp(pos, "PIN2", 4)) {
				data->security_code->type = GN_SCT_Pin2;
			}
			if (!strncmp(pos, "PUK2", 4)) {
				data->security_code->type = GN_SCT_Puk2;
			}
			if (!strncmp(pos, "PIN", 3)) {
				data->security_code->type = GN_SCT_Pin;
			}
			if (!strncmp(pos, "PUK", 3)) {
				data->security_code->type = GN_SCT_Puk;
			}
		}
	}
	return GN_ERR_NONE;
}

/* General reply function for phone responses. buffer[0] holds the compiled
 * success of the result (OK, ERROR, ... ). see links/atbus.h and links/atbus.c 
 * for reference */
static gn_error Reply(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	return (buffer[0] != GN_AT_OK) ? GN_ERR_UNKNOWN : GN_ERR_NONE;
}

static gn_error Initialise(gn_data *setupdata, struct gn_statemachine *state)
{
	at_driver_instance *drvinst;
	gn_data data;
	gn_error ret = GN_ERR_NONE;
	char model[20];
	char manufacturer[20];
	int i;

	dprintf("Initializing AT capable mobile phone ...\n");
	
	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_at, sizeof(gn_driver));

	if (!(drvinst = malloc(sizeof(at_driver_instance))))
		return GN_ERR_MEMORYFULL;

	state->driver.incoming_functions = drvinst->incoming_functions;
	AT_DRVINST(state) = drvinst;
	drvinst->memorytype = GN_MT_XX;
	drvinst->smsmemorytype = GN_MT_XX;
	drvinst->defaultcharset = AT_CHAR_NONE;
	drvinst->charset = AT_CHAR_NONE;

	drvinst->if_pos = 0;
	for (i = 0; i < GN_OP_AT_Max; i++) {
		drvinst->functions[i] = NULL;
		drvinst->incoming_functions[i].message_type = 0;
		drvinst->incoming_functions[i].functions = NULL;
	}
	for (i = 0; i < ARRAY_LEN(at_function_init); i++) {
		at_insert_send_function(at_function_init[i].gop, at_function_init[i].sfunc, state);
		at_insert_recv_function(at_function_init[i].gop, at_function_init[i].rfunc, state);
	}

	switch (state->link.connection_type) {
	case GN_CT_Serial:
#ifdef HAVE_IRDA
	case GN_CT_Irda:
#endif
		if (!strcmp(setupdata->model, "dancall"))
			ret = cbus_initialise(state);
		else if (!strcmp(setupdata->model, "AT-HW"))
			ret = atbus_initialise(true, state);
		else
			ret = atbus_initialise(false, state);
		break;
	default:
		ret = GN_ERR_NOTSUPPORTED;
		break;
	}
	if (ret) 
		goto out;

	sm_initialise(state);

	/* Dancall is crap and not real AT phone */
	if (!strcmp(setupdata->model, "dancall")) {
		data.manufacturer = "dancall";
		dprintf("Dancall initialisation completed\n");
		goto out;
	}
	
	SoftReset(&data, state);
	SetEcho(&data, state);

	/*
	 * detect manufacturer and model for further initialization
	 */
	gn_data_clear(&data);
	data.model = model;
	ret = state->driver.functions(GN_OP_GetModel, &data, state);
	if (ret) 
		goto out;
	data.manufacturer = manufacturer;
	ret = state->driver.functions(GN_OP_GetManufacturer, &data, state);
	if (ret)
		goto out;

	if (!strncasecmp(manufacturer, "bosch", 5))
		at_bosch_init(model, setupdata->model, state);
	else if (!strncasecmp(manufacturer, "ericsson", 8))
		at_ericsson_init(model, setupdata->model, state);
	else if (!strncasecmp(manufacturer, "nokia", 5))
		at_nokia_init(model, setupdata->model, state);
	else if (!strncasecmp(manufacturer, "siemens", 7))
		at_siemens_init(model, setupdata->model, state);
	
	StoreDefaultCharset(state);

	dprintf("Initialisation completed\n");
out:
	if (ret) {
		dprintf("Initialization failed (%d)\n", ret);
		free(AT_DRVINST(state));
		AT_DRVINST(state) = NULL;
	}
	return ret;
}


void splitlines(at_line_buffer *buf)
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
