/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 2001      Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2002      Pavel Machek, Petr Cech
  Copyright (C) 2002-2004 Pawel Kot
  Copyright (C) 2002-2003 Ladis Michl
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2003-2004 Igor Popik
  Copyright (C) 2004      Hugo Hass, Ron Yorston
  Copyright (C) 2007      Jeremy Laine

  This file provides functions specific to generic AT command compatible
  phones. See README for more details on supported mobile phones.

*/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "gnokii-internal.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atbosch.h"
#include "phones/ateric.h"
#include "phones/atmot.h"
#include "phones/atnok.h"
#include "phones/atsie.h"
#include "phones/atsoer.h"
#include "phones/atsam.h"
#include "links/atbus.h"

static gn_error Initialise(gn_data *setupdata, struct gn_statemachine *state);
static gn_error Terminate(gn_data *data, struct gn_statemachine *state);
static gn_error Functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error Reply(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyIdentify(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetRFLevel(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetBattery(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyMemoryRange(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyCallDivert(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetPrompt(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetSMSStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplySendSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
/* static gn_error ReplyDeleteSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state); */
static gn_error ReplyGetCharset(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetSMSCenter(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
#ifdef SECURITY
static gn_error ReplyGetSecurityCodeStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
#endif
static gn_error ReplyGetNetworkInfo(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyRing(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetDateTime(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetActiveCalls(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyIncomingSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error ReplyGetSMSMemorySize(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);

static gn_error AT_Identify(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetModel(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetRevision(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetIMEI(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetManufacturer(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetBattery(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetRFLevel(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetMemoryStatus(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetMemoryRange(gn_data *data, struct gn_statemachine *state);
static gn_error AT_ReadPhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error AT_WritePhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error AT_DeletePhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error AT_CallDivert(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SetPDUMode(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetSMSStatus(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SendSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SaveSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_WriteSMS(gn_data *data, struct gn_statemachine *state, unsigned char *cmd);
static gn_error AT_GetSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_DeleteSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetCharset(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SetCharset(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetSMSCenter(gn_data *data, struct gn_statemachine *state);
#ifdef SECURITY
static gn_error AT_EnterSecurityCode(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state);
#endif
static gn_error AT_DialVoice(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetNetworkInfo(gn_data *data, struct gn_statemachine *state);
static gn_error AT_AnswerCall(gn_data *data, struct gn_statemachine *state);
static gn_error AT_CancelCall(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SetCallNotification(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SetDateTime(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetDateTime(gn_data *data, struct gn_statemachine *state);
static gn_error AT_PrepareDateTime(gn_data *data, struct gn_statemachine *state);
static gn_error AT_SendDTMF(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetActiveCalls(gn_data *data, struct gn_statemachine *state);
static gn_error AT_OnSMS(gn_data *data, struct gn_statemachine *state);
static gn_error AT_GetSMSMemorySize(gn_data *data, struct gn_statemachine *state);

typedef struct {
	int gop;
	at_send_function_type sfunc;
	at_recv_function_type rfunc;
} at_function_init_type;

/* Mobile phone information */
static at_function_init_type at_function_init[] = {
	{ GN_OP_Init,                  NULL,                     Reply },
	{ GN_OP_Terminate,             Terminate,                Reply },
	{ GN_OP_GetModel,              AT_GetModel,              ReplyIdentify },
	{ GN_OP_GetRevision,           AT_GetRevision,           ReplyIdentify },
	{ GN_OP_GetImei,               AT_GetIMEI,               ReplyIdentify },
	{ GN_OP_GetManufacturer,       AT_GetManufacturer,       ReplyIdentify },
	{ GN_OP_Identify,              AT_Identify,              ReplyIdentify },
	{ GN_OP_GetBatteryLevel,       AT_GetBattery,            ReplyGetBattery },
	{ GN_OP_GetPowersource,        AT_GetBattery,            ReplyGetBattery },
	{ GN_OP_GetRFLevel,            AT_GetRFLevel,            ReplyGetRFLevel },
	{ GN_OP_GetMemoryStatus,       AT_GetMemoryStatus,       ReplyMemoryStatus },
	{ GN_OP_AT_GetMemoryRange,     AT_GetMemoryRange,        ReplyMemoryRange },
	{ GN_OP_ReadPhonebook,         AT_ReadPhonebook,         ReplyReadPhonebook },
	{ GN_OP_WritePhonebook,        AT_WritePhonebook,        Reply },
	{ GN_OP_DeletePhonebook,       AT_DeletePhonebook,       Reply },
	{ GN_OP_CallDivert,            AT_CallDivert,            ReplyCallDivert },
	{ GN_OP_AT_SetPDUMode,         AT_SetPDUMode,            Reply },
	{ GN_OP_AT_Prompt,             NULL,                     ReplyGetPrompt },
	{ GN_OP_GetSMSStatus,          AT_GetSMSStatus,          ReplyGetSMSStatus },
	{ GN_OP_SendSMS,               AT_SendSMS,               ReplySendSMS },
	{ GN_OP_SaveSMS,               AT_SaveSMS,               ReplySendSMS },
	{ GN_OP_GetSMS,                AT_GetSMS,                ReplyGetSMS },
	{ GN_OP_DeleteSMS,             AT_DeleteSMS,             Reply },
	{ GN_OP_AT_GetCharset,         AT_GetCharset,            ReplyGetCharset },
	{ GN_OP_AT_SetCharset,         AT_SetCharset,            Reply },
	{ GN_OP_GetSMSCenter,          AT_GetSMSCenter,          ReplyGetSMSCenter },
#ifdef SECURITY
	{ GN_OP_GetSecurityCodeStatus, AT_GetSecurityCodeStatus, ReplyGetSecurityCodeStatus },
	{ GN_OP_EnterSecurityCode,     AT_EnterSecurityCode,     Reply },
#endif
	{ GN_OP_MakeCall,              AT_DialVoice,             Reply },
	{ GN_OP_AnswerCall,            AT_AnswerCall,            Reply },
	{ GN_OP_CancelCall,            AT_CancelCall,            Reply },
	{ GN_OP_AT_Ring,               NULL,                     ReplyRing },
	{ GN_OP_SetCallNotification,   AT_SetCallNotification,   Reply },
	{ GN_OP_GetNetworkInfo,        AT_GetNetworkInfo,        ReplyGetNetworkInfo },
	{ GN_OP_SetDateTime,           AT_SetDateTime,           Reply },
	{ GN_OP_GetDateTime,           AT_GetDateTime,           ReplyGetDateTime },
	{ GN_OP_AT_PrepareDateTime,    AT_PrepareDateTime,       Reply },
	{ GN_OP_SendDTMF,              AT_SendDTMF,              Reply },
	{ GN_OP_GetActiveCalls,        AT_GetActiveCalls,        ReplyGetActiveCalls },
	{ GN_OP_OnSMS,                 AT_OnSMS,                 Reply },
	{ GN_OP_AT_IncomingSMS,        NULL,                     ReplyIncomingSMS },
	{ GN_OP_AT_GetSMSMemorySize,   AT_GetSMSMemorySize,      ReplyGetSMSMemorySize },
};

char *strip_quotes(char *s)
{
	char *t;

	if (*s == '"') {
		if ((t = strrchr(++s, '"'))) {
			*t = '\0';
		}
	}

	return s;
}

static char *strip_brackets(char *s)
{
	char *t ;

	if (*s == '(') {
		if ((t = strrchr(++s, ')'))) {
			*t = '\0';
		}
	}

	return s;
}

static void reply_simpletext(char *l1, char *l2, char *c, char *t, size_t maxlength)
{
	int i, n;

	n = strlen(c);
	if ((strncmp(l1, c, n - 2) == 0) && (t != NULL)) {
		if (strncmp(l2, c, n) == 0)  {
			for (i = n; isspace(l2[i]); i++) ;
			snprintf(t, maxlength, "%s", strip_quotes(l2 + i));
		} else {
			for (i = 0; isspace(l2[i]); i++) ;
			snprintf(t, maxlength, "%s", l2 + i);
		}
	}
}

gn_driver driver_at = {
	NULL,
	pgen_incoming_default,
	{
		"AT|AT-HW",		/* Supported models */
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

char *memorynames[] = {
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

#define NR_MEMORIES (sizeof(memorynames) / sizeof((memorynames)[0]))

typedef struct {
	char *str;
	at_charset charset;
} at_charset_map_t;

static at_charset_map_t atcharsets[] = {
	{ "GSM",	AT_CHAR_GSM },
	{ "HEX",	AT_CHAR_HEXGSM },
	{ "UCS2",	AT_CHAR_UCS2 },
	{ NULL,		AT_CHAR_UNKNOWN },
};

static gn_error Functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	if (op == GN_OP_Init)
		return Initialise(data, state);
	if (drvinst && op == GN_OP_Terminate)
		return Terminate(data, state);
	if (!drvinst)
		return GN_ERR_INTERNALERROR;
	if ((op > GN_OP_Init) && (op < GN_OP_AT_Max))
		if (drvinst->functions[op])
			return (*(drvinst->functions[op]))(data, state);
	return GN_ERR_NOTIMPLEMENTED;
}

/* Functions to encode and decode strings */
size_t at_encode(at_charset charset, char *dst, size_t dst_len, const char *src, size_t len)
{
	size_t ret;

	switch (charset) {
	case AT_CHAR_GSM:
		ret = char_ascii_encode(dst, dst_len, src, len);
		break;
	case AT_CHAR_HEXGSM:
		ret = char_hex_encode(dst, dst_len, src, len);
		break;
	case AT_CHAR_UCS2:
		ret = char_ucs2_encode(dst, dst_len, src, len);
		break; 
	default:
		memcpy(dst, src, dst_len >= len ? len : dst_len);
		ret = len;
		break;
	}
	if (ret < dst_len)
		dst[ret] = '\0';
	return ret+1;
}

void at_decode(int charset, char *dst, char *src, int len)
{
	switch (charset) {
	/* char_*_decode() functions null terminate the strings */
	case AT_CHAR_GSM:
		char_ascii_decode(dst, src, len);
		break;
	case AT_CHAR_HEXGSM:
		char_hex_decode(dst, src, len);
		break;
	case AT_CHAR_UCS2:
		char_ucs2_decode(dst, src, len);
		break;
	default:
		memcpy(dst, src, len);
		dst[len] = 0;
		break;
	}
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

at_error_function_type at_insert_manufacturer_error_function(at_error_function_type func, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_error_function_type f;

	f = drvinst->manufacturer_error;
	drvinst->manufacturer_error = func;
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

static gn_error SetExtendedError(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(10, GN_OP_Init, "AT+CMEE=1\r", state)) return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Init, data, state);
}

gn_error at_error_get(unsigned char *buffer, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	int code;

	switch (buffer[0]) {
	case GN_AT_OK:
		return GN_ERR_NONE;

	case GN_AT_ERROR:
		return GN_ERR_UNKNOWN;

	case GN_AT_CMS:
		code = 256 * buffer[1] + buffer[2];
		switch (code) {
		case 300: return GN_ERR_FAILED;		/* ME failure */
		case 301: return GN_ERR_FAILED;		/* SMS service of ME reserved */
		case 302: return GN_ERR_FAILED;		/* operation not allowed */
		case 303: return GN_ERR_NOTSUPPORTED;	/* operation not supported */
		case 304: return GN_ERR_WRONGDATAFORMAT;/* invalid PDU mode parameter */
		case 305: return GN_ERR_WRONGDATAFORMAT;/* invalid text mode parameter */

		case 310: return GN_ERR_SIMPROBLEM;	/* SIM not inserted */
		case 311: return GN_ERR_CODEREQUIRED;	/* SIM PIN required */
		case 312: return GN_ERR_CODEREQUIRED;	/* PH-SIM PIN required */
		case 313: return GN_ERR_SIMPROBLEM;	/* SIM failure */
		case 314: return GN_ERR_TRYAGAIN;	/* SIM busy */
		case 315: return GN_ERR_SIMPROBLEM;	/* SIM wrong */
		case 316: return GN_ERR_CODEREQUIRED;	/* SIM PUK required */
		case 317: return GN_ERR_CODEREQUIRED;	/* SIM PIN2 required */
		case 318: return GN_ERR_CODEREQUIRED;	/* SIM PUK2 required */

		case 320: return GN_ERR_FAILED;		/* memory failure */
		case 321: return GN_ERR_INVALIDLOCATION;/* invalid memory index */
		case 322: return GN_ERR_MEMORYFULL;	/* memory full */
		
		case 330: return GN_ERR_FAILED;		/* SMSC address unknown */
		case 331: return GN_ERR_NOCARRIER;	/* no network service */
		case 332: return GN_ERR_TIMEOUT;	/* network timeout */

		case 340: return GN_ERR_FAILED;		/* no +CNMA acknowledgement expected */

		case 500: return GN_ERR_UNKNOWN;	/* unknown error */

		default:
			if (code >= 512 && drvinst->manufacturer_error)
				return drvinst->manufacturer_error(GN_AT_CMS, code, state);
			break;
		}
		break;

	case GN_AT_CME:
		code = 256 * buffer[1] + buffer[2];
		switch (code) {
		case   0: return GN_ERR_FAILED;		/* phone failure */
		case   1: return GN_ERR_NOLINK;		/* no connection to phone */
		case   2: return GN_ERR_BUSY;		/* phone-adaptor link reserved */
		case   3: return GN_ERR_FAILED;		/* operation not allowed */
		case   4: return GN_ERR_NOTSUPPORTED;	/* operation not supported */
		case   5: return GN_ERR_CODEREQUIRED;	/* PH-SIM PIN required */
		case   6: return GN_ERR_CODEREQUIRED;	/* PH-FSIM PIN required */
		case   7: return GN_ERR_CODEREQUIRED;	/* PH-FSIM PUK required */

		case  10: return GN_ERR_SIMPROBLEM;	/* SIM not inserted */
		case  11: return GN_ERR_CODEREQUIRED;	/* SIM PIN required */
		case  12: return GN_ERR_CODEREQUIRED;	/* SIM PUK required */
		case  13: return GN_ERR_SIMPROBLEM;	/* SIM failure */
		case  14: return GN_ERR_TRYAGAIN;	/* SIM busy */
		case  15: return GN_ERR_SIMPROBLEM;	/* SIM wrong */
		case  16: return GN_ERR_INVALIDSECURITYCODE;	/* incorrect password */
		case  17: return GN_ERR_CODEREQUIRED;	/* SIM PIN2 required */
		case  18: return GN_ERR_CODEREQUIRED;	/* SIM PUK2 required */

		case  20: return GN_ERR_MEMORYFULL;	/* memory full */
		case  21: return GN_ERR_INVALIDLOCATION;/* invalid index */
		case  22: return GN_ERR_EMPTYLOCATION;	/* not found */
		case  23: return GN_ERR_FAILED;		/* memory failure */
		case  24: return GN_ERR_ENTRYTOOLONG;	/* text string too long */
		case  25: return GN_ERR_WRONGDATAFORMAT;/* invalid characters in text string */
		case  26: return GN_ERR_ENTRYTOOLONG;	/* dial string too long */
		case  27: return GN_ERR_WRONGDATAFORMAT;/* invalid characters in dial string */

		case  30: return GN_ERR_NOCARRIER;	/* no network service */
		case  31: return GN_ERR_TIMEOUT;	/* network timeout */
		case  32: return GN_ERR_FAILED;		/* network not allowed - emergency calls only */
		
		case  40: return GN_ERR_CODEREQUIRED;	/* network personalisation PIN required */
		case  41: return GN_ERR_CODEREQUIRED;	/* network personalisation PUK required */
		case  42: return GN_ERR_CODEREQUIRED;	/* network subset personalisation PIN required */
		case  43: return GN_ERR_CODEREQUIRED;	/* network subset personalisation PUK required */
		case  44: return GN_ERR_CODEREQUIRED;	/* service provider personalisation PIN required */
		case  45: return GN_ERR_CODEREQUIRED;	/* service provider personalisation PUK required */
		case  46: return GN_ERR_CODEREQUIRED;	/* corporate personalisation PIN required */
		case  47: return GN_ERR_CODEREQUIRED;	/* corporate personalisation PUK required */

		case 100: return GN_ERR_UNKNOWN;	/* unknown */

		default:
			if (code >= 512 && drvinst->manufacturer_error)
				return drvinst->manufacturer_error(GN_AT_CME, code, state);
			break;
		}
		break;

	default:
		return GN_ERR_INTERNALERROR;	/* shouldn't happen */
	}

	return GN_ERR_UNKNOWN;
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
 */
static void StoreDefaultCharset(struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	gn_error error;

	gn_data_clear(&data);
	error = state->driver.functions(GN_OP_AT_GetCharset, &data, state);
	drvinst->defaultcharset = error ? AT_CHAR_UNKNOWN : drvinst->charset;
	drvinst->charset = AT_CHAR_UNKNOWN;
}

gn_error at_memory_type_set(gn_memory_type mt, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	char req[32];
	gn_error ret = GN_ERR_NONE;

	if (mt != drvinst->memorytype) {
		if (mt >= NR_MEMORIES)
			return GN_ERR_INVALIDMEMORYTYPE;
		snprintf(req, sizeof(req), "AT+CPBS=\"%s\"\r", memorynames[mt]);
		ret = sm_message_send(13, GN_OP_Init, req, state);
		if (ret)
			return GN_ERR_NOTREADY;
		gn_data_clear(&data);
		ret = sm_block_no_retry(GN_OP_Init, &data, state);
		if (ret)
			return ret;
		drvinst->memorytype = mt;

		gn_data_clear(&data);
		ret = state->driver.functions(GN_OP_AT_GetMemoryRange, &data, state);
	}
	return ret;
}

/* GSM 07.05 section 3.2.3 specified the +CPMS (Preferred Message Store) AT
 * command as taking 3 memory parameters <mem1>, <mem2> and <mem3>.  Each of
 * these stores is used for different purposes:
 *   <mem1> - memory from which messages are read and deleted
 *   <mem2> - memory to which writing and sending operations are made
 *   <mem3> - memory to which received SMs are preferred to be stored
 *
 * According to ETSI TS 127 005 V6.0.1 (2005-01) section 3.2.2, second and
 * third parameter are optional, so we set only <mem1> to be more compatible.
 */
gn_error AT_SetSMSMemoryType(gn_memory_type mt, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	char req[32];
	gn_error ret = GN_ERR_NONE;

	if (mt != drvinst->smsmemorytype) {
		if (mt >= NR_MEMORIES)
			return GN_ERR_INVALIDMEMORYTYPE;
		gn_data_clear(&data);
		snprintf(req, sizeof(req), "AT+CPMS=\"%s\"\r", memorynames[mt]);
		ret = sm_message_send(13, GN_OP_Init, req, state);
		if (ret == GN_ERR_NONE)
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
 * see AT_GetCharset, StoreDefaultCharset
 */
static gn_error AT_SetCharset(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data tmpdata;
	gn_error error;

	if (drvinst->charset != AT_CHAR_UNKNOWN)
		return GN_ERR_NONE;
	/* check available charsets */
	error = sm_message_send(10, GN_OP_AT_GetCharset, "AT+CSCS=?\r", state);
	if (error)
		return error;
	gn_data_clear(&tmpdata);
	error = sm_block_no_retry(GN_OP_AT_GetCharset, &tmpdata, state);
	if (!error && (drvinst->availcharsets & AT_CHAR_UCS2)) {
		/* UCS2 charset found. try to set it */
		error = sm_message_send(15, GN_OP_Init, "AT+CSCS=\"UCS2\"\r", state);
		if (error)
			return error;
		error = sm_block_no_retry(GN_OP_Init, &tmpdata, state);
		if (!error)
			drvinst->charset = AT_CHAR_UCS2;
	}
	if (drvinst->charset != AT_CHAR_UNKNOWN)
		return GN_ERR_NONE;
	/* no UCS2 charset found or error occured */
	if (drvinst->availcharsets & (AT_CHAR_GSM | AT_CHAR_HEXGSM)) {
		/* try to set HEX charset */
		error = sm_message_send(14, GN_OP_Init, "AT+CSCS=\"HEX\"\r", state);
		if (error)
			return error;
		error = sm_block_no_retry(GN_OP_Init, &tmpdata, state);
		if (error) {
			/* no support for HEXGSM, be happy with GSM */ 
			drvinst->charset = AT_CHAR_GSM;
			error = GN_ERR_NONE;
		} else
			drvinst->charset = AT_CHAR_HEXGSM;
	} else {
		drvinst->charset = drvinst->defaultcharset;
		error = (drvinst->charset == AT_CHAR_UNKNOWN) ? error : GN_ERR_NONE;
	}
	return error;
}

/* AT_GetCharset
 *
 * this function detects the current charset used by the phone. if it is
 * called immediatedly after a reset of the phone, this is also the
 * original charset of the phone.
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
	gn_error err;

	if (sm_message_send(8, GN_OP_Identify, "AT+CGMM\r", state))
		return GN_ERR_NOTREADY;
	if ((err=sm_block_no_retry(GN_OP_Identify, data, state)) == GN_ERR_NONE)
		return GN_ERR_NONE;

	if (sm_message_send(7, GN_OP_Identify, "AT+GMM\r", state))
		return GN_ERR_NOTREADY;
	if (sm_block_no_retry(GN_OP_Identify, data, state) == GN_ERR_NONE)
		return GN_ERR_NONE;

	return err;
}

static gn_error AT_GetManufacturer(gn_data *data, struct gn_statemachine *state)
{
	gn_error err;

	if (sm_message_send(8, GN_OP_Identify, "AT+CGMI\r", state))
		return GN_ERR_NOTREADY;
	if ((err = sm_block_no_retry(GN_OP_Identify, data, state)) == GN_ERR_NONE)
		return GN_ERR_NONE;

	if (sm_message_send(7, GN_OP_Identify, "AT+GMI\r", state))
		return GN_ERR_NOTREADY;
	if (sm_block_no_retry(GN_OP_Identify, data, state) == GN_ERR_NONE)
		return GN_ERR_NONE;

	return err;
}

static gn_error AT_GetRevision(gn_data *data, struct gn_statemachine *state)
{
	gn_error err;

	if (sm_message_send(8, GN_OP_Identify, "AT+CGMR\r", state))
		return GN_ERR_NOTREADY;
	if ((err = sm_block_no_retry(GN_OP_Identify, data, state)) == GN_ERR_NONE)
		return GN_ERR_NONE;

	if (sm_message_send(7, GN_OP_Identify, "AT+GMR\r", state))
		return GN_ERR_NOTREADY;
	if (sm_block_no_retry(GN_OP_Identify, data, state) == GN_ERR_NONE)
		return GN_ERR_NONE;

	return err;
}

static gn_error AT_GetIMEI(gn_data *data, struct gn_statemachine *state)
{
	gn_error err;

	if (sm_message_send(8, GN_OP_Identify, "AT+CGSN\r", state))
		return GN_ERR_NOTREADY;
	if ((err = sm_block_no_retry(GN_OP_Identify, data, state)) == GN_ERR_NONE)
		return GN_ERR_NONE;

	if (sm_message_send(7, GN_OP_Identify, "AT+GSN\r", state))
		return GN_ERR_NOTREADY;
	if (sm_block_no_retry(GN_OP_Identify, data, state) == GN_ERR_NONE)
		return GN_ERR_NONE;

	return err;
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
	return sm_block_no_retry(GN_OP_GetMemoryStatus, data, state);
}

static gn_error AT_GetMemoryRange(gn_data *data, struct gn_statemachine *state)
{
	gn_error ret;

	ret = sm_message_send(10, GN_OP_AT_GetMemoryRange, "AT+CPBR=?\r", state);
	if (ret)
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_AT_GetMemoryRange, data, state);
}

static gn_error AT_ReadPhonebook(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	char req[32];
	gn_error ret;

	ret = state->driver.functions(GN_OP_AT_SetCharset, data, state);
	if (ret)
		return ret;
	ret = at_memory_type_set(data->phonebook_entry->memory_type, state);
	if (ret)
		return ret;
	snprintf(req, sizeof(req), "AT+CPBR=%d\r", data->phonebook_entry->location + drvinst->memoryoffset);
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
	if (data->phonebook_entry->empty) {
		return AT_DeletePhonebook(data, state);
	} else {
		ret = state->driver.functions(GN_OP_AT_SetCharset, data, state);
		if (ret)
			return ret;
		ofs = snprintf(req, sizeof(req), "AT+CPBW=%d,\"%s\",%s,\"",
			       data->phonebook_entry->location+drvinst->memoryoffset,
			       data->phonebook_entry->number,
			       data->phonebook_entry->number[0] == '+' ? "145" : "129");
		tmp = req + ofs;
		len = at_encode(drvinst->charset, tmp, sizeof(req) - ofs,
				data->phonebook_entry->name,
				strlen(data->phonebook_entry->name));
		tmp[len-1] = '"';
		tmp[len++] = '\r';
		len += ofs;
	}
	if (sm_message_send(len, GN_OP_WritePhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_WritePhonebook, data, state);
}

static gn_error AT_DeletePhonebook(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	int len;
	char req[64];
	gn_error ret;

	if (!data->phonebook_entry)
		return GN_ERR_INTERNALERROR;

	ret = at_memory_type_set(data->phonebook_entry->memory_type, state);
	if (ret)
		return ret;

	len = snprintf(req, sizeof(req), "AT+CPBW=%d\r", data->phonebook_entry->location+drvinst->memoryoffset);

	if (sm_message_send(len, GN_OP_DeletePhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_DeletePhonebook, data, state);
}

static gn_error AT_CallDivert(gn_data *data, struct gn_statemachine *state)
{
	char req[64], req2[64];

	if (!data->call_divert)
		return GN_ERR_UNKNOWN;

	snprintf(req, sizeof(req), "%s", "AT+CCFC=");

	switch (data->call_divert->type) {
	case GN_CDV_AllTypes:
		strncat(req, "4", sizeof(req) - strlen(req));
		break;
	case GN_CDV_Busy:
		strncat(req, "1", sizeof(req) - strlen(req));
		break;
	case GN_CDV_NoAnswer:
		strncat(req, "2", sizeof(req) - strlen(req));
		break;
	case GN_CDV_OutOfReach:
		strncat(req, "3", sizeof(req) - strlen(req));
		break;
	default:
		dprintf("3. %d\n", data->call_divert->type);
		return GN_ERR_NOTIMPLEMENTED;
	}
	if (data->call_divert->operation == GN_CDV_Register) {
		snprintf(req2, sizeof(req2), ",%d,\"%s\",%d,,,%d",
			 data->call_divert->operation,
			 data->call_divert->number.number,
			 data->call_divert->number.type,
			 data->call_divert->timeout);
		if (strlen(req2) + strlen (req) + 1 > sizeof(req))
			return GN_ERR_FAILED;
		strncat(req, req2, strlen(req2));
	} else {
		snprintf(req2, sizeof(req2), ",%d", data->call_divert->operation);
		if (strlen(req2) + strlen (req) + 1 > sizeof(req))
			return GN_ERR_FAILED;
		strncat(req, req2, strlen(req2));
	}

	if (strlen(req) + 2 > sizeof(req))
		return GN_ERR_FAILED;

	strncat(req, "\r", strlen("\r"));

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

static gn_error AT_GetSMSStatus(gn_data *data, struct gn_statemachine *state)
{
	gn_error ret;

	if (!data->sms_status) return GN_ERR_INTERNALERROR;

        if (data->memory_status) {
                ret = AT_SetSMSMemoryType(data->memory_status->memory_type,  state);
                if (ret != GN_ERR_NONE)
                        return ret;
        }

	ret = sm_message_send(9, GN_OP_GetSMSStatus, "AT+CPMS?\r", state);
	if (ret != GN_ERR_NONE)
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetSMSStatus, data, state);
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
	at_driver_instance *drvinst = AT_DRVINST(state);

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	/* Select PDU mode */
	error = state->driver.functions(GN_OP_AT_SetPDUMode, data, state);
	if (error) {
		dprintf("PDU mode not supported\n");
		return error;
	}
	dprintf("PDU mode set\n");

	/* Prepare the message and count the size */
	if(drvinst->no_smsc) {
		/* not even a length byte included */
		offset--;
	} else {
		memcpy(req2, data->raw_sms->message_center,
		       data->raw_sms->message_center[0] + 1);
		offset += data->raw_sms->message_center[0];
	}
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
	if(drvinst->no_smsc) {
		snprintf(req, sizeof(req), "AT+%s=%d\r", cmd, length);
	} else {
		snprintf(req, sizeof(req), "AT+%s=%d\r", cmd, length - data->raw_sms->message_center[0] - 1);
	}
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
		error = sm_block_no_retry_timeout(GN_OP_SendSMS, state->config.smsc_timeout, data, state);
	} while (!state->config.smsc_timeout && error == GN_ERR_TIMEOUT);
	return error;
}

static gn_error AT_GetSMS(gn_data *data, struct gn_statemachine *state)
{
	/* Sony Ericsson can return on notification a location that is 9-digits long */
	unsigned char req[32];
	gn_error err = AT_SetSMSMemoryType(data->raw_sms->memory_type,  state);

	if (err)
		return err;

	err = state->driver.functions(GN_OP_AT_SetPDUMode, data, state);
	if (err) {
		dprintf("PDU mode not supported\n");
		return err;
	}
	dprintf("PDU mode set\n");

	snprintf(req, sizeof(req), "AT+CMGR=%d\r", data->raw_sms->number);
	if (sm_message_send(strlen(req), GN_OP_GetSMS, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetSMS, data, state);
}

static gn_error AT_DeleteSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[32];
	gn_error err = AT_SetSMSMemoryType(data->raw_sms->memory_type,  state);
	if (err)
		return err;
	snprintf(req, sizeof(req), "AT+CMGD=%d\r", data->sms->number);

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

#ifdef SECURITY
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

	snprintf(req, sizeof(req), "AT+CPIN=\"%s\"\r", data->security_code->code);
 	if (sm_message_send(strlen(req), GN_OP_EnterSecurityCode, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_EnterSecurityCode, data, state);
}
#endif

static gn_error AT_DialVoice(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[32];

	if (!data->call_info)
		return GN_ERR_INTERNALERROR;
	snprintf(req, sizeof(req), "ATDT%s;\r", data->call_info->number);
	if (sm_message_send(strlen(req), GN_OP_MakeCall, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_MakeCall, data, state);
}

static gn_error AT_AnswerCall(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(4, GN_OP_AnswerCall, "ATA\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_AnswerCall, data, state);
}

static gn_error AT_CancelCall(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(8, GN_OP_CancelCall, "AT+CHUP\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_CancelCall, data, state);
}

static gn_error AT_SetCallNotification(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_error err;

	if (!drvinst->call_notification && !data->call_notification)
		return GN_ERR_NONE;

	if (!drvinst->call_notification) {
		if (sm_message_send(9, GN_OP_SetCallNotification, "AT+CRC=1\r", state))
			return GN_ERR_NOTREADY;
		if ((err = sm_block_no_retry(GN_OP_SetCallNotification, data, state)) != GN_ERR_NONE)
			return err;
		if (sm_message_send(10, GN_OP_SetCallNotification, "AT+CLIP=1\r", state))
			return GN_ERR_NOTREADY;
		/* Ignore errors when we can't set Call Line Identity, just set that we
		 * don't handle it */
		if (sm_block_no_retry(GN_OP_SetCallNotification, data, state) == GN_ERR_NONE)
			drvinst->clip_supported = 1;
		if (sm_message_send(10, GN_OP_SetCallNotification, "AT+CLCC=1\r", state))
			return GN_ERR_NOTREADY;
		/* Ignore errors when we can't set List Current Calls notifications */
		sm_block_no_retry(GN_OP_SetCallNotification, data, state);
	}

	drvinst->call_notification = data->call_notification;
	drvinst->call_callback_data = data->callback_data;

	return GN_ERR_NONE;
}

static gn_error AT_GetNetworkInfo(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	if (!data->network_info)
		return GN_ERR_INTERNALERROR;

	if (sm_message_send(10, GN_OP_GetNetworkInfo, "AT+CREG=2\r", state))
		return GN_ERR_NOTREADY;

	error = sm_block_no_retry(GN_OP_GetNetworkInfo, data, state);

	if (sm_message_send(9, GN_OP_GetNetworkInfo, "AT+CREG?\r", state))
		return GN_ERR_NOTREADY;

	if ((error = sm_block_no_retry(GN_OP_GetNetworkInfo, data, state)) != GN_ERR_NONE)
		return error;

	if (sm_message_send(9, GN_OP_GetNetworkInfo, "AT+COPS?\r", state))
		return GN_ERR_NOTREADY;

	return sm_block_no_retry(GN_OP_GetNetworkInfo, data, state);
}

static gn_error AT_SetDateTime(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	char req[64];
	gn_timestamp aux;
	gn_timestamp *dt = data->datetime;

	memset(&aux, 0, sizeof(gn_timestamp));
	/* Make sure GetDateTime doesn't overwrite dt */
	data->datetime = &aux;
	/* ignore errors */
	AT_GetDateTime(data, state);
	AT_PrepareDateTime(data, state);

	data->datetime = dt;
	memset(req, 0, 64);
	if (drvinst->timezone)
		snprintf(req, sizeof(req), "AT+CCLK=\"%02d/%02d/%02d,%02d:%02d:%02d%s\"\r",
			 dt->year % 100, dt->month, dt->day,
			 dt->hour, dt->minute, dt->second, drvinst->timezone);
	else
		snprintf(req, sizeof(req), "AT+CCLK=\"%02d/%02d/%02d,%02d:%02d:%02d\"\r",
			 dt->year % 100, dt->month, dt->day,
			 dt->hour, dt->minute, dt->second);

	if (sm_message_send(strlen(req), GN_OP_SetDateTime, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_SetDateTime, data, state);
}

static gn_error AT_GetDateTime(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(9, GN_OP_GetDateTime, "AT+CCLK?\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetDateTime, data, state);
}

static gn_error AT_PrepareDateTime(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(10, GN_OP_AT_PrepareDateTime, "AT+CCLK=?\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_AT_PrepareDateTime, data, state);
}


/*
 * This command allows to send DTMF tones and arbitrary tones.
 * DTMF is a single ASCII character in the set 0-9, *, #, A-D
 * FIXME: tone and duration parameters are not yet supported
 */
static gn_error AT_SendDTMF(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	unsigned char req[32];
	int len, i, dtmf_len;

	if (!data || !data->dtmf_string)
		return GN_ERR_INTERNALERROR;

	dtmf_len = strlen(data->dtmf_string);
	if (dtmf_len < 1)
		return GN_ERR_WRONGDATAFORMAT;

	/* First let's check out if the command is supported by the phone */
	len = snprintf(req, sizeof(req), "AT+VTS=?\r");
	if (sm_message_send(len, GN_OP_SendDTMF, req, state))
		return GN_ERR_NOTREADY;
	if (sm_block_no_retry(GN_OP_SendDTMF, data, state) != GN_ERR_NONE)	
		return GN_ERR_NOTSUPPORTED;

	/* Send it char by char */
	for (i = 0; i < dtmf_len; i++) {
		len = snprintf(req, sizeof(req), "AT+VTS=%c\r", data->dtmf_string[i]);
		if (sm_message_send(len, GN_OP_SendDTMF, req, state))
			return GN_ERR_NOTREADY;
		error = sm_block_no_retry(GN_OP_SendDTMF, data, state);
		if (error)
			break;
	}

	return error;
}

static gn_error AT_GetActiveCalls(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(8, GN_OP_GetActiveCalls, "AT+CPAS\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetActiveCalls, data, state);
}

static gn_error AT_OnSMS(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_error error;
	int mode;
	char req[13];

	mode = drvinst->cnmi_mode;
	do {
		snprintf(req, sizeof(req), "AT+CNMI=%d,1\r", mode);
		if (sm_message_send(strlen(req), GN_OP_OnSMS, req, state))
			return GN_ERR_NOTREADY;

		error = sm_block_no_retry(GN_OP_OnSMS, data, state);
	} while (mode-- && error);
	if (error == GN_ERR_NONE) {
		AT_DRVINST(state)->on_sms = data->on_sms;
		AT_DRVINST(state)->sms_callback_data = data->callback_data;
	}
	return error;
}

static gn_error AT_GetSMSMemorySize(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(18, GN_OP_AT_GetSMSMemorySize, "AT+CPMS=\"ME\",\"SM\"\r", state))
		return GN_ERR_NOTREADY;

	return sm_block_no_retry(GN_OP_AT_GetSMSMemorySize, data, state);
}

static gn_error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos, *endpos;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return (error == GN_ERR_UNKNOWN) ? GN_ERR_INVALIDLOCATION : error;

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
			data->phonebook_entry->caller_group = GN_PHONEBOOK_GROUP_None;
			data->phonebook_entry->subentries_count = 0;
			data->phonebook_entry->empty = true;
		}
		return GN_ERR_NONE;
	}
	if (data->phonebook_entry) {
		data->phonebook_entry->caller_group = GN_PHONEBOOK_GROUP_None;
		data->phonebook_entry->subentries_count = 0;
		data->phonebook_entry->empty = false;

		/* store number */
		pos = strchr(buf.line2, '\"');
		dprintf("NUMBER: %s\n", pos);
		endpos = NULL;
		if (pos)
			endpos = strchr(++pos, '\"');
		if (endpos) {
			*endpos = '\0';
			snprintf(data->phonebook_entry->number, sizeof(data->phonebook_entry->number), "%s", pos);
		}

		/* store name */
		pos = NULL;
		if (endpos)
			pos = strchr(endpos+2, ',');
		dprintf("NAME: %s\n", pos);
		endpos = NULL;
		if (pos) {
			pos = strip_quotes(pos+1);
			at_decode(drvinst->charset, data->phonebook_entry->name, pos, strlen(pos));
		}
	}
	return GN_ERR_NONE;
}

static gn_error ReplyGetSMSCenter(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	unsigned char *pos, *aux;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);
	
	if (data->message_center && strstr(buf.line2,"+CSCA")) {
		pos = strchr(buf.line2 + 8, '\"');
		if (pos) {
			*pos++ = '\0';
			data->message_center->id = 1;
			snprintf(data->message_center->smsc.number, GN_BCD_STRING_MAX_LENGTH, "%s", buf.line2 + 8);
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
			snprintf(data->message_center->name, GN_SMS_CENTER_NAME_MAX_LENGTH, "SMS Center");
			data->message_center->smsc.type = GN_GSM_NUMBER_Unknown;
		}
		data->message_center->default_name = 1; /* use default name */
		data->message_center->format = GN_SMS_MF_Text; /* whatever */
		data->message_center->validity = GN_SMS_VP_Max;
		data->message_center->recipient.number[0] = 0;
	}
	return GN_ERR_NONE;
}

static gn_error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return (error == GN_ERR_UNKNOWN) ? GN_ERR_INVALIDMEMORYTYPE : error;

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);

	if (data->memory_status && strstr(buf.line2, "+CPBS")) {
		pos = strchr(buf.line2, ',');
		if (pos) {
			data->memory_status->used = atoi(++pos);
		} else {
			data->memory_status->used = drvinst->memorysize;
			data->memory_status->free = 0;
			return GN_ERR_NOTSUPPORTED;
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

/*
 * Response of of a form:
 * +CPBR: (list of supported indexes), <nlength>, <tlength>
 * 	list of supported indexes is usually in 1, 2500 form
 * 	nlength is the maximal length of the number
 * 	tlength is the maximal length of the name
 */
static gn_error ReplyMemoryRange(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos, *s, *t;
	gn_error error;

	drvinst->memoryoffset = 0;
	drvinst->memorysize = 100;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);

	if (strncmp(buf.line2, "+CPBR: ", 7) == 0) {
		s = buf.line2 + 7;
		pos = strchr(s, ',');
		if (pos) {
			*pos = '\0';
			s = strip_brackets(s);
			t = strchr(s, '-');
			if (t) {
				int first, last;
				first = atoi(s);
				last = atoi(t+1);
				drvinst->memoryoffset = first - 1;
				drvinst->memorysize = last - first + 1;
			}
		}
	}

	return GN_ERR_NONE;
}

static gn_error ReplyGetBattery(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

	buf.line1 = buffer + 1;
	buf.length= length;
	
	splitlines(&buf);

	if (!strncmp(buf.line1, "AT+CBC", 6) && !strncmp(buf.line2, "+CBC: ", 6)) {
		if (data->battery_level) {
			if (data->battery_unit)
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
			pos = buf.line2 + strlen("+CBC: ");
			if (*pos == '1' ||  *pos == '2')
				*(data->power_source) = GN_PS_ACDC;
			else if (*pos == '0')
				*(data->power_source) = GN_PS_BATTERY;
		}
	}
	return GN_ERR_NONE;
}

static gn_error ReplyGetRFLevel(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos1, *pos2;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

	buf.line1 = buffer + 1;
	buf.length= length;
	
	splitlines(&buf);

	if (data->rf_unit && !strncmp(buf.line1, "AT+CSQ", 6)) { /* FIXME realy needed? */
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
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);
	if (!strncmp(buf.line1, "AT+CG", 5)) {
		reply_simpletext(buf.line1+2, buf.line2, "+CGSN: ", data->imei, GN_IMEI_MAX_LENGTH);
		if (!data->model[0])
			reply_simpletext(buf.line1+2, buf.line2, "+CGMM: ", data->model, GN_MODEL_MAX_LENGTH);
		reply_simpletext(buf.line1+2, buf.line2, "+CGMI: ", data->manufacturer, GN_MANUFACTURER_MAX_LENGTH);
		reply_simpletext(buf.line1+2, buf.line2, "+CGMR: ", data->revision, GN_REVISION_MAX_LENGTH);
		if (!data->model[0])
			reply_simpletext(buf.line1+2, buf.line4, "+CGMR: ", data->model, GN_MODEL_MAX_LENGTH);
	} else if (!strncmp(buf.line1, "AT+G", 4)) {
		reply_simpletext(buf.line1+2, buf.line2, "+GSN: ", data->imei, GN_IMEI_MAX_LENGTH);
		if (!data->model[0])
			reply_simpletext(buf.line1+2, buf.line2, "+GMM: ", data->model, GN_MODEL_MAX_LENGTH);
		reply_simpletext(buf.line1+2, buf.line2, "+GMI: ", data->manufacturer, GN_MANUFACTURER_MAX_LENGTH);
		reply_simpletext(buf.line1+2, buf.line2, "+GMR: ", data->revision, GN_REVISION_MAX_LENGTH);
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
	switch (buffer[0]) {
	case GN_AT_PROMPT: return GN_ERR_NONE;
	case GN_AT_OK: return GN_ERR_INTERNALERROR;
	default: return at_error_get(buffer, state);
	}
}

static gn_error ReplyGetSMSStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	if (sscanf(buf.line2, "+CPMS: \"%*c%*c\",%d,%*d", &data->sms_status->number) != 1)
		return GN_ERR_FAILED;

	data->sms_status->unread = 0;
	data->sms_status->changed = 0;
	data->sms_status->folders_count = 0;

	return GN_ERR_NONE;
}

static gn_error ReplySendSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	/* SendSMS or SaveSMS */
	if (!strncmp("+CMGW:", buf.line2, 6) ||
	    !strncmp("+CMGS:", buf.line2, 6))
		data->raw_sms->reference = atoi(buf.line2 + 6);
	else
		data->raw_sms->reference = -1;
	dprintf("Message sent (reference: %d)\n", data->raw_sms->reference);
	return GN_ERR_NONE;
}

static gn_error ReplyGetSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	gn_error ret = GN_ERR_NONE;
	unsigned int sms_len, l, extraoffset, offset = 0;
	unsigned char *tmp;
	gn_error error;
 	at_driver_instance *drvinst = AT_DRVINST(state);

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;
	
	buf.line1 = buffer + 1;
	buf.length = length;

	splitlines(&buf);

	if (!data->raw_sms)
		return GN_ERR_INTERNALERROR;

	/* Try to figure out the status first */
	tmp = strchr(buf.line2, ',');
	if (tmp != NULL && ((char *) tmp - buf.line2 - strlen("+CMGR: ")) >= 1) {
		char *status;
		int len;

		len = (char *) tmp - buf.line2 - strlen("+CMGR: ");
		status = malloc(len + 1);
		if (!status) {
			dprintf("Not enough memory for buffer.\n");
			return GN_ERR_INTERNALERROR;
		}

		memcpy(status, buf.line2 + strlen("+CMGR: "), len);
		status[len] = '\0';

		if (strstr(status, "UNREAD")) {
			data->raw_sms->status = GN_SMS_Unread;
		} else if (strstr(status, "READ")) {
			data->raw_sms->status = GN_SMS_Read;
		} else if (strstr(status, "UNSENT")) {
			data->raw_sms->status = GN_SMS_Unsent;
		} else if (strstr(status, "SENT")) {
			data->raw_sms->status = GN_SMS_Sent;
		} else {
			int s;

			s = atoi(status);
			switch (s) {
			case 0:
				data->raw_sms->status = GN_SMS_Unread;
				break;
			case 1:
				data->raw_sms->status = GN_SMS_Read;
				break;
			case 2:
				data->raw_sms->status = GN_SMS_Unsent;
				break;
			case 3:
				data->raw_sms->status = GN_SMS_Sent;
				break;
			}
		}
		free(status);
	}

	tmp = strrchr(buf.line2, ',');
	/* The following sequence is correct for emtpy location:
	 * w: AT+CMGR=9
	 * r: AT+CMGR=9
	 *  :
	 *  : OK
	 */
	if (!tmp)
		return GN_ERR_EMPTYLOCATION;
	sms_len = atoi(tmp+1);
	if (sms_len == 0)
		return GN_ERR_EMPTYLOCATION;
	
	sms_len = strlen(buf.line3) / 2;
	tmp = calloc(sms_len, 1);
	if (!tmp) {
		dprintf("Not enough memory for buffer.\n");
		return GN_ERR_INTERNALERROR;
	}
	dprintf("%s\n", buf.line3);
	hex2bin(tmp, buf.line3, sms_len);

	if(!drvinst->no_smsc) {
		l = tmp[offset] + 1;
		if (l > sms_len || l > GN_SMS_SMSC_NUMBER_MAX_LENGTH) {
			dprintf("Invalid message center length (%d)\n", l);
			ret = GN_ERR_INTERNALERROR;
			goto out;
		}
		memcpy(data->raw_sms->message_center, tmp, l);
		offset += l;
	}

	data->raw_sms->reject_duplicates   = 0;
	data->raw_sms->report_status       = 0;
	data->raw_sms->reference           = 0;
	data->raw_sms->reply_via_same_smsc = 0;
	data->raw_sms->report              = 0;

	data->raw_sms->type                = (tmp[offset] & 0x03) << 1;
	if (data->raw_sms->type == GN_SMS_MT_Deliver) { 
		dprintf("SMS-DELIVER found\n");
		data->raw_sms->more_messages       = tmp[offset] & 0x04;
		// bits 3 and 4 of the first octet unused;
		data->raw_sms->report_status       = tmp[offset] & 0x20;
		extraoffset = 10;
	} else if (data->raw_sms->type == GN_SMS_MT_Submit) {
		dprintf("SMS-SUBMIT found\n");
		data->raw_sms->reject_duplicates   = tmp[offset] & 0x04;
		data->raw_sms->validity_indicator  = (tmp[offset] & 0x18) >> 3;
		extraoffset = 3;
	} else {
		dprintf("Unknown PDU type %d\n", data->raw_sms->type);
		return GN_ERR_INTERNALERROR;
 	}
 	data->raw_sms->more_messages       = tmp[offset];
	data->raw_sms->udh_indicator       = tmp[offset] & 0x40;
	offset++;

	if (data->raw_sms->type == GN_SMS_MT_Submit) {
		data->raw_sms->reference = tmp[offset++];
	} else {
		data->raw_sms->reference = 0;
	}

	l = (tmp[offset] % 2) ? tmp[offset] + 1 : tmp[offset];
	l = l / 2 + 2;
	if (l + offset + extraoffset > sms_len || l > GN_SMS_NUMBER_MAX_LENGTH) {
		dprintf("Invalid remote number length (%d)\n", l);
		ret = GN_ERR_INTERNALERROR;
		goto out;
	}
	memcpy(data->raw_sms->remote_number, tmp + offset, l);
	offset += l;
	data->raw_sms->pid                 = tmp[offset++];
	data->raw_sms->dcs                 = tmp[offset++];
	if (data->raw_sms->type == GN_SMS_MT_Deliver) {
		memcpy(data->raw_sms->smsc_time, tmp + offset, 7);
		offset += 7;
	}
	if (data->raw_sms->type == GN_SMS_MT_Submit) {
		if (data->raw_sms->validity_indicator == GN_SMS_VP_None) {
			offset += 0;
		} else if (data->raw_sms->validity_indicator == GN_SMS_VP_RelativeFormat) {
			// FIXME save this info
			offset += 1;
		} else if (data->raw_sms->validity_indicator == GN_SMS_VP_EnhancedFormat ||
			   data->raw_sms->validity_indicator == GN_SMS_VP_AbsoluteFormat) {
			// FIXME save this info
			offset += 7;
		} else { 
			dprintf("Internal Error on validity_indicator");
			return GN_ERR_INTERNALERROR;
		}
	}
	data->raw_sms->length              = tmp[offset++];
	data->raw_sms->user_data_length = data->raw_sms->length;
	if (data->raw_sms->udh_indicator & 0x40)
		data->raw_sms->user_data_length -= tmp[offset] + 1;
	if (sms_len - offset > 1000) {
		dprintf("Phone gave as poisonous (too short?) reply %s, either phone went crazy or communication went out of sync\n", buf.line3);
		ret = GN_ERR_INTERNALERROR;
		goto out;
	}
	memcpy(data->raw_sms->user_data, tmp + offset, sms_len - offset);
out:
	free(tmp);
	return ret;
}

/* ReplyGetCharset
 *
 * parses the reponse from a check for the actual charset or the
 * available charsets. a bracket in the response is taken as a request
 * for available charsets.
 */
static gn_error ReplyGetCharset(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	int i;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

	buf.line1 = buffer + 1;
	buf.length= length;
	splitlines(&buf);

	if (!strncmp(buf.line1, "AT+CSCS?", 8)) {
		/* return current charset */
		drvinst->charset = AT_CHAR_UNKNOWN; i = 0;
		while (atcharsets[i].str && drvinst->charset != AT_CHAR_UNKNOWN) {
			if (strstr(buf.line2, atcharsets[i].str))
				drvinst->charset = atcharsets[i].charset;
			i++;
		}
		return GN_ERR_NONE;
	} else if (!strncmp(buf.line1, "AT+CSCS=", 8)) {
		/* return available charsets */
		drvinst->availcharsets = 0; i = 0;
		while (atcharsets[i].str) {
			if (strstr(buf.line2, atcharsets[i].str))
				drvinst->availcharsets |= atcharsets[i].charset;
			i++;
		}
		return GN_ERR_NONE;
	}
	return GN_ERR_FAILED;
}

#ifdef SECURITY
static gn_error ReplyGetSecurityCodeStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

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
#endif

static gn_error ReplyRing(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos;
	gn_call_info cinfo;
	gn_call_status status;

	if (!drvinst->call_notification) return GN_ERR_UNSOLICITED;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);

	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.call_id = 1;

	if (!strncmp(buf.line1, "RING", 4)) {
		return GN_ERR_INTERNALERROR; /* AT+CRC=1 disables RING */
	} else if (!strncmp(buf.line1, "+CRING: ", 8)) {
		pos = buf.line1 + 8;
		if (!strncmp(pos, "VOICE", 5))
			cinfo.type = GN_CALL_Voice;
		else
			return GN_ERR_UNHANDLEDFRAME;
		status = GN_CALL_Incoming;
	} else if (!strncmp(buf.line1, "CONNECT", 7)) {
		status = GN_CALL_Established;
	} else if (!strncmp(buf.line1, "BUSY", 4)) {
		status = GN_CALL_RemoteHangup;
	} else if (!strncmp(buf.line1, "NO ANSWER", 9)) {
		status = GN_CALL_RemoteHangup;
	} else if (!strncmp(buf.line1, "NO CARRIER", 10)) {
		status = GN_CALL_RemoteHangup;
	} else if (!strncmp(buf.line1, "NO DIALTONE", 11)) {
		status = GN_CALL_LocalHangup;
	} else if (!strncmp(buf.line1, "+CLIP: ", 7)) {
		char **items;
		int i;

		items = gnokii_strsplit(buf.line1 + 7, ",", 6);
		for (i = 0; i < 6; i++) {
			if (items[i] == NULL)
				break;
			switch (i) {
			/* Number, if known */
			case 0:
				snprintf(cinfo.number, GN_PHONEBOOK_NUMBER_MAX_LENGTH, "%s", strip_quotes(items[i]));
				break;
			/* 1 == Number "type"
			 * 2 == String type subaddress
			 * 3 == Type of subaddress
			 * All ignored
			 */
			case 1:
			case 2:
			case 3:
				break;
			/* Name, if known */
			case 4:
				snprintf(cinfo.name, GN_PHONEBOOK_NAME_MAX_LENGTH, "%s", strip_quotes(items[i]));
				break;
			/* Validity of the name/number provided */
			case 5:
				switch (atoi(items[i])) {
				case 1:
					snprintf(cinfo.name, GN_PHONEBOOK_NAME_MAX_LENGTH, _("Withheld"));
					break;
				case 2:
					snprintf(cinfo.name, GN_PHONEBOOK_NAME_MAX_LENGTH, _("Unknown"));
					break;
				}
			}
		}

		if (cinfo.name == NULL)
			snprintf(cinfo.name, GN_PHONEBOOK_NAME_MAX_LENGTH, _("Unknown"));
		cinfo.type = drvinst->last_call_type;
		drvinst->call_notification(drvinst->last_call_status, &cinfo, state, drvinst->call_callback_data);
		gnokii_strfreev(items);
		return GN_ERR_UNSOLICITED;
	} else if (!strncmp(buf.line1, "+CLCC: ", 7)) {
		char **items;
		int i;

		items = gnokii_strsplit(buf.line1 + 7, ",", 8);
		status = -1;

		for (i = 0; i < 8; i++) {
			if (items[i] == NULL)
				break;
			switch (i) {
			/* Call ID */
			case 0:
				cinfo.call_id = atoi(items[i]);
				break;
			/* Incoming, or finishing call */
			case 1:
				break;
			case 2:
				switch (atoi(items[i])) {
				case 0:
					status = GN_CALL_Established;
					break;
				case 1:
					status = GN_CALL_Held;
					break;
				case 2:
					status = GN_CALL_Dialing;
					break;
				case 3:
					/* Alerting, remote end is ringing */
					status = GN_CALL_Ringing;
					break;
				case 4:
					status = GN_CALL_Incoming;
					break;
				case 5:
					/* FIXME Call is "Waiting" */
					status = GN_CALL_Held;
					break;
				case 6:
					status = GN_CALL_LocalHangup;
					break;
				}
				break;
			case 3:
				if (atoi(items[i]) == 0)
					cinfo.type = GN_CALL_Voice;
				else {
					/* We don't handle non-voice calls */
					gnokii_strfreev(items);
					return GN_ERR_UNHANDLEDFRAME;
				}
				break;
			/* Multiparty status */
			case 4:
				break;
			case 5:
				snprintf(cinfo.number, GN_PHONEBOOK_NUMBER_MAX_LENGTH, "%s", strip_quotes (items[i]));
				break;
			/* Phone display format */
			case 6:
				break;
			case 7:
				snprintf(cinfo.name, GN_PHONEBOOK_NAME_MAX_LENGTH, "%s", strip_quotes (items[i]));
				break;
			}
		}

		/* FIXME we don't handle calls > 1 anywhere else */
		if (status < 0) {
			gnokii_strfreev(items);
			return GN_ERR_UNHANDLEDFRAME;
		}

		drvinst->call_notification(status, &cinfo, state, drvinst->call_callback_data);
		gnokii_strfreev(items);
		return GN_ERR_UNSOLICITED;
	} else {
		return GN_ERR_UNHANDLEDFRAME;
	}

	if (!drvinst->clip_supported || status != GN_CALL_Incoming) {
		drvinst->call_notification(status, &cinfo, state, drvinst->call_callback_data);
	} else {
		drvinst->last_call_status = status;
		drvinst->last_call_type = cinfo.type;
	}

	return GN_ERR_UNSOLICITED;
}

static gn_error ReplyIncomingSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *memory, *pos;
	int index, i;
	gn_memory_type mem;
	int freesms = 0;
	gn_error error = GN_ERR_NONE;

	if (!drvinst->on_sms)
		return GN_ERR_UNSOLICITED;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);

	mem = GN_MT_XX;

	if (strncmp(buf.line1, "+CMTI: ", 7))
		return GN_ERR_UNSOLICITED;

	pos = strrchr(buf.line1, ',');
	if (pos == NULL)
		return GN_ERR_UNSOLICITED;
	pos[0] = '\0';
	pos++;
	index = atoi(pos);

	memory = strip_quotes(buf.line1 + 7);
	if (memory == NULL)
		return GN_ERR_UNSOLICITED;
	for (i = 0; i < NR_MEMORIES; i++) {
		if (!strcmp(memory, memorynames[i])) {
			mem = i;
			break;
		}
	}

	if (mem == GN_MT_XX)
		return GN_ERR_UNSOLICITED;

	dprintf("Received message folder %s index %d\n", memorynames[mem], index);

	if (!data->sms) {
		freesms = 1;
		data->sms = calloc(1, sizeof(gn_sms));
		if (!data->sms)
			return GN_ERR_INTERNALERROR;
	}

	memset(data->sms, 0, sizeof(gn_sms));
	data->sms->memory_type = mem;
	data->sms->number = index;

	dprintf("get sms %d\n", index);
	error = gn_sms_get(data, state);
	if (error == GN_ERR_NONE) {
		error = GN_ERR_UNSOLICITED;
		drvinst->on_sms(data->sms, state, drvinst->sms_callback_data);
	}

	if (freesms) {
		free(data->sms);
		data->sms = NULL;
	}

	return error;
}

static gn_error ReplyGetNetworkInfo(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos;
	char **strings;
	gn_error error;
	int i;

	if (!data->network_info)
		return GN_ERR_INTERNALERROR;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE) return error;

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);

	if (!strncmp(buf.line1, "AT+CREG?", 8)) {
		char tmp[3] = {0, 0, 0};

		strings = gnokii_strsplit(buf.line2, ",", 4);
		i = strings[3] ? 2 : 1;

		if (!strings[i] || strlen(strings[i]) < 6 || !strings[i + 1] || strlen(strings[i + 1]) < 6) {
			gnokii_strfreev(strings);
			return GN_ERR_FAILED;
		}

		pos = strings[i];
		pos++;

		tmp[0] = pos[0];
		tmp[1] = pos[1];

		data->network_info->LAC[0] = strtol(tmp, NULL, 16);

		tmp[0] = pos[2];
		tmp[1] = pos[3];

		data->network_info->LAC[1] = strtol(tmp, NULL, 16);


		pos = strings[i + 1];
		pos++;

		tmp[0] = pos[0];
		tmp[1] = pos[1];

		data->network_info->cell_id[0] = strtol(tmp, NULL, 16);

		tmp[0] = pos[2];
		tmp[1] = pos[3];

		data->network_info->cell_id[1] = strtol(tmp, NULL, 16);

		gnokii_strfreev(strings);

	} else if (!strncmp(buf.line1, "AT+COPS?", 8)) {
		char tmp[128];
		int format;

		memset(tmp, 0, sizeof(tmp));
		strings = gnokii_strsplit(buf.line2, ",", 3);
		format = atoi(strings[1]);
		switch (format) {
		case 0: /* network operator name given */
			pos = strip_quotes(strings[2]);
			at_decode(drvinst->charset, tmp, pos, strlen(pos));
			snprintf(data->network_info->network_code, sizeof(data->network_info->network_code), gn_network_code_get(tmp));
			break;
		case 2: /* network operator code given */
			if (strlen(strings[2]) == 5) { 
				data->network_info->network_code[0] = strings[2][0];
				data->network_info->network_code[1] = strings[2][1];
				data->network_info->network_code[2] = strings[2][2];
				data->network_info->network_code[3] = ' ';
				data->network_info->network_code[4] = strings[2][3];
				data->network_info->network_code[5] = strings[2][4];
				data->network_info->network_code[6] = 0;
			} else if (strlen(strings[2]) >= 6) { 
				data->network_info->network_code[0] = strings[2][1];
				data->network_info->network_code[1] = strings[2][2];
				data->network_info->network_code[2] = strings[2][3];
				data->network_info->network_code[3] = ' ';
				data->network_info->network_code[4] = strings[2][4];
				data->network_info->network_code[5] = strings[2][5];
				data->network_info->network_code[6] = 0;
			} else { /* probably incorrect */
				snprintf(data->network_info->network_code, sizeof(data->network_info->network_code), strings[2]);
			}
			break;
		default: /* FIXME: add error handling */
			break;
		}
		gnokii_strfreev(strings);
	}

	return GN_ERR_NONE;
}

static gn_error ReplyGetDateTime(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	int cnt;
	at_line_buffer buf;
	gn_error error;
	gn_timestamp *dt;
	char timezone[6];

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;

	buf.line1 = buffer + 1;
	buf.length= length;

	splitlines(&buf);

	dt = data->datetime;
	memset(timezone, 0, 6);
	cnt = sscanf(buf.line2, "+CCLK: \"%d/%d/%d,%d:%d:%d%[+-1234567890]\"",
		     &dt->year, &dt->month, &dt->day,
		     &dt->hour, &dt->minute, &dt->second, timezone);
	switch (cnt) {
	case 7:
		drvinst->timezone = realloc(drvinst->timezone, strlen(timezone) + 1);
		strcpy(drvinst->timezone, timezone);
		break;
	case 6:
		break;
	default:
		return GN_ERR_FAILED;
	}

	if (dt->year < 100)
		dt->year += 2000;

	return GN_ERR_NONE;
}

static gn_error ReplyGetActiveCalls(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst;
	gn_error error;
	at_line_buffer buf;
	int status;

	if (!data->call_active)
		return GN_ERR_INTERNALERROR;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;

	status = -1;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	memset(data->call_active, 0, GN_CALL_MAX_PARALLEL * sizeof(gn_call_active));

	if (strncmp(buf.line1, "AT+CPAS", 7)) {
		return GN_ERR_UNKNOWN;
	}

	data->call_active->call_id = 1;

	switch (atoi(buf.line2 + strlen("+CPAS: "))) {
	case 0:
		status = GN_CALL_Idle;
		break;
	/* Terminal doesn't know */
	case 1:
	case 2:
		break;
	case 3:
		status = GN_CALL_Ringing;
		break;
	case 4:
		status = GN_CALL_Established;
		break;
	/* Low-power state, terminal can't answer */
	case 5:
		break;
	}

	if (status < 0)
		return GN_ERR_UNKNOWN;

	drvinst = AT_DRVINST(state);

	data->call_active->state = status;
	data->call_active->prev_state = drvinst->prev_state;

	/* States go:
	 * Idle -> Ringing -> (faked Local hangup) -> Idle
	 * Idle -> Ringing -> Established -> (faked Remote hangup) -> Idle */
	if (drvinst->prev_state == GN_CALL_Ringing && status == GN_CALL_Idle)
		data->call_active->state = GN_CALL_LocalHangup;
	else if (drvinst->prev_state == GN_CALL_Established && status == GN_CALL_Idle)
		data->call_active->state = GN_CALL_RemoteHangup;
	else
		data->call_active->state = status;

	drvinst->prev_state = data->call_active->state;
	snprintf(data->call_active->name, GN_PHONEBOOK_NAME_MAX_LENGTH, _("Unknown"));
	data->call_active->number[0] = '\0';

	return GN_ERR_NONE;
}

static gn_error ReplyGetSMSMemorySize(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	if (sscanf(buf.line2, "+CPMS: %*d,%d,%*d,%d", &drvinst->mememorysize, &drvinst->smmemorysize) != 2)
		return GN_ERR_FAILED;
	drvinst->smsmemorytype = GN_MT_ME;

	return GN_ERR_NONE;
}

/* General reply function for phone responses. buffer[0] holds the compiled
 * success of the result (OK, ERROR, ... ). see links/atbus.h and links/atbus.c 
 * for reference */
static gn_error Reply(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	return at_error_get(buffer, state);
}

#define at_manufacturer_compare(pattern)	strncasecmp(manufacturer, pattern, strlen(pattern))

static gn_error Initialise(gn_data *setupdata, struct gn_statemachine *state)
{
	at_driver_instance *drvinst;
	gn_data data;
	gn_error ret = GN_ERR_NONE;
	char model[GN_MODEL_MAX_LENGTH];
	char manufacturer[GN_MANUFACTURER_MAX_LENGTH];
	int i;

	dprintf("Initializing AT capable mobile phone ...\n");

	/* initialize variables */
	memset(model, 0, GN_MODEL_MAX_LENGTH);
	memset(manufacturer, 0, GN_MANUFACTURER_MAX_LENGTH);

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_at, sizeof(gn_driver));

	if (!(drvinst = malloc(sizeof(at_driver_instance))))
		return GN_ERR_MEMORYFULL;

	state->driver.incoming_functions = drvinst->incoming_functions;
	AT_DRVINST(state) = drvinst;
	drvinst->manufacturer_error = NULL;
	drvinst->memorytype = GN_MT_XX;
	drvinst->memoryoffset = 0;
	drvinst->memorysize = 100;
	drvinst->smsmemorytype = GN_MT_XX;
	drvinst->defaultcharset = AT_CHAR_UNKNOWN;
	drvinst->charset = AT_CHAR_UNKNOWN;
	drvinst->no_smsc = 0;
	drvinst->call_notification = NULL;
	drvinst->call_callback_data = NULL;
	drvinst->on_cell_broadcast = NULL;
	drvinst->cb_callback_data = NULL;
	drvinst->on_sms = NULL;
	drvinst->sms_callback_data = NULL;
	drvinst->clip_supported = 0;
	drvinst->last_call_type = GN_CALL_Voice;
	drvinst->last_call_status = GN_CALL_Idle;
	drvinst->prev_state = GN_CALL_Idle;
	drvinst->mememorysize = -1;
	drvinst->smmemorysize = -1;
	drvinst->timezone = NULL;
	/* FIXME: detect if from AT+CNMI=? */
	drvinst->cnmi_mode = 3;

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

	switch (state->config.connection_type) {
	case GN_CT_Serial:
	case GN_CT_Bluetooth:
	case GN_CT_Irda:
	case GN_CT_TCP:
		if (!strcmp(setupdata->model, "AT-HW"))
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

	SoftReset(&data, state);
	SetEcho(&data, state);
	SetExtendedError(&data, state);

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

	if (!at_manufacturer_compare("bosch"))
		at_bosch_init(model, setupdata->model, state);
	else if (!at_manufacturer_compare("ericsson"))
		at_ericsson_init(model, setupdata->model, state);
	else if (!at_manufacturer_compare("nokia"))
		at_nokia_init(model, setupdata->model, state);
	else if (!at_manufacturer_compare("siemens"))
		at_siemens_init(model, setupdata->model, state);
	else if (!at_manufacturer_compare("sony ericsson"))
		at_sonyericsson_init(model, setupdata->model, state);
	else if (!at_manufacturer_compare("samsung"))
		at_samsung_init(model, setupdata->model, state);
	else if (!at_manufacturer_compare("motorola"))
		at_motorola_init(model, setupdata->model, state);
	
	StoreDefaultCharset(state);

	dprintf("Initialisation completed\n");
out:
	if (ret) {
		dprintf("Initialization failed (%d)\n", ret);
		/* ignore return value from GN_OP_Terminate, will use previous error code instead */
		state->driver.functions(GN_OP_Terminate, &data, state);
	}
	return ret;
}

static gn_error Terminate(gn_data *data, struct gn_statemachine *state)
{
	if (AT_DRVINST(state)) {
		if (AT_DRVINST(state)->timezone) {
			free(AT_DRVINST(state)->timezone);
			AT_DRVINST(state)->timezone = NULL;
		}
		free(AT_DRVINST(state));
		AT_DRVINST(state) = NULL;
	}
	return pgen_terminate(data, state);
}

void splitlines(at_line_buffer *buf)
{
	char *pos;
	int length = buf->length;

	pos = findcrlf(buf->line1, 0, length);
	if (pos) {
		*pos = 0;
		buf->line2 = skipcrlf(++pos);
		length -= (buf->line2 - buf->line1);
	} else {
		buf->line2 = buf->line1;
	}
	pos = findcrlf(buf->line2, 1, length);
	if (pos) {
		*pos = 0;
		buf->line3 = skipcrlf(++pos);
		length -= (buf->line3 - buf->line2);
	} else {
		buf->line3 = buf->line2;
	}
	pos = findcrlf(buf->line3, 1, length);
	if (pos) {
		*pos = 0;
		buf->line4 = skipcrlf(++pos);
		length -= (buf->line4 - buf->line3);
	} else {
		buf->line4 = buf->line3;
	}
	pos = findcrlf(buf->line4, 1, length);
	if (pos) {
		*pos = 0;
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
	while ((max > 0) && (*str != '\n') && (*str != '\r') && ((*str != '\0') || test)) {
		str++;
		max--;
	}
	if ((*str == '\0') || ((max == 0) && (*str != '\n') && (*str != '\r')))
		return NULL;
	return str;
}
