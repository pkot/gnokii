/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Library for parsing and creating Short Messages (SMS).

  $Log$
  Revision 1.9  2001-11-18 00:54:32  pkot
  Bugfixes. I18n of the user responses. UDH support in libsms. Business Card UDH Type

  Revision 1.8  2001/11/17 20:19:29  pkot
  smslib cleanups, fixes and debugging

  Revision 1.7  2001/11/15 12:15:04  pkot
  smslib updates. begin work on sms in 6100 series

  Revision 1.6  2001/11/14 18:21:19  pkot
  Fixing some problems with UDH and Unicode, but still doesn't work yet :-(

  Revision 1.5  2001/11/14 14:26:18  pkot
  Changed offset of DCS field to the right value in 6210/7110

  Revision 1.4  2001/11/14 11:26:18  pkot
  Getting SMS in 6210/7110 does finally work in some cases :)

  Revision 1.3  2001/11/13 16:12:20  pkot
  Preparing libsms to get to work. 6210/7110 SMS and SMS Folder updates

  Revision 1.2  2001/11/09 14:25:04  pkot
  DEBUG cleanups

  Revision 1.1  2001/11/08 16:23:21  pkot
  New version of libsms. Not functional yet, but it reasonably stable API.

  Revision 1.1  2001/07/09 23:06:26  pkot
  Moved sms.* files from my hard disk to CVS

*/

#include <stdlib.h>

#include "gsm-common.h"
#include "gsm-encoding.h"

struct udh_data {
	unsigned int length;
	char *header;
};

/* Number of header specific octets in SMS header (before data) */
static unsigned short DataOffset[] = {
	4, /* SMS Deliver */
	3, /* SMS Deliver Report */
	5, /* SMS Submit */
	3, /* SMS Submit Report */
	3, /* SMS Command */
	3  /* SMS Status Report */
};

/* User data headers */
static struct udh_data headers[] = {
	{ 0x00, "" },
	{ 0x05, "\x00\x03\x01\x00\x00" }, /* Concatenated messages */
	{ 0x06, "\x05\x04\x15\x82\x00\x00" }, /* Operator logos */
	{ 0x06, "\x05\x04\x15\x83\x00\x00" }, /* Caller logos */
	{ 0x06, "\x05\x04\x15\x81\x00\x00" }, /* Ringtones */
	{ 0x04, "\x03\x01\x00\x00" }, /* Voice Messages */
	{ 0x04, "\x03\x01\x01\x00" }, /* Fax Messages */
	{ 0x04, "\x03\x01\x02\x00" }, /* Email Messages */
	{ 0x06, "\x05\x04\x23\xf4\x00\x00" }, /* Business Card */
	{ 0x00, "" }
};


/***
 *** Util functions
 ***/

static char *GetBCDNumber(u8 *Number)
{
        static char Buffer[20] = "";
        int length = Number[0]; /* This is the length of BCD coded number */
        int count, Digit;

        switch (Number[1]) {
	case 0xd0:
		Unpack7BitCharacters(0, length, length, Number+2, Buffer);
		Buffer[length] = 0;
		break;
	default:
		if (Number[1] == SMS_International)
			sprintf(Buffer, "+");
		else
			Buffer[0] = '\0';
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

static SMS_DateTime *UnpackDateTime(u8 *Number, SMS_DateTime *dt)
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
		fprintf(stderr, _("Not supported User Data Header type\n"));
		break;
	}
	return GE_NONE;
}

static GSM_Error EncodeSMSSubmitHeader(GSM_SMSMessage *SMS, char *frame)
{
	GSM_Error error = GE_NONE;
	int i;

	/* SMS Center */

	/* Reply Path */
	if (SMS->ReplyViaSameSMSC) frame[13] |= 0x80;

	/* User Data Header */
	for (i = 0; SMS->UDH[i].Type != SMS_NoUDH; i++) {
		frame[13] |= 0x40;
		error = EncodeUDH(SMS->UDH[i], frame);
		if (error != GE_NONE) return error;
	}

	/* Status (Delivery) Report Request */
	if (SMS->Report) frame[13] |= 0x20;

	/* Validity Period Format: mask - 0x00, 0x10, 0x08, 0x18 */
	frame[13] |= ((SMS->Validity.VPF & 0x03) << 3);

	/* Reject Duplicates */
	if (SMS->RejectDuplicates) frame[13] |= 0x04;

	/* Message Type is already set */

	/* Message Reference */
	/* Can we set this? */

	/* Protocol Identifier */
	/* FIXME: allow to change this in better way.
	   currently only 0x5f == `Return Call Message' is used */
	frame[16] = SMS->PID;

	/* Data Coding Scheme */
	switch (SMS->DCS.Type) {
	case SMS_GeneralDataCoding:
		if (SMS->DCS.u.General.Compressed) frame[17] |= 0x20;
		if (SMS->DCS.u.General.Class) frame[17] |= (0x10 | (SMS->DCS.u.General.Class - 1));
		frame[17] |= ((SMS->DCS.u.General.Alphabet & 0x03) << 2);
		break;
	case SMS_MessageWaiting:
		if (SMS->DCS.u.MessageWaiting.Discard) frame[17] |= 0xc0;
		else if (SMS->DCS.u.MessageWaiting.Alphabet == SMS_UCS2) frame[17] |= 0xe0;
		else frame[17] |= 0xd0;
		if (SMS->DCS.u.MessageWaiting.Active) frame[17] |= 0x80;
		frame[17] |= (SMS->DCS.u.MessageWaiting.Type & 0x03);
		break;
	default:
		fprintf(stderr, _("Wrong Data Coding Scheme (DCS) format\n"));
		return GE_SMSWRONGFORMAT;
	}

	/* User Data Length */
	/* Will be filled later. */

	/* Destination Address */

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

	return GE_NONE;
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
	frame[12] |= (SMS->Type >> 1);
	switch (SMS->Type) {
	case SMS_Submit: /* we send SMS or save it in Outbox */
		return EncodeSMSSubmitHeader(SMS, frame);
	case SMS_Deliver: /* we save SMS in Inbox */
		return EncodeSMSSubmitHeader(SMS, frame);
	default: /* we don't create other formats of SMS */
		return GE_SMSWRONGFORMAT;
	}
}

/* This function encodes SMS as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9
*/
GSM_Error EncodePDUSMS(GSM_SMSMessage *SMS, char *frame)
{
	GSM_Error error = GE_NONE;
	int i;

	/* Short Message Data info element id */

	/* Length of Short Message Data */

	/* Short Message Reference value */
	if (SMS->Number) frame[3] = SMS->Number;

	/* Short Message status */
	if (SMS->Status) frame[1] = SMS->Status;

	/* Message Type */

	/* SMSC number */
	if (SMS->MessageCenter.Number) {
//		error = GSM->GetSMSCenter(&SMS->MessageCenter);
		
		if (error != GE_NONE)
			return error;
		strcpy(SMS->MessageCenter.Number, "0");
	}
	dprintf("Sending SMS to %s via message center %s\n", SMS->RemoteNumber.number, SMS->MessageCenter.Number);

	/* Header coding */
//	EncodeUDH(SMS, frame);
//	if (error != GE_NONE) return error;

	/* User Data Header - if present */
	for (i = 0; SMS->UDH[i].Type != SMS_NoUDH; i++) {
		error = EncodeUDH(SMS->UDH[i], frame);
		if (error != GE_NONE) return error;
	}

	/* User Data */
	//	EncodeData(&(SMS->MessageText), &(SMS->DCS), frame);

	return error;
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
        switch (SMS->Type = message[2]) {
	case SMS_Deliver:
		dprintf("Mobile Terminated message:\n");
		break;
	case SMS_Delivery_Report:
		dprintf("Delivery Report:\n");
		UnpackDateTime(message + 34 + DataOffset[SMS->Type], &(SMS->SMSCTime));
		dprintf("\tDelivery date: %s\n", PrintDateTime(message + 34 + DataOffset[SMS->Type]));
		break;
	case SMS_Submit:
		dprintf("Mobile Originated message:\n");
		break;
	default:
		dprintf("Not supported message type:\n");
		break;
	}

	/* Short Message location in memory */
	SMS->Number = message[1];
	dprintf("\tLocation: %d\n", SMS->Number);

	/* Short Message Center */
        strcpy(SMS->MessageCenter.Number, GetBCDNumber(message + 3));
        dprintf("\tSMS center number: %s\n", SMS->MessageCenter.Number);
        SMS->ReplyViaSameSMSC = false;
        if (SMS->RemoteNumber.number[0] == 0 && (message[6] & 0x80)) {
		SMS->ReplyViaSameSMSC = true;
	}

        /* Remote number */
        message[15+DataOffset[SMS->Type]] = ((message[15+DataOffset[SMS->Type]])+1)/2+1;
        dprintf("\tRemote number (recipient or sender): %s\n", GetBCDNumber(message + 15 + DataOffset[SMS->Type]));
        strcpy(SMS->RemoteNumber.number, GetBCDNumber(message + 15 + DataOffset[SMS->Type]));

	UnpackDateTime(message + 27 + DataOffset[SMS->Type], &(SMS->Time));
        dprintf("\tDate: %s\n", PrintDateTime(message + 27 + DataOffset[SMS->Type]));

	/* Message length */
	SMS->Length = message[14+DataOffset[SMS->Type]];

	/* Data Coding Scheme */
	if (SMS->Type != SMS_Delivery_Report)
		SMS->DCS.Type = message[13 + DataOffset[SMS->Type]];
	else
		SMS->DCS.Type = 0;

	/* User Data Header */
        if (message[15] & 0x40) { /* UDH header available */
		dprintf("UDH found\n");
		DecodeUDH(message + 34 + DataOffset[SMS->Type], SMS);
	} else {                    /* No UDH */
		dprintf("No UDH\n");
		SMS->UDH_No = 0;
	}

	return GE_NONE;
}

/* This function decodes SMS as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9
*/
GSM_Error DecodePDUSMS(unsigned char *message, GSM_SMSMessage *SMS, int MessageLength)
{
	DecodeSMSHeader(message, SMS);
	if (SMS->Type == SMS_Delivery_Report) {
		SMSStatus(message[17], SMS);
	} else {
		int size = MessageLength -
			   34 -                    /* Header Length */
			   DataOffset[SMS->Type] - /* offset */
			   SMS->UDH_Length -       /* UDH Length */
			   5;                      /* checksum */
		dprintf("UDH Len : %d\n", SMS->UDH_Length);
		DecodeData(message + 34 + DataOffset[SMS->Type] + SMS->UDH_Length,
			   (unsigned char *)&(SMS->MessageText),
			   SMS->Length, size, SMS->UDH_Length, SMS->DCS);
		/* Just in case */
		SMS->MessageText[SMS->Length] = 0;
	}
	
	return GE_NONE;
}

/* This function does simple SMS decoding - no PDU coding */
GSM_Error DecodeTextSMS(unsigned char *message, GSM_SMSMessage *SMS)
{
        return GE_NONE;
}
