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
  Copyright (C) 2001 Manfred Jonsson
  Copyright (C) 2002 BORBELY Zoltan, Pawel Kot

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  The various routines are called PNOK_(whatever).

*/

#include <string.h>

#include "gsm-data.h"
#include "links/fbus.h"
#include "phones/nokia.h"

/* This function provides a way to detect the manufacturer of a phone
 * because it is only used by the fbus/mbus protocol phones and only
 * nokia is using those protocols, the result is clear.
 * the error reporting is also very simple
 * the strncpy and PNOK_MAX_MODEL_LENGTH is only here as a reminder
 */
GSM_Error PNOK_GetManufacturer(char *manufacturer)
{
	strcpy(manufacturer, "Nokia");
	return (GE_NONE);
}


void PNOK_DecodeString(unsigned char *dest, size_t max, const unsigned char *src, size_t len)
{
	int i, n;

	n = (len >= max) ? max-1 : len;

	for (i = 0; i < n; i++)
		dest[i] = src[i];
	dest[i] = 0;
}

size_t PNOK_EncodeString(unsigned char *dest, size_t max, const unsigned char *src)
{
	size_t i;

	for (i = 0; i < max && src[i]; i++)
		dest[i] = src[i];
	return i;
}

/* Call Divert */
GSM_Error PNOK_CallDivert(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned short length = 0x09;
	char req[55] = { FBUS_FRAME_HEADER, 0x01, 0x00, /* operation */
						0x00,
						0x00, /* divert type */
						0x00, /* call type */
						0x00 };
	if (!data->CallDivert) return GE_UNKNOWN;
	switch (data->CallDivert->Operation) {
	case GSM_CDV_Query:
		req[4] = 0x05;
		break;
	case GSM_CDV_Register:
		req[4] = 0x03;
		length = 0x37;
		req[8] = 0x01;
		req[9] = SemiOctetPack(data->CallDivert->Number.Number, req + 10, data->CallDivert->Number.Type);
		req[54] = data->CallDivert->Timeout;
		break;
	case GSM_CDV_Erasure:
		req[4] = 0x04;
		break;
	default:
		return GE_NOTIMPLEMENTED;
	}
	switch (data->CallDivert->CType) {
	case GSM_CDV_AllCalls:
		break;
	case GSM_CDV_VoiceCalls:
		req[7] = 0x0b;
		break;
	case GSM_CDV_FaxCalls:
		req[7] = 0x0d;
		break;
	case GSM_CDV_DataCalls:
		req[7] = 0x19;
		break;
	default:
		return GE_NOTIMPLEMENTED;
	}
	switch (data->CallDivert->DType) {
	case GSM_CDV_AllTypes:
		req[6] = 0x15;
		break;
	case GSM_CDV_Busy:
		req[6] = 0x43;
		break;
	case GSM_CDV_NoAnswer:
		req[6] = 0x3d;
		break;
	case GSM_CDV_OutOfReach:
		req[6] = 0x3e;
		break;
	default:
		return GE_NOTIMPLEMENTED;
	}
	if ((data->CallDivert->DType == GSM_CDV_AllTypes) &&
	    (data->CallDivert->CType == GSM_CDV_AllCalls))
		req[6] = 0x02;

	if (SM_SendMessage(state, length, 0x06, req) != GE_NONE) return GE_NOTREADY;
	return SM_BlockTimeout(state, data, 0x06, 100);
}

GSM_Error PNOK_IncomingCallDivert(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char *pos;
	GSM_CallDivert *cd;

	switch (message[3]) {
	/* Get call diverts ok */
	case 0x02:
		pos = message + 4;
		cd = data->CallDivert;
		if (*pos != 0x05 && *pos != 0x04) return GE_UNHANDLEDFRAME;
		pos++;
		if (*pos++ != 0x00) return GE_UNHANDLEDFRAME;
		switch (*pos++) {
		case 0x02:
		case 0x15: cd->DType = GSM_CDV_AllTypes; break;
		case 0x43: cd->DType = GSM_CDV_Busy; break;
		case 0x3d: cd->DType = GSM_CDV_NoAnswer; break;
		case 0x3e: cd->DType = GSM_CDV_OutOfReach; break;
		default: return GE_UNHANDLEDFRAME;
		}
		if (*pos++ != 0x02) return GE_UNHANDLEDFRAME;
		switch (*pos++) {
		case 0x00: cd->CType = GSM_CDV_AllCalls; break;
		case 0x0b: cd->CType = GSM_CDV_VoiceCalls; break;
		case 0x0d: cd->CType = GSM_CDV_FaxCalls; break;
		case 0x19: cd->CType = GSM_CDV_DataCalls; break;
		default: return GE_UNHANDLEDFRAME;
		}
		if (message[4] == 0x04 && pos[0] == 0x00) {
			return GE_EMPTYLOCATION;
		} else if (message[4] == 0x04 || (pos[0] == 0x01 && pos[1] == 0x00)) {
			cd->Number.Type = SMS_Unknown;
			memset(cd->Number.Number, 0, sizeof(cd->Number.Number));
		} else if (pos[0] == 0x02 && pos[1] == 0x01) {
			pos += 2;
			snprintf(cd->Number.Number, sizeof(cd->Number.Number), "%-*.*s", *pos+1, *pos+1, GetBCDNumber(pos+1));
			pos += 12 + 22;
			cd->Timeout = *pos++;
		}
		break;

	/* FIXME: failed calldivert result code? */
	case 0x03:
		return GE_UNHANDLEDFRAME;
	
	/* FIXME: call divert is active */
	case 0x06:
		return GE_UNSOLICITED;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}

GSM_Error PNOK_FBUS_SendSMS(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x01, 0x02, 0x00};
	GSM_Error error;

	memset(req + 6, 0, 249);
	memcpy(req + 6, data->RawSMS->MessageCenter, 12);
	req[18] = 0x01; /* SMS Submit */
	if (data->RawSMS->ReplyViaSameSMSC)  req[18] |= 0x80;
	if (data->RawSMS->RejectDuplicates)  req[18] |= 0x04;
	if (data->RawSMS->Report)            req[18] |= 0x20;
	if (data->RawSMS->UDHIndicator)      req[18] |= 0x40;
	if (data->RawSMS->ValidityIndicator) req[18] |= 0x10;
	req[19] = data->RawSMS->Reference;
	req[20] = data->RawSMS->PID;
	req[21] = data->RawSMS->DCS;
	req[22] = data->RawSMS->Length;
	memcpy(req + 23, data->RawSMS->RemoteNumber, 12);
	memcpy(req + 35, data->RawSMS->Validity, 7);
	memcpy(req + 42, data->RawSMS->UserData, data->RawSMS->UserDataLength);
	dprintf("Sending SMS...(%d)\n", 42 + data->RawSMS->UserDataLength);
	if (SM_SendMessage(state, 42 + data->RawSMS->UserDataLength, PNOK_MSG_SMS, req) != GE_NONE) return GE_NOTREADY;
	do {
		error = SM_BlockNoRetryTimeout(state, data, PNOK_MSG_SMS, state->Link.SMSTimeout);
	} while (!state->Link.SMSTimeout && error == GE_TIMEOUT);
	return error;
}
