/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright 2001 Pavel Machek <pavel@ucw.cz>
  Copyright 2001 Manfred Jonsson
  Copyright 2002-2003 Ladislav Michl <ladis@linux-mips.org>
  Copyright 2001-2004 Pawel Kot

  This file provides functions specific to the Dancall 2711.
  See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "links/atbus.h"

static gn_error FakeCharset(gn_data *data, struct gn_statemachine *state)
{
	AT_DRVINST(state)->charset = AT_CHAR_GSM;
	return GN_ERR_NONE;
}

static gn_error GetSMSStatus(gn_data *data, struct gn_statemachine *state)
{
	if (sm_message_send(18, GN_OP_GetSMSStatus, "AT+CPMS=\"SM\",\"SM\"\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetSMSStatus, data, state);
}

static gn_error ReplyGetSMSStatus(int type, unsigned char *buffer, int length,
				  gn_data *data, struct gn_statemachine *state)
{
	int i, j, k, l;

	if (buffer[0] != GN_AT_OK)
		return GN_ERR_FAILED;
	if (sscanf(buffer + 1, "+CPMS: \"SM\",%d,%d,\"SM\",%d,%d",
		   &i, &j, &k, &l) != 4)
		return GN_ERR_FAILED;
	data->sms_status->unread = i;
	data->sms_status->number = k;
	return GN_ERR_NONE;
}

/*
 * FIXME: SMSs are not ready for text mode :-(
 *
 * message looks like this one (stupid and lying advertisement by Eurotel):
 * +CMGR: "REC READ","999102",,"02/12/20,10:17:57+40"<cr><lf>Prejeme Vam vesele Vanoce a stastny novy rok 2003. I v pristim roce muzete ocekavat spoustu zajimavych nabidek a prekvapeni od Go Clubu.<cr><lf>
*/
static gn_error ReplyGetSMS(int type, unsigned char *buffer, int length,
			    gn_data *data, struct gn_statemachine *state)
{
	if (buffer[0] != GN_AT_OK)
		return GN_ERR_FAILED;
	
	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	data->raw_sms->time[0] = 0;
	data->raw_sms->smsc_time[0] = 0;
	strncpy(data->raw_sms->user_data, buffer, GN_SMS_LONG_MAX_LENGTH);
	data->raw_sms->length = 161;

	return GN_ERR_NONE;
}

static gn_error Unsupported(gn_data *data, struct gn_statemachine *state)
{
	return GN_ERR_NOTSUPPORTED;
}

void dc2711_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	at_insert_send_function(GN_OP_AT_GetCharset, FakeCharset, state);
	at_insert_send_function(GN_OP_AT_SetCharset, FakeCharset, state);
	at_insert_send_function(GN_OP_GetSMSStatus, GetSMSStatus, state);
	at_insert_recv_function(GN_OP_GetSMSStatus, ReplyGetSMSStatus, state);
	at_insert_recv_function(GN_OP_GetSMS, ReplyGetSMS, state);
}
