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
gn_error PNOK_GetManufacturer(char *manufacturer)
{
	strcpy(manufacturer, "Nokia");
	return GN_ERR_NONE;
}


static wchar_t PNOK_nokia_to_uni(unsigned char ch)
{
	switch (ch) {
	case 0x82: return 0x00e1; /* LATIN SMALL LETTER A WITH ACUTE */
	case 0x1c: return 0x00c1; /* LATIN CAPITAL LETTER A WITH ACUTE */
	case 0xe9: return 0x00e9; /* LATIN SMALL LETTER E WITH ACUTE (!) */
	case 0xc9: return 0x00c9; /* LATIN CAPITAL LETTER E WITH ACUTE (!) */
	case 0x8a: return 0x00ed; /* LATIN SMALL LETTER I WITH ACUTE */
	case 0x5e: return 0x00cd; /* LATIN CAPITAL LETTER I WITH ACUTE */
	case 0x90: return 0x00f3; /* LATIN SMALL LETTER O WITH ACUTE */
	case 0x7d: return 0x00d3; /* LATIN CAPITAL LETTER O WITH ACUTE */
	case 0xf6: return 0x00f6; /* LATIN SMALL LETTER O WITH DIAERESIS (!) */
	case 0xd6: return 0x00d6; /* LATIN CAPITAL LETTER O WITH DIAERESIS (!) */
	case 0x96: return 0x0151; /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
	case 0x95: return 0x0150; /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
	case 0x97: return 0x00fa; /* LATIN SMALL LETTER U WITH ACUTE */
	case 0x80: return 0x00da; /* LATIN CAPITAL LETTER U WITH ACUTE */
	case 0xfc: return 0x00fc; /* LATIN SMALL LETTER U WITH DIAERESIS (!) */
	case 0xdc: return 0x00dc; /* LATIN CAPITAL LETTER U WITH DIAERESIS (!) */
	case 0xce: return 0x0171; /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
	case 0xcc: return 0x0170; /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
	default: return char_decode_def_alphabet(ch);
	}
}

static unsigned char PNOK_uni_to_nokia(wchar_t wch)
{
	switch (wch) {
	case 0x00e1: return 0x82; /* LATIN SMALL LETTER A WITH ACUTE */
	case 0x00c1: return 0x1c; /* LATIN CAPITAL LETTER A WITH ACUTE */
	case 0x00e9: return 0x05; /* LATIN SMALL LETTER E WITH ACUTE (!) */
	case 0x00c9: return 0x1f; /* LATIN CAPITAL LETTER E WITH ACUTE (!) */
	case 0x00ed: return 0x8a; /* LATIN SMALL LETTER I WITH ACUTE */
	case 0x00cd: return 0x5e; /* LATIN CAPITAL LETTER I WITH ACUTE */
	case 0x00f3: return 0x90; /* LATIN SMALL LETTER O WITH ACUTE */
	case 0x00d3: return 0x7d; /* LATIN CAPITAL LETTER O WITH ACUTE */
	case 0x00f6: return 0x7c; /* LATIN SMALL LETTER O WITH DIAERESIS (!) */
	case 0x00d6: return 0x5c; /* LATIN CAPITAL LETTER O WITH DIAERESIS (!) */
	case 0x0151: return 0x96; /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
	case 0x0150: return 0x95; /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
	case 0x00fa: return 0x97; /* LATIN SMALL LETTER U WITH ACUTE */
	case 0x00da: return 0x80; /* LATIN CAPITAL LETTER U WITH ACUTE */
	case 0x00fc: return 0x7e; /* LATIN SMALL LETTER U WITH DIAERESIS (!) */
	case 0x00dc: return 0x5e; /* LATIN CAPITAL LETTER U WITH DIAERESIS (!) */
	case 0x0171: return 0xce; /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
	case 0x0170: return 0xcc; /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
	default: return char_encode_def_alphabet((unsigned char)wch);
	}
}

void PNOK_DecodeString(unsigned char *dest, size_t max, const unsigned char *src, size_t len)
{
	size_t i, j, n;
	unsigned char buf[16];

	for (i = 0, j = 0; j < len; i += n, j++) {
		n = char_decode_uni_alphabet(PNOK_nokia_to_uni(src[j]), buf);
		if (i + n >= max) break;
		memcpy(dest + i, buf, n);
	}
	dest[i] = 0;
}

size_t PNOK_EncodeString(unsigned char *dest, size_t max, const unsigned char *src)
{
	size_t i, j, n;
	wchar_t wch;

	for (i = 0, j = 0; i < max && src[j]; i++, j += n) {
		n = char_encode_uni_alphabet(src + j, &wch);
		dest[i] = PNOK_uni_to_nokia(wch);
	}
	return i;
}

/* Call Divert */
gn_error PNOK_CallDivert(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned short length = 0x09;
	char req[55] = { FBUS_FRAME_HEADER, 0x01, 0x00, /* operation */
						0x00,
						0x00, /* divert type */
						0x00, /* call type */
						0x00 };
	if (!data->CallDivert) return GN_ERR_UNKNOWN;
	switch (data->CallDivert->Operation) {
	case GSM_CDV_Query:
		req[4] = 0x05;
		break;
	case GSM_CDV_Register:
		req[4] = 0x03;
		length = 0x37;
		req[8] = 0x01;
		req[9] = char_semi_octet_pack(data->CallDivert->Number.Number,
					      req + 10, data->CallDivert->Number.Type);
		req[54] = data->CallDivert->Timeout;
		break;
	case GSM_CDV_Erasure:
		req[4] = 0x04;
		break;
	default:
		return GN_ERR_NOTIMPLEMENTED;
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
		return GN_ERR_NOTIMPLEMENTED;
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
		return GN_ERR_NOTIMPLEMENTED;
	}
	if ((data->CallDivert->DType == GSM_CDV_AllTypes) &&
	    (data->CallDivert->CType == GSM_CDV_AllCalls))
		req[6] = 0x02;

	if (SM_SendMessage(state, length, 0x06, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_BlockTimeout(state, data, 0x06, 100);
}

gn_error PNOK_IncomingCallDivert(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char *pos;
	GSM_CallDivert *cd;

	switch (message[3]) {
	/* Get call diverts ok */
	case 0x02:
		pos = message + 4;
		cd = data->CallDivert;
		if (*pos != 0x05 && *pos != 0x04) return GN_ERR_UNHANDLEDFRAME;
		pos++;
		if (*pos++ != 0x00) return GN_ERR_UNHANDLEDFRAME;
		switch (*pos++) {
		case 0x02:
		case 0x15: cd->DType = GSM_CDV_AllTypes; break;
		case 0x43: cd->DType = GSM_CDV_Busy; break;
		case 0x3d: cd->DType = GSM_CDV_NoAnswer; break;
		case 0x3e: cd->DType = GSM_CDV_OutOfReach; break;
		default: return GN_ERR_UNHANDLEDFRAME;
		}
		if (*pos++ != 0x02) return GN_ERR_UNHANDLEDFRAME;
		switch (*pos++) {
		case 0x00: cd->CType = GSM_CDV_AllCalls; break;
		case 0x0b: cd->CType = GSM_CDV_VoiceCalls; break;
		case 0x0d: cd->CType = GSM_CDV_FaxCalls; break;
		case 0x19: cd->CType = GSM_CDV_DataCalls; break;
		default: return GN_ERR_UNHANDLEDFRAME;
		}
		if (message[4] == 0x04 && pos[0] == 0x00) {
			return GN_ERR_EMPTYLOCATION;
		} else if (message[4] == 0x04 || (pos[0] == 0x01 && pos[1] == 0x00)) {
			cd->Number.Type = SMS_Unknown;
			memset(cd->Number.Number, 0, sizeof(cd->Number.Number));
		} else if (pos[0] == 0x02 && pos[1] == 0x01) {
			pos += 2;
			snprintf(cd->Number.Number, sizeof(cd->Number.Number),
				 "%-*.*s", *pos+1, *pos+1, char_get_bcd_number(pos+1));
			pos += 12 + 22;
			cd->Timeout = *pos++;
		}
		break;

	/* FIXME: failed calldivert result code? */
	case 0x03:
		return GN_ERR_UNHANDLEDFRAME;
	
	/* FIXME: call divert is active */
	case 0x06:
		return GN_ERR_UNSOLICITED;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}

int PNOK_FBUS_EncodeSMS(GSM_Data *data, GSM_Statemachine *state, unsigned char *req)
{
	int pos = 0;

	if (data->RawSMS->MessageCenter[0] != '\0') 
		memcpy(req, data->RawSMS->MessageCenter, 12);
	pos += 12;

	if (data->RawSMS->Type != SMS_Deliver)
		req[pos] = 0x01; /* SMS Submit */
	else
		req[pos] = 0x00; /* SMS Deliver */

	if (data->RawSMS->ReplyViaSameSMSC)  req[pos] |= 0x80;
	if (data->RawSMS->RejectDuplicates)  req[pos] |= 0x04;
	if (data->RawSMS->Report)            req[pos] |= 0x20;
	if (data->RawSMS->UDHIndicator)      req[pos] |= 0x40;
	if (data->RawSMS->Type != SMS_Deliver) {
		if (data->RawSMS->ValidityIndicator) req[pos] |= 0x10;
		pos++;
		req[pos++] = data->RawSMS->Reference;
	} else
		pos++;

	req[pos++] = data->RawSMS->PID;
	req[pos++] = data->RawSMS->DCS;
	req[pos++] = data->RawSMS->Length;
	memcpy(req + pos, data->RawSMS->RemoteNumber, 12);
	pos += 12;

	if (data->RawSMS->Type != SMS_Deliver)
		memcpy(req + pos, data->RawSMS->Validity, 7);
	else
		memcpy(req + pos, data->RawSMS->SMSCTime, 7);  /* FIXME: Real date instead of hardcoded */
	pos += 7;

	memcpy(req + pos, data->RawSMS->UserData, data->RawSMS->UserDataLength);
	pos += data->RawSMS->UserDataLength;

	return pos;
}
