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

*/

#include <stdlib.h>
#include <string.h>

#include "gsm-data.h"
#include "gsm-encoding.h"
#include "gsm-statemachine.h"

#include "gsm-ringtones.h"
#include "gsm-bitmaps.h"

#include "sms-nokia.h"

#define ERROR() do { if (error != GE_NONE) return error; } while (0)

struct udh_data {
	unsigned int length;
	char *header;
};

#define MAX_SMS_PART 128

/* These are positions in the following (headers[]) table. */
#define UDH_CONCATENATED_MESSAGES  1
#define UDH_RINGTONES              2
#define UDH_OPERATOR_LOGOS         3
#define UDH_CALLER_LOGOS           4
#define UDH_MULTIPART_MESSAGES     5
#define UDH_WAP_VCARD              6
#define UDH_WAP_CALENDAR           7
#define UDH_WAP_VCARDSECURE        8
#define UDH_WAP_VCALENDARSECURE    9
#define UDH_VOICE_MESSAGES        10
#define UDH_FAX_MESSAGES          11
#define UDH_EMAIL_MESSAGES        12

/* User data headers */
static struct udh_data headers[] = {
	{ 0x00, "" },
	{ 0x05, "\x00\x03\x01\x00\x00" },     /* Concatenated messages */
	{ 0x06, "\x05\x04\x15\x81\x00\x00" }, /* Ringtones */
	{ 0x06, "\x05\x04\x15\x82\x00\x00" }, /* Operator logos */
	{ 0x06, "\x05\x04\x15\x83\x00\x00" }, /* Caller logos */
	{ 0x06, "\x05\x04\x15\x8a\x00\x00" }, /* Multipart Message */
	{ 0x06, "\x05\x04\x23\xf4\x00\x00" }, /* WAP vCard */
	{ 0x06, "\x05\x04\x23\xf5\x00\x00" }, /* WAP vCalendar */
	{ 0x06, "\x05\x04\x23\xf6\x00\x00" }, /* WAP vCardSecure */
	{ 0x06, "\x05\x04\x23\xf7\x00\x00" }, /* WAP vCalendarSecure */
	{ 0x04, "\x01\x02\x00\x00" },         /* Voice Messages */
	{ 0x04, "\x01\x02\x01\x00" },         /* Fax Messages */
	{ 0x04, "\x01\x02\x02\x00" },         /* Email Messages */
	{ 0x00, "" },
};


/**
 * DefaultSMS - fills in SMS structure with the default values
 * @SMS: pointer to a structure we need to fill in
 *
 * Default settings:
 *  - no delivery report
 *  - no Class Message
 *  - no compression
 *  - 7 bit data
 *  - message validity for 3 days
 *  - unsent status
 */
static void DefaultSMS(GSM_API_SMS *sms)
{
	memset(sms, 0, sizeof(GSM_API_SMS));

	sms->Type = SMS_Deliver;
	sms->DeliveryReport = false;
	sms->Status = SMS_Unsent;
	sms->Validity = 4320; /* 4320 minutes == 72 hours */
	sms->DCS.Type = SMS_GeneralDataCoding;
	sms->DCS.u.General.Compressed = false;
	sms->DCS.u.General.Alphabet = SMS_DefaultAlphabet;
	sms->DCS.u.General.Class = 0;
}

API void DefaultSubmitSMS(GSM_API_SMS *sms)
{
	DefaultSMS(sms);
	sms->Type = SMS_Submit;
	sms->MemoryType = GMT_SM;
}

API void DefaultDeliverSMS(GSM_API_SMS *sms)
{
	DefaultSMS(sms);
	sms->Type = SMS_Deliver;
	sms->MemoryType = GMT_ME;
}


/***
 *** Util functions
 ***/

/**
 * PrintDateTime - formats date and time in the human readable manner
 * @number: binary coded date and time
 *
 * The function converts binary date time into a easily readable form.
 * It is Y2K compliant.
 */
static char *PrintDateTime(u8 *number)
{
#ifdef DEBUG
#define LOCAL_DATETIME_MAX_LENGTH 23
	static unsigned char buffer[LOCAL_DATETIME_MAX_LENGTH] = "";

	if (!number) return NULL;

	memset(buffer, 0, LOCAL_DATETIME_MAX_LENGTH);

	/* Ugly hack, but according to the GSM specs, the year is stored
         * as the 2 digit number. */
	if (number[0] < 70) sprintf(buffer, "20");
	else sprintf(buffer, "19");

	sprintf(buffer, "%s%d%d-", buffer, number[0] & 0x0f, number[0] >> 4);
	sprintf(buffer, "%s%d%d-", buffer, number[1] & 0x0f, number[1] >> 4);
	sprintf(buffer, "%s%d%d ", buffer, number[2] & 0x0f, number[2] >> 4);
	sprintf(buffer, "%s%d%d:", buffer, number[3] & 0x0f, number[3] >> 4);
	sprintf(buffer, "%s%d%d:", buffer, number[4] & 0x0f, number[4] >> 4);
	sprintf(buffer, "%s%d%d",  buffer, number[5] & 0x0f, number[5] >> 4);

	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (number[6] & 0x08) sprintf(buffer, "%s-", buffer);
	else sprintf(buffer, "%s+", buffer);
	/* The timezone is given in quarters. The base is GMT. */
	sprintf(buffer, "%s%02d00", buffer, (10 * (number[6] & 0x07) + (number[6] >> 4)) / 4);

	return buffer;
#undef LOCAL_DATETIME_MAX_LENGTH
#else
	return NULL;
#endif /* DEBUG */
}

/**
 * UnpackDateTime - converts binary datetime to the gnokii's datetime struct
 * @number: binary coded date and time
 * @dt: datetime structure to be filled
 *
 * The function fills int gnokii datetime structure basing on the given datetime
 * in the binary format. It is Y2K compliant.
 */
SMS_DateTime *UnpackDateTime(u8 *number, SMS_DateTime *dt)
{
	if (!dt) return NULL;
	memset(dt, 0, sizeof(SMS_DateTime));
	if (!number) return dt;

	dt->Year     =  10 * (number[0] & 0x0f) + (number[0] >> 4);

	/* Ugly hack, but according to the GSM specs, the year is stored
         * as the 2 digit number. */
	if (dt->Year < 70) dt->Year += 2000;
	else dt->Year += 1900;

	dt->Month    =  10 * (number[1] & 0x0f) + (number[1] >> 4);
	dt->Day      =  10 * (number[2] & 0x0f) + (number[2] >> 4);
	dt->Hour     =  10 * (number[3] & 0x0f) + (number[3] >> 4);
	dt->Minute   =  10 * (number[4] & 0x0f) + (number[4] >> 4);
	dt->Second   =  10 * (number[5] & 0x0f) + (number[5] >> 4);

	/* The timezone is given in quarters. The base is GMT */
	dt->Timezone = (10 * (number[6] & 0x07) + (number[6] >> 4)) / 4;
	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (number[6] & 0x08) dt->Timezone = -dt->Timezone;

	return dt;
}

/***
 *** DECODING SMS
 ***/

/**
 * SMSStatus - decodes the error code contained in the delivery report.
 * @status: error code to decode
 * @sms: SMS struct to store the result
 *
 * This function checks for the error (or success) code contained in the
 * received SMS - delivery report and fills in the SMS structure with
 * the correct message according to the GSM specification.
 * This function only applies to the delivery reports. Delivery reports
 * contain only one part, so it is assumed all other part except 0th are
 * NULL.
 */
static GSM_Error SMSStatus(unsigned char status, GSM_API_SMS *sms)
{
	sms->UserData[0].Type = SMS_PlainText;
	sms->UserData[1].Type = SMS_NoData;
	if (status < 0x03) {
		strcpy(sms->UserData[0].u.Text, _("Delivered"));
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
	} else if (status & 0x40) {
		strcpy(sms->UserData[0].u.Text, _("Failed"));
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
	} else if (status & 0x20) {
		strcpy(sms->UserData[0].u.Text, _("Pending"));
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
	} else {
		strcpy(sms->UserData[0].u.Text, _("Unknown"));

		/* more detailed reason only for debug */
		dprintf("Reserved/Specific to SC: %x", status);
	}
	dprintf("\n");
	sms->UserData[0].Length = strlen(sms->UserData[0].u.Text);
	return GE_NONE;
}

/**
 * DecodeData - decodes encoded text from the SMS.
 * @message: encoded text
 * @output: room for the encoded text
 * @length: decoded text length
 * @size: encoded text length
 * @udhlen: udh length 
 * @dcs: data coding scheme
 *
 * This function decodes either 7bit or 8bit os Unicode text to the
 * readable text format according to the locale set.
 */
static GSM_Error DecodeData(unsigned char *message, unsigned char *output, unsigned int length,
			    unsigned int size, unsigned int udhlen, SMS_DataCodingScheme dcs)
{
	/* Unicode */
	if ((dcs.Type & 0x08) == 0x08) {
		dprintf("Unicode message\n");
		DecodeUnicode(output, message, length);
	} else {
		/* 8bit SMS */
		if ((dcs.Type & 0xf4) == 0xf4) {
			dprintf("8bit message\n");
			memcpy(output, message + udhlen, length);
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

/**
 * DecodeUDH - interprete the User Data Header contents
 * @message: received UDH
 * @sms: SMS structure to fill in
 *
 * At this stage we already need to know thet SMS has UDH.
 * This function decodes UDH as described in:
 *   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
 *   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
 */
static GSM_Error DecodeUDH(unsigned char *message, SMS_UserDataHeader *udh)
{
	unsigned char length, pos, nr;

	udh->Length = length = message[0];
	pos = 1;
	nr = 0;
	while (length > 1) {
		unsigned char udh_length;

		udh_length = message[pos+1];
		switch (message[pos]) {
		case 0x00: /* Concatenated short messages */
			dprintf("Concatenated messages\n");
			udh->UDH[nr].Type = SMS_ConcatenatedMessages;
			udh->UDH[nr].u.ConcatenatedShortMessage.ReferenceNumber = message[pos + 2];
			udh->UDH[nr].u.ConcatenatedShortMessage.MaximumNumber   = message[pos + 3];
			udh->UDH[nr].u.ConcatenatedShortMessage.CurrentNumber   = message[pos + 4];
			break;
		case 0x01: /* Special SMS Message Indication */
			switch (message[pos + 2] & 0x03) {
			case 0x00:
				dprintf("Voice Message\n");
				udh->UDH[nr].Type = SMS_VoiceMessage;
				break;
			case 0x01:
				dprintf("Fax Message\n");
				udh->UDH[nr].Type = SMS_FaxMessage;
				break;
			case 0x02:
				dprintf("Email Message\n");
				udh->UDH[nr].Type = SMS_EmailMessage;
				break;
			default:
				dprintf("Unknown\n");
				udh->UDH[nr].Type = SMS_UnknownUDH;
				break;
			}
			udh->UDH[nr].u.SpecialSMSMessageIndication.Store = (message[pos + 2] & 0x80) >> 7;
			udh->UDH[nr].u.SpecialSMSMessageIndication.MessageCount = message[pos + 3];
			break;
		case 0x05: /* Application port addression scheme, 16 bit address */
			switch (((0x00ff & message[pos + 2]) << 8) | (0x00ff & message[pos + 3])) {
			case 0x1581:
				dprintf("Ringtone\n");
				udh->UDH[nr].Type = SMS_Ringtone;
				break;
			case 0x1582:
				dprintf("Operator Logo\n");
				udh->UDH[nr].Type = SMS_OpLogo;
				break;
			case 0x1583:
				dprintf("Caller Icon\n");
				udh->UDH[nr].Type = SMS_CallerIDLogo;
				break;
			case 0x158a:
				dprintf("Multipart Message\n");
				udh->UDH[nr].Type = SMS_MultipartMessage;
				break;
			case 0x23f4:
				dprintf("WAP vCard\n");
				udh->UDH[nr].Type = SMS_WAPvCard;
				break;
			case 0x23f5:
				dprintf("WAP vCalendar\n");
				udh->UDH[nr].Type = SMS_WAPvCalendar;
				break;
			case 0x23f6:
				dprintf("WAP vCardSecure\n");
				udh->UDH[nr].Type = SMS_WAPvCardSecure;
				break;
			case 0x23f7:
				dprintf("WAP vCalendarSecure\n");
				udh->UDH[nr].Type = SMS_WAPvCalendarSecure;
				break;
			default:
				dprintf("Unknown\n");
				udh->UDH[nr].Type = SMS_UnknownUDH;
				break;
			}
			break;
		case 0x04: /* Application port addression scheme, 8 bit address */
		case 0x06: /* SMSC Control Parameters */
		case 0x07: /* UDH Source Indicator */
		default:
			udh->UDH[nr].Type = SMS_UnknownUDH;
			dprintf("Not supported UDH\n");
			break;
		}
		length -= (udh_length + 2);
		pos += (udh_length + 2);
		nr++;
	}
	udh->Number = nr;

	return GE_NONE;
}

/**
 * DecodeSMSHeader - Doecodes PDU SMS header
 * @rawsms:
 * @sms:
 * @udh:
 *
 * This function parses received SMS header information and stores
 * them in hihger level SMS struct. It also checks for the UDH and when
 * it's found calls the function to extract the UDH.
 */
static GSM_Error DecodeSMSHeader(GSM_SMSMessage *rawsms, GSM_API_SMS *sms, SMS_UserDataHeader *udh)
{
	switch (sms->Type = rawsms->Type) {
	case SMS_Deliver:
		dprintf("Mobile Terminated message:\n");
		break;
	case SMS_Delivery_Report:
		dprintf("Delivery Report:\n");
		break;
	case SMS_Submit:
		dprintf("Mobile Originated (stored) message:\n");
		break;
	case SMS_SubmitSent:
		dprintf("Mobile Originated (sent) message:\n");
		break;
	case SMS_Picture:
		dprintf("Picture Message:\n");
		break;
	case SMS_PictureTemplate:
		dprintf("Picture Template:\n");
		break;
	case SMS_TextTemplate:
		dprintf("Text Template:\n");
		break;
	default:
		dprintf("Not supported message type: %d\n", sms->Type);
		return GE_NOTSUPPORTED;
	}

	/* Sending date */
	UnpackDateTime(rawsms->SMSCTime, &(sms->SMSCTime));
	dprintf("\tDate: %s\n", PrintDateTime(rawsms->SMSCTime));

	/* Remote number */
	rawsms->RemoteNumber[0] = (rawsms->RemoteNumber[0] + 1) / 2 + 1;
	snprintf(sms->Remote.Number, sizeof(sms->Remote.Number), "%s", GetBCDNumber(rawsms->RemoteNumber));
	dprintf("\tRemote number (recipient or sender): %s\n", sms->Remote.Number);

	/* Short Message Center */
	snprintf(sms->SMSC.Number, sizeof(sms->SMSC.Number), "%s", GetBCDNumber(rawsms->MessageCenter));
	dprintf("\tSMS center number: %s\n", sms->SMSC.Number);

	/* Delivery time */
	if (sms->Type == SMS_Delivery_Report) {
		UnpackDateTime(rawsms->Time, &(sms->Time));
		dprintf("\tDelivery date: %s\n", PrintDateTime(rawsms->Time));
	}

	/* Data Coding Scheme */
	sms->DCS.Type = rawsms->DCS;

	/* User Data Header */
	if (rawsms->UDHIndicator & 0x40) { /* UDH header available */
		dprintf("UDH found\n");
		DecodeUDH(rawsms->UserData, udh);
	}

	return GE_NONE;
}

/**
 * DecodePDUSMS - This function decodes the PDU SMS
 * @rawsms - SMS read by the phone driver
 * @sms - place to store the decoded message
 *
 * This function decodes SMS as described in GSM 03.40 version 6.1.0
 * Release 1997, section 9
 */
static GSM_Error DecodePDUSMS(GSM_SMSMessage *rawsms, GSM_API_SMS *sms)
{
	unsigned int size = 0;
	GSM_Error error;

	error = DecodeSMSHeader(rawsms, sms, &sms->UDH);
	ERROR();
	switch (sms->Type) {
	case SMS_Delivery_Report:
		SMSStatus(rawsms->ReportStatus, sms);
		break;
	case SMS_PictureTemplate:
	case SMS_Picture:
		/* This is incredible. Nokia violates it's own format in 6210 */
		/* Indicate that it is Multipart Message. Remove it if not needed */
		/* [I believe Nokia said in their manuals that any order is permitted --pavel] */
		sms->UDH.Number = 1;
		sms->UDH.UDH[0].Type = SMS_MultipartMessage;
		if ((rawsms->UserData[0] == 0x48) && (rawsms->UserData[1] == 0x1c)) {

			dprintf("First picture then text!\n");

			/* First part is a Picture */
			sms->UserData[0].Type = SMS_BitmapData;
			GSM_ReadSMSBitmap(GSM_PictureMessage, rawsms->UserData,
					  NULL, &sms->UserData[0].u.Bitmap);
			GSM_PrintBitmap(&sms->UserData[0].u.Bitmap, stderr);

			size = rawsms->UserDataLength - 4 - sms->UserData[0].u.Bitmap.size;
			/* Second part is a text */
			sms->UserData[1].Type = SMS_NokiaText;
			DecodeData(rawsms->UserData + 5 + sms->UserData[0].u.Bitmap.size,
				   (unsigned char *)&(sms->UserData[1].u.Text),
				   rawsms->Length - sms->UserData[0].u.Bitmap.size - 4,
				   size, 0, sms->DCS);
		} else {

			dprintf("First text then picture!\n");

			/* First part is a text */
			sms->UserData[1].Type = SMS_NokiaText;
			DecodeData(rawsms->UserData + 3,
				   (unsigned char *)&(sms->UserData[1].u.Text),
				   rawsms->UserData[1], rawsms->UserData[0], 0, sms->DCS);

			/* Second part is a Picture */
			sms->UserData[0].Type = SMS_BitmapData;
			GSM_ReadSMSBitmap(GSM_PictureMessage,
					  rawsms->UserData + rawsms->UserData[0] + 7,
					  NULL, &sms->UserData[0].u.Bitmap);
			GSM_PrintBitmap(&sms->UserData[0].u.Bitmap, stderr);
		}
		break;
	/* Plain text message */
	default:
		size = rawsms->Length - sms->UDH.Length;
		DecodeData(rawsms->UserData + sms->UDH.Length,        /* Skip the UDH */
			   (unsigned char *)&sms->UserData[0].u.Text, /* With a plain text message we have only 1 part */
			   rawsms->Length,                            /* Length of the decoded text */
			   size,                                      /* Length of the encoded text (in full octets) without UDH */
			   sms->UDH.Length,                          /* To skip the certain number of bits when unpacking 7bit message */
			   sms->DCS);
		break;
	}

	return GE_NONE;
}

/**
 * ParseSMS - High-level function for the SMS parsing
 * @data: GSM data from the phone driver
 *
 * This function parses the SMS message from the lowlevel RawSMS to
 * the highlevel SMS. In data->RawSMS there's SMS read by the phone
 * driver, data->SMS is the place for the parsed SMS.
 */
API GSM_Error ParseSMS(GSM_Data *data)
{
	if (!data->RawSMS || !data->SMS) return GE_INTERNALERROR;
	/* Let's assume at the moment that all messages are PDU coded */
	return DecodePDUSMS(data->RawSMS, data->SMS);
}

/**
 * RequestSMS - High-level function for the expicit SMS reading from the phone
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This is the function for explicit requesting the SMS from the
 * phone driver. Not that RawSMS field in the GSM_Data structure must
 * be initialized
 */
API GSM_Error RequestSMS(GSM_Data *data, GSM_Statemachine *state)
{
	if (!data->RawSMS) return GE_INTERNALERROR;
	return SM_Functions(GOP_GetSMS, data, state);
}

/**
 * GetSMS - High-level function for reading SMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frotnend for reading SMS. Note that SMS field
 * in the GSM_Data structure must be initialized.
 */
API GSM_Error GetSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error;
	GSM_SMSMessage rawsms;

	if (!data->SMS) return GE_INTERNALERROR;
	memset(&rawsms, 0, sizeof(GSM_SMSMessage));
	rawsms.Number = data->SMS->Number;
	rawsms.MemoryType = data->SMS->MemoryType;
	data->RawSMS = &rawsms;
	error = RequestSMS(data, state);
	ERROR();
	data->SMS->Status = rawsms.Status;
	return ParseSMS(data);
}

/**
 * DeleteSMS - High-level function for delting SMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frontend for deleting SMS. Note that SMS field
 * in the GSM_Data structure must be initialized.
 */
API GSM_Error DeleteSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_SMSMessage rawsms;

	if (!data->SMS) return GE_INTERNALERROR;
	memset(&rawsms, 0, sizeof(GSM_SMSMessage));
	rawsms.Number = data->SMS->Number;
	rawsms.MemoryType = data->SMS->MemoryType;
	data->RawSMS = &rawsms;
	return SM_Functions(GOP_DeleteSMS, data, state);
}

/***
 *** OTHER FUNCTIONS
 ***/

GSM_Error RequestSMSnoValidate(GSM_Data *data, GSM_Statemachine *state)
{
	return SM_Functions(GOP_GetSMSnoValidate, data, state);
}

API GSM_Error GetSMSnoValidate(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error;
	GSM_SMSMessage rawsms;

	if (!data->SMS) return GE_INTERNALERROR;
	memset(&rawsms, 0, sizeof(GSM_SMSMessage));
	rawsms.Number = data->SMS->Number;
	rawsms.MemoryType = data->SMS->MemoryType;
	data->RawSMS = &rawsms;
	error = RequestSMSnoValidate(data, state);
	ERROR();
	data->SMS->Status = rawsms.Status;
	return ParseSMS(data);
}

API GSM_Error DeleteSMSnoValidate(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_SMSMessage rawsms;

	if (!data->SMS) return GE_INTERNALERROR;
	memset(&rawsms, 0, sizeof(GSM_SMSMessage));
	rawsms.Number = data->SMS->Number;
	rawsms.MemoryType = data->SMS->MemoryType;
	data->RawSMS = &rawsms;
	return SM_Functions(GOP_DeleteSMSnoValidate, data, state);
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

	for (i = 0; i < folder.Number; i++) {		/* cycle through all messages in phone */
		found = 0;
		for (j = 0; j < data->FolderStats[folder.FolderID]->Used; j++) {		/* and compare them to those alread in list */
			if (data->SMSFolder->Locations[i] == data->MessagesList[j][folder.FolderID]->Location) found = 1;
		}
		if (!found) {
			dprintf("Found new (read) message. Will store it at #%i!\n", data->FolderStats[folder.FolderID]->Used);
			dprintf("%i\n", folder.Locations[i]);
			data->MessagesList[data->FolderStats[folder.FolderID]->Used][folder.FolderID]->Location = folder.Locations[i];
			data->MessagesList[data->FolderStats[folder.FolderID]->Used][folder.FolderID]->Type = SMS_New;
			data->FolderStats[folder.FolderID]->Used++;
			data->FolderStats[folder.FolderID]->Changed++;
			data->SMSStatus->Changed++;
		}
	}
	return GE_NONE;
}

GSM_Error GetDeletedMessages(GSM_Data *data, SMS_Folder folder)
{
	int i, j, found = 0;

	for (i = 0; i < data->FolderStats[folder.FolderID]->Used; i++) {		/* for all previously found locations in folder */
		found = 0;

		for (j = 0; j < folder.Number; j++) {	/* see if there is a corresponding message in phone */
			if (data->MessagesList[i][folder.FolderID]->Location == folder.Locations[j]) found = 1;
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
			for (j = 0; j < folder.Number; j++) {
				if (data->MessagesList[i][folder.FolderID]->Location == folder.Locations[j]) {
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


API GSM_Error GetFolderChanges(GSM_Data *data, GSM_Statemachine *state, int has_folders)
{
	GSM_Error error;
	SMS_Folder tmp_folder, SMSFolder;
	SMS_FolderList tmp_list, SMSFolderList;
	int i, previous_unread, previous_total;

	previous_total = data->SMSStatus->Number;
	previous_unread = data->SMSStatus->Unread;
	dprintf("GetFolderChanges: Old status: %d %d\n", data->SMSStatus->Number, data->SMSStatus->Unread);

	error = SM_Functions(GOP_GetSMSStatus, data, state);	/* Check overall SMS Status */
	ERROR();
	dprintf("GetFolderChanges: Status: %d %d\n", data->SMSStatus->Number, data->SMSStatus->Unread);

	if (!has_folders) {
		if ((previous_total == data->SMSStatus->Number) && (previous_unread == data->SMSStatus->Unread))
			data->SMSStatus->Changed = 0;
		else
			data->SMSStatus->Changed = 1;
		return GE_NONE;
	}

	data->SMSFolderList = &SMSFolderList;
	error = SM_Functions(GOP_GetSMSFolders, data, state);
	ERROR();

	data->SMSStatus->NumberOfFolders = data->SMSFolderList->Number;
	memcpy(&tmp_list, data->SMSFolderList, sizeof(SMS_FolderList));	/* We need it because data->SMSFolderlist can get garbled */

	for (i = 0; i < data->SMSStatus->NumberOfFolders; i++) {
		dprintf("GetFolderChanges: Freeing deleted messages for folder #%i\n", i);
		error = FreeDeletedMessages(data, i);
		ERROR();

		data->SMSFolder = &SMSFolder;
		data->SMSFolder->FolderID = (GSM_MemoryType) i + 12;
		dprintf("GetFolderChanges: Getting folder status for folder #%i\n", data->SMSFolder->FolderID);
		error = SM_Functions(GOP_GetSMSFolderStatus, data, state);
		ERROR();

		memcpy(&tmp_folder, data->SMSFolder, sizeof(SMS_Folder));	/* We need it because data->SMSFolder can get garbled */

		tmp_folder.FolderID = i;	/* so we don't need to do a modulo 8 each time */
			
		dprintf("GetFolderChanges: Reading read messages (%i) for folder #%i\n", data->SMSFolder->Number, i);
		error = GetReadMessages(data, tmp_folder);
		ERROR();

		dprintf("GetFolderChanges: Getting deleted messages for folder #%i\n", i);
		error = GetDeletedMessages(data, tmp_folder);
		ERROR();

		dprintf("GetFolderChanges: Verifying messages for folder #%i\n", i);
		error = VerifyMessagesStatus(data, tmp_folder);
		ERROR();
	}
	return GE_NONE;
}

/***
 *** ENCODING SMS
 ***/


/**
 * EncodeUDH - encodes User Data Header
 * @UDHi: User Data Header information
 * @SMS: SMS structure with the data source
 * @UDH: phone frame where to save User Data Header
 *
 * returns pointer to data it added;
 *
 * This function encodes the UserDataHeader as described in:
 *  o GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
 *  o Smart Messaging Specification, Revision 1.0.0, September 15, 1997
 *  o Smart Messaging Specification, Revision 3.0.0
 */
static char *EncodeUDH(GSM_SMSMessage *rawsms, int type)
{
	unsigned char pos;
	char *UDH = rawsms->UserData;
	char *res = NULL;

	pos = UDH[0];

	switch (type) {
	case SMS_NoUDH:
		break;
	case SMS_VoiceMessage:
	case SMS_FaxMessage:
	case SMS_EmailMessage:
		return NULL;
#if 0
		UDH[pos+4] = UDHi.u.SpecialSMSMessageIndication.MessageCount;
		if (UDHi.u.SpecialSMSMessageIndication.Store) UDH[pos+3] |= 0x80;
#endif
	case SMS_ConcatenatedMessages:
		dprintf("Adding ConcatMsg header\n");
	case SMS_OpLogo:
	case SMS_CallerIDLogo:
	case SMS_Ringtone:
	case SMS_MultipartMessage:
		UDH[0] += headers[type].length;
		res = UDH+pos+1;
		memcpy(res, headers[type].header, headers[type].length);
		rawsms->UserDataLength += headers[type].length;
		rawsms->Length += headers[type].length;
		break;
	default:
		dprintf("Not supported User Data Header type\n");
		break;
	}
	if (!rawsms->UDHIndicator) {
		rawsms->UDHIndicator = 1;
		rawsms->Length++;		/* Length takes one byte, too */
		rawsms->UserDataLength++;
	}
	return res;
}

/**
 * EncodeConcatHeader - Adds concatenated messages header
 * @rawsms: processed SMS
 * @this: current part number
 * @total: total parts number
 *
 * This function adds sequent part of the concatenated messages header. Note
 * that this header should be the first of all headers.
 */
static GSM_Error EncodeConcatHeader(GSM_SMSMessage *rawsms, int this, int total)
{
	char *header = EncodeUDH(rawsms, SMS_ConcatenatedMessages);
	if (!header) return GE_NOTSUPPORTED;
	header[2] = 0xce;		/* Message serial number. Is 0xce value somehow special? -- pkot */
	header[3] = total;
	header[4] = this;
	return GE_NONE;
}

/**
 * EncodeData - encodes the date from the SMS structure to the phone frame
 *
 * @SMS: SMS structure to be encoded
 * @dcs: Data Coding Scheme field in the frame to be set
 * @message: phone frame to be filled in
 * @multipart: do we send one message or more?
 * @clen: coded data length
 *
 * This function does the phone frame encoding basing on the given SMS
 * structure. This function is capable to create only one frame at a time.
 */
GSM_Error EncodeData(GSM_API_SMS *sms, GSM_SMSMessage *rawsms)
{
	SMS_AlphabetType al = SMS_DefaultAlphabet;
	unsigned int i, size = 0;
	GSM_Error error;

	/* Additional Headers */
	switch (sms->DCS.Type) {
	case SMS_GeneralDataCoding:
		switch (sms->DCS.u.General.Class) {
		case 0: break;
		case 1: rawsms->DCS |= 0xf0; break; /* Class 0 */
		case 2: rawsms->DCS |= 0xf1; break; /* Class 1 */
		case 3: rawsms->DCS |= 0xf2; break; /* Class 2 */
		case 4: rawsms->DCS |= 0xf3; break; /* Class 3 */
		default: dprintf("What ninja-mutant class is this?\n"); break; 
		}
		if (sms->DCS.u.General.Compressed) {
			/* Compression not supported yet */
			/* dcs[0] |= 0x20; */
		}
		al = sms->DCS.u.General.Alphabet;
		break;
	case SMS_MessageWaiting:
		al = sms->DCS.u.MessageWaiting.Alphabet;
		if (sms->DCS.u.MessageWaiting.Discard) rawsms->DCS |= 0xc0;
		else if (sms->DCS.u.MessageWaiting.Alphabet == SMS_UCS2) rawsms->DCS |= 0xe0;
		else rawsms->DCS |= 0xd0;

		if (sms->DCS.u.MessageWaiting.Active) rawsms->DCS |= 0x08;
		rawsms->DCS |= (sms->DCS.u.MessageWaiting.Type & 0x03);

		break;
	default:
		return GE_WRONGDATAFORMAT;
	}

	rawsms->Length = rawsms->UserDataLength = 0;

	for (i = 0; i < SMS_MAX_PART_NUMBER; i++) {
		switch (sms->UserData[i].Type) {
		case SMS_BitmapData:
			switch (sms->UserData[0].u.Bitmap.type) {
			case GSM_PictureMessage:
				size = GSM_EncodeNokiaBitmap(&(sms->UserData[i].u.Bitmap),
							     rawsms->UserData + rawsms->UserDataLength,
							     (i == 0));
				break;
			case GSM_OperatorLogo:
				if (!EncodeUDH(rawsms, SMS_OpLogo)) return GE_NOTSUPPORTED;
			default:
				size = GSM_EncodeSMSBitmap(&(sms->UserData[i].u.Bitmap),
							   rawsms->UserData + rawsms->UserDataLength);
				break;
			}
			rawsms->Length += size;
			rawsms->UserDataLength += size;
			rawsms->DCS = 0xf5;
			rawsms->UDHIndicator = 1;
			break;

		case SMS_AnimationData: {
			int j;
			for (j = 0; j < 4; j++) {
				size = GSM_EncodeSMSBitmap(&(sms->UserData[i].u.Animation[j]), rawsms->UserData + rawsms->UserDataLength);
				rawsms->Length += size;
				rawsms->UserDataLength += size;
			}
			rawsms->DCS = 0xf5;
			rawsms->UDHIndicator = 1;
			break;
		}

		case SMS_PlainText: {
			unsigned int length, offset = rawsms->UserDataLength;

			length = strlen(sms->UserData[i].u.Text);
			switch (al) {
			case SMS_DefaultAlphabet:
#define UDH_Length 0
				size = Pack7BitCharacters((7 - (UDH_Length % 7)) % 7,
							  sms->UserData[i].u.Text,
							  rawsms->UserData + offset,
							  &length);
				rawsms->Length = length;
				rawsms->UserDataLength = size + offset;
				break;
			case SMS_8bit:
				rawsms->DCS |= 0xf4;
				memcpy(rawsms->UserData + offset, sms->UserData[i].u.Text, sms->UserData[i].u.Text[0]);
				rawsms->UserDataLength = rawsms->Length = length + offset;
				break;
			case SMS_UCS2:
				rawsms->DCS |= 0x08;
				EncodeUnicode(rawsms->UserData + offset, sms->UserData[i].u.Text, length);
				length *= 2;
				rawsms->UserDataLength = rawsms->Length = length + offset;
				break;
			default:
				return GE_WRONGDATAFORMAT;
			}
			break;
		}

		case SMS_NokiaText:
			size = EncodeNokiaText(sms->UserData[i].u.Text, rawsms->UserData + rawsms->UserDataLength, (i == 0));
			rawsms->Length += size;
			rawsms->UserDataLength += size;
			break;

		case SMS_iMelodyText:
			size = GSM_EncodeSMSiMelody(sms->UserData[i].u.Text, rawsms->UserData + rawsms->UserDataLength);
			dprintf("Imelody, size %d\n", size);
			rawsms->Length += size;
			rawsms->UserDataLength += size;
			rawsms->DCS = 0xf5;
			rawsms->UDHIndicator = 1;
			break;

		case SMS_MultiData:
			size = sms->UserData[0].Length;
			if (!EncodeUDH(rawsms, SMS_MultipartMessage)) return GE_NOTSUPPORTED;
			error = EncodeConcatHeader(rawsms, sms->UserData[i].u.Multi.this, sms->UserData[i].u.Multi.total);
			ERROR();

			memcpy(rawsms->UserData + rawsms->UserDataLength, sms->UserData[i].u.Multi.Binary, MAX_SMS_PART);
			rawsms->Length += size;
			rawsms->UserDataLength += size;
			rawsms->DCS = 0xf5;
			break;

		case SMS_RingtoneData:
			if (!EncodeUDH(rawsms, SMS_Ringtone)) return GE_NOTSUPPORTED;
			size = GSM_EncodeSMSRingtone(rawsms->UserData + rawsms->Length, &sms->UserData[i].u.Ringtone);
			rawsms->Length += size;
			rawsms->UserDataLength += size;
			rawsms->DCS = 0xf5;
			break;

		case SMS_Concat:
			dprintf("Encoding concat header\n");
			al = SMS_8bit;
			rawsms->DCS = 0xf5;
			EncodeConcatHeader(rawsms, sms->UserData[i].u.Concat.this, sms->UserData[i].u.Concat.total);
			break;

		case SMS_NoData:
			return GE_NONE;

		default:
			dprintf("What kind of ninja-mutant UserData is this?\n");
			break;
		}
	}
	return GE_NONE;
}

GSM_Error PrepareSMS(GSM_API_SMS *sms, GSM_SMSMessage *rawsms)
{
	switch (rawsms->Type = sms->Type) {
	case SMS_Submit:
	case SMS_Deliver:
	case SMS_Picture:
		break;
	case SMS_Delivery_Report:
	default:
		dprintf("Not supported message type: %d\n", rawsms->Type);
		return GE_NOTSUPPORTED;
	}
	/* Encoding the header */
	rawsms->Report = sms->DeliveryReport;
	rawsms->RemoteNumber[0] = SemiOctetPack(sms->Remote.Number, rawsms->RemoteNumber + 1, sms->Remote.Type);
	rawsms->ValidityIndicator = SMS_RelativeFormat;
	rawsms->Validity[0] = 0xa9;

	EncodeData(sms, rawsms);

	return GE_NONE;
}

static void DumpRawSMS(GSM_SMSMessage *rawsms)
{
	char buf[10240];

	memset(buf, 0, 10240);

	dprintf("DCS: 0x%x\n", rawsms->DCS);
	dprintf("Length: 0x%x\n", rawsms->Length);
	dprintf("UserDataLength: 0x%x\n", rawsms->UserDataLength);
	dprintf("ValidityIndicator: %d\n", rawsms->ValidityIndicator);
	bin2hex(buf, rawsms->UserData, rawsms->UserDataLength);
	dprintf("UserData: %s\n", buf);
}

GSM_Error SendLongSMS(GSM_Data *data, GSM_Statemachine *state);

/**
 * SendSMS - The main function for the SMS sending
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * The high level function to send SMS. You need to fill in data->SMS
 * (GSM_API_SMS) in the higher level. This is converted to RawSMS here,
 * and then phone driver takes the fields it needs and sends it in the
 * phone specific way to the phone.
 */
API GSM_Error SendSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error = GE_NONE;

	if (!data->SMS) return GE_INTERNALERROR;

	data->RawSMS = malloc(sizeof(*data->RawSMS));
	memset(data->RawSMS, 0, sizeof(*data->RawSMS));

	data->RawSMS->MessageCenter[0] = SemiOctetPack(data->SMS->SMSC.Number, data->RawSMS->MessageCenter + 1, data->SMS->SMSC.Type);
	if (data->RawSMS->MessageCenter[0] % 2) data->RawSMS->MessageCenter[0]++;
	data->RawSMS->MessageCenter[0] = data->RawSMS->MessageCenter[0] / 2 + 1;

	error = PrepareSMS(data->SMS, data->RawSMS);
	ERROR();

	if (data->RawSMS->Length > GSM_MAX_SMS_LENGTH) {
		dprintf("SMS is too long? %d\n", data->RawSMS->Length);
		error = SendLongSMS(data, state);
		goto cleanup;
	}

	error = SM_Functions(GOP_SendSMS, data, state);
 cleanup:
	free(data->RawSMS);
	data->RawSMS = NULL;
	return error;
}

API GSM_Error SendLongSMS(GSM_Data *data, GSM_Statemachine *state)
{
	int i, count;
	GSM_SMSMessage LongSMS, *rawsms = &LongSMS;	/* We need local copy for our dirty tricks */
	GSM_API_SMS sms;
	GSM_Error error = GE_NONE;

	LongSMS = *data->RawSMS;
	sms = *data->SMS;

	count = (rawsms->UserDataLength + MAX_SMS_PART - 1) / MAX_SMS_PART;
	dprintf("Will need %d sms-es\n", count);
	for (i = 0; i < count; i++) {
		dprintf("Sending sms #%d\n", i);
		sms.UserData[0].Type = SMS_MultiData;
		sms.UserData[0].Length = MAX_SMS_PART;
		if (i + 1 == count)
			sms.UserData[0].Length = rawsms->UserDataLength % MAX_SMS_PART;
		memcpy(sms.UserData[0].u.Multi.Binary, rawsms->UserData + i*MAX_SMS_PART, MAX_SMS_PART);
		sms.UserData[0].u.Multi.this = i + 1;
		sms.UserData[0].u.Multi.total = count;
		sms.UserData[1].Type = SMS_NoData;
		data->SMS = &sms;
		error = SendSMS(data, state);
		ERROR();
	}
	return GE_NONE;
}

API GSM_Error SaveSMS(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error = GE_NONE;
	GSM_SMSMessage rawsms;

	data->RawSMS = &rawsms;
	memset(&rawsms, 0, sizeof(rawsms));
	data->RawSMS->Number = data->SMS->Number;
	data->RawSMS->Status = data->SMS->Status;

	error = PrepareSMS(data->SMS, data->RawSMS);
	ERROR();

	if (data->RawSMS->Length > GSM_MAX_SMS_LENGTH) {
		dprintf("SMS is too long? %d\n", data->RawSMS->Length);
		goto cleanup;
	}

	error = SM_Functions(GOP_SaveSMS, data, state);
 cleanup:
	data->RawSMS = NULL;
	return error;
}
