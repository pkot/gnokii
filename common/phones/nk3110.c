/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  This file provides functions specific to the 3110 series.
  See README for more details on supported mobile phones.

*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk3110.h"
#include "links/fbus-3110.h"
#include "phones/nokia.h"

#include "gnokii-internal.h"
#include "gsm-api.h"

/* Prototypes */
static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_Initialise(struct gn_statemachine *state);
static gn_error P3110_GetSMSInfo(gn_data *data, struct gn_statemachine *state);
static gn_error P3110_GetPhoneInfo(gn_data *data, struct gn_statemachine *state);
static gn_error P3110_GetStatusInfo(gn_data *data, struct gn_statemachine *state);
static gn_error P3110_Identify(gn_data *data, struct gn_statemachine *state);
static gn_error P3110_GetMemoryStatus(gn_data *data, struct gn_statemachine *state);
static gn_error P3110_GetSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error P3110_DeleteSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error P3110_SendSMSMessage(gn_data *data, struct gn_statemachine *state, bool save_sms);
static gn_error P3110_IncomingNothing(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingCall(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingCallAnswered(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingCallEstablished(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingEndOfOutgoingCall(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingEndOfIncomingCall(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingEndOfOutgoingCall2(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingRestart(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingInitFrame_0x15(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingInitFrame_0x16(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingInitFrame_0x17(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSUserData(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSSend(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSSendError(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSSave(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSSaveError(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSHeader(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSError(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSDelete(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSDeleteError(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSDelivered(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingNoSMSInfo(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingSMSInfo(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingPINEntered(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingStatusInfo(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error P3110_IncomingPhoneInfo(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);

static int sms_header_encode(gn_data *data, struct gn_statemachine *state, unsigned char *req, int ulength, bool save_sms);
static int get_memory_type(gn_memory_type memory_type);

static gn_incoming_function_type incoming_functions[] = {
	{ 0x0a, P3110_IncomingNothing },
	{ 0x0b, P3110_IncomingCall },
	{ 0x0c, P3110_IncomingNothing },
	{ 0x0d, P3110_IncomingCallAnswered },
	{ 0x0e, P3110_IncomingCallEstablished },
	{ 0x0f, P3110_IncomingNothing },
	{ 0x10, P3110_IncomingEndOfOutgoingCall },
	{ 0x11, P3110_IncomingEndOfIncomingCall },
	{ 0x12, P3110_IncomingEndOfOutgoingCall2 },
	{ 0x13, P3110_IncomingRestart },
	{ 0x15, P3110_IncomingInitFrame_0x15 },
	{ 0x16, P3110_IncomingInitFrame_0x16 },
	{ 0x17, P3110_IncomingInitFrame_0x17 },
	{ 0x20, P3110_IncomingNothing },
	/*{ 0x21, P3110_IncomingDTMFSucess },*/
	/*{ 0x22, P3110_IncomingDTMFFailure },*/
	{ 0x23, P3110_IncomingNothing },
	{ 0x24, P3110_IncomingNothing },
	{ 0x25, P3110_IncomingNothing },
	{ 0x26, P3110_IncomingNothing },
	{ 0x27, P3110_IncomingSMSUserData },
	{ 0x28, P3110_IncomingSMSSend },
	{ 0x29, P3110_IncomingSMSSendError },
	{ 0x2a, P3110_IncomingSMSSave },
	{ 0x2b, P3110_IncomingSMSSaveError },
	/* ... */
	{ 0x2c, P3110_IncomingSMSHeader },
	{ 0x2d, P3110_IncomingSMSError },
	{ 0x2e, P3110_IncomingSMSDelete },
	{ 0x2f, P3110_IncomingSMSDeleteError },
	/* ... */
	{ 0x32, P3110_IncomingSMSDelivered },
	{ 0x3f, P3110_IncomingNothing },
	{ 0x40, P3110_IncomingNoSMSInfo },
	{ 0x41, P3110_IncomingSMSInfo },
	{ 0x48, P3110_IncomingPINEntered },
	{ 0x4a, P3110_IncomingNothing },
	{ 0x4b, P3110_IncomingStatusInfo },
	{ 0x4c, P3110_IncomingNothing },
	{ 0x4d, P3110_IncomingPhoneInfo },
	{ 0, NULL}
};

gn_driver driver_nokia_3110 = {
	incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"3110|3810|8110|8110i",	/* Models */
		4,			/* Max RF Level */
		0,			/* Min RF Level */
		GN_RF_Arbitrary,	/* RF level units */
		4,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GN_BU_Arbitrary,	/* Battery level units */
		GN_DT_None,		/* No date/time support */
		GN_DT_None,		/* No alarm support */
		0,			/* Max alarms = 0 */
		0, 0,                   /* Startup logo size */
		0, 0,                   /* Op logo size */
		0, 0                    /* Caller logo size */
	},
	functions,
	NULL
};

static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	switch (op) {
	case GN_OP_Init:
		return P3110_Initialise(state);
	case GN_OP_Terminate:
		return pgen_terminate(data, state);
	case GN_OP_GetModel:
	case GN_OP_GetRevision:
	case GN_OP_GetImei:
		return P3110_GetPhoneInfo(data, state);
	case GN_OP_Identify:
		return P3110_Identify(data, state);
	case GN_OP_GetBatteryLevel:
	case GN_OP_GetRFLevel:
		return P3110_GetStatusInfo(data, state);
	case GN_OP_GetMemoryStatus:
		return P3110_GetMemoryStatus(data, state);
	case GN_OP_ReadPhonebook:
	case GN_OP_WritePhonebook:
	case GN_OP_GetPowersource:
	case GN_OP_GetAlarm:
	case GN_OP_GetSMSStatus:
	case GN_OP_GetIncomingCallNr:
	case GN_OP_GetNetworkInfo:
		return GN_ERR_NOTIMPLEMENTED;
	case GN_OP_GetSMS:
		return P3110_GetSMSMessage(data, state);
	case GN_OP_DeleteSMS:
		return P3110_DeleteSMSMessage(data, state);
	case GN_OP_SendSMS:
		return P3110_SendSMSMessage(data, state, false);
	case GN_OP_SaveSMS:
		return P3110_SendSMSMessage(data, state, true);
	case GN_OP_GetSMSCenter:
		return P3110_GetSMSInfo(data, state);
	case GN_OP_GetSpeedDial:
	case GN_OP_GetDateTime:
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static bool SimAvailable = false;
static int user_data_count = 0;

/* These are related to keepalive functionality */
static bool RequestTerminate;
static bool DisableKeepAlive;
static int KeepAliveTimer;

/* Initialise is the only function allowed to 'use' state */
static gn_error P3110_Initialise(struct gn_statemachine *state)
{
	gn_data data;
	u8 init_sequence[20] = {0x02, 0x01, 0x07, 0xa2, 0x88, 0x81, 0x21, 0x55, 0x63, 0xa8, 0x00, 0x00, 0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80};

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_nokia_3110, sizeof(gn_driver));

	/* Only serial connection is supported */
	if (state->config.connection_type != GN_CT_Serial) return GN_ERR_NOTSUPPORTED;

	/* Initialise FBUS link */
	if (fb3110_initialise(state) != GN_ERR_NONE) {
		dprintf("Error in link initialisation\n");
		return GN_ERR_NOTREADY;
	}

	/* Initialise state machine */
	sm_initialise(state);

	/* 0x15 messages are sent by the PC during the initialisation phase.
	   Anyway, the contents of the message are not understood so we
	   simply send the same sequence observed between the W95 PC and
	   the phone.  The init sequence may still be a bit flaky and is not
	   fully understood. */
	if (sm_message_send(20, 0x15, init_sequence, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;

	/* Wait for response to 0x15 sequence */
	gn_data_clear(&data);
	if (sm_block(0x16, &data, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;

	/* Start sending keepalive messages in separate thread */
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	RequestTerminate = false;
	DisableKeepAlive = false;
	return GN_ERR_NONE;
}


static gn_error P3110_GetSMSInfo(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Getting SMS info...\n");
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (sm_message_send(0, 0x3f, NULL, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return sm_block(0x41, data, state);
}

static gn_error P3110_GetPhoneInfo(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Getting phone info...\n");
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (sm_message_send(0, 0x4c, NULL, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return sm_block(0x4d, data, state);
}

static gn_error P3110_GetStatusInfo(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Getting phone status...\n");
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (sm_message_send(0, 0x4a, NULL, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return sm_block(0x4b, data, state);
}

static gn_error P3110_GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Getting memory status...\n");

	/* Check if this type of memory is available */
	switch (data->memory_status->memory_type) {
	case GN_MT_SM:
		if (!SimAvailable) return GN_ERR_NOTREADY;
		return P3110_GetSMSInfo(data, state);
	case GN_MT_ME:
		if (P3110_MEMORY_SIZE_ME == 0) return GN_ERR_NOTREADY;
		return P3110_GetSMSInfo(data, state);
	default:
		break;
	}
	return GN_ERR_NOTREADY;
}


static gn_error P3110_Identify(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Identifying...\n");
	pnok_manufacturer_get(data->manufacturer);
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (sm_message_send(0, 0x4c, NULL, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	sm_block(0x4d, data, state);

	/* Check that we are back at state Initialised */
	if (gn_sm_loop(0, state) != GN_SM_Initialised) return GN_ERR_UNKNOWN;
	return GN_ERR_NONE;
}


static gn_error P3110_GetSMSMessage(gn_data *data, struct gn_statemachine *state)
{
	int timeout, c, memory_type;
	u8 response = 0, request[2];
	gn_error error = GN_ERR_INTERNALERROR;

	dprintf("Getting SMS message...\n");

	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	/* Set memory type and location in the request */
	memory_type = get_memory_type(data->raw_sms->memory_type);
	if (memory_type == 0) return GN_ERR_INVALIDMEMORYTYPE;
	request[0] = memory_type;
	request[1] = data->raw_sms->number;

	/* 0x25 messages requests the contents of an SMS message
	   from the phone.  The first byte has only ever been
	   observed to be 0x02 - could be selecting internal versus
	   external memory.  Specifying memory 0x00 may request the
	   first location?  Phone replies with 0x2c and 0x27 messages
	   for valid locations, 0x2d for empty ones. */

	if (sm_message_send(2, 0x25, request, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	sm_wait_for(0x2d, data, state);	
	sm_wait_for(0x2c, data, state);	

	timeout = 30; /* ~3secs timeout */

	do {
		gn_sm_loop(1, state);
		timeout--;
	} while ((timeout > 0) && state->received_number == 0);

	/* timeout */
	if (state->received_number == 0) return GN_ERR_TIMEOUT;

	/* find response in state machine */
	for (c = 0; c < state->waiting_for_number; c++) {
		if (state->ResponseError[c] != GN_ERR_BUSY) {
			response = state->waiting_for[c];
			error = state->ResponseError[c];
		}
	}

	/* if (!data->raw_data) return GN_ERR_INTERNALERROR; */

	/* reset state machine */
	sm_reset(state);

	/* process response */
	switch (response) {
	case 0x2c:
		if (error != GN_ERR_NONE) return error;

		/* Block for subsequent content frames... */
		do {
			dprintf("Waiting for content frames...\n");
			sm_block(0x27, data, state);
		} while (user_data_count < data->raw_sms->length);

		return GN_ERR_NONE;
	case 0x2d:
		return error;
	default:
		return GN_ERR_INTERNALERROR;
	}
}


static gn_error P3110_DeleteSMSMessage(gn_data *data, struct gn_statemachine *state)
{
	int timeout, c, memory_type;
	u8 response = 0, request[2];
	gn_error error = GN_ERR_INTERNALERROR;

	dprintf("Deleting SMS message...\n");

	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;

	/* Set memory type and location in the request */
	memory_type = get_memory_type(data->raw_sms->memory_type);
	if (memory_type == 0) return GN_ERR_INVALIDMEMORYTYPE;
	request[0] = memory_type;
	request[1] = data->raw_sms->number;

	/* 0x26 message deletes an SMS message from the phone.
	   The first byte has only ever been observed to be 0x02
	   but is assumed to be selecting internal versus
	   external memory.  Phone replies with 0x2e for valid locations,
	   0x2f for invalid ones.  If a location is empty but otherwise
	   valid 0x2e is still returned. */
	if (sm_message_send(2, 0x26, request, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;

	sm_wait_for(0x2e, data, state);
	sm_wait_for(0x2f, data, state);

	timeout = 30; /* ~3secs timeout */

	do {
		gn_sm_loop(1, state);
		timeout--;
	} while ((timeout > 0) && state->received_number == 0);

	/* timeout */
	if (state->received_number == 0) return GN_ERR_TIMEOUT;

	/* find response in state machine */
	for (c = 0; c < state->waiting_for_number; c++) {
		if (state->ResponseError[c] != GN_ERR_BUSY) {
			response = state->waiting_for[c];
			error = state->ResponseError[c];
		}
	}

	/* reset state machine */
	sm_reset(state);

	/* process response */
	switch (response) {
	case 0x2e:
	case 0x2f:
		return error;
	default:
		return GN_ERR_INTERNALERROR;
	}
}


static gn_error P3110_SendSMSMessage(gn_data *data, struct gn_statemachine *state, bool save_sms)
{
	unsigned char msgtype, hreq[256], req[256], udata[256];
		/* FIXME hardcoded buffer sizes are ugly */
	int c, response, hsize, retry_count, timeout;
	int block_count, uoffset, uremain, ulength, blength;
	gn_error error = GN_ERR_NONE;

	msgtype = save_sms ? 0x24 : 0x23;

	/* FIXME Dirty hacks ahead:
	 * The message data can either be unpacked from the raw_sms
	 * structure - which is stupid - or it can be read directly from
	 * the data->sms structure - which is ugly and breaks the code
	 * structure. But the latter seems to work a bit better with
	 * special characters, not sure why, but anyway this is not the
	 * way things should be done, but it should be fixed elsewhere. */

#if 0
	/* The phone expects ASCII data instead of 7bit packed data, which
	 * is the format used in raw_sms. So the data has to be unpacked
	 * again. */
	
	ulength = char_7bit_unpack(0, data->raw_sms->user_data_length, 
					sizeof(udata),
					data->raw_sms->user_data, udata);
#else
	ulength = strlen(data->sms->user_data[0].u.text);
	memcpy(udata, data->sms->user_data[0].u.text, ulength);
#endif

	hsize = sms_header_encode(data, state, hreq, ulength, save_sms);

	/* We have a loop here as if the response from the phone is
	   0x65 0x26 the rule appears to be just to try sending the
	   message again.  We do this a maximum of P3110_SMS_SEND_RETRY_COUNT
	   times before giving up.  This value is empirical only! */

	retry_count = P3110_SMS_SEND_RETRY_COUNT;

	while (retry_count > 0) {
		KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;

		if (sm_message_send(hsize, msgtype, hreq, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;

		error = sm_block(msgtype, data, state);
		if (error != GN_ERR_NONE) return error;

		/* Now send as many blocks of maximum 55 characters as required
		   to send complete message. */
		block_count = 0;
		uoffset = 0;
		uremain = ulength;

		while (uremain > 0) {
			++block_count;
			blength = uremain;

			/* Limit block length */
			if (blength > 55)	blength = 55;

			/* Create block */
			req[0] = block_count;
			memcpy(req+1, udata + uoffset, blength);

			/* Send block */
			if (sm_message_send(blength+1, 0x27, req, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;
			error = sm_block(0x27, data, state);
			if (error != GN_ERR_NONE) return error;

			/* update remaining and offset values for next block */
			uremain -= blength;
			uoffset += blength;
		}

		/* Now wait for response from network which will see
		   CurrentSMSMessageError change from busy. */
		if (save_sms) {
			sm_wait_for(0x2a, data, state);
			sm_wait_for(0x2b, data, state);
		} else {
			sm_wait_for(0x28, data, state);
			sm_wait_for(0x29, data, state);
		}

		timeout = 1200; /* 120secs timeout */

		do {
			gn_sm_loop(1, state);
			timeout--;
		} while ((timeout > 0) && state->received_number == 0);

		/* timeout */
		if (state->received_number == 0) return GN_ERR_TIMEOUT;

		/* find response in state machine */
		for (c = 0; c < state->waiting_for_number; c++) {
			if (state->ResponseError[c] != GN_ERR_BUSY) {
				response = state->waiting_for[c];
				error = state->ResponseError[c];
			}
		}

		/* reset state machine */
		sm_reset(state);

		/* process response */
		switch (response) {
		case 0x28:
			return error;
		case 0x29:
			/* Got a retry response so try again! */
			dprintf("SMS send attempt failed, trying again...\n");
			retry_count--;
			/* After an empirically determined pause... */
			usleep(500000); /* 0.5 seconds. */
			break;
		case 0x2a:
			return error;
		case 0x2b:
			dprintf("SMS save attempt failed.\n");
			return GN_ERR_FAILED;
		default:
			return GN_ERR_INTERNALERROR;
		}

	}

	/* Retries must have failed. */
	return GN_ERR_FAILED;
}



static gn_error P3110_IncomingNothing(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	return GN_ERR_NONE;
}

/* 0x0b messages are sent by phone when an incoming call occurs,
   this message must be acknowledged. */
static gn_error P3110_IncomingCall(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int     count;
	char    buffer[256];

	/* Get info out of message.  At present, first three bytes are
	   unknown (though third seems to correspond to length of
	   number).  Remaining bytes are the phone number, ASCII
	   encoded. */
	for (count = 0; count < message[4]; count ++) {
		buffer[count] = message[5 + count];
	}
	buffer[count] = 0x00;

	/* Now display incoming call message. */
	dprintf("Incoming call - Type: %s. %02x, Number %s.\n",
		(message[2] == 0x05 ? "Voice":"Data?"), message[3], buffer);

	return GN_ERR_NONE;
}

static gn_error P3110_IncomingCallAnswered(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("Incoming call answered from phone.\n");

	return GN_ERR_NONE;
}

/* Fairly self explanatory these two, though the outgoing
   call message has three (unexplained) data bytes. */
static gn_error P3110_IncomingCallEstablished(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("%s call established - status bytes %02x %02x.\n",
		(message[2] == 0x05 ? "voice":"data(?)"), message[3], message[4]);

	return GN_ERR_NONE;
}

/* 0x10 messages are sent by the phone when an outgoing
   call terminates. */
static gn_error P3110_IncomingEndOfOutgoingCall(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("Call terminated from phone (0x10 message).\n");
	/* FIXME: Tell datapump code that the call has terminated. */
	/*if (CallPassup) {
		CallPassup(' ');
		}*/

	return GN_ERR_NONE;
}

/* 0x11 messages are sent by the phone when an incoming call
   terminates.  There is some other data in the message,
   purpose as yet undertermined. */
static gn_error P3110_IncomingEndOfIncomingCall(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("Call terminated from opposite end of line (or from network).\n");

	/* FIXME: Tell datapump code that the call has terminated. */
	/*if (CallPassup) {
		CallPassup(' ');
		}*/

	return GN_ERR_NONE;
}

/* 0x12 messages are sent after the 0x10 message at the
   end of an outgoing call.  Significance of two messages
   versus the one at the end of an incoming call  is as
   yet undertermined. */
static gn_error P3110_IncomingEndOfOutgoingCall2(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("Call terminated from phone (0x12 message).\n");

	/* FIXME: Tell datapump code that the call has terminated. */
	/*if (CallPassup) {
		CallPassup(' ');
		}*/

	return GN_ERR_NONE;
}


/* 0x13 messages are sent after the phone restarts.
   Re-initialise */

static gn_error P3110_IncomingRestart(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	/* FIXME: send 0x15 message somehow */
	return GN_ERR_NONE;
}


/* 0x15 messages are sent by the phone in response to the
   init sequence sent so we don't acknowledge them! */

static gn_error P3110_IncomingInitFrame_0x15(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	return GN_ERR_NONE;
}


/* 0x16 messages are sent by the phone during initialisation, to response
   to the 0x15 message.
   Sequence bytes have been observed to change with differing software
   versions: V06.61 (19/08/97) sends 0x10 0x02, V07.02 (17/03/98) sends
   0x30 0x02. The actual data byte is 0x02 when SIM memory is available,
   and 0x01 when not (e.g. when SIM card isn't inserted to phone or when
   it is waiting for PIN) */

static gn_error P3110_IncomingInitFrame_0x16(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	SimAvailable = (message[2] == 0x02);
	dprintf("SIM available: %s.\n", (SimAvailable ? "Yes" : "No"));
	return GN_ERR_NONE;
}


static gn_error P3110_IncomingInitFrame_0x17(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("0x17 Registration Response: Failure!\n");
	return GN_ERR_NONE;
}


/* 0x27 messages are a little unusual when sent by the phone in that
   they can either be an acknowledgement of an 0x27 message we sent
   to the phone with message text in it or they could
   contain message text for a message we requested. */

static gn_error P3110_IncomingSMSUserData(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int count;

	/* First see if it was an acknowledgement to one of our messages,
	   if so then nothing to do */
	if (length == 0x02) return GN_ERR_NONE;

	/* This function may be called several times; it accumulates the
	 * SMS content in data->raw_sms->user_data. The static global
	 * user_data_count is used as a counter. */

	/* If this is the first block, reset accumulated message length. */
	if (message[2] == 1) user_data_count = 0;

	count = user_data_count + length - 3;

	memcpy(data->raw_sms->user_data + user_data_count, message + 3, length - 3);

	user_data_count += length - 3;

	return GN_ERR_NONE;
}


/* 0x28 messages are sent by the phone to acknowledge succesfull
   sending of an SMS message.  The byte returned is a receipt
   number of some form, not sure if it's from the network, sending
   sending of an SMS message.  The byte returned is the TP-MR
   (TP-Message-Reference) from sending phone (Also sent to network).
   TP-MR is send from phone within 0x32 message. TP-MR is increased
   by phone after each sent SMS */

static gn_error P3110_IncomingSMSSend(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("SMS send OK (0x%02hhx)\n", message[2]);
	data->raw_sms->number = (int) message[2];
	return GN_ERR_NONE;
}


/* 0x29 messages are sent by the phone to indicate an error in
   sending an SMS message.  Observed values are 0x65 0x15 when
   the phone originated SMS was disabled by the network for
   the particular phone.  0x65 0x26 was observed too, whereupon
   the message was retried. */

static gn_error P3110_IncomingSMSSendError(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("SMS send failed (0x%02hhx 0x%02hhx)\n", message[2], message[3]);
	return GN_ERR_FAILED;
}

/* 0x2a messages are sent by the phone to acknowledge succesful
   saving of an SMS message. */

static gn_error P3110_IncomingSMSSave(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("SMS save OK (0x%02hhx)\n", message[2]);
	data->raw_sms->number = (int) message[2];
	return GN_ERR_NONE;
}


/* 0x2b messages are sent by the phone to indicate an error in
   saving an SMS message. */

static gn_error P3110_IncomingSMSSaveError(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("SMS send failed (0x%02hhx)\n", message[2]);
	return GN_ERR_FAILED;
}


/* 0x2c messages are generated by the phone when we request an SMS
   message with an 0x25 message.  Appears to have the same fields
   as the 0x30 notification but with one extra.  Immediately after
   the 0x2c nessage, the phone sends 0x27 message(s) */

static gn_error P3110_IncomingSMSHeader(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int smsc_length, remote_length, l;
	gn_gsm_number_type smsc_number_type, remote_number_type;
	unsigned char smsc[256], remote[256];	/* should be enough for anyone */

	data->raw_sms->status = message[4];

	/* Check UDHI */
	if (message[5] & 0x40)	/* FO_UDHI */
		data->raw_sms->udh_indicator = 1;
	else
		data->raw_sms->udh_indicator = 0;


	data->raw_sms->dcs = message[7];
	/* FIXME the DCS is set to indicate an 8-bit message in order
	 * to avoid the conversion from 7bit in gsm-sms.c
	 * This really should be done some other way... */
	data->raw_sms->dcs = 0xf4;

	/* Set message length */
	data->raw_sms->length = message[15];

	/* Only received messages have meaningful timestamps and numbers,
	 * saved messages only seem to have garbage in these fields.  So
	 * they are not read if the message is a saved one.  It seems that
	 * 0x01 indicates a saved message, however both 0x04 and 0x24 have
	 * been seen in received messages.  What their difference is is
	 * unknown (at least it's not just read/unread status - the value
	 * seems not to change for a given message). */

	if (message[5] != 0x01) {
		memcpy(data->raw_sms->smsc_time, message + 8, 7);

		/* Now get remote and message center length & type */
		smsc_length = message[16];
		remote_length = message[17 + smsc_length];
		remote_number_type = message[17 + smsc_length + 1 + remote_length];

		/* Need to do some stupid tricks with SMSC number and remote
		 * number, because the gn_sms_raw data structure stores them
		 * in BCD format whereas the phone uses non-zero-terminated
		 * strings with a length byte. So first the data is copied to
		 * a temp string, zero termination is added and the result is
		 * fed to a BCD conversion function. */
	
		strncpy(smsc, message + 17, smsc_length);
		smsc[smsc_length] = '\0';
		smsc_number_type = (smsc[0] == '+') ? GN_GSM_NUMBER_International : GN_GSM_NUMBER_Unknown;
		l = char_semi_octet_pack(smsc, data->raw_sms->message_center + 1, smsc_number_type);

		/* and here's the magic for SMSC number length... */
		data->raw_sms->message_center[0] = (l + 1) / 2 + 1;

		strncpy(remote, message + 17 + smsc_length + 1, remote_length);
		remote[remote_length] = '\0';
		l = char_semi_octet_pack(remote, data->raw_sms->remote_number + 1, remote_number_type);
		data->raw_sms->remote_number[0] = l;
	}

	dprintf("PID:%02x DCS:%02x Timezone:%02x Stat1:%02x Stat2:%02x\n",
		message[6], message[7], message[14], message[4], message[5]);

	dprintf("Message Read:\n");
	dprintf("  Location: %d. Type: %d Status: %d\n", data->raw_sms->number, data->raw_sms->type, data->raw_sms->status);
	dprintf("  Sender: %s\n", remote);
	dprintf("  Message Center: %s\n", smsc);

	return GN_ERR_NONE;
}


/* 0x2d messages are generated when an SMS message is requested
   that does not exist or is empty. */

static gn_error P3110_IncomingSMSError(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	if (message[2] == 0x74)
		return GN_ERR_INVALIDLOCATION;
	else
		return GN_ERR_EMPTYLOCATION;
}


/* 0x2e messages are generated when an SMS message is deleted
   successfully. */

static gn_error P3110_IncomingSMSDelete(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	return GN_ERR_NONE;
}


/* 0x2f messages are generated when an SMS message is deleted
   that does not exist.  Unlike responses to a getsms message
   no error is returned when the entry is already empty */

static gn_error P3110_IncomingSMSDeleteError(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	/* Note 0x74 is the only value that has been seen! */
	if (message[2] == 0x74)
		return GN_ERR_INVALIDLOCATION;
	else
		return GN_ERR_EMPTYLOCATION;
}


/* 0x32 messages are generated when delivery notification arrives
   to phone from network */

static gn_error P3110_IncomingSMSDelivered(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	static gn_sms_raw sms;
/*	u8 dest_length, smsc_length, l;*/
	u8 U0, U1, U2;

	data->raw_sms = &sms;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	U0 = message[1];
	U1 = message[2];
	U2 = message[17];

	/* DecodePDUSMS(message, data->raw_sms, length); */

	/* All these are moved fo gsm-sms.c
	P3110_DecodeTime(message+3, &(data->raw_sms->Time));
	P3110_DecodeTime(message+10, &(data->raw_sms->SMSCTime));


	Get message id
	data->raw_sms->MessageNumber = (int) message[18];

	Get sender and message center length
	dest_length = message[19];
	smsc_length = message[20 + dest_length];

	Copy destination number
	l = dest_length < GSM_MAX_DESTINATION_LENGTH ? dest_length : GSM_MAX_DESTINATION_LENGTH;
	strncpy(data->raw_sms->Destination, message + 20, l);
	data->raw_sms->Destination[l] = 0;

	Copy SMSC number
	l = smsc_length < GSM_MAX_SMS_CENTER_LENGTH ? smsc_length : GSM_MAX_SMS_CENTER_LENGTH;
	strncpy(data->raw_sms->MessageCenter.Number, message + 21 + dest_length, l);
	data->raw_sms->MessageCenter.Number[l] = 0;

	dprintf("Message [0x%02x] Delivered!\n", data->raw_sms->Number);
	dprintf("   Destination: %s\n", data->raw_sms->RemoteNumber.number);
	dprintf("   Message Center: %s\n", data->raw_sms->MessageCenter.Number);
	dprintf("   Unknowns: 0x%02x 0x%02x 0x%02x\n", U0, U1, U2);
	dprintf("   Discharge Time: %02d.%02d.%02d %02d:%02d:%02d\n",
		data->raw_sms->Time.Day, data->raw_sms->Time.Month, data->raw_sms->Time.Year, data->raw_sms->Time.Hour, data->raw_sms->Time.Minute, data->raw_sms->Time.Second);
	dprintf("   SMSC Time Stamp:  %02d.%02d.%02d %02d:%02d:%02d\n",
		data->raw_sms->SMSCTime.Day, data->raw_sms->SMSCTime.Month, data->raw_sms->SMSCTime.Year, data->raw_sms->SMSCTime.Hour, data->raw_sms->SMSCTime.Minute, data->raw_sms->SMSCTime.Second);
	*/
	return GN_ERR_NONE;
}


/* 0x40 Messages are sent to response to an 0x3f request.
   e.g. when phone is waiting for PIN */

static gn_error P3110_IncomingNoSMSInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	dprintf("SMS Message Center Data not reachable.\n");
	return GN_ERR_NOTREADY;
}


/* Handle 0x41 message which is sent by phone in response to an
   0x3f request.  Contains data about the Message Center in use */

static gn_error P3110_IncomingSMSInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	u8 center_number_length;
	u8 option_number_length;
	int count;

	if (!data) return GN_ERR_INTERNALERROR;

	/* We don't know what this option number is, just handle it */
	option_number_length = message[12];

	/* Get message center length */
	center_number_length = message[13 + option_number_length];

	dprintf("SMS Message Center Data:\n");
	dprintf("   Selected memory: 0x%02x\n", message[2]);
	dprintf("   Messages in Phone: 0x%02x Unread: 0x%02x\n", message[3], message[4]);
	dprintf("   Messages in SIM: 0x%02x Unread: 0x%02x\n", message[5], message[6]);
	dprintf("   Reply via own centre: 0x%02x (%s)\n", message[10], (message[10] == 0x02 ? "Yes" : "No"));
	dprintf("   Delivery reports: 0x%02x (%s)\n", message[11], (message[11] == 0x02 ? "Yes" : "No"));
	dprintf("   Messages sent as: 0x%02x\n", message[7]);
	dprintf("   Message validity: 0x%02x\n", message[9]);
	dprintf("   Unknown: 0x%02x\n", message[8]);

	dprintf("   UnknownNumber: ");
	for (count = 0; count < option_number_length; count ++)
		dprintf("%c", message[13 + count]);
	dprintf("\n");

	dprintf("   Message center number: ");
	for (count = 0; count < center_number_length; count ++) {
		dprintf("%c", message[14 + option_number_length + count]);
	}
	dprintf("\n");

	/* Get message center related info if upper layer wants to know */
	if (data->message_center) {
		data->message_center->format = message[7];
		data->message_center->validity = message[9];

		if (center_number_length == 0) {
			data->message_center->smsc.number[0] = 0x00; /* Null terminate */
		} else {
			memcpy(data->message_center->smsc.number,
			       message + 14 + option_number_length,
			       center_number_length);
			data->message_center->smsc.number[center_number_length] = '\0';
			data->message_center->smsc.type = 
				(data->message_center->smsc.number[0] == '+') ? 
					GN_GSM_NUMBER_International :
					GN_GSM_NUMBER_Unknown;
		}

		/* 3810 series doesn't support Name or multiple center numbers
		   so put in null data for them . */
		data->message_center->name[0] = 0x00;
		data->message_center->id = 0;
	}

	/* Get SMS related info if upper layer wants to know */
	if (data->sms_status) {
		data->sms_status->unread = message[4] + message[6];
		data->sms_status->number = message[3] + message[5];
	}

	/* Get memory info if upper layer wants to know */
	if (data->memory_status) {
		switch (data->memory_status->memory_type) {
		case GN_MT_SM:
			data->memory_status->used = message[5];
			data->memory_status->free = P3110_MEMORY_SIZE_SM - message[5];
			break;
		case GN_MT_ME:
			data->memory_status->used = message[3];
			data->memory_status->free = P3110_MEMORY_SIZE_ME - message[3];
			break;
		default:
			break;
		}
	}

	return GN_ERR_NONE;
}


/* 0x48 is sent during power-on of the phone, after the 0x13
   message is received and the PIN (if any) has been entered
   correctly. */

static gn_error P3110_IncomingPINEntered(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	SimAvailable = true;
	dprintf("PIN [possibly] entered.\n");
	return GN_ERR_NONE;
}


/* 0x4b messages are sent by phone in response (it seems) to the keep
   alive packet.  We must acknowledge these it seems by sending a
   response with the "sequence number" byte loaded appropriately. */

static gn_error P3110_IncomingStatusInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	/* Strings for the status byte received from phone. */
	char *StatusStr[] = {
		"Unknown",
		"Ready",
		"Interworking",
		"Call in progress",
		"No network access"
	};

	/* There are three data bytes in the status message, two have been
	   attributed to signal level, the third is presently unknown.
	   Unknown byte has been observed to be 0x01 when connected to normal
	   network, 0x04 when no network available.   Steps through 0x02, 0x03
	   when incoming or outgoing calls occur...*/

	/* Note: GetRFLevel function in fbus-3810.c does conversion
	   into required units. */
	if (data->rf_level) {
		*(data->rf_unit) = GN_RF_Arbitrary;
		*(data->rf_level) = message[3];
	}

	/* Note: GetBatteryLevel function in fbus-3810.c does conversion
	   into required units. */
	if (data->battery_level) {
		*(data->battery_unit) = GN_BU_Arbitrary;
		*(data->battery_level) = message[4];
	}

	/* Only output connection status byte now as the RF and Battery
 	   levels are displayed by the main gnokii code. */
	dprintf("Status: %s, Battery level: %d, RF level: %d.\n",
		StatusStr[message[2]], message[4], message[3]);
	return GN_ERR_NONE;
}

/* 0x4d Message provides IMEI, Revision and Model information. */

static gn_error P3110_IncomingPhoneInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	size_t imei_length, rev_length, model_length;

	imei_length = strlen(message + 2);
	rev_length = strlen(message + 3 + imei_length);
	model_length = strlen(message + 4 + imei_length + rev_length);

	if (data->imei)
		strcpy(data->imei, message + 2);

	if (data->revision)
		strcpy(data->revision, message + 3 + imei_length);

	if (data->model)
		strcpy(data->model, message + 4 + imei_length + rev_length);

	dprintf("Mobile Phone Identification:\n");
	dprintf("   IMEI: %s\n", message + 2);
	dprintf("   Model: %s\n", message + 4 + imei_length + rev_length);
	dprintf("   Revision: %s\n", message + 3 + imei_length);

	return GN_ERR_NONE;
}

/* This function creates a header for sending or saving a SMS message.
 * If save_sms is true, a header for saving is created; if false, a
 * header for sending will be created instead. */

static int sms_header_encode(gn_data *data, struct gn_statemachine *state, unsigned char *req, int ulength, bool save_sms)
{
	int pos = 0;
	unsigned char fo;	/* some magic flags */
	unsigned char smsc[256], remote[256];

	/* why the heck is this necessary? gsm-sms.c does this too  -Osma Suominen */
	data->raw_sms->remote_number[0] = (data->raw_sms->remote_number[0] + 1) / 2 + 1;

	/* SMSC and remote number are in BCD format, but the phone wants them
	 * in plain ASCII, so we have to convert. */
	 
	snprintf(smsc, sizeof(smsc), "%s", char_bcd_number_get(data->raw_sms->message_center));
	snprintf(remote, sizeof(remote), "%s", char_bcd_number_get(data->raw_sms->remote_number));

	dprintf("smsc:'%s' remote:'%s'", smsc, remote);
	
	if (save_sms) { /* make header for saving SMS */
		req[pos++] = get_memory_type(data->raw_sms->memory_type);
		req[pos++] = data->raw_sms->status;
		req[pos++] = 0x01;	/* status byte for "saved SMS" */
	} else { /* make header for sending SMS */
		/* the magic "first octet" or FO */
		fo = 0x31;	/* FO_SUBMIT | FO_VPF_REL | FO_SRR */
		if (data->raw_sms->udh_indicator)	fo |= 0x40;
		req[pos++] = fo;
	}

	req[pos++] = data->raw_sms->pid;
 	req[pos++] = data->raw_sms->dcs;

	/* The following 7 bytes define validity for SMSs to be sent.
	 * When saving they are apparently ignored - or somehow incorporated
	 * into the garbage you get in the date bytes when you read a saved
	 * SMS. So we'll just put validity in here now. */ 

	req[pos++] = 0xff;	/* FIXME Max validity - hardcoded! */
	req[pos++] = 0x00;
	req[pos++] = 0x00;
	req[pos++] = 0x00;
	req[pos++] = 0x00;
	req[pos++] = 0x00;
	req[pos++] = 0x00;

	req[pos++] = ulength;	/* user data length */

	/* The SMSC and remote number are not particularly relevant for
	 * saving SMSs, but there's a place for them in the request, so
	 * we'll put them there. The phone ignores them or at least doesn't
	 * return them when reading the message. */

	req[pos++] = strlen(smsc);
	memcpy(req + pos, smsc, strlen(smsc));
	pos += strlen(smsc);	
	req[pos++] = strlen(remote);
	memcpy(req + pos, remote, strlen(remote));
	pos += strlen(remote);

	/* For some reason, the header for saving SMSs has a byte for the
	 * remote number type, according to Docs/protocol/nk3110.txt
	 * Sent SMS messages don't. */

	if (save_sms)
		req[pos++] = data->raw_sms->remote_number[1]; /* number type */

	return pos;	/* length of encoded header is returned */
}

static int get_memory_type(gn_memory_type memory_type)
{
	int result;

	switch (memory_type) {
	case GN_MT_CB:	result = 0x01; break;
	case GN_MT_SM:	result = 0x02; break;
	case GN_MT_ME:	result = 0x03; break;
	case GN_MT_ON:	result = 0x04; break;
	default:	result = 0; break;	/* error code */
	}
	return result;
}
