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

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Library for parsing and creating Short Messages (SMS).

*/

#include <stdlib.h>
#include <string.h>

#include "gsm-data.h"
#include "gsm-encoding.h"
#include "gsm-statemachine.h"
#include "gsm-ringtones.h"
#include "gsm-bitmaps.h"

SMSMessage_PhoneLayout layout;
static SMSMessage_Layout llayout;
static unsigned short header_offset;

struct udh_data {
	unsigned int length;
	char *header;
};

/* User data headers */
static struct udh_data headers[] = {
	{ 0x00, "" },
	{ 0x05, "\x00\x03\x01\x00\x00" },     /* Concatenated messages */
	{ 0x06, "\x05\x04\x15\x81\x00\x00" }, /* Ringtones */
	{ 0x06, "\x05\x04\x15\x82\x00\x00" }, /* Operator logos */
	{ 0x06, "\x05\x04\x15\x83\x00\x00" }, /* Caller logos */
	{ 0x06, "\x05\x04\x15\x8a\x00\x00" }, /* Multipart Message */
	{ 0x06, "\x05\x04\x23\xf4\x00\x00" }, /* WAP vCard */
	{ 0x06, "\x05\0x4\x23\xf5\x00\x00" }, /* WAP vCalendar */
	{ 0x06, "\x05\x04\x23\xf6\x00\x00" }, /* WAP vCardSecure */
	{ 0x06, "\x05\0x4\x23\xf7\x00\x00" }, /* WAP vCalendarSecure */
	{ 0x04, "\x03\x01\x00\x00" },         /* Voice Messages */
	{ 0x04, "\x03\x01\x01\x00" },         /* Fax Messages */
	{ 0x04, "\x03\x01\x02\x00" },         /* Email Messages */
	{ 0x00, "" }
};


/***
 *** Util functions
 ***/

/* This function implements packing of numbers (SMS Center number and
   destination number) for SMS sending function. */
int SemiOctetPack(char *Number, unsigned char *Output, SMS_NumberType type)
{
#ifndef WIN32
	unsigned char *IN = Number;  /* Pointer to the input number */
	unsigned char *OUT = Output; /* Pointer to the output */
	int count = 0; /* This variable is used to notify us about count of already
			  packed numbers. */

	/* The first byte in the Semi-octet representation of the address field is
	   the Type-of-Address. This field is described in the official GSM
	   specification 03.40 version 6.1.0, section 9.1.2.5, page 33. We support
	   only international and unknown number. */

	*OUT++ = type;
	if (type == SMS_International) IN++; /* Skip '+' */
	if ((type == SMS_Unknown) && (*IN == '+')) IN++; /* Optional '+' in Unknown number type */

	/* The next field is the number. It is in semi-octet representation - see
	   GSM scpecification 03.40 version 6.1.0, section 9.1.2.3, page 31. */
	while (*IN) {
		if (count & 0x01) {
			*OUT = *OUT | ((*IN - '0') << 4);
			OUT++;
		}
		else
			*OUT = *IN - '0';
		count++; IN++;
	}

	/* We should also fill in the most significant bits of the last byte with
	   0x0f (1111 binary) if the number is represented with odd number of
	   digits. */
	if (count & 0x01) {
		*OUT = *OUT | 0xf0;
		OUT++;
	}

	return (2 * (OUT - Output - 1) - (count % 2));
#endif
}

char *GetBCDNumber(u8 *Number)
{
	static char Buffer[20] = "";
	int length = Number[0]; /* This is the length of BCD coded number */
	int count, Digit;

	memset(Buffer, 0, 20);
	switch (Number[1]) {
	case SMS_Alphanumeric:
		Unpack7BitCharacters(0, length, length, Number+2, Buffer);
		Buffer[length] = 0;
		break;
	case SMS_International:
		sprintf(Buffer, "+");
	case SMS_Unknown:
	case SMS_National:
	case SMS_Network:
	case SMS_Subscriber:
	case SMS_Abbreviated:
	default:
		for (count = 0; count < length - 1; count++) {
			Digit = Number[count+2] & 0x0f;
			if (Digit < 10) sprintf(Buffer, "%s%d", Buffer, Digit);
			Digit = Number[count+2] >> 4;
			if (Digit < 10) sprintf(Buffer, "%s%d", Buffer, Digit);
		}
		break;
	}
	return Buffer;
}

#ifdef DEBUG
static char *PrintDateTime(u8 *Number)
{
	static char Buffer[23] = "";

	memset(Buffer, 0, 23);
	if (Number[0] < 70) sprintf(Buffer, "20");
	else sprintf(Buffer, "19");
	sprintf(Buffer, "%s%d%d-", Buffer, Number[0] & 0x0f, Number[0] >> 4);
	sprintf(Buffer, "%s%d%d-", Buffer, Number[1] & 0x0f, Number[1] >> 4);
	sprintf(Buffer, "%s%d%d ", Buffer, Number[2] & 0x0f, Number[2] >> 4);
	sprintf(Buffer, "%s%d%d:", Buffer, Number[3] & 0x0f, Number[3] >> 4);
	sprintf(Buffer, "%s%d%d:", Buffer, Number[4] & 0x0f, Number[4] >> 4);
	sprintf(Buffer, "%s%d%d",  Buffer, Number[5] & 0x0f, Number[5] >> 4);
	if (Number[6] & 0x08)
		sprintf(Buffer, "%s-", Buffer);
	else
		sprintf(Buffer, "%s+", Buffer);
	sprintf(Buffer, "%s%02d00", Buffer, (10 * (Number[6] & 0x07) + (Number[6] >> 4)) / 4);

	return Buffer;
}
#endif

SMS_DateTime *UnpackDateTime(u8 *Number, SMS_DateTime *dt)
{
	dt->Year     =  10 * (Number[0] & 0x0f) + (Number[0] >> 4);
	if (dt->Year < 70) dt->Year += 2000;
	else dt->Year += 1900;
	dt->Month    =  10 * (Number[1] & 0x0f) + (Number[1] >> 4);
	dt->Day      =  10 * (Number[2] & 0x0f) + (Number[2] >> 4);
	dt->Hour     =  10 * (Number[3] & 0x0f) + (Number[3] >> 4);
	dt->Minute   =  10 * (Number[4] & 0x0f) + (Number[4] >> 4);
	dt->Second   =  10 * (Number[5] & 0x0f) + (Number[5] >> 4);
	dt->Timezone = (10 * (Number[6] & 0x07) + (Number[6] >> 4)) / 4;
	if (Number[6] & 0x08) dt->Timezone = -dt->Timezone;

	return dt;
}

static int CountSMSParts(GSM_SMSMessage *SMS)
{
	unsigned int i, j, count, length = 0, header = 0;
	bool multi = false;
	SMS_AlphabetType alph;

	if (!SMS) return 0;
	/* Multipart Message */
	if (SMS->UserData[1].Type != SMS_NoData) {
		multi = true;
	}

	for (i = 0; SMS->UserData[i].Type != SMS_NoData; i++) {
		/* multipart message part type (1) + part's length (2) */
		if (multi) length += 3;
		switch (SMS->UserData[i].Type) {
		case SMS_PlainText:
			switch (SMS->DCS.Type) {
			case SMS_GeneralDataCoding:
				alph = SMS->DCS.u.General.Alphabet;
				break;
			case SMS_MessageWaiting:
				alph = SMS->DCS.u.MessageWaiting.Alphabet;
				break;
			default:
				/* Malformed SMS struct */
				return -1;
			}
			switch (alph) {
			case SMS_DefaultAlphabet:
				length += (SMS->UserData[i].Length - SMS->UserData[i].Length / 8);
				break;
			case SMS_8bit:
				length += SMS->UserData[i].Length;
				break;
			case SMS_UCS2:
				length += SMS->UserData[i].Length;
				break;
			default:
				/* Malformed SMS struct */
				return -1;
			}
			break;
		case SMS_BitmapData:
			if (!multi) {
				/* Look for the bitmap type in UDH headers */
				for (j = 0; j < SMS->UDH_No; j++) {
					bool found = false;
					switch (SMS->UDH[j].Type) {
					case SMS_OpLogo:
						header += headers[SMS_OpLogo].length;
						length += 5;
						found = true;
						break;
					case SMS_CallerIDLogo:
						length += headers[SMS_CallerIDLogo].length;
						length += 1;
						found = true;
						break;
					default:
						break;
					}
					if (found) break;
				}

			}
			break;
		case SMS_RingtoneData:
			/* FIXME */
			/* All bitmaps have the same length of the UDH */
			if (!multi) {
				header += headers[SMS_OpLogo].length;
			}
			break;
		default:
			/* Malformed SMS struct */
			return -1;
		}
	}
	/* Smart Messaging Specification number */
	if (multi) length++;

	if (length + header > (GSM_MAX_8BIT_SMS_LENGTH - (SMS->UDH_No ? 1 : 0))) {
		count = (length + header) /
			(GSM_MAX_8BIT_SMS_LENGTH - 1 -
			 headers[SMS_ConcatenatedMessages].length -
			 (multi ? headers[SMS_MultipartMessage].length : 0));
		if ((length + header) %
		    (GSM_MAX_8BIT_SMS_LENGTH - 1 -
		     headers[SMS_ConcatenatedMessages].length -
		     (multi ? headers[SMS_MultipartMessage].length : 0)))
			count++;
	} else {
		count = 1;
	}
	return count;
}


/* These 4 functions are mainly for the AT mode, where MessageCenter and
 * RemoteNumber have variable length */

/* Returns a new offset of the field */
static short change_offset(short base, short orig, short offset)
{
	if (orig <= base) return orig;
	else return (orig + offset);
}
static void change_offsets2(short pos, short offset)
{
	if (pos < 0) return;
	llayout.MessageCenter = change_offset(pos, llayout.MessageCenter, offset);
	llayout.MoreMessages = change_offset(pos, llayout.MoreMessages, offset);
	llayout.ReplyViaSameSMSC = change_offset(pos, llayout.ReplyViaSameSMSC, offset);
	llayout.RejectDuplicates = change_offset(pos, llayout.RejectDuplicates, offset);
	llayout.Report = change_offset(pos, llayout.Report, offset);
	llayout.Number = change_offset(pos, llayout.Number, offset);
	llayout.Reference = change_offset(pos, llayout.Reference, offset);
	llayout.PID = change_offset(pos, llayout.PID, offset);
	llayout.ReportStatus = change_offset(pos, llayout.ReportStatus, offset);
	llayout.Length = change_offset(pos, llayout.Length, offset);
	llayout.DataCodingScheme = change_offset(pos, llayout.DataCodingScheme, offset);
	llayout.Validity = change_offset(pos, llayout.Validity, offset);
	llayout.UserDataHeader = change_offset(pos, llayout.UserDataHeader, offset);
	llayout.RemoteNumber = change_offset(pos, llayout.RemoteNumber , offset);
	llayout.SMSCTime = change_offset(pos, llayout.SMSCTime, offset);
	llayout.Time = change_offset(pos, llayout.Time, offset);
	llayout.MemoryType = change_offset(pos, llayout.MemoryType, offset);
	llayout.Status = change_offset(pos, llayout.Status, offset);
	llayout.UserData = change_offset(pos, llayout.UserData, offset);
}

/* Use this one when you have encoded message */
static void change_offsets(unsigned char *message)
{
	if (llayout.MessageCenter < llayout.RemoteNumber) {
		if (!llayout.HasMessageCenterFixedLen && llayout.MessageCenter > -1)
			change_offsets2(llayout.MessageCenter, message[llayout.MessageCenter]);
		if (!llayout.HasRemoteNumberFixedLen && llayout.RemoteNumber > -1)
			change_offsets2(llayout.RemoteNumber,
					llayout.IsRemoteNumberCoded ?
					(message[llayout.RemoteNumber] + message[llayout.RemoteNumber] % 2) / 2 + 1 :
					message[llayout.RemoteNumber]);
	} else {
		if (!llayout.HasRemoteNumberFixedLen && llayout.RemoteNumber > -1)
			change_offsets2(llayout.RemoteNumber,
					llayout.IsRemoteNumberCoded ?
					(message[llayout.RemoteNumber] + message[llayout.RemoteNumber] % 2) / 2 + 1 :
					message[llayout.RemoteNumber]);
		if (!llayout.HasMessageCenterFixedLen && llayout.MessageCenter > -1)
			change_offsets2(llayout.MessageCenter, message[llayout.MessageCenter]);
	}
}

/* Use this one when you have decoded message (in struct SMS) */
static void change_offsets_struct(SMS_MessageCenter mc, SMS_Number rn)
{
	unsigned char aux[16];
	unsigned short mc_len, rn_len;

	mc_len = SemiOctetPack(mc.Number, aux, mc.Type);
	rn_len = SemiOctetPack(rn.number, aux, rn.type);
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
	if (llayout.MessageCenter < llayout.RemoteNumber) {
		if (!llayout.HasMessageCenterFixedLen && llayout.MessageCenter > -1)
			change_offsets2(llayout.MessageCenter, mc_len);
		if (!llayout.HasRemoteNumberFixedLen && llayout.RemoteNumber > -1)
			change_offsets2(llayout.RemoteNumber, rn_len);
	} else {
		if (!llayout.HasRemoteNumberFixedLen && llayout.RemoteNumber > -1)
			change_offsets2(llayout.RemoteNumber, rn_len);
		if (!llayout.HasMessageCenterFixedLen && llayout.MessageCenter > -1)
			change_offsets2(llayout.MessageCenter, mc_len);
	}
}

/***
 *** ENCODING SMS
 ***/

static GSM_Error EncodeData(GSM_SMSMessage *SMS, char *dcs, char *message, bool multipart, int *clen)
{
	SMS_AlphabetType al;
	unsigned short i, length, size = 0, offset = 0;
	short text_index = -1, bitmap_index = -1, ringtone_index = -1;

	/* Decide what to encode */
	if (multipart) {
		/* Version: Smart Messaging Specification 3.0.0 */
		message[0] = 0x30;
		for (i = 0; i < 3; i++) {
			switch (SMS->UserData[i].Type) {
			case SMS_PlainText:
				text_index     = i; break;
			case SMS_BitmapData:
				bitmap_index   = i; break;
			case SMS_RingtoneData:
				ringtone_index = i; break;
			default:
				break;
			}
		}
	} else if (SMS->UDH_No) {
		switch (SMS->UDH[0].Type) {
		case SMS_OpLogo:
		case SMS_CallerIDLogo:
			bitmap_index   = 0; break;
		case SMS_Ringtone:
			ringtone_index = 0; break;
		default:
			text_index     = 0; break;
		}
	} else {
		text_index = 0;
	}

	length = strlen(SMS->UserData[0].u.Text);

	/* Additional Headers */
	switch (SMS->DCS.Type) {
	case SMS_GeneralDataCoding:
		switch (SMS->DCS.u.General.Class) {
		case 1: dcs[0] |= 0xf0; break; /* Class 0 */
		case 2: dcs[0] |= 0xf1; break; /* Class 1 */
		case 3: dcs[0] |= 0xf2; break; /* Class 2 */
		case 4: dcs[0] |= 0xf3; break; /* Class 3 */
		default: break;
		}
		if (SMS->DCS.u.General.Compressed) {
			/* Compression not supported yet */
			/* dcs[0] |= 0x20; */
		}
		al = SMS->DCS.u.General.Alphabet;
		break;
	case SMS_MessageWaiting:
		al = SMS->DCS.u.MessageWaiting.Alphabet;
		if (SMS->DCS.u.MessageWaiting.Discard) dcs[0] |= 0xc0;
		else if (SMS->DCS.u.MessageWaiting.Alphabet == SMS_UCS2) dcs[0] |= 0xe0;
		else dcs[0] |= 0xd0;

		if (SMS->DCS.u.MessageWaiting.Active) dcs[0] |= 0x08;
		dcs[0] |= (SMS->DCS.u.MessageWaiting.Type & 0x03);

		break;
	default:
		return GE_SMSWRONGFORMAT;
	}

	if ((al == SMS_8bit) && multipart) al = SMS_DefaultAlphabet;
	SMS->Length = *clen = 0;

	/* Text Coding */
	if (text_index != -1) {
		switch (al) {
		case SMS_DefaultAlphabet:
			if (multipart) {
				offset = 4;
				memcpy(message + 1, "\x00\x00\x00", 3);
				dcs[0] |= 0xf4;
			}
			size = Pack7BitCharacters((7 - (SMS->UDH_Length % 7)) % 7, SMS->UserData[text_index].u.Text, message + offset);
			// SMS->Length = 8 * SMS->UDH_Length + (7 - (SMS->UDH_Length % 7)) % 7 + length + offset;
			SMS->Length = strlen(SMS->UserData[text_index].u.Text);
			*clen = size + offset;
			if (multipart) {
				message[2] = (size & 0xff00) >> 8;
				message[3] = (size & 0x00ff);
			}
			break;
		case SMS_8bit:
			dcs[0] |= 0xf4;
			memcpy(message, SMS->UserData[text_index].u.Text + 1, SMS->UserData[text_index].u.Text[0]);
			*clen = SMS->Length = SMS->UserData[text_index].u.Text[0];
			break;
		case SMS_UCS2:
			if (multipart) {
				offset = 4;
				memcpy(message + 1, "\x02\x00\x00", 3);
			}
			dcs[0] |= 0x08;
			EncodeUnicode(message + offset, SMS->UserData[text_index].u.Text, length);
			*clen = SMS->Length = length + offset;
			if (multipart) {
				size = 2 * length;
				message[2] = (size & 0xff00) >> 8;
				message[3] = (size & 0x00ff);
			}
			break;
		default:
			return GE_SMSWRONGFORMAT;
		}
	}

	/* Bitmap coding */
	if (bitmap_index != -1) {
		size = GSM_EncodeSMSBitmap(&(SMS->UserData[bitmap_index].u.Bitmap), message + SMS->Length);
		SMS->Length += size;
		*clen += size;
	}

	/* Ringtone coding */
	if (ringtone_index != -1) {
		size = GSM_EncodeSMSRingtone(message + SMS->Length, &SMS->UserData[ringtone_index].u.Ringtone);
		SMS->Length += size;
		*clen += size;
	}
	return GE_NONE;
}

/* This function encodes the UserDataHeader as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
*/
static GSM_Error EncodeUDH(SMS_UDHInfo UDHi, GSM_SMSMessage *SMS, char *UDH)
{
	unsigned char pos;

	pos = UDH[0];
	switch (UDHi.Type) {
	case SMS_NoUDH:
		break;
	case SMS_VoiceMessage:
	case SMS_FaxMessage:
	case SMS_EmailMessage:
		UDH[pos+4] = UDHi.u.SpecialSMSMessageIndication.MessageCount;
		if (UDHi.u.SpecialSMSMessageIndication.Store) UDH[pos+3] |= 0x80;
	case SMS_ConcatenatedMessages:
	case SMS_OpLogo:
	case SMS_CallerIDLogo:
	case SMS_Ringtone:
	case SMS_MultipartMessage:
		UDH[0] += headers[UDHi.Type].length;
		memcpy(UDH+pos+1, headers[UDHi.Type].header, headers[UDHi.Type].length);
		break;
	default:
		dprintf("Not supported User Data Header type\n");
		break;
	}
	return GE_NONE;
}

static GSM_Error EncodeSMSSubmitHeader(GSM_SMSMessage *SMS, char *frame)
{
	GSM_Error error = GE_NONE;

	/* Standard Header: */
	memcpy(frame + llayout.UserDataHeader, "\x11\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xa9\x00\x00\x00\x00\x00\x00", 24);

	/* Reply Path */
	if (llayout.ReplyViaSameSMSC > -1) {
		if (SMS->ReplyViaSameSMSC) frame[llayout.ReplyViaSameSMSC] |= 0x80;
	}

	/* User Data Header Indicator */
	if (llayout.UserDataHeader > -1) {
		if (SMS->UDH_No) frame[llayout.UserDataHeader] |= 0x40;
	}

	/* Status (Delivery) Report Request */
	if (llayout.Report > -1) {
		if (SMS->Report) frame[llayout.Report] |= 0x20;
	}

	/* Validity Period Format: mask - 0x00, 0x10, 0x08, 0x18 */
	if (llayout.Validity > -1) {
		frame[llayout.Validity] |= ((SMS->Validity.VPF & 0x03) << 3);
	}

	/* Reject Duplicates */
	if (llayout.RejectDuplicates > -1) {
		if (SMS->RejectDuplicates) frame[llayout.RejectDuplicates] |= 0x04;
	}

	/* Protocol Identifier */
	/* FIXME: allow to change this in better way.
	   currently only 0x5f == `Return Call Message' is used */
	if (llayout.PID > -1) {
		if (SMS->PID) frame[llayout.PID] = SMS->PID;
	}

	/* Data Coding Scheme */
	if (llayout.DataCodingScheme > -1) {
		switch (SMS->DCS.Type) {
		case SMS_GeneralDataCoding:
			if (SMS->DCS.u.General.Compressed) frame[llayout.DataCodingScheme] |= 0x20;
			if (SMS->DCS.u.General.Class) frame[llayout.DataCodingScheme] |= (0x10 | (SMS->DCS.u.General.Class - 1));
			frame[llayout.DataCodingScheme] |= ((SMS->DCS.u.General.Alphabet & 0x03) << 2);
			break;
		case SMS_MessageWaiting:
			if (SMS->DCS.u.MessageWaiting.Discard) frame[llayout.DataCodingScheme] |= 0xc0;
			else if (SMS->DCS.u.MessageWaiting.Alphabet == SMS_UCS2) frame[llayout.DataCodingScheme] |= 0xe0;
			else frame[llayout.DataCodingScheme] |= 0xd0;
			if (SMS->DCS.u.MessageWaiting.Active) frame[llayout.DataCodingScheme] |= 0x80;
			frame[llayout.DataCodingScheme] |= (SMS->DCS.u.MessageWaiting.Type & 0x03);
			break;
		default:
			dprintf("Wrong Data Coding Scheme (DCS) format\n");
			return GE_SMSWRONGFORMAT;
		}
	}

	/* Destination Address */
	if (llayout.RemoteNumber > -1) {
		frame[llayout.RemoteNumber] = SemiOctetPack(SMS->RemoteNumber.number, frame + llayout.RemoteNumber + 1, SMS->RemoteNumber.type);
	}

	/* Validity Period */
	if (llayout.Validity > -1) {
		switch (SMS->Validity.VPF) {
		case SMS_EnhancedFormat:
			return GE_NOTSUPPORTED;
		case SMS_RelativeFormat:
			frame[llayout.Validity] = SMS->Validity.u.Relative;
			break;
		case SMS_AbsoluteFormat:
			break;
		default:
			return GE_SMSWRONGFORMAT;
		}
	}
	return error;
}

static GSM_Error EncodeSMSDeliverHeader()
{
	return GE_NONE;
}

static GSM_Error EncodeSMSHeader(GSM_SMSMessage *SMS, char *frame)
/* We can create either SMS DELIVER (for saving in Inbox) or SMS SUBMIT
   (for sending or saving in Outbox) message */
{
	/* Set SMS type */
	switch (SMS->Type) {
	case SMS_Submit: /* we send SMS or save it in Outbox */
		return EncodeSMSSubmitHeader(SMS, frame);
	case SMS_Deliver: /* we save SMS in Inbox */
		return EncodeSMSDeliverHeader(SMS, frame);
	default: /* we don't create other formats of SMS */
		return GE_SMSWRONGFORMAT;
	}
}

/* This function encodes SMS as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9
*/
static GSM_Error EncodePDUSMS(GSM_SMSMessage *SMS, char *message, unsigned short num, int *length)
{
	GSM_Error error = GE_NONE;
	int i, clen, mm = 0;

	*length = 0;
	switch (SMS->Type) {
	case SMS_Submit:
		llayout = layout.Submit;
		dprintf("Sending SMS to %s via message center %s\n", SMS->RemoteNumber.number, SMS->MessageCenter.Number);
		break;
	case SMS_Deliver:
		llayout = layout.Deliver;
		dprintf("Saving SMS to Inbox\n");
		break;
	case SMS_Picture:
	case SMS_Delivery_Report:
	default:
		dprintf("Not supported message type: %d\n", SMS->Type);
		return GE_NOTSUPPORTED;
	}

	if (!llayout.IsSupported) return GE_NOTSUPPORTED;

	change_offsets_struct(SMS->MessageCenter, SMS->RemoteNumber);

	/* SMSC number */
	if (llayout.MessageCenter > -1) {
		message[llayout.MessageCenter] = SemiOctetPack(SMS->MessageCenter.Number, message + llayout.MessageCenter + 1, SMS->MessageCenter.Type);
		if (message[llayout.MessageCenter]) {
			if (message[llayout.MessageCenter] % 2) message[llayout.MessageCenter]++;
			message[llayout.MessageCenter] = message[llayout.MessageCenter] / 2 + 1;
		}
	}

	/* Common Header */
	error = EncodeSMSHeader(SMS, message);
	if (error != GE_NONE) return error;

	/* User Data Header - if present */
	for (i = 0; i < SMS->UDH_No; i++) {
		error = EncodeUDH(SMS->UDH[i], SMS, message + llayout.UserData);
		if (SMS->UDH[i].Type == SMS_MultipartMessage) mm = 1;
		if (error != GE_NONE) return error;
	}
	SMS->UDH_Length = 0;

	/* User Data */
	EncodeData(SMS, message + llayout.DataCodingScheme, message + llayout.UserData + SMS->UDH_Length, mm, &clen);
	message[llayout.Length] = SMS->Length;
	*length = clen + llayout.UserData;
	return GE_NONE;
}

GSM_Error SendSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error = GE_NONE;
	GSM_RawData rawdata;
	int i, count;

	header_offset = layout.SendHeader;
	count = CountSMSParts(data->SMSMessage);

	if (data->SMSMessage->MessageCenter.No) {
		data->MessageCenter = &data->SMSMessage->MessageCenter;
		SM_Functions(GOP_GetSMSCenter, data, state);
	}

	if (count < 1) return GE_SMSWRONGFORMAT;

	dprintf("Strlen: %d\n", strlen(data->SMSMessage->UserData[0].u.Text));
	memset(&rawdata, 0, sizeof(rawdata));
	data->RawData = &rawdata;
	for (i = 0; i < count; i++) {
		data->RawData->Data = calloc(256, 1);
		error = EncodePDUSMS(data->SMSMessage, data->RawData->Data, i, &data->RawData->Length);
		if (error == GE_NONE) error = SM_Functions(GOP_SendSMS, data, state);
		dprintf("%d\n", data->SMSMessage->Length);
		free(data->RawData->Data);
		if (error != GE_SMSSENDOK) break;
	}
	data->RawData = NULL;
	return error;
}

GSM_Error SaveSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error = GE_NONE;
	GSM_RawData rawdata;
	int i, count;

	header_offset = layout.SendHeader;
	count = CountSMSParts(data->SMSMessage);

	memset(&data->SMSMessage->MessageCenter, 0, sizeof(data->SMSMessage->MessageCenter));

	if (count < 1) return GE_SMSWRONGFORMAT;

	memset(&rawdata, 0, sizeof(rawdata));
	data->RawData = &rawdata;
	for (i = 0; i < count; i++) {
		data->RawData->Data = calloc(256, 1);
		error = EncodePDUSMS(data->SMSMessage, data->RawData->Data, i, &data->RawData->Length);
		if (error == GE_NONE) error = SM_Functions(GOP_SaveSMS, data, state);
		free(data->RawData->Data);
		if (error != GE_NONE) break;
	}
	data->RawData = NULL;
	return error;
}

/***
 *** DECODING SMS
 ***/

static GSM_Error SMSStatus(unsigned char status, GSM_SMSMessage *SMS)
{
	if (status < 0x03) {
		strcpy(SMS->UserData[0].u.Text, _("Delivered"));
		switch (status) {
		case 0x00:
			dprintf("SM received by the SME");
			break;
		case 0x01:
			dprintf("SM forwarded by the SC to the SME but the SC is unable to confirm delivery");
			break;
		case 0x02:
			dprintf("SM replaced by the SC");
			break;
		}
		SMS->Length = strlen(_("Delivered"));

	} else if (status & 0x40) {
		strcpy(SMS->UserData[0].u.Text, _("Failed"));

		/* more detailed reason only for debug */

		if (status & 0x20) {
			dprintf("Temporary error, SC is not making any more transfer attempts\n");

			switch (status) {
			case 0x60:
				dprintf("Congestion");
				break;
			case 0x61:
				dprintf("SME busy");
				break;
			case 0x62:
				dprintf("No response from SME");
				break;
			case 0x63:
				dprintf("Service rejected");
				break;
			case 0x64:
				dprintf("Quality of service not aviable");
				break;
			case 0x65:
				dprintf("Error in SME");
				break;
			default:
				dprintf("Reserved/Specific to SC: %x", status);
				break;
			}

		} else {
			dprintf("Permanent error, SC is not making any more transfer attempts\n");
			switch (status) {
			case 0x40:
				dprintf("Remote procedure error");
				break;
			case 0x41:
				dprintf("Incompatibile destination");
				break;
			case 0x42:
				dprintf("Connection rejected by SME");
				break;
			case 0x43:
				dprintf("Not obtainable");
				break;
			case 0x44:
				dprintf("Quality of service not aviable");
				break;
			case 0x45:
				dprintf("No internetworking available");
				break;
			case 0x46:
				dprintf("SM Validity Period Expired");
				break;
			case 0x47:
				dprintf("SM deleted by originating SME");
				break;
			case 0x48:
				dprintf("SM Deleted by SC Administration");
				break;
			case 0x49:
				dprintf("SM does not exist");
				break;
			default:
				dprintf("Reserved/Specific to SC: %x", status);
				break;
			}
		}
		SMS->Length = strlen(_("Failed"));

	} else if (status & 0x20) {
		strcpy(SMS->UserData[0].u.Text, _("Pending"));

		/* more detailed reason only for debug */
		dprintf("Temporary error, SC still trying to transfer SM\n");
		switch (status) {
		case 0x20:
			dprintf("Congestion");
			break;
		case 0x21:
			dprintf("SME busy");
			break;
		case 0x22:
			dprintf("No response from SME");
			break;
		case 0x23:
			dprintf("Service rejected");
			break;
		case 0x24:
			dprintf("Quality of service not aviable");
			break;
		case 0x25:
			dprintf("Error in SME");
			break;
		default:
			dprintf("Reserved/Specific to SC: %x", status);
			break;
		}
		SMS->Length = strlen(_("Pending"));

	} else {
		strcpy(SMS->UserData[0].u.Text, _("Unknown"));

		/* more detailed reason only for debug */
		dprintf("Reserved/Specific to SC: %x", status);
		SMS->Length = strlen(_("Unknown"));
	}
	dprintf("\n");
	return GE_NONE;
}

static GSM_Error DecodeData(char *message, char *output, int length, int size, int udhlen, SMS_DataCodingScheme dcs)
{
	/* PDU SMS */
	if (llayout.IsUserDataCoded) {
		/* Unicode */
		if ((dcs.Type & 0x08) == 0x08) {
			dprintf("Unicode message\n");
			length = (length - udhlen)/2;
			DecodeUnicode(output, message, length);
		} else {
			/* 8bit SMS */
			if ((dcs.Type & 0xf4) == 0xf4) {
				dprintf("8bit message\n");
				memcpy(output, message, length);
			/* 7bit SMS */
			} else {
				dprintf("Default Alphabet\n");
				length = length - (udhlen * 8 + ((7-(udhlen%7))%7)) / 7;
				Unpack7BitCharacters((7-udhlen)%7, size, length, message, output);
				DecodeAscii(output, output, length);
			}
		}
	/* Text SMS */
	} else {
		strncpy(output, message, length);
	}
	dprintf("%s\n", output);
	return GE_NONE;
}

/* This function decodes UDH as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
*/
static GSM_Error DecodeUDH(char *message, GSM_SMSMessage *SMS)
{
	unsigned char length, pos, nr;

	SMS->UDH_Length = length = message[0] + 1;
	pos = 1;
	nr = 0;
	while (length > 1) {
		unsigned char udh_length;

		udh_length = message[pos+1];
		switch (message[pos]) {
		case 0x00: /* Concatenated short messages */
			dprintf("Concatenated messages\n");
			SMS->UDH[nr].Type = SMS_ConcatenatedMessages;
			SMS->UDH[nr].u.ConcatenatedShortMessage.ReferenceNumber = message[pos + 2];
			SMS->UDH[nr].u.ConcatenatedShortMessage.MaximumNumber   = message[pos + 3];
			SMS->UDH[nr].u.ConcatenatedShortMessage.CurrentNumber   = message[pos + 4];
			break;
		case 0x01: /* Special SMS Message Indication */
			switch (message[pos + 2] & 0x03) {
			case 0x00:
				dprintf("Voice Message\n");
				SMS->UDH[nr].Type = SMS_VoiceMessage;
				break;
			case 0x01:
				dprintf("Fax Message\n");
				SMS->UDH[nr].Type = SMS_FaxMessage;
				break;
			case 0x02:
				dprintf("Email Message\n");
				SMS->UDH[nr].Type = SMS_EmailMessage;
				break;
			default:
				dprintf("Unknown\n");
				SMS->UDH[nr].Type = SMS_UnknownUDH;
				break;
			}
			SMS->UDH[nr].u.SpecialSMSMessageIndication.Store = (message[pos + 2] & 0x80) >> 7;
			SMS->UDH[nr].u.SpecialSMSMessageIndication.MessageCount = message[pos + 3];
			break;
		case 0x04: /* Application port addression scheme, 8 bit address */
			break;
		case 0x05: /* Application port addression scheme, 16 bit address */
			switch (((0x00ff & message[pos + 2]) << 8) | (0x00ff & message[pos + 3])) {
			case 0x1581:
				dprintf("Ringtone\n");
				SMS->UDH[nr].Type = SMS_Ringtone;
				break;
			case 0x1582:
				dprintf("Operator Logo\n");
				SMS->UDH[nr].Type = SMS_OpLogo;
				break;
			case 0x1583:
				dprintf("Caller Icon\n");
				SMS->UDH[nr].Type = SMS_CallerIDLogo;
				break;
			case 0x158a:
				dprintf("Multipart Message\n");
				SMS->UDH[nr].Type = SMS_MultipartMessage;
				break;
			case 0x23f4:
				dprintf("WAP vCard\n");
				SMS->UDH[nr].Type = SMS_WAPvCard;
				break;
			case 0x23f5:
				dprintf("WAP vCalendar\n");
				SMS->UDH[nr].Type = SMS_WAPvCalendar;
				break;
			case 0x23f6:
				dprintf("WAP vCardSecure\n");
				SMS->UDH[nr].Type = SMS_WAPvCardSecure;
				break;
			case 0x23f7:
				dprintf("WAP vCalendarSecure\n");
				SMS->UDH[nr].Type = SMS_WAPvCalendarSecure;
				break;
			default:
				dprintf("Unknown\n");
				SMS->UDH[nr].Type = SMS_UnknownUDH;
				break;
			}
			break;
		case 0x06: /* SMSC Control Parameters */
			break;
		case 0x07: /* UDH Source Indicator */
			break;
		default:
			dprintf("Not supported UDH\n");
			break;
		}
		length -= (udh_length + 2);
		pos += (udh_length + 2);
		nr++;
	}
	SMS->UDH_No = nr;

	return GE_NONE;
}

static GSM_Error DecodeSMSHeader(unsigned char *message, GSM_SMSMessage *SMS)
{
	/* Short Message Type */
	SMS->Type = message[layout.Type - header_offset];
	switch (SMS->Type) {
	case SMS_Deliver:
		llayout = layout.Deliver;
		dprintf("Mobile Terminated message:\n");
		break;
	case SMS_Delivery_Report:
		llayout = layout.DeliveryReport;
		dprintf("Delivery Report:\n");
		break;
	case SMS_Submit:
		llayout = layout.Submit;
		dprintf("Mobile Originated (stored) message:\n");
		break;
	case SMS_SubmitSent:
		llayout = layout.SubmitSent;
		dprintf("Mobile Originated (sent) message:\n");
		break;
	case SMS_Picture:
		llayout = layout.Picture;
		dprintf("Picture Message:\n");
		break;
	case SMS_PictureTemplate:
		llayout = layout.PictureTemplate;
		dprintf("Picture Template:\n");
		break;
	case SMS_TextTemplate:
		llayout = layout.TextTemplate;
		dprintf("Text Template:\n");
		break;
	default:
		dprintf("Not supported message type: %d\n", SMS->Type);
		return GE_NOTSUPPORTED;
	}

	if (!llayout.IsSupported) return GE_NOTSUPPORTED;

	change_offsets(message);

	/* Delivery date */
	if (llayout.Time > -1) {
		UnpackDateTime(message + llayout.Time, &(SMS->SMSCTime));
/* Required for VC */
#ifdef DEBUG
		dprintf("\tDelivery date: %s\n", PrintDateTime(message + llayout.Time));
#endif
	}

	/* Remote number */
	if (llayout.RemoteNumber > -1) {
		if (llayout.IsRemoteNumberCoded) {
			if (message[llayout.RemoteNumber] == 0x00) llayout.MessageCenter -= 4;
				/* FIXME Is this an ugly hack or correct? */
				/* at least it works with 6210, 6510 and 6110 with the message I tested */
			message[llayout.RemoteNumber] = (message[llayout.RemoteNumber] + 1) / 2 + 1;
			strcpy(SMS->RemoteNumber.number, GetBCDNumber(message + llayout.RemoteNumber));
			dprintf("\tRemote number (recipient or sender): %s\n", SMS->RemoteNumber.number);
		} else {
			/* SMS struct should be zeroed for now, so there's no
			 * need to add an extra '\0' at the end of the string */
			strncpy(SMS->RemoteNumber.number,
				message + 1 + llayout.RemoteNumber,
				message[llayout.RemoteNumber] < GSM_MAX_SMS_CENTER_LENGTH ? message[llayout.RemoteNumber] : GSM_MAX_SMS_CENTER_LENGTH);
		}
	}

	/* Remote number */
	if (llayout.RemoteNumber > -1) {
		if (llayout.IsRemoteNumberCoded) {
			if (message[llayout.RemoteNumber] == 0x00) llayout.MessageCenter -= 4;
				/* FIXME Is this an ugly hack or correct? */
				/* at least it works with 6210, 6510 and 6110 with the message I tested */
			message[llayout.RemoteNumber] = (message[llayout.RemoteNumber] + 1) / 2 + 1;
			strcpy(SMS->RemoteNumber.number, GetBCDNumber(message + llayout.RemoteNumber));
			dprintf("\tRemote number (recipient or sender): %s\n", SMS->RemoteNumber.number);
		} else {
			/* SMS struct should be zeroed for now, so there's no
			 * need to add an extra '\0' at the end of the string */
			strncpy(SMS->RemoteNumber.number,
				message + 1 + llayout.RemoteNumber,
				message[llayout.RemoteNumber] < GSM_MAX_SMS_CENTER_LENGTH ? message[llayout.RemoteNumber] : GSM_MAX_SMS_CENTER_LENGTH);
		}
	}

	/* Short Message Center */
	if (llayout.MessageCenter > -1) {
		if (llayout.IsMessageCenterCoded) {
			strcpy(SMS->MessageCenter.Number, GetBCDNumber(message +  llayout.MessageCenter));
			dprintf("\tSMS center number: %s\n", SMS->MessageCenter.Number);
			SMS->ReplyViaSameSMSC = false;
			if (SMS->RemoteNumber.number[0] == 0 && (message[llayout.ReplyViaSameSMSC] & 0x80)) {
				SMS->ReplyViaSameSMSC = true;
			}
		} else {
			/* SMS struct should be zeroed for now, so there's no
			 * need to add an extra '\0' at the end of the string */
			strncpy(SMS->MessageCenter.Number,
				message + 1 + llayout.MessageCenter,
				message[llayout.MessageCenter] < GSM_MAX_SMS_CENTER_LENGTH ? message[llayout.MessageCenter] : GSM_MAX_SMS_CENTER_LENGTH);
			SMS->ReplyViaSameSMSC = false;
		}
	}


	/* Sending time */
	if (llayout.SMSCTime > -1) {
		UnpackDateTime(message + llayout.SMSCTime, &(SMS->Time));
/* Required for VC */
#ifdef DEBUG
		dprintf("\tDate: %s\n", PrintDateTime(message + llayout.SMSCTime));
#endif
	}

	/* Message length */
	if (llayout.Length > -1)
		SMS->Length = message[llayout.Length];

	/* Data Coding Scheme */
	if (llayout.DataCodingScheme > -1)
		SMS->DCS.Type = message[llayout.DataCodingScheme];

	/* User Data Header */
	if (llayout.UserDataHeader > -1)
		if (message[llayout.UserDataHeader] & 0x40) { /* UDH header available */
			dprintf("UDH found\n");
			DecodeUDH(message + llayout.UserData, SMS);
		}

	return GE_NONE;
}

/* This function decodes SMS as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9
*/
static GSM_Error DecodePDUSMS(unsigned char *message, GSM_SMSMessage *SMS, int MessageLength)
{
	int size;
	GSM_Error error;

	error = DecodeSMSHeader(message, SMS);
	if (error != GE_NONE) return error;
	switch (SMS->Type) {
	case SMS_Delivery_Report:
		if (llayout.UserData > -1) SMSStatus(message[llayout.UserData], SMS);
		break;
	case SMS_PictureTemplate:	
	case SMS_Picture:
		/* This is incredible. Nokia violates it's own format in 6210 */
		/* Indicate that it is Multipart Message. Remove it if not needed */
		if ((message[llayout.UserData] == 0x48) && (message[llayout.UserData + 1] == 0x1c)) {
			SMS->UDH_No = 1;
			SMS->UDH[0].Type = SMS_MultipartMessage;
			/* First part is a Picture */
			SMS->UserData[0].Type = SMS_BitmapData;
			GSM_ReadSMSBitmap(SMS_Picture, message + llayout.UserData, NULL, &SMS->UserData[0].u.Bitmap);
			GSM_PrintBitmap(&SMS->UserData[0].u.Bitmap);
			size = MessageLength - llayout.UserData - 4 - SMS->UserData[0].u.Bitmap.size;
			SMS->Length = message[llayout.UserData + 4 + SMS->UserData[0].u.Bitmap.size];
			/* Second part is a text */
			SMS->UserData[1].Type = SMS_PlainText;
			DecodeData(message + llayout.UserData + 5 + SMS->UserData[0].u.Bitmap.size,
				   (unsigned char *)&(SMS->UserData[1].u.Text),
				   SMS->Length, size, 0, SMS->DCS);
			SMS->UserData[1].u.Text[SMS->Length] = 0;
		} else {
			/* First part is a text */
			SMS->UDH_No = 1;
			SMS->UserData[1].Type = SMS_PlainText;
			size = MessageLength - llayout.UserData - 4 - (72 * 28 / 8);
			SMS->Length = message[llayout.UserData];
			dprintf("SMS length: %i, size: %i \n", SMS->Length, size);
			if (size > SMS->Length) {
				DecodeData(message + llayout.UserData + 1,
					   (unsigned char *)&(SMS->UserData[1].u.Text),
					   SMS->Length, size, 0, SMS->DCS);
				SMS->UserData[1].u.Text[SMS->Length] = 0;
			}

			SMS->UDH[0].Type = SMS_MultipartMessage;
			/* Second part is a Picture */
			SMS->UserData[0].Type = SMS_BitmapData;
			GSM_ReadSMSBitmap(SMS_Picture, message + llayout.UserData + size, NULL, &SMS->UserData[0].u.Bitmap);
			GSM_PrintBitmap(&SMS->UserData[0].u.Bitmap);
		}
		break;
	default:
		size = MessageLength -
			llayout.UserData + 1 - /* Header Length */
			SMS->UDH_Length;       /* UDH Length */
		dprintf("length: %i\n", SMS->Length);
		DecodeData(message + llayout.UserData + SMS->UDH_Length,
			   (unsigned char *)&(SMS->UserData[0].u.Text),
			   SMS->Length, size, SMS->UDH_Length, SMS->DCS);
		/* Just in case */
		SMS->UserData[0].u.Text[SMS->Length] = 0;
		break;
	}

	return GE_NONE;
}

GSM_Error ParseSMS(GSM_Data *data, int offset)
{
	header_offset = offset;
	if (!data->RawData || !data->SMSMessage) return GE_INTERNALERROR;
	return DecodePDUSMS(data->RawData->Data, data->SMSMessage, data->RawData->Length);
}

GSM_Error RequestSMS(GSM_Data *data, GSM_Statemachine *state)
{
	return SM_Functions(GOP_GetSMS, data, state);
}

GSM_Error GetSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error;
	GSM_RawData rawdata;

	memset(&rawdata, 0, sizeof(GSM_RawData));
	data->RawData = &rawdata;
	error = RequestSMS(data, state);
	if (error != GE_NONE) goto cleanup;
	error = ParseSMS(data, layout.ReadHeader);
cleanup:
	if (data->RawData->Data) free(data->RawData->Data);
	return error;
}

/* Find the fist unread message in given folder statting from the *last position
 * As a help we have the array of the sorted read locations in 'location' with
 * no_loc entries. We can easily skip these locations.
 */
static GSM_Error FindUnreadSMS(GSM_Data *data, GSM_Statemachine *state, int *last, const unsigned int *locations, unsigned int no_loc)
{
	GSM_Error error;
	int i, index = 0;

	dprintf("Starting at %d\n", *last);
	for (i = *last;; i++) {
		/* Skip all read messages */
		while (index < no_loc && locations[index] < i) index++;
		if (locations[index] == i) continue;

		dprintf("Reading: %d\n", i);
		memset(data->SMSMessage, 0, sizeof(GSM_SMSMessage));
		data->SMSMessage->Number = i;
		data->SMSMessage->MemoryType = GMT_IN;
		error = SM_Functions(GOP_GetSMS, data, state);
		dprintf("ERROR: %d (%s)\n", error, print_error(error));
		if (error == GE_EMPTYSMSLOCATION) continue;
		if (error != GE_NONE) return error;
		if (data->SMSMessage->Status == SMS_Unread) {
			dprintf("Got unread %d\n", i);
			*last = i + 1;
			return GE_NONE;
		}
	}
	return GE_NONE;
}

static void sort(unsigned int *table, unsigned int number)
{
	int i, j;

	for (i = 0; i < number; i++) {
		int min = table[i], no = i;
		for (j = i; j < number; j++) {
			if (table[j] < min) {
				min = table[j];
				no = j;
			}
		}
		if (no > i) {
			int aux;

			aux = table[i];
			table[i] = table[no];
			table[no] = aux;
		}
	}
}

			
static GSM_Error FreeDeletedMessages(GSM_Data *data, int folder)
{
	int i, j;
	
	if (!data->SMSStatus) return GE_INTERNALERROR;

	for (i = 0; i < data->FolderStats[folder]->Used; i++) {		/* for all previously found locations */
		if (data->MessagesList[i][folder]->Type == SMS_ToBeRemoved) {	/* previously deleted and read message */
			dprintf("Found deleted message, which will now be freed! %i , %i\n", 
					i, data->MessagesList[i][folder]->Location);
			data->FolderStats[folder]->Used--;
			for (j = i; j < data->FolderStats[folder]->Used; j++) {
				memcpy(data->MessagesList[j][folder], data->MessagesList[j + 1][folder],
						sizeof(SMS_MessagesList));
			}
			i--;
		}
	}
	return GE_NONE;
}

GSM_Error GetReadMessages(GSM_Data *data, SMS_Folder folder)
{
	int i, j, found;
	
	if (!data->MessagesList || !data->FolderStats) return GE_INTERNALERROR;
	
	for (i = 0; i < folder.number; i++) {		/* cycle through all messages in phone */
		found = 0;
		for (j = 0; j < data->FolderStats[folder.FolderID]->Used; j++) {		/* and compare them to those alread in list */
			if (data->SMSFolder->locations[i] == data->MessagesList[j][folder.FolderID]->Location) found = 1;
		}
		if (!found) {
			dprintf("Found new (read) message. Will store it at #%i!\n", data->FolderStats[folder.FolderID]->Used);
			dprintf("%i\n", folder.locations[i]);
			data->MessagesList[data->FolderStats[folder.FolderID]->Used][folder.FolderID]->Location = folder.locations[i];
			data->MessagesList[data->FolderStats[folder.FolderID]->Used][folder.FolderID]->Type = SMS_New;
			data->FolderStats[folder.FolderID]->Used++;
			data->FolderStats[folder.FolderID]->Changed++;
			data->SMSStatus->Changed++;
		}
	}
	return GE_NONE;
}
	
GSM_Error GetUnreadMessages(GSM_Data *data, GSM_Statemachine *state, SMS_Folder folder)
{
	GSM_Error error;
	GSM_SMSMessage SMSMessage;
	GSM_RawData RawData;
	int dummy, i, last = 1, current_folder, previous_unread = 0;
	
	data->SMSMessage = &SMSMessage;
	data->RawData = &RawData;
		
	current_folder = 0;
	
	dprintf("GetUnreadMessages: Looking for unread (%d)\n", data->SMSStatus->Unread);

	for (i = 0; i < data->FolderStats[current_folder]->Used; i++) {		/* cycle through all saved position */
		if ((data->MessagesList[i][current_folder]->Type == SMS_NotRead) ||
				(data->MessagesList[i][current_folder]->Type == SMS_NotReadHandled)) {	/* add already found new SMS to SMSFolder */
			folder.locations[folder.number] = data->MessagesList[i][current_folder]->Location;
			folder.number++;
			previous_unread++;
		}
	}
	dummy = data->SMSStatus->Unread - previous_unread;
	
	if (dummy < 1) {
		return GE_NONE;
	}
	dprintf("GetUnreadMessages: sorting... previous unread: %i\n", previous_unread);
	sort((int *)folder.locations, folder.number); 
	
	dprintf("GetUnreadMessages: sorting finished! Trying to get %i unread mails\n",dummy);
	for (i = 0; i < dummy; i++) {
		error = FindUnreadSMS(data, state, &last, folder.locations, folder.number);
		if (error == GE_NONE) {
			dprintf("Found new (unread) message!\n");
			data->MessagesList[data->FolderStats[current_folder]->Used][current_folder]->Location = data->SMSMessage->Number;
			data->MessagesList[data->FolderStats[current_folder]->Used][current_folder]->Type = SMS_NotRead;
			data->FolderStats[current_folder]->Used++;
			data->FolderStats[current_folder]->Changed++;
			data->SMSStatus->Changed++;
		} else return error;
	}
	return GE_NONE;
}
	
GSM_Error GetDeletedMessages(GSM_Data *data, SMS_Folder folder)
{
	int i, j, found = 0;
	
	for (i = 0; i < data->FolderStats[folder.FolderID]->Used; i++) {		/* for all previously found locations in folder */
		found = 0;
		
		for (j = 0; j < folder.number; j++) {	/* see if there is a corresponding message in phone */
			if (data->MessagesList[i][folder.FolderID]->Location == folder.locations[j]) found = 1;
		}
		if ((found == 0) && (data->MessagesList[i][folder.FolderID]->Type == SMS_Old)) {	/* we have found a deleted message */
			dprintf("found a deleted message!!!! i: %i, loc: %i, MT: %i \n", 
					i, data->MessagesList[i][folder.FolderID]->Location, folder.FolderID);

			data->MessagesList[i][folder.FolderID]->Type = SMS_Deleted;
			data->SMSStatus->Changed++;
			data->FolderStats[folder.FolderID]->Changed++;
		}
	}
	return GE_NONE;
}

GSM_Error VerifyMessagesStatus(GSM_Data *data, SMS_Folder folder)
{
	int i, j, found = 0;
	
	for (i = 0; i < data->FolderStats[folder.FolderID]->Used; i++) {		/* Cycle through all messages we know of */
		found = 0;
		if ((data->MessagesList[i][folder.FolderID]->Type == SMS_NotRead) ||	/* if it is a unread one, i.e. not in folderstatus */
				(data->MessagesList[i][folder.FolderID]->Type == SMS_NotReadHandled)) {
			for (j = 0; j < folder.number; j++) {
				if (data->MessagesList[i][folder.FolderID]->Location == folder.locations[j]) {
					/* We have a found a formerly unread message which has been read in the meantime */
					dprintf("Found a formerly unread message which has been read in the meantime: loc: %i\n", 
							data->MessagesList[i][folder.FolderID]->Location);
					data->MessagesList[i][folder.FolderID]->Type = SMS_Changed;
					data->SMSStatus->Changed++;
					data->FolderStats[folder.FolderID]->Changed++;
				}
			}
		}
	}
	return GE_NONE;
}


GSM_Error GetFolderChanges(GSM_Data *data, GSM_Statemachine *state, int has_folders)
{
	GSM_Error error;
	SMS_Folder tmp_folder, SMSFolder;
	SMS_FolderList tmp_list, SMSFolderList;
	int i, previous_unread, previous_total;
		
	previous_total = data->SMSStatus->Number;
	previous_unread = data->SMSStatus->Unread;
	dprintf("GetFolderChanges: Old status: %d %d\n", data->SMSStatus->Number, data->SMSStatus->Unread);
	
	error = SM_Functions(GOP_GetSMSStatus, data, state);	/* Check overall SMS Status */
	if (error != GE_NONE) return error;
	dprintf("GetFolderChanges: Status: %d %d\n", data->SMSStatus->Number, data->SMSStatus->Unread);
	
	if (!has_folders) {
		if ((previous_total == data->SMSStatus->Number) && (previous_unread == data->SMSStatus->Unread))
			data->SMSStatus->Changed = 0;
		else 
			data->SMSStatus->Changed = 1;
		return GE_NONE;
	}
	
	data->SMSFolderList = &SMSFolderList;
	if ((error = SM_Functions(GOP_GetSMSFolders, data, state)) != GE_NONE) return error;
	
	data->SMSStatus->NumberOfFolders = data->SMSFolderList->number;
	memcpy(&tmp_list, data->SMSFolderList, sizeof(SMS_FolderList));	/* We need it because data->SMSFolderlist can get garbled */
	
	for (i = 0; i < data->SMSStatus->NumberOfFolders; i++) {
		dprintf("GetFolderChanges: Freeing deleted messages for folder #%i\n", i);
		if ((error = FreeDeletedMessages(data, i)) != GE_NONE) return error;
		
		data->SMSFolder = &SMSFolder;
		data->SMSFolder->FolderID = tmp_list.FolderID[i];
		dprintf("GetFolderChanges: Getting folder status for folder #%i\n", data->SMSFolder->FolderID);
		if ((error = SM_Functions(GOP_GetSMSFolderStatus, data, state)) != GE_NONE) return error;
		memcpy(&tmp_folder, data->SMSFolder, sizeof(SMS_Folder));	/* We need it because data->SMSFolder can get garbled */
		tmp_folder.FolderID = i;	/* so we don't need to do a modulo 8 each time */
		
		dprintf("GetFolderChanges: Reading unread messages for folder #%i\n", i);	/* Only for INBOX */
		if (i == 0) if ((error = GetUnreadMessages(data, state, tmp_folder)) != GE_NONE) return error; 
		
		dprintf("GetFolderChanges: Reading read messages (%i) for folder #%i\n", data->SMSFolder->number, i);
		if ((error = GetReadMessages(data, tmp_folder)) != GE_NONE) return error;
		
		dprintf("GetFolderChanges: Getting deleted messages for folder #%i\n", i);
		if ((error = GetDeletedMessages(data, tmp_folder)) != GE_NONE) return error;
		
		dprintf("GetFolderChanges: Verifying messages for folder #%i\n", i);
		if ((error = VerifyMessagesStatus(data, tmp_folder)) != GE_NONE) return error;
	}
	return GE_NONE;
}

/**
 * DefaultSMS - fills in SMS structure with the default values
 * @SMS: pointer to a structure we need to fill in
 *
 * Default settings:
 *  - no delivery report
 *  - no Class Message
 *  - no compression
 *  - 7 bit data
 *  - SMSC no. 1
 *  - message validity for 3 days
 *  - unset User Data Header Indicator
 */
void DefaultSMS(GSM_SMSMessage *SMS)
{
	memset(&SMS, 0, sizeof(GSM_SMSMessage));

	SMS->Type = SMS_Deliver;
	SMS->DCS.Type = SMS_GeneralDataCoding;
	SMS->DCS.u.General.Compressed = false;
	SMS->DCS.u.General.Alphabet = SMS_DefaultAlphabet;
	SMS->DCS.u.General.Class = 0;
	SMS->MessageCenter.No = 1;
	SMS->Validity.VPF = SMS_RelativeFormat;
	SMS->Validity.u.Relative = 4320; /* 4320 minutes == 72 hours */
	SMS->UDH_No = 0;
	SMS->Report = false;
	SMS->Status = SMS_Unsent;
}

void DefaultSubmitSMS(GSM_SMSMessage *SMS)
{
	DefaultSMS(SMS);
	SMS->Type = SMS_Submit;
}

void DefaultDeliverSMS(GSM_SMSMessage *SMS)
{
	DefaultSMS(SMS);
	SMS->Type = SMS_Deliver;
}
