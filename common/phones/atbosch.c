/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright 2002-2003 Ladislav Michl <ladis@linux-mips.org>

  This file provides functions specific to AT commands on Bosch phones.
  See README for more details on supported mobile phones.

*/

#include "config.h"
#include "compat.h"
#include "gnokii-internal.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atbosch.h"
#include "links/atbus.h"


static gn_error FakeCharset(gn_data *data, struct gn_statemachine *state)
{
	AT_DRVINST(state)->charset = AT_CHAR_GSM;
	return GN_ERR_NONE;
}

static gn_error Unsupported(gn_data *data, struct gn_statemachine *state)
{
	return GN_ERR_NOTSUPPORTED;
}

static at_recv_function_type replygetsms;

/*
 * Bosch 909 (and probably also Bosch 908) doesn't provide SMSC information
 * We insert it here to satisfy generic PDU SMS receiving function in atgen.c
 */
static gn_error ReplyGetSMS(int type, unsigned char *buffer, int length,
			    gn_data *data, struct gn_statemachine *state)
{
	int i, ofs, len;
	char *pos, *lenpos = NULL;
	char tmp[8];

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_INVALIDLOCATION;

	pos = buffer + 1;
	for (i = 0; i < 2; i++) {
		pos = findcrlf(pos, 1, length);
		if (!pos)
			return GN_ERR_INTERNALERROR;
		pos = skipcrlf(pos);
		if (i == 0) {
			int j;
			lenpos = pos;
			for (j = 0; j < 2; j++) {
				lenpos = strchr(lenpos, ',');
				if (!lenpos)
					return GN_ERR_INTERNALERROR;
				lenpos++;
			}
		}
	}
	if (!lenpos) return GN_ERR_INTERNALERROR;
	len = atoi(lenpos);

	/* Do we need one more digit? */
	if (len / 10 < (len + 2) / 10)
		memmove(lenpos + 1, lenpos, lenpos - (char*)buffer);
	ofs = snprintf(tmp, 8, "%d", len + 2);
	if (ofs < 1)
		return GN_ERR_INTERNALERROR; /* something went very wrong */
	memcpy(lenpos, tmp, ofs);
	
	/* Insert zero length SMSC field */
	ofs = pos - (char*)buffer;
	memmove(pos + 2, pos, length - ofs);
	buffer[ofs]   = '0';
	buffer[ofs+1] = '0';
	return (*replygetsms)(type, buffer, length + 2, data, state);
}

void at_bosch_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	at_insert_send_function(GN_OP_AT_GetCharset, FakeCharset, state);
	at_insert_send_function(GN_OP_AT_SetCharset, FakeCharset, state);
	replygetsms = at_insert_recv_function(GN_OP_GetSMS, ReplyGetSMS, state);
	/* phone lacks many useful commands :( */
	at_insert_send_function(GN_OP_GetBatteryLevel, Unsupported, state);
	at_insert_send_function(GN_OP_GetRFLevel, Unsupported, state);
	at_insert_send_function(GN_OP_GetSecurityCodeStatus, Unsupported, state);
	at_insert_send_function(GN_OP_EnterSecurityCode, Unsupported, state);
	at_insert_send_function(GN_OP_SaveSMS, Unsupported, state);
}
