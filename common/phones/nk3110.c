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

/* Prototypes */
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_Initialise(GSM_Statemachine *state);
static GSM_Error P3110_GetSMSInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetPhoneInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetStatusInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_GetSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_DeleteSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_SendSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P3110_IncomingNothing(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error P3110_IncomingCall(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingCallAnswered(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingCallEstablished(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingEndOfOutgoingCall(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingEndOfIncomingCall(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingEndOfOutgoingCall2(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingRestart(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingInitFrame_0x15(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingInitFrame_0x16(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingInitFrame_0x17(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSUserData(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSSend(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSSendError(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSHeader(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSError(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSDelete(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSDeleteError(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSDelivered(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingNoSMSInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingSMSInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingPINEntered(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingStatusInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P3110_IncomingPhoneInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
void P3110_KeepAliveLoop(GSM_Statemachine *state);
void P3110_DecodeTime(unsigned char *b, GSM_DateTime *dt);
int P3110_bcd2int(u8 x);

/* Some globals */

static SMSMessage_Layout nk3110_deliver = {
	true, /* IsSupported */
	16, false, false, /* MessageCenter */
	-1, -1, -1, -1,  3, -1, -1, -1, 15,  7,  5,
	-1, -1, -1, /* Validity */
	17, false, false, /* RemoteNumber */
	 8, /* SMSC Time */
	-1, /* Time */
	 2, /* Memory Type */
	 4, /* Status */
	18, false /* User Data */
};

static SMSMessage_PhoneLayout nk3110_layout;


static GSM_IncomingFunctionType IncomingFunctions[] = {
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

GSM_Phone phone_nokia_3110 = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"3110|3810|8110|8110i",	/* Models */
		4,			/* Max RF Level */
		0,			/* Min RF Level */
		GRF_Arbitrary,		/* RF level units */
		4,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GBU_Arbitrary,		/* Battery level units */
		GDT_None,		/* No date/time support */
		GDT_None,		/* No alarm support */
		0,			/* Max alarms = 0 */
		0, 0,                   /* Startup logo size */
		0, 0,                   /* Op logo size */
		0, 0                    /* Caller logo size */
	},
	Functions
};

static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return P3110_Initialise(state);
	case GOP_Terminate:
		return PGEN_Terminate(data, state);
	case GOP_GetModel:
	case GOP_GetRevision:
	case GOP_GetImei:
		return P3110_GetPhoneInfo(data, state);
	case GOP_Identify:
		return P3110_Identify(data, state);
	case GOP_GetBatteryLevel:
	case GOP_GetRFLevel:
		return P3110_GetStatusInfo(data, state);
	case GOP_GetMemoryStatus:
		return P3110_GetMemoryStatus(data, state);
	case GOP_ReadPhonebook:
	case GOP_WritePhonebook:
	case GOP_GetPowersource:
	case GOP_GetAlarm:
	case GOP_GetSMSStatus:
	case GOP_GetIncomingCallNr:
	case GOP_GetNetworkInfo:
		return GE_NOTIMPLEMENTED;
	case GOP_GetSMS:
		return P3110_GetSMSMessage(data, state);
	case GOP_DeleteSMS:
		return P3110_DeleteSMSMessage(data, state);
	case GOP_SendSMS:
		return P3110_SendSMSMessage(data, state);
	case GOP_GetSMSCenter:
		return P3110_GetSMSInfo(data, state);
	case GOP_GetSpeedDial:
	case GOP_GetDateTime:
	default:
		return GE_NOTIMPLEMENTED;
	}
}

static bool SimAvailable = false;
static int user_data_count = 0;

/* These are related to keepalive functionality */
static bool RequestTerminate;
static bool DisableKeepAlive;
static int KeepAliveTimer;

/* Initialise is the only function allowed to 'use' state */
static GSM_Error P3110_Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	u8 init_sequence[20] = {0x02, 0x01, 0x07, 0xa2, 0x88, 0x81, 0x21, 0x55, 0x63, 0xa8, 0x00, 0x00, 0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80};

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_3110, sizeof(GSM_Phone));

	/* SMS Layout */
	nk3110_layout.Type = 0;
	nk3110_layout.SendHeader = 0;
	nk3110_layout.ReadHeader = 0;
	nk3110_layout.Deliver = nk3110_deliver;
	nk3110_layout.Submit = nk3110_deliver;
	nk3110_layout.DeliveryReport = nk3110_deliver;
	nk3110_layout.Picture = nk3110_deliver;
	layout = nk3110_layout;

	/* Only serial connection is supported */
	if (state->Link.ConnectionType != GCT_Serial) return GE_NOTSUPPORTED;

	/* Initialise FBUS link */
	if (FB3110_Initialise(&(state->Link), state) != GE_NONE) {
		dprintf("Error in link initialisation\n");
		return GE_NOTREADY;
	}

	/* Initialise state machine */
	SM_Initialise(state);

	/* 0x15 messages are sent by the PC during the initialisation phase.
	   Anyway, the contents of the message are not understood so we
	   simply send the same sequence observed between the W95 PC and
	   the phone.  The init sequence may still be a bit flaky and is not
	   fully understood. */
	if (SM_SendMessage(state, 20, 0x15, init_sequence) != GE_NONE) return GE_NOTREADY;

	/* Wait for response to 0x15 sequence */
	GSM_DataClear(&data);
	if (SM_Block(state, &data, 0x16) != GE_NONE) return GE_NOTREADY;

	/* Start sending keepalive messages in separate thread */
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	RequestTerminate = false;
	DisableKeepAlive = false;
	return GE_NONE;
}


static GSM_Error P3110_GetSMSInfo(GSM_Data *data, GSM_Statemachine *state)
{
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (SM_SendMessage(state, 0, 0x3f, NULL) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x41);
}

static GSM_Error P3110_GetPhoneInfo(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting phone info...\n");
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (SM_SendMessage(state, 0, 0x4c, NULL) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x4d);
}

static GSM_Error P3110_GetStatusInfo(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting phone status...\n");
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (SM_SendMessage(state, 0, 0x4a, NULL) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x4b);
}

static GSM_Error P3110_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting memory status...\n");

	/* Check if this type of memory is available */
	switch (data->MemoryStatus->MemoryType) {
	case GMT_SM:
		if (!SimAvailable) return GE_NOTREADY;
		return P3110_GetSMSInfo(data, state);
	case GMT_ME:
		if (P3110_MEMORY_SIZE_ME == 0) return GE_NOTREADY;
		return P3110_GetSMSInfo(data, state);
	default:
		break;
	}
	return GE_NOTREADY;
}


static GSM_Error P3110_Identify(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Identifying...\n");
	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
	if (SM_SendMessage(state, 0, 0x4c, NULL) != GE_NONE) return GE_NOTREADY;
	SM_Block(state, data, 0x4d);

	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GE_UNKNOWN;
	return GE_NONE;
}


static GSM_Error P3110_GetSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	int timeout, c;
	u8 response = 0, request[2];
	GSM_Error error = GE_INTERNALERROR;

	dprintf("Getting SMS message...\n");

	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;

	if (!data->SMSMessage) return GE_INTERNALERROR;

	switch(data->SMSMessage->MemoryType) {
	case GMT_ME:
		data->SMSMessage->MemoryType = 1; /* 3 in 8110, 1 is GMT_CB */
		break;
	case GMT_SM:
		data->SMSMessage->MemoryType = 2;
		break;
	default:
		return (GE_INVALIDMEMORYTYPE);
	}

	/* Set memory type and location in the request */
	request[0] = data->SMSMessage->MemoryType;
	request[1] = data->SMSMessage->Number;

	/* 0x25 messages requests the contents of an SMS message
	   from the phone.  The first byte has only ever been
	   observed to be 0x02 - could be selecting internal versus
	   external memory.  Specifying memory 0x00 may request the
	   first location?  Phone replies with 0x2c and 0x27 messages
	   for valid locations, 0x2d for empty ones. */
	if (SM_SendMessage(state, 2, 0x25, request) != GE_NONE) return GE_NOTREADY;

	SM_WaitFor(state, data, 0x2d);
	SM_WaitFor(state, data, 0x2c);

	timeout = 30; /* ~3secs timeout */

	do {
		SM_Loop(state, 1);
		timeout--;
	} while ((timeout > 0) && state->NumReceived == 0);

	/* timeout */
	if (state->NumReceived == 0) return GE_TIMEOUT;

	/* find response in state machine */
	for (c = 0; c < state->NumWaitingFor; c++) {
		if (state->ResponseError[c] != GE_BUSY) {
			response = state->WaitingFor[c];
			error = state->ResponseError[c];
		}
	}

	if (!data->RawData) return GE_INTERNALERROR;

	/* reset state machine */
	SM_Reset(state);

	/* process response */
	switch (response) {
	case 0x2c:
		if (error != GE_NONE) return error;

		/* Block for subsequent content frames... */
		do {
			SM_Block(state, data, 0x27);
		} while (!data->RawData->Full);

		return GE_NONE;
	case 0x2d:
		return error;
	default:
		return GE_INTERNALERROR;
	}
}


static GSM_Error P3110_DeleteSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	int timeout, c;
	u8 response = 0, request[2];
	GSM_Error error = GE_INTERNALERROR;

	dprintf("Deleting SMS message...\n");

	KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;

	switch(data->SMSMessage->MemoryType) {
	case GMT_ME:
		data->SMSMessage->MemoryType = 1; /* 3 in 8110, 1 is GMT_CB */
		break;
	case GMT_SM:
		data->SMSMessage->MemoryType = 2;
		break;
	default:
		return (GE_INVALIDMEMORYTYPE);
	}

	/* Set memory type and location in the request */
	request[0] = data->SMSMessage->MemoryType;
	request[1] = data->SMSMessage->Number;

	/* 0x26 message deletes an SMS message from the phone.
	   The first byte has only ever been observed to be 0x02
	   but is assumed to be selecting internal versus
	   external memory.  Phone replies with 0x2e for valid locations,
	   0x2f for invalid ones.  If a location is empty but otherwise
	   valid 0x2e is still returned. */
	if (SM_SendMessage(state, 2, 0x26, request) != GE_NONE) return GE_NOTREADY;

	SM_WaitFor(state, data, 0x2e);
	SM_WaitFor(state, data, 0x2f);

	timeout = 30; /* ~3secs timeout */

	do {
		SM_Loop(state, 1);
		timeout--;
	} while ((timeout > 0) && state->NumReceived == 0);

	/* timeout */
	if (state->NumReceived == 0) return GE_TIMEOUT;

	/* find response in state machine */
	for (c = 0; c < state->NumWaitingFor; c++) {
		if (state->ResponseError[c] != GE_BUSY) {
			response = state->WaitingFor[c];
			error = state->ResponseError[c];
		}
	}

	/* reset state machine */
	SM_Reset(state);

	/* process response */
	switch (response) {
	case 0x2e:
	case 0x2f:
		return error;
	default:
		return GE_INTERNALERROR;
	}
}


static GSM_Error P3110_SendSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	int timeout, c, retry_count, block_count, block_length;
	u8 userdata[GSM_MAX_SMS_LENGTH];
	int userdata_length = 0, userdata_offset, userdata_remaining;
	u8 response = 0, request[GSM_MAX_SMS_LENGTH];
	GSM_Error error = GE_NONE;
	SMS_MessageCenter smsc;
	int i;

	/* Get default SMSC from phone if not set in SMS */
	if (!data->SMSMessage->MessageCenter.Number[0]) {
		data->MessageCenter = &smsc;
		error = P3110_GetSMSInfo(data, state);
		if (error != GE_NONE) return error;
		data->SMSMessage->MessageCenter = smsc;
	}

	dprintf("Sending SMS to %s via message center %s\n", data->SMSMessage->RemoteNumber.number, data->SMSMessage->MessageCenter.Number);

/*
	Moved to gsm-sms.c
	if (data->SMSMessage->UDH[0].Type) {
		userdata_offset = 1 + data->SMSMessage->udhlen;
		memcpy(userdata, data->SMSMessage->UserData[0].u.Text, userdata_offset);
		fo |= FO_UDHI;
	} else {
		userdata_offset = 0;
	}
*/

/*
	Moved to gsm-sms.c
	if (data->SMSMessage->EightBit) {
		memcpy(userdata + userdata_offset, data->SMSMessage->UserData[0].u.Text, data->SMSMessage->Length);
		userdata_length = data->SMSMessage->Length + userdata_offset;
		max_userdata_length = GSM_MAX_SMS_8_BIT_LENGTH;
		dcs = DCS_DATA | DCS_CLASS1;
	} else {
		userdata_length = strlen(data->SMSMessage->UserData[0].u.Text);
		memcpy(userdata + userdata_offset, data->SMSMessage->UserData[0].u.Text, userdata_length);
		userdata_length += userdata_offset;
		max_userdata_length = GSM_MAX_SMS_LENGTH;
	}
*/

/*
	Moved to gsm-sms.c
	request[0] = fo;
	request[1] = PID_DEFAULT;
	request[2] = dcs;
	request[3] = GSMV_Max_Time;
	request[4] = 0x00;
	request[5] = 0x00;
	request[6] = 0x00;
	request[7] = 0x00;
	request[8] = 0x00;
	request[9] = 0x00;
	request[10] = userdata_length;
	request[11] = smsc_length;
	memcpy(request+12, data->SMSMessage->MessageCenter.Number, smsc_length);
	request[12+smsc_length] = dest_length;
	memcpy(request+13+smsc_length, data->SMSMessage->Destination, dest_length);
*/

	/* error = EncodePDUSMS(data->SMSMessage, request); */
	if (error) return error;

	/* We have a loop here as if the response from the phone is
	   0x65 0x26 the rule appears to be just to try sending the
	   message again.  We do this a maximum of FB38_SMS_SEND_RETRY_COUNT
	   times before giving up.  This value is empirical only! */
	retry_count = P3110_SMS_SEND_RETRY_COUNT;

	while (retry_count > 0) {

		KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;

		dprintf("Transferring FBUS SMS header [");
		for (i = 0; i < data->SMSMessage->Length; i++) dprintf(" %02hhX", request[i]);
		dprintf(" ]\n");

		if (SM_SendMessage(state, data->SMSMessage->Length, 0x23, request) != GE_NONE) return GE_NOTREADY;

		error = SM_Block(state, data, 0x23);
		if (error != GE_NONE) return error;

		/* Now send as many blocks of maximum 55 characters as required
		   to send complete message. */
		block_count = 1;
		userdata_offset = 0;
		userdata_remaining = userdata_length;

		while (userdata_remaining > 0) {
			block_length = userdata_remaining;

			/* Limit block length */
			if (block_length > 55) block_length = 55;

			/* Create block */
			request[0] = block_count;
			memcpy(request+1, userdata + userdata_offset, block_length);

			/* Send block */
			if (SM_SendMessage(state, block_length+1, 0x27, request) != GE_NONE) return GE_NOTREADY;
			error = SM_Block(state, data, 0x27);
			if (error != GE_NONE) return error;

			/* update remaining and offset values for next block */
			userdata_remaining -= block_length;
			userdata_offset += block_length;
			block_count ++;
		}

		/* Now wait for response from network which will see
		   CurrentSMSMessageError change from busy. */
		SM_WaitFor(state, data, 0x28);
		SM_WaitFor(state, data, 0x29);

		timeout = 1200; /* 120secs timeout */

		do {
			SM_Loop(state, 1);
			timeout--;
		} while ((timeout > 0) && state->NumReceived == 0);

		/* timeout */
		if (state->NumReceived == 0) return GE_TIMEOUT;

		/* find response in state machine */
		for (c = 0; c < state->NumWaitingFor; c++) {
			if (state->ResponseError[c] != GE_BUSY) {
				response = state->WaitingFor[c];
				error = state->ResponseError[c];
			}
		}

		/* reset state machine */
		SM_Reset(state);

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
		default:
			return GE_INTERNALERROR;
		}
	}
	/* Retries must have failed. */
	return GE_SMSSENDFAILED;
}


static GSM_Error P3110_IncomingNothing(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	return GE_NONE;
}

/* 0x0b messages are sent by phone when an incoming call occurs,
   this message must be acknowledged. */
static GSM_Error P3110_IncomingCall(int messagetype, unsigned char *message, int length, GSM_Data *data)
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

	return GE_NONE;
}

static GSM_Error P3110_IncomingCallAnswered(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("Incoming call answered from phone.\n");

	return GE_NONE;
}

/* Fairly self explanatory these two, though the outgoing
   call message has three (unexplained) data bytes. */
static GSM_Error P3110_IncomingCallEstablished(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("%s call established - status bytes %02x %02x.\n",
		(message[2] == 0x05 ? "voice":"data(?)"), message[3], message[4]);

	return GE_NONE;
}

/* 0x10 messages are sent by the phone when an outgoing
   call terminates. */
static GSM_Error P3110_IncomingEndOfOutgoingCall(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("Call terminated from phone (0x10 message).\n");
	/* FIXME: Tell datapump code that the call has terminated. */
	/*if (CallPassup) {
		CallPassup(' ');
		}*/

	return GE_NONE;
}

/* 0x11 messages are sent by the phone when an incoming call
   terminates.  There is some other data in the message,
   purpose as yet undertermined. */
static GSM_Error P3110_IncomingEndOfIncomingCall(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("Call terminated from opposite end of line (or from network).\n");

	/* FIXME: Tell datapump code that the call has terminated. */
	/*if (CallPassup) {
		CallPassup(' ');
		}*/

	return GE_NONE;
}

/* 0x12 messages are sent after the 0x10 message at the
   end of an outgoing call.  Significance of two messages
   versus the one at the end of an incoming call  is as
   yet undertermined. */
static GSM_Error P3110_IncomingEndOfOutgoingCall2(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("Call terminated from phone (0x12 message).\n");

	/* FIXME: Tell datapump code that the call has terminated. */
	/*if (CallPassup) {
		CallPassup(' ');
		}*/

	return GE_NONE;
}


/* 0x13 messages are sent after the phone restarts.
   Re-initialise */

static GSM_Error P3110_IncomingRestart(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	/* FIXME: send 0x15 message somehow */
	return GE_NONE;
}


/* 0x15 messages are sent by the phone in response to the
   init sequence sent so we don't acknowledge them! */

static GSM_Error P3110_IncomingInitFrame_0x15(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	return GE_NONE;
}


/* 0x16 messages are sent by the phone during initialisation, to response
   to the 0x15 message.
   Sequence bytes have been observed to change with differing software
   versions: V06.61 (19/08/97) sends 0x10 0x02, V07.02 (17/03/98) sends
   0x30 0x02. The actual data byte is 0x02 when SIM memory is available,
   and 0x01 when not (e.g. when SIM card isn't inserted to phone or when
   it is waiting for PIN) */

static GSM_Error P3110_IncomingInitFrame_0x16(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	SimAvailable = (message[2] == 0x02);
	dprintf("SIM available: %s.\n", (SimAvailable ? "Yes" : "No"));
	return GE_NONE;
}


static GSM_Error P3110_IncomingInitFrame_0x17(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("0x17 Registration Response: Failure!\n");
	return GE_NONE;
}


/* 0x27 messages are a little unusual when sent by the phone in that
   they can either be an acknowledgement of an 0x27 message we sent
   to the phone with message text in it or they could
   contain message text for a message we requested. */

static GSM_Error P3110_IncomingSMSUserData(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int count;

	/* First see if it was an acknowledgement to one of our messages,
	   if so then nothing to do */
	if (length == 0x02) return GE_NONE;

	/* Copy into current SMS message as long as it's non-NULL */
	if (!data->RawData) return GE_INTERNALERROR;

	/* If this is the first block, reset accumulated message length. */
	if (message[2] == 1) user_data_count = 0;

	count = data->RawData->Length + length - 3;

	/* Copy message text after the stored header */
	data->RawData->Data = realloc(data->RawData->Data, count);
	memcpy(data->RawData->Data + data->RawData->Length, message + 3, length - 3);
	data->RawData->Length = count;

	/* Check whether this is the last user data frame */
	user_data_count += length - 3;
	if (user_data_count >= data->RawData->Data[15])
		data->RawData->Full = 1;

	return GE_NONE;
}


/* 0x28 messages are sent by the phone to acknowledge succesfull
   sending of an SMS message.  The byte returned is a receipt
   number of some form, not sure if it's from the network, sending
   sending of an SMS message.  The byte returned is the TP-MR
   (TP-Message-Reference) from sending phone (Also sent to network).
   TP-MR is send from phone within 0x32 message. TP-MR is increased
   by phone after each sent SMS */

static GSM_Error P3110_IncomingSMSSend(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("SMS send OK (0x%02hhx)\n", message[2]);
	data->SMSMessage->Number = (int) message[2];
	return GE_NONE;
}


/* 0x29 messages are sent by the phone to indicate an error in
   sending an SMS message.  Observed values are 0x65 0x15 when
   the phone originated SMS was disabled by the network for
   the particular phone.  0x65 0x26 was observed too, whereupon
   the message was retried. */

static GSM_Error P3110_IncomingSMSSendError(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("SMS send failed (0x%02hhx 0x%02hhx)\n", message[2], message[3]);
	return GE_SMSSENDFAILED;
}


/* 0x2c messages are generated by the phone when we request an SMS
   message with an 0x25 message.  Appears to have the same fields
   as the 0x30 notification but with one extra.  Immediately after
   the 0x2c nessage, the phone sends 0x27 message(s) */

static GSM_Error P3110_IncomingSMSHeader(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (!data->SMSMessage) return GE_INTERNALERROR;

	/* Set standard code of the message type */
	switch (message[5]) {
	case 0x24:
		message[0] = SMS_Deliver;
		break;
	case 0x01:
		message[0] = SMS_Submit;
		break;
	default:
		return GE_INTERNALERROR;
	}

	if (!data->RawData) return GE_INTERNALERROR;

	data->RawData->Length = length;
	data->RawData->Data = calloc(length, 1);
	memcpy(data->RawData->Data, message, length);

	/* All these moved to gsm-sms.c
	Set memory type
	switch(message[2]) {
	case 1:
		data->SMSMessage->MemoryType = GMT_ME;
		break;
	case 2:
		data->SMSMessage->MemoryType = GMT_SM;
		break;
	default:
		data->SMSMessage->MemoryType = GMT_XX;
		break;
	}

	Set location in memory
	data->SMSMessage->Location = message[3];
	data->SMSMessage->MessageNumber = message[3];

	  3810 series has limited support for different SMS "mailboxes"
	   to the extent that the only know differentiation is between
	   received messages 0x01, 0x04 and written messages 0x07 0x01.
	   No flag has been found (yet) that indicates whether the
	   message has been sent or not.

	Default to unknown message type
	data->SMSMessage->Type = GST_UN;

	Consider received messages "Inbox" (Mobile Terminated)
	if (message[4] == 0x01 && message[5] == 0x04)
		data->SMSMessage->Type = GST_MT;

	Consider written messages "Outbox" (Mobile Originated)
	if (message[4] == 0x07 && message[5] == 0x01)
		data->SMSMessage->Type = GST_MO;

	We don't know about read/unread or sent/unsent status.
	   so assume has been sent or read
	data->SMSMessage->Status = GSS_SENTREAD;

	Based on experiences with a 3110 it seems, that
	   0x03 means a received unread message,
	   0x07 means a stored unsent message
	if (message[4] == 0x03 || message[4] == 0x07)
		data->SMSMessage->Status = GSS_NOTSENTREAD;
	else
		data->SMSMessage->Status = GSS_SENTREAD;

	Check UDHI
	if (message[5] & FO_UDHI)
		data->SMSMessage->UDHType = GSM_RingtoneUDH; FIXME
	else
		data->SMSMessage->UDHType = GSM_NoUDH;

	Check Data Coding Scheme and set text/binary flag
	if (message[7] == 0xf5)
		data->SMSMessage->EightBit = true;
	else
		data->SMSMessage->EightBit = false;

	Extract date and time information which is packed in to
	   nibbles of each byte in reverse order.  Thus day 28 would be
	   encoded as 0x82
	P3110_DecodeTime(message+8, &(data->SMSMessage->Time));

	Set message length
	data->SMSMessage->Length = message[15];

	Now get sender and message center length
	smsc_length = message[16];
	sender_length = message[17 + smsc_length];

	Copy SMSC number
	l = smsc_length < GSM_MAX_SMS_CENTER_LENGTH ? smsc_length : GSM_MAX_SMS_CENTER_LENGTH;
	strncpy(data->SMSMessage->MessageCenter.Number, message + 17, l);
	data->SMSMessage->MessageCenter.Number[l] = 0;

	Copy sender number
	l = sender_length < GSM_MAX_SENDER_LENGTH ? sender_length : GSM_MAX_SENDER_LENGTH;
	strncpy(data->SMSMessage->Sender, message + 18 + smsc_length, l);
	data->SMSMessage->Sender[l] = 0;

	dprintf("PID:%02x DCS:%02x Timezone:%02x Stat1:%02x Stat2:%02x\n",
		message[6], message[7], message[14], message[4], message[5]);

	dprintf("Message Read:\n");
	dprintf("  Location: %d. Type: %d Status: %d\n", data->SMSMessage->Number, data->SMSMessage->Type, data->SMSMessage->Status);
	dprintf("  Sender: %s\n", data->SMSMessage->RemoteNumber.number);
	dprintf("  Message Center: %s\n", data->SMSMessage->MessageCenter.Number);
	dprintf("  Time: %02d.%02d.%02d %02d:%02d:%02d\n",
		data->SMSMessage->Time.Day, data->SMSMessage->Time.Month, data->SMSMessage->Time.Year, data->SMSMessage->Time.Hour, data->SMSMessage->Time.Minute, data->SMSMessage->Time.Second);
	*/
	return GE_NONE;
}


/* 0x2d messages are generated when an SMS message is requested
   that does not exist or is empty. */

static GSM_Error P3110_IncomingSMSError(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (message[2] == 0x74)
		return GE_INVALIDSMSLOCATION;
	else
		return GE_EMPTYSMSLOCATION;
}


/* 0x2e messages are generated when an SMS message is deleted
   successfully. */

static GSM_Error P3110_IncomingSMSDelete(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	return GE_NONE;
}


/* 0x2f messages are generated when an SMS message is deleted
   that does not exist.  Unlike responses to a getsms message
   no error is returned when the entry is already empty */

static GSM_Error P3110_IncomingSMSDeleteError(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	/* Note 0x74 is the only value that has been seen! */
	if (message[2] == 0x74)
		return GE_INVALIDSMSLOCATION;
	else
		return GE_EMPTYSMSLOCATION;
}


/* 0x32 messages are generated when delivery notification arrives
   to phone from network */

static GSM_Error P3110_IncomingSMSDelivered(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	static GSM_SMSMessage sms;
/*	u8 dest_length, smsc_length, l;*/
	u8 U0, U1, U2;

	data->SMSMessage = &sms;

	if (!data->SMSMessage) return GE_INTERNALERROR;

	U0 = message[1];
	U1 = message[2];
	U2 = message[17];

	/* DecodePDUSMS(message, data->SMSMessage, length); */

	/* All these are moved fo gsm-sms.c
	P3110_DecodeTime(message+3, &(data->SMSMessage->Time));
	P3110_DecodeTime(message+10, &(data->SMSMessage->SMSCTime));


	Get message id
	data->SMSMessage->MessageNumber = (int) message[18];

	Get sender and message center length
	dest_length = message[19];
	smsc_length = message[20 + dest_length];

	Copy destination number
	l = dest_length < GSM_MAX_DESTINATION_LENGTH ? dest_length : GSM_MAX_DESTINATION_LENGTH;
	strncpy(data->SMSMessage->Destination, message + 20, l);
	data->SMSMessage->Destination[l] = 0;

	Copy SMSC number
	l = smsc_length < GSM_MAX_SMS_CENTER_LENGTH ? smsc_length : GSM_MAX_SMS_CENTER_LENGTH;
	strncpy(data->SMSMessage->MessageCenter.Number, message + 21 + dest_length, l);
	data->SMSMessage->MessageCenter.Number[l] = 0;

	dprintf("Message [0x%02x] Delivered!\n", data->SMSMessage->Number);
	dprintf("   Destination: %s\n", data->SMSMessage->RemoteNumber.number);
	dprintf("   Message Center: %s\n", data->SMSMessage->MessageCenter.Number);
	dprintf("   Unknowns: 0x%02x 0x%02x 0x%02x\n", U0, U1, U2);
	dprintf("   Discharge Time: %02d.%02d.%02d %02d:%02d:%02d\n",
		data->SMSMessage->Time.Day, data->SMSMessage->Time.Month, data->SMSMessage->Time.Year, data->SMSMessage->Time.Hour, data->SMSMessage->Time.Minute, data->SMSMessage->Time.Second);
	dprintf("   SMSC Time Stamp:  %02d.%02d.%02d %02d:%02d:%02d\n",
		data->SMSMessage->SMSCTime.Day, data->SMSMessage->SMSCTime.Month, data->SMSMessage->SMSCTime.Year, data->SMSMessage->SMSCTime.Hour, data->SMSMessage->SMSCTime.Minute, data->SMSMessage->SMSCTime.Second);
	*/
	return GE_NONE;
}


/* 0x40 Messages are sent to response to an 0x3f request.
   e.g. when phone is waiting for PIN */

static GSM_Error P3110_IncomingNoSMSInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	dprintf("SMS Message Center Data not reachable.\n");
	return GE_NOTREADY;
}


/* Handle 0x41 message which is sent by phone in response to an
   0x3f request.  Contains data about the Message Center in use */

static GSM_Error P3110_IncomingSMSInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	u8 center_number_length;
	u8 option_number_length;
	int count;

	if (!data) return GE_INTERNALERROR;

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
	if (data->MessageCenter) {
		data->MessageCenter->Format = message[7];
		data->MessageCenter->Validity = message[9];

		if (center_number_length == 0) {
			data->MessageCenter->Number[0] = 0x00; /* Null terminate */
		} else {
			memcpy(data->MessageCenter->Number,
			       message + 14 + option_number_length,
			       center_number_length);
			data->MessageCenter->Number[center_number_length] = '\0';
		}

		/* 3810 series doesn't support Name or multiple center numbers
		   so put in null data for them . */
		data->MessageCenter->Name[0] = 0x00;
		data->MessageCenter->No = 0;
	}

	/* Get SMS related info if upper layer wants to know */
	if (data->SMSStatus) {
		data->SMSStatus->Unread = message[4] + message[6];
		data->SMSStatus->Number = message[3] + message[5];
	}

	/* Get memory info if upper layer wants to know */
	if (data->MemoryStatus) {
		switch (data->MemoryStatus->MemoryType) {
		case GMT_SM:
			data->MemoryStatus->Used = message[5];
			data->MemoryStatus->Free = P3110_MEMORY_SIZE_SM - message[5];
			break;
		case GMT_ME:
			data->MemoryStatus->Used = message[3];
			data->MemoryStatus->Free = P3110_MEMORY_SIZE_ME - message[3];
			break;
		default:
			break;
		}
	}

	return GE_NONE;
}


/* 0x48 is sent during power-on of the phone, after the 0x13
   message is received and the PIN (if any) has been entered
   correctly. */

static GSM_Error P3110_IncomingPINEntered(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	SimAvailable = true;
	dprintf("PIN [possibly] entered.\n");
	return GE_NONE;
}


/* 0x4b messages are sent by phone in response (it seems) to the keep
   alive packet.  We must acknowledge these it seems by sending a
   response with the "sequence number" byte loaded appropriately. */

static GSM_Error P3110_IncomingStatusInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
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
	if (data->RFLevel) {
		*(data->RFUnits) = GRF_Arbitrary;
		*(data->RFLevel) = message[3];
	}

	/* Note: GetBatteryLevel function in fbus-3810.c does conversion
	   into required units. */
	if (data->BatteryLevel) {
		*(data->BatteryUnits) = GBU_Arbitrary;
		*(data->BatteryLevel) = message[4];
	}

	/* Only output connection status byte now as the RF and Battery
 	   levels are displayed by the main gnokii code. */
	dprintf("Status: %s, Battery level: %d, RF level: %d.\n",
		StatusStr[message[2]], message[4], message[3]);
	return GE_NONE;
}

/* 0x4d Message provides IMEI, Revision and Model information. */

static GSM_Error P3110_IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	size_t imei_length, rev_length, model_length;

	imei_length = strlen(message + 2);
	rev_length = strlen(message + 3 + imei_length);
	model_length = strlen(message + 4 + imei_length + rev_length);

	if (data->Imei)
		strcpy(data->Imei, message + 2);

	if (data->Revision)
		strcpy(data->Revision, message + 3 + imei_length);

	if (data->Model)
		strcpy(data->Model, message + 4 + imei_length + rev_length);

	dprintf("Mobile Phone Identification:\n");
	dprintf("   IMEI: %s\n", message + 2);
	dprintf("   Model: %s\n", message + 4 + imei_length + rev_length);
	dprintf("   Revision: %s\n", message + 3 + imei_length);

	return GE_NONE;
}


void P3110_KeepAliveLoop(GSM_Statemachine *state)
{
	GSM_Data data;
	GSM_DataClear(&data);

	while (!RequestTerminate) {

		if (KeepAliveTimer == 0) {
			KeepAliveTimer = P3110_KEEPALIVE_TIMEOUT;
			/* Dont send keepalive packets when statemachine
			   is doing other transactions. */
			if (state->CurrentState == Initialised) {
				dprintf("Sending keepalive message.\n");
				P3110_GetStatusInfo(&data, state);
			}
		} else {
			KeepAliveTimer--;
		}
		usleep(100000); /* Avoid becoming a "busy" loop. */
	}
}

void P3110_DecodeTime(unsigned char *b, GSM_DateTime *dt)
{
	dt->Year = P3110_bcd2int(b[0]);
	dt->Month = P3110_bcd2int(b[1]);
	dt->Day = P3110_bcd2int(b[2]);
	dt->Hour = P3110_bcd2int(b[3]);
	dt->Minute = P3110_bcd2int(b[4]);
	dt->Second = P3110_bcd2int(b[5]);
	dt->Timezone = P3110_bcd2int(b[6]);
	return;
}

int P3110_bcd2int(u8 x)
{
	return (int)(10 * ((x & 0x0f)) + (x >> 4));
}
