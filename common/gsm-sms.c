/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Library for parsing and creating Short Messages (SMS).

*/

#include <stdlib.h>
#include <string.h>

#include "gsm-common.h"
#include "gsm-encoding.h"
#include "gsm-bitmaps.h"

SMSMessage_PhoneLayout layout;
static SMSMessage_Layout llayout;

struct udh_data {
	unsigned int length;
	char *header;
};

/* User data headers */
static struct udh_data headers[] = {
	{ 0x00, "" },
	{ 0x05, "\x00\x03\x01\x00\x00" },     /* Concatenated messages */
	{ 0x06, "\x05\x04\x15\x82\x00\x00" }, /* Operator logos */
	{ 0x06, "\x05\x04\x15\x83\x00\x00" }, /* Caller logos */
	{ 0x06, "\x05\x04\x15\x81\x00\x00" }, /* Ringtones */
	{ 0x04, "\x03\x01\x00\x00" },         /* Voice Messages */
	{ 0x04, "\x03\x01\x01\x00" },         /* Fax Messages */
	{ 0x04, "\x03\x01\x02\x00" },         /* Email Messages */
	{ 0x06, "\x05\x04\x23\xf4\x00\x00" }, /* Business Card */
	{ 0x00, "" }
};


/***
 *** Util functions
 ***/

/* This function implements packing of numbers (SMS Center number and
   destination number) for SMS sending function. */
static int SemiOctetPack(char *Number, unsigned char *Output, SMS_NumberType type)
{
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

/***
 *** ENCODING SMS
 ***/

static GSM_Error EncodeData(GSM_SMSMessage *SMS, char *dcs, char *message)
{
	SMS_AlphabetType al;
	unsigned short length = strlen(SMS->MessageText);

	switch (SMS->DCS.Type) {
	case SMS_GeneralDataCoding:
		switch (SMS->DCS.u.General.Class) {
		case 1: dcs[0] |= 0xf0; break;
		case 2: dcs[0] |= 0xf1; break;
		case 3: dcs[0] |= 0xf2; break;
		case 4: dcs[0] |= 0xf3; break;
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
	switch (al) {
	case SMS_DefaultAlphabet:
		Pack7BitCharacters((7 - (SMS->UDH_Length % 7)) % 7, SMS->MessageText, message);
		SMS->Length = 8 * SMS->UDH_Length + (7 - (SMS->UDH_Length % 7)) % 7 + length;
		break;
	case SMS_8bit:
		dcs[0] |= 0xf4;
		memcpy(message, SMS->MessageText + 1, SMS->MessageText[0]);
		SMS->Length = SMS->UDH_Length + SMS->MessageText[0];
		break;
	case SMS_UCS2:
		dcs[0] |= 0x08;
		EncodeUnicode(message, SMS->MessageText, length);
		SMS->Length = length;
		break;
	default:
		return GE_SMSWRONGFORMAT;
	}
	return GE_NONE;
}

/* This function encodes the UserDataHeader as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
*/
static GSM_Error EncodeUDH(SMS_UDHInfo UDHi, char *UDH)
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
		if (SMS->ReportStatus) frame[llayout.Report] |= 0x20;
	}

	/* Validity Period Format: mask - 0x00, 0x10, 0x08, 0x18 */
	if (llayout.Validity > -1) {
		frame[llayout.Validity] |= ((SMS->Validity.VPF & 0x03) << 3);
	}

	/* Reject Duplicates */
	if (llayout.RejectDuplicates > -1) {
		if (SMS->RejectDuplicates) frame[llayout.RejectDuplicates] |= 0x04;
	}

	/* Message Type is already set */

	/* Message Reference */
	/* Can we set this? */

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
	switch (SMS->Validity.VPF) {
	case SMS_EnhancedFormat:
		break;
	case SMS_RelativeFormat:
		break;
	case SMS_AbsoluteFormat:
		break;
	default:
		break;
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
int EncodePDUSMS(GSM_SMSMessage *SMS, char *message)
{
	GSM_Error error = GE_NONE;
	int i;

	switch (SMS->Type) {
	case SMS_Submit:
		llayout = layout.Submit;
		dprintf("Sending SMS to %s via message center %s\n", SMS->RemoteNumber.number, SMS->MessageCenter.Number);
		break;
	case SMS_Deliver:
		llayout = layout.Deliver;
		dprintf("Saving SMS to Inbox\n");
		break;
	case SMS_Delivery_Report:
	case SMS_Picture:
	default:
		dprintf("Not supported message type: %d\n", SMS->Type);
		return GE_NOTSUPPORTED;
	}

	/* SMSC number */
	if (llayout.MessageCenter > -1) {
		dprintf("%d %s\n", SMS->MessageCenter.Type, SMS->MessageCenter.Number);
		message[llayout.MessageCenter] = SemiOctetPack(SMS->MessageCenter.Number, message + llayout.MessageCenter + 1, SMS->MessageCenter.Type);
		if (message[llayout.MessageCenter] % 2) message[llayout.MessageCenter]++;
		message[llayout.MessageCenter] = message[llayout.MessageCenter] / 2 + 1;
	}

	/* Common Header */
	error = EncodeSMSHeader(SMS, message);
	if (error != GE_NONE) return error;

	/* User Data Header - if present */
//	for (i = 0; i < SMS->UDH_No; i++) {
//		error = EncodeUDH(SMS->UDH[i], message + 24);
//		if (error != GE_NONE) return error;
//	}
	SMS->UDH_Length = 0;

	/* User Data */
	EncodeData(SMS, message + llayout.DataCodingScheme, message + llayout.UserData + SMS->UDH_Length);
	message[llayout.Length] = SMS->Length;
	return SMS->Length + llayout.UserData - 1;
}

/* This function does simple SMS encoding - no PDU coding */
GSM_Error EncodeTextSMS()
{
	return GE_NONE;
}

/***
 *** DECODING SMS
 ***/

static GSM_Error SMSStatus(unsigned char status, GSM_SMSMessage *SMS)
{
	if (status < 0x03) {
		strcpy(SMS->MessageText, _("Delivered"));
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

		strcpy(SMS->MessageText, _("Failed"));

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
		strcpy(SMS->MessageText, _("Pending"));

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
		strcpy(SMS->MessageText, _("Unknown"));

		/* more detailed reason only for debug */
		dprintf("Reserved/Specific to SC: %x", status);
		SMS->Length = strlen(_("Unknown"));
	}
	dprintf("\n");
	return GE_NONE;
}

static GSM_Error DecodeData(char *message, char *output, int length, int size, int udhlen, SMS_DataCodingScheme dcs)
{
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
		case 0x00: // Concatenated short messages
			dprintf("Concatenated messages\n");
			SMS->UDH[nr].Type = SMS_ConcatenatedMessages;
			SMS->UDH[nr].u.ConcatenatedShortMessage.ReferenceNumber = message[pos + 2];
			SMS->UDH[nr].u.ConcatenatedShortMessage.MaximumNumber   = message[pos + 3];
			SMS->UDH[nr].u.ConcatenatedShortMessage.CurrentNumber   = message[pos + 4];
			break;
		case 0x01: // Special SMS Message Indication
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
		case 0x04: // Application port addression scheme, 8 bit address
			break;
		case 0x05: // Application port addression scheme, 16 bit address
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
			case 0x23f4:
				dprintf("Business Card\n");
				SMS->UDH[nr].Type = SMS_BusinessCard;
				break;
			default:
				dprintf("Unknown\n");
				SMS->UDH[nr].Type = SMS_UnknownUDH;
				break;
			}
			break;
		case 0x06: // SMSC Control Parameters
			break;
		case 0x07: // UDH Source Indicator
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
	SMS->Type = message[layout.Type];
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
		dprintf("Mobile Originated message:\n");
		break;
	case SMS_Picture:
		llayout = layout.Picture;
		dprintf("Picture Message:\n");
		break;
	default:
		dprintf("Not supported message type: %d\n", SMS->Type);
		return GE_NOTSUPPORTED;
	}

	if (!llayout.IsSupported) return GE_NOTSUPPORTED;

	/* Delivery date */
	if (llayout.Time > -1) {
		UnpackDateTime(message + llayout.Time, &(SMS->SMSCTime));
		dprintf("\tDelivery date: %s\n", PrintDateTime(message + llayout.Time));
	}

	/* Short Message location in memory */
	if (llayout.Number > -1) {
		SMS->Number = message[llayout.Number];
		dprintf("\tLocation: %d\n", SMS->Number);
	}

	/* Short Message Center */
	if (llayout.MessageCenter > -1) {
		strcpy(SMS->MessageCenter.Number, GetBCDNumber(message + llayout.MessageCenter));
		dprintf("\tSMS center number: %s\n", SMS->MessageCenter.Number);
		SMS->ReplyViaSameSMSC = false;
		if (SMS->RemoteNumber.number[0] == 0 && (message[llayout.ReplyViaSameSMSC] & 0x80)) {
			SMS->ReplyViaSameSMSC = true;
		}
	}

	/* Remote number */
	if (llayout.RemoteNumber > -1) {
		message[llayout.RemoteNumber] = ((message[llayout.RemoteNumber])+1)/2+1;
		strcpy(SMS->RemoteNumber.number, GetBCDNumber(message + llayout.RemoteNumber));
		dprintf("\tRemote number (recipient or sender): %s\n", SMS->RemoteNumber.number);
	}
	
	/* Sending time */
	if (llayout.SMSCTime > -1) {
		UnpackDateTime(message + llayout.SMSCTime, &(SMS->Time));
		dprintf("\tDate: %s\n", PrintDateTime(message + llayout.SMSCTime));
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
GSM_Error DecodePDUSMS(unsigned char *message, GSM_SMSMessage *SMS, int MessageLength)
{
	int size;
	GSM_Bitmap bitmap;
	GSM_Error error;

	error = DecodeSMSHeader(message, SMS);
	if (error != GE_NONE) return error;
	switch (SMS->Type) {
	case SMS_Delivery_Report:
		if (llayout.UserData > -1) SMSStatus(message[llayout.UserData], SMS);
		break;
	case SMS_Picture:
		GSM_ReadSMSBitmap(SMS_Picture, message + llayout.UserData, NULL, &bitmap);
		GSM_PrintBitmap(&bitmap);
		size = MessageLength - llayout.UserData - 4 - bitmap.size;
		SMS->Length = message[llayout.UserData + 4 + bitmap.size];
		DecodeData(message + llayout.UserData + 5 + bitmap.size,
			   (unsigned char *)&(SMS->MessageText),
			   SMS->Length, size, 0, SMS->DCS);
		SMS->MessageText[SMS->Length] = 0;
		break;
	default:
		size = MessageLength -
			llayout.UserData + 1 - /* Header Length */
			SMS->UDH_Length;       /* UDH Length */
		DecodeData(message + llayout.UserData + SMS->UDH_Length,
			   (unsigned char *)&(SMS->MessageText),
			   SMS->Length, size, SMS->UDH_Length, SMS->DCS);
		/* Just in case */
		SMS->MessageText[SMS->Length] = 0;
		break;
	}

	return GE_NONE;
}

/* This function does simple SMS decoding - no PDU coding */
GSM_Error DecodeTextSMS(unsigned char *message, GSM_SMSMessage *SMS)
{
	return GE_NONE;
}
