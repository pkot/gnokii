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

  Copyright (C) 2001-2002 Pawe³ Kot <pkot@linuxnews.pl>
  Copyright (C) 2001-2002 Pavel Machek <pavel@ucw.cz>

  Library for parsing and creating Short Messages (SMS).

  THIS IS OBSOLETE, PLEASE TRY TO AVOID USING THIS

*/

#define DEBUG

#include <stdlib.h>
#include <string.h>

#include "gsm-data.h"
#include "gsm-encoding.h"
#include "gsm-statemachine.h"

#define BITMAP_SUPPORT 1
#define RINGTONE_SUPPORT 1

#ifdef BITMAP_SUPPORT
#  include "gsm-ringtones.h"
#endif
#ifdef RINGTONE_SUPPORT
#  include "gsm-bitmaps.h"
#endif


/***
 *** Functions playing with layout, used for encoding SMS
 ***/

/* These 4 functions are mainly for the AT mode, where MessageCenter and
 * RemoteNumber have variable length */

/* Returns a new offset of the field */
static int change_offset(int base, int orig, int offset)
{
	if (orig <= base) return orig;
	else return (orig + offset);
}

static void change_offsets2(SMSMessage_Layout *layout, int pos, int offset)
{
	if (pos < 0) return;
	layout->MessageCenter = change_offset(pos, layout->MessageCenter, offset);
	layout->MoreMessages = change_offset(pos, layout->MoreMessages, offset);
	layout->ReplyViaSameSMSC = change_offset(pos, layout->ReplyViaSameSMSC, offset);
	layout->RejectDuplicates = change_offset(pos, layout->RejectDuplicates, offset);
	layout->Report = change_offset(pos, layout->Report, offset);
	layout->Number = change_offset(pos, layout->Number, offset);
	layout->Reference = change_offset(pos, layout->Reference, offset);
	layout->PID = change_offset(pos, layout->PID, offset);
	layout->ReportStatus = change_offset(pos, layout->ReportStatus, offset);
	layout->Length = change_offset(pos, layout->Length, offset);
	layout->DataCodingScheme = change_offset(pos, layout->DataCodingScheme, offset);
	layout->Validity = change_offset(pos, layout->Validity, offset);
	layout->UserDataHeader = change_offset(pos, layout->UserDataHeader, offset);
	layout->RemoteNumber = change_offset(pos, layout->RemoteNumber , offset);
	layout->SMSCTime = change_offset(pos, layout->SMSCTime, offset);
	layout->Time = change_offset(pos, layout->Time, offset);
	layout->MemoryType = change_offset(pos, layout->MemoryType, offset);
	layout->Status = change_offset(pos, layout->Status, offset);
	layout->UserData = change_offset(pos, layout->UserData, offset);
}

/* Use this one when you have decoded message (in struct SMS) */
static void change_offsets_struct(SMSMessage_Layout *layout, SMS_Number mc, SMS_Number rn)
{
	unsigned char aux[16];
	unsigned int mc_len, rn_len;

	mc_len = SemiOctetPack(mc.Number, aux, mc.Type);
	rn_len = SemiOctetPack(rn.Number, aux, rn.Type);
	if (mc_len) {
		if (mc_len % 2) mc_len++;
		mc_len /= 2;
		mc_len++;
	}
	if (rn_len) {
		if (rn_len % 2) rn_len++;
		rn_len /= 2;
		rn_len++;
	}
	if (layout->MessageCenter < layout->RemoteNumber) {
		if (!layout->HasMessageCenterFixedLen && layout->MessageCenter > -1)
			change_offsets2(layout, layout->MessageCenter, mc_len);
		if (!layout->HasRemoteNumberFixedLen && layout->RemoteNumber > -1)
			change_offsets2(layout, layout->RemoteNumber, rn_len);
	} else {
		if (!layout->HasRemoteNumberFixedLen && layout->RemoteNumber > -1)
			change_offsets2(layout, layout->RemoteNumber, rn_len);
		if (!layout->HasMessageCenterFixedLen && layout->MessageCenter > -1)
			change_offsets2(layout, layout->MessageCenter, mc_len);
	}
}

/***
 *** ENCODING SMS
 ***/

/**
 * EncodeSMSSubmitHeader - prepares a phone frame with a SMS header
 * @SMS - SMS structure with a data source
 * @frame - a phone frame to fill in
 *
 * We prepare here a part of the frame with a submit header (phone originated)
 */
static GSM_Error EncodeSMSSubmitHeader(SMSMessage_Layout *layout, GSM_API_SMS *sms, GSM_SMSMessage *rawsms, char *frame)
{
	GSM_Error error = GE_NONE;

	/* Standard Header: */
	memcpy(frame + layout->UserDataHeader, "\x11\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xa9\x00\x00\x00\x00\x00\x00", 24);

	/* Reply Path */
	if (layout->ReplyViaSameSMSC > -1) {
		if (rawsms->ReplyViaSameSMSC) frame[layout->ReplyViaSameSMSC] |= 0x80;
	}

	/* User Data Header Indicator */
	if (layout->UserDataHeader > -1) {
		if (rawsms->UDHIndicator) frame[layout->UserDataHeader] |= 0x40;
	}

	/* Status (Delivery) Report Request */
	if (layout->Report > -1) {
		if (rawsms->Report) frame[layout->Report] |= 0x20;
	}

	/* Validity Period Format: mask - 0x00, 0x10, 0x08, 0x18 */
	if (layout->Validity > -1) {
		frame[layout->Validity] |= 0xab; // rawsms->Validity[0];
	}

	/* Reject Duplicates */
	if (layout->RejectDuplicates > -1) {
		if (rawsms->RejectDuplicates) frame[layout->RejectDuplicates] |= 0x04;
	}

	/* Protocol Identifier */
	/* FIXME: allow to change this in better way.
	   currently only 0x5f == `Return Call Message' is used */
	if (layout->PID > -1) {
		if (rawsms->PID) frame[layout->PID] = rawsms->PID;
	}

	/* Data Coding Scheme */
	if (layout->DataCodingScheme > -1) {
		switch (sms->DCS.Type) {
		case SMS_GeneralDataCoding:
			if (sms->DCS.u.General.Compressed) frame[layout->DataCodingScheme] |= 0x20;
			if (sms->DCS.u.General.Class) frame[layout->DataCodingScheme] |= (0x10 | (sms->DCS.u.General.Class - 1));
			frame[layout->DataCodingScheme] |= ((sms->DCS.u.General.Alphabet & 0x03) << 2);
			break;
		case SMS_MessageWaiting:
			if (sms->DCS.u.MessageWaiting.Discard) frame[layout->DataCodingScheme] |= 0xc0;
			else if (sms->DCS.u.MessageWaiting.Alphabet == SMS_UCS2) frame[layout->DataCodingScheme] |= 0xe0;
			else frame[layout->DataCodingScheme] |= 0xd0;
			if (sms->DCS.u.MessageWaiting.Active) frame[layout->DataCodingScheme] |= 0x80;
			frame[layout->DataCodingScheme] |= (sms->DCS.u.MessageWaiting.Type & 0x03);
			break;
		default:
			dprintf("Wrong Data Coding Scheme (DCS) format\n");
			return GE_SMSWRONGFORMAT;
		}
	}

	/* Destination Address */
	if (layout->RemoteNumber > -1) {
		frame[layout->RemoteNumber] = SemiOctetPack(sms->Remote.Number, frame + layout->RemoteNumber + 1, sms->Remote.Type);
	}

#if 0
	/* Validity Period */
	if (layout->Validity > -1) {
		switch (SMS->Validity.VPF) {
		case SMS_EnhancedFormat:
			return GE_NOTSUPPORTED;
		case SMS_RelativeFormat:
			frame[layout->Validity] = SMS->Validity.u.Relative;
			break;
		case SMS_AbsoluteFormat:
			break;
		default:
			return GE_SMSWRONGFORMAT;
		}
	}
#endif
	return error;
}

/**
 * EncodePDUSMS - prepares a phone frame with a given SMS
 * @SMS:
 * @message:
 * @num:
 * @length:
 *
 * This function encodes SMS as described in:
 *  o GSM 03.40 version 6.1.0 Release 1997, section 9
 */
GSM_Error EncodeByLayout(GSM_Data *data, SMSMessage_Layout *clayout, unsigned int num)
{
	GSM_API_SMS *SMS = data->SMS;
	GSM_SMSMessage *rawsms = data->RawSMS;
	GSM_Error error = GE_NONE;
	int i, clen = rawsms->UserDataLength, mm = 0;
	char *message = data->RawData->Data;

	/* We actually scribble to our layout, and we do not want to propagate that uglyness elsewhere */

	SMSMessage_Layout vlayout = *clayout, *layout = &vlayout;

	data->RawData->Length = 0;

	change_offsets_struct(layout, SMS->SMSC, SMS->Remote);

	/* rawsmsC number */
	if (layout->MessageCenter > -1) {
		message[layout->MessageCenter] = SemiOctetPack(SMS->SMSC.Number, message + layout->MessageCenter + 1, SMS->SMSC.Type);
		if (message[layout->MessageCenter]) {
			if (message[layout->MessageCenter] % 2) message[layout->MessageCenter]++;
			message[layout->MessageCenter] = message[layout->MessageCenter] / 2 + 1;
		}
	}

	/* Common Header */
	error = EncodeSMSSubmitHeader(layout, SMS, rawsms, message);	/* Very depenend on layout, basically builds header */
	if (error != GE_NONE) return error;

	memcpy(message + layout->UserData, rawsms->UserData, clen);
	message[layout->DataCodingScheme] = rawsms->DCS;
	message[layout->Length] = rawsms->Length + 0;
	data->RawData->Length = clen + layout->UserData + 0;
	return GE_NONE;
}
