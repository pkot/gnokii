/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2001-2002 Markus Plail, Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2002-2003 Ladis Michl
  Copyright (C) 2003-2004 BORBELY Zoltan
  Copyright (C) 2006      Igor Popik
  
  Library for parsing and creating Short Messages (SMS).

*/

#include "config.h"

#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gnokii-internal.h"
#include "gnokii.h"

#include "sms-nokia.h"

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#undef ERROR
#define ERROR() do { if (error != GN_ERR_NONE) return error; } while (0)

struct sms_udh_data {
	unsigned int length;
	char *header;
};

#define MAX_SMS_PART 140

/* User data headers */
static struct sms_udh_data headers[] = {
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
	{ 0x06, "\x05\x04\x0b\x84\x23\xf0" }, /* WAP PUSH */
	{ 0x06, "\x05\x04\x0b\x84\x0b\x84" },
	{ 0x00, "" },
};


/**
 * sms_default - fills in SMS structure with the default values
 * @sms: pointer to a structure we need to fill in
 *
 * Default settings:
 *  - no delivery report
 *  - no Class Message
 *  - no compression
 *  - 7 bit data
 *  - message validity for 3 days
 *  - unsent status
 */
static void sms_default(gn_sms *sms)
{
	memset(sms, 0, sizeof(gn_sms));

	sms->type = GN_SMS_MT_Deliver;
	sms->delivery_report = false;
	sms->status = GN_SMS_Unsent;
	sms->validity = 4320; /* 4320 minutes == 72 hours */
	sms->dcs.type = GN_SMS_DCS_GeneralDataCoding;
	sms->dcs.u.general.compressed = false;
	sms->dcs.u.general.alphabet = GN_SMS_DCS_DefaultAlphabet;
	sms->dcs.u.general.m_class = 0;
}

GNOKII_API void gn_sms_default_submit(gn_sms *sms)
{
	sms_default(sms);
	sms->type = GN_SMS_MT_Submit;
	sms->memory_type = GN_MT_SM;
}

GNOKII_API void gn_sms_default_deliver(gn_sms *sms)
{
	sms_default(sms);
	sms->type = GN_SMS_MT_Deliver;
	sms->memory_type = GN_MT_ME;
}


/***
 *** Util functions
 ***/

/**
 * sms_timestamp_print - formats date and time in the human readable manner
 * @number: binary coded date and time
 *
 * The function converts binary date time into an easily readable form.
 * It is Y2K compliant.
 */
static char *sms_timestamp_print(u8 *number)
{
#ifdef DEBUG
#define LOCAL_DATETIME_MAX_LENGTH 26
	static char buffer[LOCAL_DATETIME_MAX_LENGTH];
	char buf[5];
	int i;

	if (!number) return NULL;

	memset(buffer, 0, LOCAL_DATETIME_MAX_LENGTH);

	/* Ugly hack, but according to the GSM specs, the year is stored
	 * as the 2 digit number. */
	if ((10*(number[0] & 0x0f) + (number[0] >> 4)) < 70)
		snprintf(buffer, sizeof(buffer), "20");
	else
		snprintf(buffer, sizeof(buffer), "19");

	for (i = 0; i < 6; i++) {
		int c;
		char buf2[4];
		switch (i) {
		case 0:
		case 1:
			c = '-';
			break;
		case 3:
		case 4:
			c = ':';
			break;
		default:
			c = ' ';
			break;
		}
		snprintf(buf2, 4, "%d%d%c", number[i] & 0x0f, number[i] >> 4, c);
		strncat(buffer, buf2, sizeof(buffer) - strlen(buffer));
	}

	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (number[6] & 0x08)
		strncat(buffer, "-", sizeof(buffer) - strlen(buffer));
	else
		strncat(buffer, "+", sizeof(buffer) - strlen(buffer));
	/* The timezone is given in quarters. The base is GMT. */
	snprintf(buf, sizeof(buf), "%02d00", (10 * (number[6] & 0x07) + (number[6] >> 4)) / 4);
	strncat(buffer, buf, sizeof(buffer) - strlen(buffer));

	return buffer;
#undef LOCAL_DATETIME_MAX_LENGTH
#else
	return NULL;
#endif /* DEBUG */
}

/**
 * sms_timestamp_unpack - converts binary datetime to the gnokii's datetime struct
 * @number: binary coded date and time
 * @dt: datetime structure to be filled
 *
 * The function fills int gnokii datetime structure basing on the given datetime
 * in the binary format. It is Y2K compliant.
 */
gn_timestamp *sms_timestamp_unpack(unsigned char *number, gn_timestamp *dt)
{
	if (!dt)
		return NULL;
	memset(dt, 0, sizeof(gn_timestamp));
	if (!number)
		return dt;

	dt->year = 10 * (number[0] & 0x0f) + (number[0] >> 4);

	/* Ugly hack, but according to the GSM specs, the year is stored
	 * as the 2 digit number. */
	if (dt->year < 70)
		dt->year += 2000;
	else
		dt->year += 1900;

	dt->month    =  10 * (number[1] & 0x0f) + (number[1] >> 4);
	dt->day      =  10 * (number[2] & 0x0f) + (number[2] >> 4);
	dt->hour     =  10 * (number[3] & 0x0f) + (number[3] >> 4);
	dt->minute   =  10 * (number[4] & 0x0f) + (number[4] >> 4);
	dt->second   =  10 * (number[5] & 0x0f) + (number[5] >> 4);

	/* The timezone is given in quarters. The base is GMT */
	dt->timezone = (10 * (number[6] & 0x07) + (number[6] >> 4)) / 4;
	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (number[6] & 0x08)
		dt->timezone = -dt->timezone;

	return dt;
}

/**
 * sms_timestamp_pack - converts gnokii's datetime struct to the binary datetime
 * @dt: datetime structure to be read
 * @number: binary variable to be filled
 *
 */
unsigned char *sms_timestamp_pack(gn_timestamp *dt, unsigned char *number)
{
	if (!number)
		return NULL;
	memset(number, 0, GN_SMS_DATETIME_MAX_LENGTH);
	if (!dt)
		return number;

	/* Ugly hack, but according to the GSM specs, the year is stored
	 * as the 2 digit number. */
	if (dt->year < 2000)
		dt->year -= 1900;
	else
		dt->year -= 2000;

	number[0]    = (dt->year   / 10) | ((dt->year   % 10) << 4);
	number[1]    = (dt->month  / 10) | ((dt->month  % 10) << 4);
	number[2]    = (dt->day    / 10) | ((dt->day    % 10) << 4);
	number[3]    = (dt->hour   / 10) | ((dt->hour   % 10) << 4);
	number[4]    = (dt->minute / 10) | ((dt->minute % 10) << 4);
	number[5]    = (dt->second / 10) | ((dt->second % 10) << 4);

	/* The timezone is given in quarters. The base is GMT */
	number[6]    = (dt->timezone / 10) | ((dt->second % 10) << 4) * 4;
	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (dt->timezone < 0)
		number[6] |= 0x08;

	return number;
}

/***
 *** DECODING SMS
 ***/

/**
 * sms_status - decodes the error code contained in the delivery report.
 * @status: error code to decode
 * @sms: SMS struct to store the result
 *
 * This function checks for the error (or success) code contained in the
 * received SMS - delivery report and fills in the SMS structure with
 * the correct message according to the GSM specification.
 * This function only applies to the delivery reports. Delivery reports
 * contain only one part, so it is assumed all other parts except 0th are
 * NULL.
 */
static gn_error sms_status(unsigned char status, gn_sms *sms)
{
	sms->user_data[0].type = GN_SMS_DATA_Text;
	sms->user_data[1].type = GN_SMS_DATA_None;
	if (status < 0x03) {
		sms->user_data[0].dr_status = GN_SMS_DR_Status_Delivered;
		snprintf(sms->user_data[0].u.text, sizeof(sms->user_data[0].u.text), "%s", _("Delivered"));
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
		snprintf(sms->user_data[0].u.text, sizeof(sms->user_data[0].u.text), "%s", _("Failed"));
		/* more detailed reason only for debug */
		if (status & 0x20) {
			dprintf("Temporary error, SC is not making any more transfer attempts\n");
			sms->user_data[0].dr_status = GN_SMS_DR_Status_Failed_Permanent;
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
				dprintf("Quality of service not available");
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
			sms->user_data[0].dr_status = GN_SMS_DR_Status_Failed_Temporary;
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
				dprintf("Quality of service not available");
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
				dprintf("Reserved/Specific to SC: 0x%02x", status);
				break;
			}
		}
	} else if (status & 0x20) {
		sms->user_data[0].dr_status = GN_SMS_DR_Status_Pending;
		snprintf(sms->user_data[0].u.text, sizeof(sms->user_data[0].u.text), "%s", _("Pending"));
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
			dprintf("Quality of service not available");
			break;
		case 0x25:
			dprintf("Error in SME");
			break;
		default:
			dprintf("Reserved/Specific to SC: 0x%02x", status);
			break;
		}
	} else {
		sms->user_data[0].dr_status = GN_SMS_DR_Status_Invalid;
		snprintf(sms->user_data[0].u.text, sizeof(sms->user_data[0].u.text), "%s", _("Unknown"));

		/* more detailed reason only for debug */
		dprintf("Reserved/Specific to SC: 0x%02x", status);
	}
	dprintf("\n");
	sms->user_data[0].length = strlen(sms->user_data[0].u.text);
	return GN_ERR_NONE;
}

/**
 * sms_data_decode - decodes encoded text from the SMS.
 * @message: encoded text
 * @output: room for the encoded text
 * @length: decoded text length
 * @size: encoded text length
 * @udhlen: udh length
 * @dcs: data coding scheme
 *
 * This function decodes either 7bit or 8bit or Unicode text to the
 * readable text format according to the locale set.
 */
static gn_error sms_data_decode(unsigned char *message, unsigned char *output, unsigned int length,
				 unsigned int size, unsigned int udhlen, gn_sms_dcs dcs)
{
	/* Unicode */
	if ((dcs.type & 0x08) == 0x08) {
		dprintf("Unicode message\n");
		char_unicode_decode(output, message, length);
	} else {
		/* 8bit SMS */
		if ((dcs.type & 0xf4) == 0xf4) {
			dprintf("8bit message\n");
			memcpy(output, message + udhlen, length);
		/* 7bit SMS */
		} else {
			char *aux;

			dprintf("Default Alphabet\n");
			length = length - (udhlen * 8 + ((7-(udhlen%7))%7)) / 7;
			aux = calloc(length + 1, 1);
			char_7bit_unpack((7-udhlen)%7, size, length, message, aux);
			char_default_alphabet_decode(output, aux, length);
			free(aux);
		}
	}
	dprintf("%s\n", output);
	return GN_ERR_NONE;
}

/**
 * sms_udh_decode - interprets the User Data Header contents
 * @message: received UDH
 * @sms: SMS structure to fill in
 *
 * At this stage we already need to know that SMS has UDH.
 * This function decodes UDH as described in:
 *   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
 *   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
 */
static gn_error sms_udh_decode(unsigned char *message, gn_sms_udh *udh)
{
	unsigned char length, pos, nr;

	udh->length = length = message[0];
	pos = 1;
	nr = 0;
	while (length > 1) {
		unsigned char udh_length;

		udh_length = message[pos+1];
		switch (message[pos]) {
		case 0x00: /* Concatenated short messages */
			dprintf("Concatenated messages\n");
			udh->udh[nr].type = GN_SMS_UDH_ConcatenatedMessages;
			udh->udh[nr].u.concatenated_short_message.reference_number = message[pos + 2];
			udh->udh[nr].u.concatenated_short_message.maximum_number   = message[pos + 3];
			udh->udh[nr].u.concatenated_short_message.current_number   = message[pos + 4];
			break;
		case 0x08: /* Concatenated short messages, 16-bit reference number */
			dprintf("Concatenated messages, 16-bit reference number\n");
			udh->udh[nr].type = GN_SMS_UDH_ConcatenatedMessages;
			udh->udh[nr].u.concatenated_short_message.reference_number = 256 * message[pos + 2] + message[pos + 3];
			udh->udh[nr].u.concatenated_short_message.maximum_number   = message[pos + 4];
			udh->udh[nr].u.concatenated_short_message.current_number   = message[pos + 5];
			break;
		case 0x01: /* Special SMS Message Indication */
			switch (message[pos + 2] & 0x03) {
			case 0x00:
				dprintf("Voice Message\n");
				udh->udh[nr].type = GN_SMS_UDH_VoiceMessage;
				break;
			case 0x01:
				dprintf("Fax Message\n");
				udh->udh[nr].type = GN_SMS_UDH_FaxMessage;
				break;
			case 0x02:
				dprintf("Email Message\n");
				udh->udh[nr].type = GN_SMS_UDH_EmailMessage;
				break;
			default:
				dprintf("Unknown\n");
				udh->udh[nr].type = GN_SMS_UDH_Unknown;
				break;
			}
			udh->udh[nr].u.special_sms_message_indication.store = (message[pos + 2] & 0x80) >> 7;
			udh->udh[nr].u.special_sms_message_indication.message_count = message[pos + 3];
			break;
		case 0x05: /* Application port addressing scheme, 16 bit address */
			switch (((0x00ff & message[pos + 2]) << 8) | (0x00ff & message[pos + 3])) {
			case 0x1581:
				dprintf("Ringtone\n");
				udh->udh[nr].type = GN_SMS_UDH_Ringtone;
				break;
			case 0x1582:
				dprintf("Operator Logo\n");
				udh->udh[nr].type = GN_SMS_UDH_OpLogo;
				break;
			case 0x1583:
				dprintf("Caller Icon\n");
				udh->udh[nr].type = GN_SMS_UDH_CallerIDLogo;
				break;
			case 0x158a:
				dprintf("Multipart Message\n");
				udh->udh[nr].type = GN_SMS_UDH_MultipartMessage;
				break;
			case 0x23f4:
				dprintf("WAP vCard\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCard;
				break;
			case 0x23f5:
				dprintf("WAP vCalendar\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCalendar;
				break;
			case 0x23f6:
				dprintf("WAP vCardSecure\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCardSecure;
				break;
			case 0x23f7:
				dprintf("WAP vCalendarSecure\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCalendarSecure;
				break;
			case 0x0b84:
				dprintf("WAP Push\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPPush;
				break;
			default:
				dprintf("Unknown\n");
				udh->udh[nr].type = GN_SMS_UDH_Unknown;
				break;
			}
			break;
		case 0x04: /* Application port addressing scheme, 8 bit address */
		case 0x06: /* SMSC Control Parameters */
		case 0x07: /* UDH Source Indicator */
		default:
			udh->udh[nr].type = GN_SMS_UDH_Unknown;
			dprintf("User Data Header type 0x%02x isn't supported\n", message[pos]);
			break;
		}
		length -= (udh_length + 2);
		pos += (udh_length + 2);
		nr++;
	}
	udh->number = nr;

	/* We need to add the length field itself */
	udh->length++;

	return GN_ERR_NONE;
}

/**
 * sms_header_decode - Decodes PDU SMS header
 * @rawsms:
 * @sms:
 * @udh:
 *
 * This function parses received SMS header information and stores
 * them in higher level SMS struct. It also checks for the UDH and when
 * it's found calls the function to extract the UDH.
 */
static gn_error sms_header_decode(gn_sms_raw *rawsms, gn_sms *sms, gn_sms_udh *udh)
{
	switch (sms->type = rawsms->type) {
	case GN_SMS_MT_Deliver:
		dprintf("Mobile Terminated message:\n");
		break;
	case GN_SMS_MT_DeliveryReport:
		dprintf("Delivery Report:\n");
		break;
	case GN_SMS_MT_Submit:
		dprintf("Mobile Originated (stored) message:\n");
		break;
	case GN_SMS_MT_StatusReport:
		dprintf("Status Report message:\n");
		break;
	case GN_SMS_MT_Picture:
		dprintf("Picture Message:\n");
		break;
	case GN_SMS_MT_TextTemplate:
		dprintf("Text Template:\n");
		break;
	case GN_SMS_MT_PictureTemplate:
		dprintf("Picture Template:\n");
		break;
	case GN_SMS_MT_SubmitSent:
		dprintf("Mobile Originated (sent) message:\n");
		break;
	default:
		dprintf("SMS message type 0x%02x isn't supported\n", sms->type);
		return GN_ERR_NOTSUPPORTED;
	}

	/* Sending date */
	sms_timestamp_unpack(rawsms->smsc_time, &(sms->smsc_time));
	dprintf("\tDate: %s\n", sms_timestamp_print(rawsms->smsc_time));

	/* Remote number */
	rawsms->remote_number[0] = (rawsms->remote_number[0] + 1) / 2 + 1;
	snprintf(sms->remote.number, sizeof(sms->remote.number), "%s", char_bcd_number_get(rawsms->remote_number));
	dprintf("\tRemote number (recipient or sender): %s\n", sms->remote.number);

	/* Short Message Center */
	snprintf(sms->smsc.number, sizeof(sms->smsc.number), "%s", char_bcd_number_get(rawsms->message_center));
	dprintf("\tSMS center number: %s\n", sms->smsc.number);

	/* Delivery time */
	if (sms->type == GN_SMS_MT_StatusReport) {
		sms_timestamp_unpack(rawsms->time, &(sms->time));
		dprintf("\tDelivery date: %s\n", sms_timestamp_print(rawsms->time));
	}

	/* Data Coding Scheme */
	sms->dcs.type = rawsms->dcs;

	/* User Data Header */
	if (rawsms->udh_indicator & 0x40) { /* UDH header available */
		dprintf("UDH found\n");
		return sms_udh_decode(rawsms->user_data, udh);
	}

	return GN_ERR_NONE;
}

/**
 * sms_pdu_decode - This function decodes the PDU SMS
 * @rawsms - SMS read by the phone driver
 * @sms - place to store the decoded message
 *
 * This function decodes SMS as described in GSM 03.40 version 6.1.0
 * Release 1997, section 9
 */
static gn_error sms_pdu_decode(gn_sms_raw *rawsms, gn_sms *sms)
{
	unsigned int size = 0;
	gn_error error;

	error = sms_header_decode(rawsms, sms, &sms->udh);
	ERROR();
	sms->user_data[0].dr_status = GN_SMS_DR_Status_None;
	switch (sms->type) {
	case GN_SMS_MT_DeliveryReport:
	case GN_SMS_MT_StatusReport:
		sms_status(rawsms->report_status, sms);
		break;
	case GN_SMS_MT_PictureTemplate:
	case GN_SMS_MT_Picture:
		/* This is incredible. Nokia violates it's own format in 6210 */
		/* Indicate that it is Multipart Message. Remove it if not needed */
		/* [I believe Nokia said in their manuals that any order is permitted --pavel] */
		sms->udh.number = 1;
		sms->udh.udh[0].type = GN_SMS_UDH_MultipartMessage;
		if ((rawsms->user_data[0] == 0x48) && (rawsms->user_data[1] == 0x1c)) {

			dprintf("First picture then text!\n");

			/* First part is a Picture */
			sms->user_data[0].type = GN_SMS_DATA_Bitmap;
			gn_bmp_sms_read(GN_BMP_PictureMessage, rawsms->user_data,
					NULL, &sms->user_data[0].u.bitmap);
#ifdef DEBUG
			gn_bmp_print(&sms->user_data[0].u.bitmap, stderr);
#endif
			size = rawsms->user_data_length - 4 - sms->user_data[0].u.bitmap.size;
			/* Second part is a text */
			sms->user_data[1].type = GN_SMS_DATA_NokiaText;
			sms_data_decode(rawsms->user_data + 5 + sms->user_data[0].u.bitmap.size,
					(unsigned char *)&(sms->user_data[1].u.text),
					rawsms->length - sms->user_data[0].u.bitmap.size - 4,
					size, 0, sms->dcs);
		} else {

			dprintf("First text then picture!\n");

			/* First part is a text */
			sms->user_data[1].type = GN_SMS_DATA_NokiaText;
			sms_data_decode(rawsms->user_data + 3,
					(unsigned char *)&(sms->user_data[1].u.text),
					rawsms->user_data[1], rawsms->user_data[0], 0, sms->dcs);

			/* Second part is a Picture */
			sms->user_data[0].type = GN_SMS_DATA_Bitmap;
			gn_bmp_sms_read(GN_BMP_PictureMessage,
					rawsms->user_data + rawsms->user_data[0] + 7,
					NULL, &sms->user_data[0].u.bitmap);
#ifdef DEBUG
			gn_bmp_print(&sms->user_data[0].u.bitmap, stderr);
#endif
		}
		break;
	/* Plain text message */
	default:
		return sms_data_decode(rawsms->user_data + sms->udh.length, /* Skip the UDH */
				(unsigned char *)&sms->user_data[0].u.text, /* With a plain text message we have only 1 part */
				rawsms->length,                             /* Length of the decoded text */
				rawsms->user_data_length,                   /* Length of the encoded text (in full octets) without UDH */
				sms->udh.length,                            /* To skip the certain number of bits when unpacking 7bit message */
				sms->dcs);
		break;
	}

	return GN_ERR_NONE;
}

/**
 * gn_sms_parse - High-level function for the SMS parsing
 * @data: GSM data from the phone driver
 *
 * This function parses the SMS message from the lowlevel raw_sms to
 * the highlevel SMS. In data->raw_sms there's SMS read by the phone
 * driver, data->sms is the place for the parsed SMS.
 */
gn_error gn_sms_parse(gn_data *data)
{
	if (!data->raw_sms || !data->sms) return GN_ERR_INTERNALERROR;
	/* Let's assume at the moment that all messages are PDU coded */
	return sms_pdu_decode(data->raw_sms, data->sms);
}

/**
 * gn_sms_pdu2raw - Copy PDU data into gn_sms_raw structure
 * @rawsms: gn_sms_raw structure to be filled
 * @pdu: buffer containing PDU data
 * @pdu_len: length of buffer containing PDU data
 * @flags: GN_SMS_PDU_* flags to control copying
 *
 * This function fills a gn_sms_raw structure copying raw values from
 * a buffer containing PDU data.
 */
gn_error gn_sms_pdu2raw(gn_sms_raw *rawsms, unsigned char *pdu, int pdu_len, int flags)
{
#define COPY_REMOTE_NUMBER(pdu, offset) { \
	l = (pdu[offset] % 2) ? pdu[offset] + 1 : pdu[offset]; \
	l = l / 2 + 2; \
	if (l + offset + extraoffset > pdu_len || l > GN_SMS_NUMBER_MAX_LENGTH) { \
		dprintf("Invalid remote number length (%d)\n", l); \
		ret = GN_ERR_INTERNALERROR; \
		goto out; \
	} \
	memcpy(rawsms->remote_number, pdu + offset, l); \
	offset += l; \
}

#define COPY_USER_DATA(pdu, offset) { \
	rawsms->length = pdu[offset++]; \
	rawsms->user_data_length = rawsms->length; \
	if (rawsms->length > 0) { \
		if (rawsms->udh_indicator) \
			rawsms->user_data_length -= pdu[offset] + 1; \
		if (pdu_len - offset > GN_SMS_USER_DATA_MAX_LENGTH) { \
			dprintf("Phone gave as poisonous (too short?) reply, either phone went crazy or communication went out of sync\n"); \
			ret = GN_ERR_INTERNALERROR; \
			goto out; \
		} \
		memcpy(rawsms->user_data, pdu + offset, pdu_len - offset); \
	} \
}

	gn_error ret = GN_ERR_NONE;
	unsigned int l, extraoffset, offset = 0;
	int parameter_indicator;

	if (!flags & GN_SMS_PDU_NOSMSC) {
		l = pdu[offset] + 1;
		if (l > pdu_len || l > GN_SMS_SMSC_NUMBER_MAX_LENGTH) {
			dprintf("Invalid message center length (%d)\n", l);
			ret = GN_ERR_INTERNALERROR;
			goto out;
		}
		memcpy(rawsms->message_center, pdu, l);
		offset += l;
	}

	rawsms->reject_duplicates   = 0;
	rawsms->report_status       = 0;
	rawsms->reference           = 0;
	rawsms->reply_via_same_smsc = 0;
	rawsms->report              = 0;

	/* NOTE: offsets are taken from subclause 9.2.3 "Definition of the TPDU parameters"
	   of 3GPP TS 23.040 version 7.0.1 Release 7 == ETSI TS 123 040 V7.0.1 (2007-03)
	   *not* from the tables in subclause 9.2.2 "PDU Type repertoire at SM-TL" which ignore unused bits
	*/

	/* NOTE: this function handles only short messages received by the phone.
	   Since the same TP-MTI value is used both for SC->MS and for MS->SC,
	   it can't simply shift to get the enum value: (pdu[offset] & 0x03) << 1
	*/

	/* TP-MTI TP-Message-Type-Indicator 9.2.3.1 */
	switch (pdu[offset] & 0x03) {
	case 0x00:
		rawsms->type = GN_SMS_MT_Deliver;
		break;
	case 0x01:
		rawsms->type = GN_SMS_MT_Submit;
		break;
	case 0x02:
		rawsms->type = GN_SMS_MT_StatusReport;
		break;
	case 0x03:
		dprintf("Reserved TP-MTI found\n");
		return GN_ERR_INTERNALERROR;
	}
	switch (rawsms->type) {
	case GN_SMS_MT_Deliver:
		dprintf("SMS-DELIVER found\n");
		/* TP-MMS  TP-More-Messages-to-Send */
		rawsms->more_messages       = pdu[offset] & 0x04;
		/* Bits 3 and 4 of the first octet are unused */
		/* TP-SRI  TP-Status-Report-Indication */
		rawsms->report_status       = pdu[offset] & 0x20;
		/* TP-UDHI TP-User-Data-Header-Indication */
		rawsms->udh_indicator       = pdu[offset] & 0x40;
		/* TP-RP   TP-Reply-Path */
		rawsms->reply_via_same_smsc = pdu[offset] & 0x80;
		offset++;
		/* TP-OA   TP-Originating-Address */
		extraoffset = 10;
		COPY_REMOTE_NUMBER(pdu, offset);
		/* TP-PID  TP-Protocol-Identifier */
		rawsms->pid = pdu[offset++];
		/* TP-DCS  TP-Data-Coding-Scheme */
		rawsms->dcs = pdu[offset++];
		/* TP-SCTS TP-Service-Centre-Time-Stamp */
		memcpy(rawsms->smsc_time, pdu + offset, 7);
		offset += 7;
		/* TP-UDL  TP-User-Data-Length */
		/* TP-UD   TP-User-Data */
		COPY_USER_DATA(pdu, offset);
		break;
	case GN_SMS_MT_Submit:
		dprintf("SMS-SUBMIT found\n");
		/* TP-RD   TP-Reject-Duplicates */
		rawsms->reject_duplicates   = pdu[offset] & 0x04;
		/* TP-VPF  TP-Validity-Period-Format */
		rawsms->validity_indicator  = (pdu[offset] & 0x18) >> 3;
		/* TP-SRR  TP-Status-Report-Request */
		rawsms->report              = pdu[offset] & 0x20;
		/* TP-UDHI TP-User-Data-Header-Indication */
		rawsms->udh_indicator       = pdu[offset] & 0x40;
		/* TP-RP   TP-Reply-Path */
		rawsms->reply_via_same_smsc = pdu[offset] & 0x80;
		offset++;
		/* TP-MR   TP-Message-Reference */
		rawsms->reference           = pdu[offset++];
		/* TP-DA   TP-Destination-Address */
		extraoffset = 3;
		COPY_REMOTE_NUMBER(pdu, offset);
		/* TP-PID  TP-Protocol-Identifier */
		rawsms->pid = pdu[offset++];
		/* TP-DCS  TP-Data-Coding-Scheme */
		rawsms->dcs = pdu[offset++];
		/* TP-VP   TP-Validity-Period */
		switch (rawsms->validity_indicator) {
		case GN_SMS_VP_None:
			/* nothing to convert */
			break;
		case GN_SMS_VP_RelativeFormat:
			rawsms->validity[0] = pdu[offset++];
			break;
		case GN_SMS_VP_EnhancedFormat:
			/* FALL THROUGH */
		case GN_SMS_VP_AbsoluteFormat:
			memcpy(rawsms->validity, pdu + offset, 7);
			offset += 7;
			break;
		default:
			dprintf("Unknown validity_indicator 0x%02x\n", rawsms->validity_indicator);
			return GN_ERR_INTERNALERROR;
		}
		/* TP-UDL  TP-User-Data-Length */
		/* TP-UD   TP-User-Data */
		COPY_USER_DATA(pdu, offset);
		break;
	case GN_SMS_MT_StatusReport:
		dprintf("SMS-STATUS-REPORT found\n");
		/* TP-MMS  TP-More-Messages-to-Send */
		rawsms->more_messages       = pdu[offset] & 0x04;
		/* TP-SRQ  TP-Status-Report-Qualifier */
		rawsms->report              = pdu[offset] & 0x10;
		/* TP-UDHI TP-User-Data-Header-Indication */
		rawsms->udh_indicator       = pdu[offset] & 0x40;
		offset++;
		/* TP-MR   TP-Message-Reference */
		rawsms->reference           = pdu[offset++];
		/* TP-RA   TP-Recipient-Address */
		extraoffset = 0;
		COPY_REMOTE_NUMBER(pdu, offset);
		/* TP-SCTS TP-Service-Centre-Time-Stamp */
		memcpy(rawsms->smsc_time, pdu + offset, 7);
		offset += 7;
		/* TP-DT   TP-Discharge-Time */
		memcpy(rawsms->time, pdu + offset, 7);
		offset += 7;
		/* TP-ST   TP-Status */
		rawsms->report_status       = pdu[offset++];
		/* TP-PI   TP-Parameter-Indicator */
		parameter_indicator         = pdu[offset];
		/* handle the "extension bit" skipping the following octects, if any (see 9.2.3.27 TP-Parameter-Indicator):
		 *   The most significant bit in octet 1 and any other TP-PI octets which may be added later is reserved as an extension bit
		 *   which when set to a 1 shall indicate that another TP-PI octet follows immediately afterwards.
		 */
		while ((offset < pdu_len) && (pdu[offset++] & 0x80))
			;
		if ((offset < pdu_len) && (parameter_indicator & 0x01)) {
			/* TP-PID  TP-Protocol-Identifier */
			rawsms->pid = pdu[offset++];
		}
		if ((offset < pdu_len) && (parameter_indicator & 0x02)) {
			/* TP-DCS  TP-Data-Coding-Scheme */
			rawsms->dcs = pdu[offset++];
		}
		if ((offset < pdu_len) && (parameter_indicator & 0x04)) {
			/* TP-UDL  TP-User-Data-Length */
			/* TP-UD   TP-User-Data */
			COPY_USER_DATA(pdu, offset);
		}
		break;
	case GN_SMS_MT_Command:
#if 0
		dprintf("SMS-COMMAND found\n");
		/* TP-Message-Type-Indicator */
		/** decoded above **/
		offset = 5;
		/* TP-User-Data-Header-Indication */
		rawsms->udh_indicator       = pdu[offset] & 0x40;
		/* TP-Status-Report-Request */
		rawsms->report_status       = pdu[offset] & 0x08;
		offset++;
		/* TP-Message Reference */
		rawsms->reference           = pdu[offset++];
		/* TP-Protocol-Identifier */
		rawsms->pid                 = pdu[offset++];
		/* TP-Command-Type */
		dprintf("TP-Command-Type 0x%02x\n", pdu[offset++]);
		/* TP-Message-Number */
		dprintf("TP-Message-Number 0x%02x\n", pdu[offset++]);
		/* TP-Destination-Address */
	l = (pdu[offset] % 2) ? pdu[offset] + 1 : pdu[offset];
	l = l / 2 + 2;
	memcpy(rawsms->remote_number, pdu + offset, l);
	offset += l;
		/* TP-Command-Data-Length */
		dprintf("TP-Command-Data-Length 0x%02x\n", pdu[offset++]);
		/* TP-Command-Data */
		dprintf("TP-Command-Data 0x%02x ...\n", pdu[offset++]);
		goto out;
		break;
#endif
		/* FALL THROUGH */
	default:
		dprintf("Unknown PDU type %d\n", rawsms->type);
		return GN_ERR_INTERNALERROR;
	}

out:
	return ret;
#undef COPY_USER_DATA
#undef COPY_REMOTE_NUMBER
}

/**
 * gn_sms_request - High-level function for the explicit SMS reading from the phone
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This is the function for explicit requesting the SMS from the
 * phone driver. Note that raw_sms field in the gn_data structure must
 * be initialized
 */
gn_error gn_sms_request(gn_data *data, struct gn_statemachine *state)
{
	if (!data->raw_sms) return GN_ERR_INTERNALERROR;
	return gn_sm_functions(GN_OP_GetSMS, data, state);
}

/**
 * gn_sms_get- High-level function for reading SMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frontend for reading SMS. Note that SMS field
 * in the gn_data structure must be initialized.
 */
GNOKII_API gn_error gn_sms_get(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_sms_raw rawsms;

	if (!data->sms)
		return GN_ERR_INTERNALERROR;
	if (data->sms->number < 0)
		return GN_ERR_EMPTYLOCATION;
	if (data->sms->memory_type > GN_MT_LAST)
		return GN_ERR_INVALIDMEMORYTYPE;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	error = gn_sms_request(data, state);
	ERROR();
	data->sms->status = rawsms.status;
	return gn_sms_parse(data);
}

/**
 * gn_sms_delete - High-level function for deleting SMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frontend for deleting SMS. Note that SMS field
 * in the gn_data structure must be initialized.
 */
GNOKII_API gn_error gn_sms_delete(gn_data *data, struct gn_statemachine *state)
{
	gn_sms_raw rawsms;

	if (!data->sms) return GN_ERR_INTERNALERROR;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	return gn_sm_functions(GN_OP_DeleteSMS, data, state);
}

/***
 *** OTHER FUNCTIONS
 ***/

static gn_error sms_request_no_validate(gn_data *data, struct gn_statemachine *state)
{
	return gn_sm_functions(GN_OP_GetSMSnoValidate, data, state);
}

GNOKII_API gn_error gn_sms_get_no_validate(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_sms_raw rawsms;

	if (!data->sms) return GN_ERR_INTERNALERROR;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	error = sms_request_no_validate(data, state);
	ERROR();
	data->sms->status = rawsms.status;
	return gn_sms_parse(data);
}

GNOKII_API gn_error gn_sms_delete_no_validate(gn_data *data, struct gn_statemachine *state)
{
	gn_sms_raw rawsms;

	if (!data->sms) return GN_ERR_INTERNALERROR;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	return gn_sm_functions(GN_OP_DeleteSMSnoValidate, data, state);
}

static gn_error sms_free_deleted(gn_data *data, int folder)
{
	int i, j;

	if (!data->sms_status) return GN_ERR_INTERNALERROR;

	for (i = 0; i < data->folder_stats[folder]->used; i++) {		/* for all previously found locations */
		if (data->message_list[i][folder]->status == GN_SMS_FLD_ToBeRemoved) {	/* previously deleted and read message */
			dprintf("Found deleted message, which will now be freed! %i , %i\n",
					i, data->message_list[i][folder]->location);
			for (j = i; j < data->folder_stats[folder]->used; j++) {
				memcpy(data->message_list[j][folder], data->message_list[j + 1][folder],
						sizeof(gn_sms_message_list));
			}
			data->folder_stats[folder]->used--;
			i--;
		}
	}
	return GN_ERR_NONE;
}

static gn_error sms_get_read(gn_data *data)
{
	int i, j, found;

	if (!data->message_list || !data->folder_stats || !data->sms_folder) return GN_ERR_INTERNALERROR;

	for (i = 0; i < data->sms_folder->number; i++) {		/* cycle through all messages in phone */
		found = 0;
		for (j = 0; j < data->folder_stats[data->sms_folder->folder_id]->used; j++) {		/* and compare them to those alread in list */
			if (data->sms_folder->locations[i] == data->message_list[j][data->sms_folder->folder_id]->location) found = 1;
		}
		if (data->folder_stats[data->sms_folder->folder_id]->used >= GN_SMS_MESSAGE_MAX_NUMBER) {
			dprintf("Max messages number in folder exceeded (%d)\n", GN_SMS_MESSAGE_MAX_NUMBER);
			return GN_ERR_MEMORYFULL;
		}
		if (!found) {
			dprintf("Found new (read) message. Will store it at #%i!\n", data->folder_stats[data->sms_folder->folder_id]->used);
			dprintf("%i\n", data->sms_folder->locations[i]);
			data->message_list[data->folder_stats[data->sms_folder->folder_id]->used][data->sms_folder->folder_id]->location =
				data->sms_folder->locations[i];
			data->message_list[data->folder_stats[data->sms_folder->folder_id]->used][data->sms_folder->folder_id]->status = GN_SMS_FLD_New;
			data->folder_stats[data->sms_folder->folder_id]->used++;
			data->folder_stats[data->sms_folder->folder_id]->changed++;
			data->sms_status->changed++;
		}
	}
	return GN_ERR_NONE;
}

static gn_error sms_get_deleted(gn_data *data)
{
	int i, j, found = 0;

	for (i = 0; i < data->folder_stats[data->sms_folder->folder_id]->used; i++) {		/* for all previously found locations in folder */
		found = 0;

		for (j = 0; j < data->sms_folder->number; j++) {	/* see if there is a corresponding message in phone */
			if (data->message_list[i][data->sms_folder->folder_id]->location == data->sms_folder->locations[j]) found = 1;
		}
		if ((found == 0) && (data->message_list[i][data->sms_folder->folder_id]->status == GN_SMS_FLD_Old)) {	/* we have found a deleted message */
			dprintf("found a deleted message!!!! i: %i, loc: %i, MT: %i \n",
					i, data->message_list[i][data->sms_folder->folder_id]->location, data->sms_folder->folder_id);

			data->message_list[i][data->sms_folder->folder_id]->status = GN_SMS_FLD_Deleted;
			data->sms_status->changed++;
			data->folder_stats[data->sms_folder->folder_id]->changed++;
		}
	}
	return GN_ERR_NONE;
}

static gn_error sms_verify_status(gn_data *data)
{
	int i, j, found = 0;

	for (i = 0; i < data->folder_stats[data->sms_folder->folder_id]->used; i++) {		/* Cycle through all messages we know of */
		found = 0;
		if ((data->message_list[i][data->sms_folder->folder_id]->status == GN_SMS_FLD_NotRead) ||	/* if it is a unread one, i.e. not in folderstatus */
				(data->message_list[i][data->sms_folder->folder_id]->status == GN_SMS_FLD_NotReadHandled)) {
			for (j = 0; j < data->sms_folder->number; j++) {
				if (data->message_list[i][data->sms_folder->folder_id]->location == data->sms_folder->locations[j]) {
					/* We have found a formerly unread message which has been read in the meantime */
					dprintf("Found a formerly unread message which has been read in the meantime: loc: %i\n",
							data->message_list[i][data->sms_folder->folder_id]->location);
					data->message_list[i][data->sms_folder->folder_id]->status = GN_SMS_FLD_Changed;
					data->sms_status->changed++;
					data->folder_stats[data->sms_folder->folder_id]->changed++;
				}
			}
		}
	}
	return GN_ERR_NONE;
}


GNOKII_API gn_error gn_sms_get_folder_changes(gn_data *data, struct gn_statemachine *state, int has_folders)
{
	gn_error error;
	gn_sms_folder  sms_folder;
	gn_sms_folder_list sms_folder_list;
	int i, previous_unread, previous_total;

	previous_total = data->sms_status->number;
	previous_unread = data->sms_status->unread;
	dprintf("GetFolderChanges: Old status: %d %d\n", data->sms_status->number, data->sms_status->unread);

	error = gn_sm_functions(GN_OP_GetSMSStatus, data, state);	/* Check overall SMS Status */
	ERROR();
	dprintf("GetFolderChanges: Status: %d %d\n", data->sms_status->number, data->sms_status->unread);

	if (!has_folders) {
		if ((previous_total == data->sms_status->number) && (previous_unread == data->sms_status->unread))
			data->sms_status->changed = 0;
		else
			data->sms_status->changed = 1;
		return GN_ERR_NONE;
	}

	data->sms_folder_list = &sms_folder_list;
	error = gn_sm_functions(GN_OP_GetSMSFolders, data, state);
	ERROR();

	data->sms_status->folders_count = data->sms_folder_list->number;

	for (i = 0; i < data->sms_status->folders_count; i++) {
		dprintf("GetFolderChanges: Freeing deleted messages for folder #%i\n", i);
		error = sms_free_deleted(data, i);
		ERROR();

		data->sms_folder = &sms_folder;
		data->sms_folder->folder_id = (gn_memory_type) i + 12;
		dprintf("GetFolderChanges: Getting folder status for folder #%i\n", i);
		error = gn_sm_functions(GN_OP_GetSMSFolderStatus, data, state);
		ERROR();

		data->sms_folder->folder_id = i;	/* so we don't need to do a modulo 8 each time */
			
		dprintf("GetFolderChanges: Reading read messages (%i) for folder #%i\n", data->sms_folder->number, i);
		error = sms_get_read(data);
		ERROR();

		dprintf("GetFolderChanges: Getting deleted messages for folder #%i\n", i);
		error = sms_get_deleted(data);
		ERROR();

		dprintf("GetFolderChanges: Verifying messages for folder #%i\n", i);
		error = sms_verify_status(data);
		ERROR();
	}
	return GN_ERR_NONE;
}

/***
 *** ENCODING SMS
 ***/


/**
 * sms_udh_encode - encodes User Data Header
 * @sms: SMS structure with the data source
 * @type:
 *
 * returns pointer to data it added;
 *
 * This function encodes the UserDataHeader as described in:
 *  o GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
 *  o Smart Messaging Specification, Revision 1.0.0, September 15, 1997
 *  o Smart Messaging Specification, Revision 3.0.0
 */
static char *sms_udh_encode(gn_sms_raw *rawsms, int type)
{
	unsigned char pos;
	char *udh = rawsms->user_data;
	char *res = NULL;

	pos = udh[0];

	switch (type) {
	case GN_SMS_UDH_None:
		break;
	case GN_SMS_UDH_VoiceMessage:
	case GN_SMS_UDH_FaxMessage:
	case GN_SMS_UDH_EmailMessage:
		return NULL;
#if 0
		udh[pos+4] = udhi.u.SpecialSMSMessageIndication.MessageCount;
		if (udhi.u.SpecialSMSMessageIndication.Store) udh[pos+3] |= 0x80;
#endif
	case GN_SMS_UDH_ConcatenatedMessages:
		dprintf("Adding ConcatMsg header\n");
	case GN_SMS_UDH_OpLogo:
	case GN_SMS_UDH_CallerIDLogo:
	case GN_SMS_UDH_Ringtone:
	case GN_SMS_UDH_MultipartMessage:
	case GN_SMS_UDH_WAPPush:
		udh[0] += headers[type].length;
		res = udh+pos+1;
		memcpy(res, headers[type].header, headers[type].length);
		rawsms->user_data_length += headers[type].length;
		rawsms->length += headers[type].length;
		break;
	default:
		dprintf("User Data Header type 0x%02x isn't supported\n", type);
		break;
	}
	if (!rawsms->udh_indicator) {
		rawsms->udh_indicator = 1;
		rawsms->length++;		/* Length takes one byte, too */
		rawsms->user_data_length++;
	}
	return res;
}

/**
 * sms_concat_header_encode - Adds concatenated messages header
 * @rawsms: processed SMS
 * @curr: current part number
 * @total: total parts number
 *
 * This function adds sequent part of the concatenated messages header. Note
 * that this header should be the first of all headers.
 */
static gn_error sms_concat_header_encode(gn_sms_raw *rawsms, int curr, int total)
{
	char *header = sms_udh_encode(rawsms, GN_SMS_UDH_ConcatenatedMessages);
	if (!header) return GN_ERR_NOTSUPPORTED;
	header[2] = 0xce;		/* Message serial number. Is 0xce value somehow special? -- pkot */
	header[3] = total;
	header[4] = curr;
	return GN_ERR_NONE;
}

/**
 * sms_data_encode - Encodes the data from the SMS structure to the phone frame
 *
 * @sms: #gn_sms structure holding user provided data
 * @rawsms: #gn_sms_raw structure to be filled with encoded data
 *
 * This function converts values from libgnokii format to a format more similar
 * to the GSM standard.
 * This function is capable of creating only one frame at a time.
 */
static gn_error sms_data_encode(gn_sms *sms, gn_sms_raw *rawsms)
{
	gn_sms_dcs_alphabet_type al = GN_SMS_DCS_DefaultAlphabet;
	unsigned int i, size = 0;
	gn_error error;

	/* Additional Headers */
	switch (sms->dcs.type) {
	case GN_SMS_DCS_GeneralDataCoding:
		dprintf("General Data Coding\n");
		switch (sms->dcs.u.general.m_class) {
		case 0: break;
		case 1: rawsms->dcs |= 0xf0; break; /* Class 0 */
		case 2: rawsms->dcs |= 0xf1; break; /* Class 1 */
		case 3: rawsms->dcs |= 0xf2; break; /* Class 2 */
		case 4: rawsms->dcs |= 0xf3; break; /* Class 3 */
		default: dprintf("General Data Coding class 0x%02x isn't supported\n", sms->dcs.u.general.m_class); break;
		}
		if (sms->dcs.u.general.compressed) {
			/* Compression not supported yet */
			dprintf("SMS message compression isn't supported\n");
			/* dcs[0] |= 0x20; */
		}
		al = sms->dcs.u.general.alphabet;
		break;
	case GN_SMS_DCS_MessageWaiting:
		al = sms->dcs.u.message_waiting.alphabet;
		if (sms->dcs.u.message_waiting.discard) rawsms->dcs |= 0xc0;
		else if (sms->dcs.u.message_waiting.alphabet == GN_SMS_DCS_UCS2) rawsms->dcs |= 0xe0;
		else rawsms->dcs |= 0xd0;

		if (sms->dcs.u.message_waiting.active) rawsms->dcs |= 0x08;
		rawsms->dcs |= (sms->dcs.u.message_waiting.type & 0x03);

		break;
	default:
		dprintf("Data Coding Scheme type 0x%02x isn't supported\n", sms->dcs.type);
		return GN_ERR_WRONGDATAFORMAT;
	}

	for (i = 0; i < GN_SMS_PART_MAX_NUMBER; i++) {
		switch (sms->user_data[i].type) {
		case GN_SMS_DATA_Bitmap:
			switch (sms->user_data[i].u.bitmap.type) {
			case GN_BMP_PictureMessage:
				size = sms_nokia_bitmap_encode(&(sms->user_data[i].u.bitmap),
							       rawsms->user_data + rawsms->user_data_length,
							       (i == 0));
				break;
			case GN_BMP_OperatorLogo:
				if (!sms_udh_encode(rawsms, GN_SMS_UDH_OpLogo)) return GN_ERR_NOTSUPPORTED;
				size = gn_bmp_sms_encode(&(sms->user_data[i].u.bitmap),
							 rawsms->user_data + rawsms->user_data_length);
				break;
			case GN_BMP_CallerLogo:
				if (!sms_udh_encode(rawsms, GN_SMS_UDH_CallerIDLogo)) return GN_ERR_NOTSUPPORTED;
				size = gn_bmp_sms_encode(&(sms->user_data[i].u.bitmap),
							 rawsms->user_data + rawsms->user_data_length);
				break;
			default:
				size = gn_bmp_sms_encode(&(sms->user_data[i].u.bitmap),
							 rawsms->user_data + rawsms->user_data_length);
				break;
			}
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			rawsms->udh_indicator = 1;
			break;

		case GN_SMS_DATA_Animation: {
			int j;

			for (j = 0; j < 4; j++) {
				size = gn_bmp_sms_encode(&(sms->user_data[i].u.animation[j]), rawsms->user_data + rawsms->user_data_length);
				rawsms->length += size;
				rawsms->user_data_length += size;
			}
			rawsms->dcs = 0xf5;
			rawsms->udh_indicator = 1;
			break;
		}

		case GN_SMS_DATA_Text: {
			unsigned int length, udh_length, offset = rawsms->user_data_length;

			length = strlen(sms->user_data[i].u.text);
			if (sms->udh.length)
				udh_length = sms->udh.length + 1;
			else
				udh_length = 0;
			switch (al) {
			case GN_SMS_DCS_DefaultAlphabet:
				dprintf("Default Alphabet\n");
				size = char_7bit_pack((7 - (udh_length % 7)) % 7,
						      sms->user_data[i].u.text,
						      rawsms->user_data + offset,
						      &length);
				rawsms->length = length + (udh_length * 8 + 6) / 7;
				rawsms->user_data_length = size + offset;
				dprintf("\tencoded size: %d\n\trawsms length: %d\n\trawsms user data length: %d\n", size, rawsms->length, rawsms->user_data_length);
				break;
			case GN_SMS_DCS_8bit:
				dprintf("8bit\n");
				rawsms->dcs |= 0xf4;
				memcpy(rawsms->user_data + offset, sms->user_data[i].u.text, sms->user_data[i].u.text[0]);
				rawsms->user_data_length = rawsms->length = length + udh_length;
				break;
			case GN_SMS_DCS_UCS2:
				dprintf("UCS-2\n");
				rawsms->dcs |= 0x08;
				length = ucs2_encode(rawsms->user_data + offset, GN_SMS_LONG_MAX_LENGTH, sms->user_data[i].u.text, length);
				rawsms->user_data_length = rawsms->length = length + udh_length;
				break;
			default:
				return GN_ERR_WRONGDATAFORMAT;
			}
			break;
		}

		case GN_SMS_DATA_NokiaText:
			size = sms_nokia_text_encode(sms->user_data[i].u.text,
						     rawsms->user_data + rawsms->user_data_length,
						     (i == 0));
			rawsms->length += size;
			rawsms->user_data_length += size;
			break;

		case GN_SMS_DATA_iMelody:
			size = imelody_sms_encode(sms->user_data[i].u.text, rawsms->user_data + rawsms->user_data_length);
			dprintf("Imelody, size %d\n", size);
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			rawsms->udh_indicator = 1;
			break;

		case GN_SMS_DATA_Multi:
			size = sms->user_data[0].length;
			if (!sms_udh_encode(rawsms, GN_SMS_UDH_MultipartMessage)) return GN_ERR_NOTSUPPORTED;
			error = sms_concat_header_encode(rawsms, sms->user_data[i].u.multi.curr, sms->user_data[i].u.multi.total);
			ERROR();
			memcpy(rawsms->user_data + rawsms->user_data_length, sms->user_data[i].u.multi.binary, MAX_SMS_PART - 6);
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			break;

		case GN_SMS_DATA_Ringtone:
			if (!sms_udh_encode(rawsms, GN_SMS_UDH_Ringtone)) return GN_ERR_NOTSUPPORTED;
			size = ringtone_sms_encode(rawsms->user_data + rawsms->length, &sms->user_data[i].u.ringtone);
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			break;

		case GN_SMS_DATA_WAPPush:
			if (!sms_udh_encode(rawsms, GN_SMS_UDH_WAPPush)) return GN_ERR_NOTSUPPORTED;
			size = sms->user_data[i].length;
			memcpy(rawsms->user_data + rawsms->user_data_length, sms->user_data[i].u.text, size );
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			break;

		case GN_SMS_DATA_Concat:
			al = GN_SMS_DCS_8bit;
			rawsms->dcs = 0xf5;
			sms_concat_header_encode(rawsms, sms->user_data[i].u.concat.curr, sms->user_data[i].u.concat.total);
			break;

		case GN_SMS_DATA_None:
			return GN_ERR_NONE;

		default:
			dprintf("User Data type 0x%02x isn't supported\n", sms->user_data[i].type);
			break;
		}
	}
	return GN_ERR_NONE;
}

gn_error sms_prepare(gn_sms *sms, gn_sms_raw *rawsms)
{
	int i;

	switch (rawsms->type = sms->type) {
	case GN_SMS_MT_Submit:
	case GN_SMS_MT_Deliver:
	case GN_SMS_MT_Picture:
		break;
	case GN_SMS_MT_DeliveryReport:
	default:
		dprintf("Raw SMS message type 0x%02x isn't supported\n", rawsms->type);
		return GN_ERR_NOTSUPPORTED;
	}
	/* Encoding the header */
	rawsms->report = sms->delivery_report;
	rawsms->remote_number[0] = char_semi_octet_pack(sms->remote.number, rawsms->remote_number + 1, sms->remote.type);
	if (rawsms->remote_number[0] > GN_SMS_NUMBER_MAX_LENGTH) {
		dprintf("Remote number length %d > %d\n", rawsms->remote_number[0], GN_SMS_NUMBER_MAX_LENGTH);
		return GN_ERR_ENTRYTOOLONG;
	}
	rawsms->validity_indicator = GN_SMS_VP_RelativeFormat;
	rawsms->validity[0] = GN_SMS_VP_72H;

	for (i = 0; i < sms->udh.number; i++)
		if (sms->udh.udh[i].type == GN_SMS_UDH_ConcatenatedMessages)
			sms_concat_header_encode(rawsms, sms->udh.udh[i].u.concatenated_short_message.current_number,
						sms->udh.udh[i].u.concatenated_short_message.maximum_number);

	return sms_data_encode(sms, rawsms);
}

static void sms_dump_raw(gn_sms_raw *rawsms)
{
#ifdef DEBUG
	char buf[10240];

	memset(buf, 0, 10240);

	dprintf("dcs: 0x%02x\n", rawsms->dcs);
	dprintf("Length: 0x%02x\n", rawsms->length);
	dprintf("user_data_length: 0x%02x\n", rawsms->user_data_length);
	dprintf("ValidityIndicator: %d\n", rawsms->validity_indicator);
	bin2hex(buf, rawsms->user_data, rawsms->user_data_length);
	dprintf("user_data: %s\n", buf);
#endif
}

static gn_error sms_send_long(gn_data *data, struct gn_statemachine *state);

/**
 * gn_sms_send - The main function for the SMS sending
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * The high level function to send SMS. You need to fill in data->sms
 * (gn_sms) in the higher level. This is converted to raw_sms here,
 * and then phone driver takes the fields it needs and sends it in the
 * phone specific way to the phone.
 */
GNOKII_API gn_error gn_sms_send(gn_data *data, struct gn_statemachine *state)
{
	int i;
	gn_error error = GN_ERR_NONE;

	/*
	 * This is for long sms handling. There we have sequence:
	 * 1.		gn_sms_send()
	 * 1.1			rawsms = malloc()
	 * 1.2			sms_send_long()
	 * 1.2.1			gn_sms_send()
	 * 1.2.1.1				rawsms = malloc() // original rawsms from 1.1 is lost here
	 * 1.2.1.2				free(rawsms)
	 * 1.2.2			gn_sms_send()
	 * 1.2.2.1				rawsms = malloc()
	 * 1.2.2.2				free(rawsms)
	 * ...
	 * 1.2.N			gn_sms_send()
	 * 1.2.N.1				rawsms = malloc()
	 * 1.2.N.2				free(rawsms)
	 * 1.3			free(rawsms) // rawsms was freed here just above
	 * Therefore we store old rawsms value on the entrance and restore it on exit.
	 */
	gn_sms_raw *old = data->raw_sms;

	if (!data->sms)
		return GN_ERR_INTERNALERROR;


	dprintf("=====> ENTER gn_sms_send()\n");
	/*
	 * We need to convert sms text to a known encoding (UTF-8) to count the input chars.
	 */
	i = 0;
	while (data->sms->user_data[i].type != GN_SMS_DATA_None) {
		gchar *str;
		gsize inlen, outlen;

	       	str = g_locale_to_utf8(data->sms->user_data[i].u.text, -1, &inlen, &outlen, NULL);
	       	data->sms->user_data[i].chars = g_utf8_strlen(str, outlen);
	       	g_free(str);
	       	i++;
	}

	data->raw_sms = malloc(sizeof(*data->raw_sms));
	memset(data->raw_sms, 0, sizeof(*data->raw_sms));

	data->raw_sms->status = GN_SMS_Sent;

	data->raw_sms->message_center[0] = char_semi_octet_pack(data->sms->smsc.number, data->raw_sms->message_center + 1, data->sms->smsc.type);
	if (data->raw_sms->message_center[0] % 2) data->raw_sms->message_center[0]++;
	if (data->raw_sms->message_center[0])
		data->raw_sms->message_center[0] = data->raw_sms->message_center[0] / 2 + 1;

	error = sms_prepare(data->sms, data->raw_sms);
	if (error != GN_ERR_NONE)
		goto cleanup;

	sms_dump_raw(data->raw_sms);
	dprintf("Input is %d characters long\n", data->sms->user_data[0].length);
	dprintf("SMS is %d octets long\n", data->raw_sms->user_data_length);
	if (data->sms->user_data[i].type == GN_SMS_DCS_DefaultAlphabet)
		dprintf("Number of extended alphabet chars: %d\n", char_def_alphabet_ext_count(data->sms->user_data[0].u.text, data->sms->user_data[0].length));
	if (data->raw_sms->user_data_length > MAX_SMS_PART) {
		dprintf("SMS is %d octects long but we can only send %d octects in a single SMS\n", data->raw_sms->user_data_length, MAX_SMS_PART);
		error = sms_send_long(data, state);
		goto cleanup;
	}

	dprintf("Sending\n");
	error = gn_sm_functions(GN_OP_SendSMS, data, state);
	if (data->sms->parts == 0) {
		data->sms->parts = 1;
		/* This is not multipart SMS. For multipart reference is allocated in sms_send_long() */
		data->sms->reference = calloc(1, sizeof(unsigned int));
	}
	
	/* We send SMS parts from the first part to last. */
	i = 0;
	while (i < data->sms->parts-1 && data->sms->reference[i] != 0)
		i++;
	data->sms->reference[i] = data->raw_sms->reference;

cleanup:
	if (data->raw_sms)
		free(data->raw_sms);
	data->raw_sms = old;
	return error;
}

GNOKII_API int gn_sms_udh_add(gn_sms *sms, gn_sms_udh_type type)
{
	sms->udh.length += headers[type].length;
	sms->udh.udh[sms->udh.number].type = type;
	sms->udh.number++;
	return sms->udh.number - 1;
}

static gn_error sms_send_long(gn_data *data, struct gn_statemachine *state)
{
	static int init = 0;
	int i, j, k, size, count, start, copied, total, refnum, isConcat = -1, max_sms_len = MAX_SMS_PART;
	gn_sms sms;
	gn_sms_user_data ud[GN_SMS_PART_MAX_NUMBER];
	gn_error error = GN_ERR_NONE;

	dprintf("=====> ENTER sms_send_long()\n");
	/* count -- number of SMS to be sent
	 * total -- total number of octets to be sent
	 */
	sms = *data->sms;

	/* If there's no concat header we need to add one */
	for (i = 0; i < data->sms->number; i++)
		if (data->sms->udh.udh[i].type == GN_SMS_UDH_ConcatenatedMessages)
			isConcat = i;

	if (isConcat == -1)
		isConcat = gn_sms_udh_add(data->sms, GN_SMS_UDH_ConcatenatedMessages);

	/* Count the total length of the message text octets to be sent */
	total = 0;
	i = 0;
	while (data->sms->user_data[i].type != GN_SMS_DATA_None) {
		switch (data->sms->dcs.u.general.alphabet) {
		case GN_SMS_DCS_DefaultAlphabet:
			/*
			 * Extended alphabet chars are doubled on the input.
			 */
			total += ((data->sms->user_data[i].length + char_def_alphabet_ext_count(data->sms->user_data[i].u.text, data->sms->user_data[i].length)) * 7 + 7) / 8;
			break;
		case GN_SMS_DCS_UCS2:
			total += (data->sms->user_data[i].chars * 2);
			break;
		default:
			total += data->sms->user_data[i].length;
			break;
		}
		memcpy(&ud[i], &data->sms->user_data[i], sizeof(gn_sms_user_data));
		i++;
	}

	/* We need to attach user data header to each part */
	max_sms_len -= (data->sms->udh.length + 1);
	/* Count number of SMS to be sent */
	count = (total + max_sms_len - 1) / max_sms_len;
	dprintf("Will need %d sms-es\n", count);
	dprintf("SMS is %d octects long but we can only send %d octects in a single SMS after adding %d octects for udh\n", total, max_sms_len, data->sms->udh.length + 1);

	data->sms->parts = count;
	data->sms->reference = calloc(count, sizeof(unsigned int));

	start = 0;
	copied = 0;

	/* Generate reference number */
	if (!init) {
		time_t t;
		time(&t);
		srand(t);
		init = 1;
	}
	refnum = (int)(255.0*rand()/(RAND_MAX+1.0));

	for (i = 0; i < count; i++) {
		dprintf("Sending sms #%d (refnum: %d)\n", i+1, refnum);
		data->sms->udh.udh[isConcat].u.concatenated_short_message.reference_number = refnum;
		data->sms->udh.udh[isConcat].u.concatenated_short_message.maximum_number = count;
		data->sms->udh.udh[isConcat].u.concatenated_short_message.current_number = i+1;
		switch (data->sms->dcs.u.general.alphabet) {
		case GN_SMS_DCS_DefaultAlphabet:
			start += copied;
			memset(&data->sms->user_data[0], 0, sizeof(gn_sms_user_data));
			data->sms->user_data[0].type = ud[0].type;
			dprintf("%d %d\n", ud[0].length, start);
			/*
			 * copied: number of input characters processed
			 * size: number of octets as encoded in the default alphabet (extended characters count as 2 octets)
			 */
			copied = 0;
			size = 0;
			while ((size < max_sms_len * 8 / 7) && (copied < ud[0].length - start)) {
				if (char_def_alphabet_ext(ud[0].u.text[start + copied]))
					size++;
				size++;
				copied++;
			}
			/* avoid off-by-one problem */
			if (size > max_sms_len * 8 / 7)
				copied--;
			dprintf("\tnumber of processed characters: %d\n\tsize of the input: %d\n", copied, size);
			data->sms->user_data[0].length = copied;
			memcpy(data->sms->user_data[0].u.text, ud[0].u.text+start, copied);
			break;
		case GN_SMS_DCS_UCS2:
			/* We need to copy precisely not to cut character in the middle */
			start += copied;
			memset(&data->sms->user_data[0], 0, sizeof(gn_sms_user_data));
			data->sms->user_data[0].type = ud[0].type;
			/* FIXME: We assume UTF8 input */
			size = 1;
#define C ud[0].u.text[start + j]
			for (j = 0, k = 0; start + j < ud[0].length && k < max_sms_len / 2; j++) {
				size--;
				if (!size) {
					/* Let's detect size of UTF8 char */
					if (C >= 0 && C < 128)
						size = 1;
					else if (C >= 192 && C < 224)
						size = 2;
					else if (C >= 224 && C < 240)
						size = 3;
					else if (C >= 240 && C < 248)
						size = 4;
					else if (C >= 248 && C < 252)
						size = 5;
					else if (C >= 252 && C < 254)
						size = 6;
					else
						/* FIXME: handle it somehow */
						dprintf("CHARACTER ENCODING ERROR\n");
					k++;
				}
				/* Avoid cutting a character */
				if (k < max_sms_len / 2)
					data->sms->user_data[0].u.text[j] = ud[0].u.text[start + j];
				else
					j--;
			}
#undef C
			data->sms->user_data[0].length = copied = j;
			dprintf("DEBUG: copied: %d\n", copied);
			break;
		default:
			start += copied;
			if (ud[0].length - start >= max_sms_len) {
				copied = max_sms_len;
			} else {
				copied = (ud[0].length - start) % (max_sms_len);
			}
			memset(&data->sms->user_data[0], 0, sizeof(gn_sms_user_data));
			data->sms->user_data[0].type = ud[0].type;
			data->sms->user_data[0].length = copied;
			memcpy(data->sms->user_data[0].u.text, ud[0].u.text+start, copied);
			switch (ud[0].type) {
			case GN_SMS_DATA_Bitmap:
				break;
			case GN_SMS_DATA_Ringtone:
				break;
			default:
				break;
			}
			break;
		}
		dprintf("Text to be sent in this part: %s\n", data->sms->user_data[0].u.text);
		error = gn_sms_send(data, state);
		ERROR();
	}
	return GN_ERR_NONE;
}

GNOKII_API gn_error gn_sms_save(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	gn_sms_raw rawsms;

	data->raw_sms = &rawsms;
	memset(&rawsms, 0, sizeof(rawsms));

	data->raw_sms->number = data->sms->number;
	data->raw_sms->status = data->sms->status;
	data->raw_sms->memory_type = data->sms->memory_type;

	sms_timestamp_pack(&data->sms->smsc_time, data->raw_sms->smsc_time);
	dprintf("\tDate: %s\n", sms_timestamp_print(data->raw_sms->smsc_time));

	if (data->sms->smsc.number[0] != '\0') {
		data->raw_sms->message_center[0] =
			char_semi_octet_pack(data->sms->smsc.number, data->raw_sms->message_center + 1, data->sms->smsc.type);
		if (data->raw_sms->message_center[0] % 2) data->raw_sms->message_center[0]++;
		if (data->raw_sms->message_center[0])
			data->raw_sms->message_center[0] = data->raw_sms->message_center[0] / 2 + 1;
	}
	error = sms_prepare(data->sms, data->raw_sms);
	ERROR();

	if (data->raw_sms->length > GN_SMS_MAX_LENGTH) {
		dprintf("SMS is too long? %d\n", data->raw_sms->length);
		goto cleanup;
	}

	error = gn_sm_functions(GN_OP_SaveSMS, data, state);

	/* the message was perhaps not stored at the specified location,
	   but the phone driver probably knows where, so copy it over */
	data->sms->number = data->raw_sms->number;

cleanup:
	data->raw_sms = NULL;
	return error;
}

/* WAPPush functions */

char *encode_attr_inline_string(char token, char *string, int *data_len)
{
	char *data = NULL;
	
	/* we need 3 extra bytes for tags */
	*data_len = strlen(string) + 3;
	data = malloc(*data_len);

	if (!data) {
	    return NULL;
	}
	
	data[0] = token;
	data[1] = TAG_INLINE;
	memcpy(data + 2, string, strlen(string));
	data[*data_len - 1] = 0x00;

	return data;
}

char *encode_indication(gn_wap_push *wp, int *data_len)
{
	char *data = NULL;
	char *attr = NULL;
	int attr_len = 0;
	int offset = 0;
	
	/* encode tag attribute */
	attr = encode_attr_inline_string(ATTR_HREF, wp->url, &attr_len);
	
	if (!attr || !attr_len) {
	    return NULL;
	}
	
	/* need 5 extra bytes for indication token & attributes */
	*data_len = attr_len + strlen(wp->text) + 5;
	data = malloc(*data_len);
	
	if (!data) {
	    return NULL;
	}
	
	/* indication tag token */
	data[offset++] = TOKEN_KNOWN_AC | TAG_INDICATION;
	
	/* attribute */
	memcpy(data + offset, attr, attr_len);
	offset += attr_len;
	data[offset++] = TAG_END;

	/* wappush text */
	data[offset++] = TAG_INLINE;
	memcpy(data + offset, wp->text, strlen(wp->text));
	offset += strlen(wp->text);
	data[offset++] = 0x00;
	
	/* tag end */
	data[offset++] = TAG_END;

	free(attr);

	return data;
}

char *encode_si(gn_wap_push *wp, int *data_len)
{
	char *data = NULL;
	char *child = NULL;
	int child_len = 0;
	
	child = encode_indication(wp, &child_len);

	if (!child || !data_len) {
	    return NULL;
	}

	/* we need extra 2 bytes for si token */
	*data_len = child_len + 2;
	data = malloc(*data_len);
	
	if (!data) {
	    free(child);
	    return NULL;
	}
	
	data[0] = TOKEN_KNOWN_C | TAG_SI;
	memcpy(data + 1, child, child_len);
	data[*data_len - 1] = TAG_END;
	
	free(child);
	
	return data;
}

GNOKII_API gn_error gn_wap_push_encode(gn_wap_push *wp)
{
	
	char *data = NULL;
	int data_len = 0;
	
	data = encode_si(wp, &data_len);
	
	if (!data || !data_len) {
	    return GN_ERR_FAILED;
	}
	
	wp->data = malloc(data_len + sizeof(gn_wap_push_header));

	if (!wp->data) {
	    return GN_ERR_FAILED;
	}
	
	memcpy(wp->data, &wp->header, sizeof(gn_wap_push_header));
	memcpy(wp->data + sizeof(gn_wap_push_header), data, data_len);
	
	wp->data_len = data_len + sizeof(gn_wap_push_header);

	return GN_ERR_NONE;
}

GNOKII_API void gn_wap_push_init(gn_wap_push *wp)
{

	if (!wp) {
	    return;
	}

	memset(wp, 0, sizeof(gn_wap_push));
	
	wp->header.wsp_tid 		= 0x00;
	wp->header.wsp_pdu 		= PDU_TYPE_Push;
	wp->header.wsp_hlen 		= 0x01;
	wp->header.wsp_content_type 	= CONTENT_TYPE;

	wp->header.version 	= WBXML_VERSION;
	wp->header.public_id 	= TAG_SI;
	wp->header.charset 	= WAPPush_CHARSET;
	wp->header.stl 		= 0x00; /* string table length */
}

static char *status2str(gn_sms_message_status status)
{
	switch (status) {
	case GN_SMS_Unread:
		return "Unread";
	case GN_SMS_Sent:
		return "Sent";
	case GN_SMS_Unsent:
		return "Unsent";
	default:
		return "Read";
	}
}

#define MAX_TEXT_SUMMARY	20
#define MAX_SUBJECT_LENGTH	25
#define MAX_DATE_LENGTH		255

/* From snprintf(3) manual */
static char *allocate(char *fmt, ...)
{
	char *str, *nstr;
	int len, size = 100;
	va_list ap;

	str = calloc(100, sizeof(char));
	if (!str)
		return NULL;

	while (1) {
		va_start(ap, fmt);
		len = vsnprintf(str, size, fmt, ap);
		va_end(ap);
		if (len >= size) /* too small buffer */
			size = len + 1;
		else if (len > -1) /* buffer OK */
			return str;
		else /* let's try with larger buffer */
			size *= 2;
		nstr = realloc(str, size);
		if (!nstr) {
			free(str);
			return NULL;
		}
		str = nstr;
	}
}

/* Here we allocate place for the line to append and we append it.
 * We free() the appended line. */
#define APPEND(dst, src, size) \
do { \
	char *ndst; \
	int old = size; \
	size += strlen(src); \
	ndst = realloc(dst, size + 1); \
	if (!ndst) { \
		free(dst); \
		goto error; \
	} \
	dst = ndst; \
	dst[old] = 0; \
	strcat(dst, src); \
	free(src); \
} while (0)

#define CONCAT(dst, src, size, pattern, ...) \
do { \
	src = allocate(pattern, __VA_ARGS__); \
	if (!src) \
		goto error; \
	APPEND(dst, src, size); \
} while (0);

/* Returns allocated space for mbox-compatible formatted SMS */
/* TODO:
 *   o add encoding information
 *   o support non-text SMS (MMS, EMS, delivery report, ...)
 */
GNOKII_API char *gn_sms2mbox(gn_sms *sms, char *from)
{
	struct tm t, *loctime;
	time_t caltime;
	int size = 0;
	char *tmp;
#ifdef ENABLE_NLS
	char *loc;
#endif
	char *buf = NULL, *aux = NULL;

	t.tm_sec = sms->smsc_time.second;
	t.tm_min = sms->smsc_time.minute;
	t.tm_hour = sms->smsc_time.hour;
	t.tm_mday = sms->smsc_time.day;
	t.tm_mon = sms->smsc_time.month - 1;
	t.tm_year = sms->smsc_time.year - 1900;
#ifdef HAVE_TM_GMTON
	if (sms->smsc_time.timezone)
		t.tm_gmtoff = sms->smsc_time.timezone * 3600;
#endif
	caltime = mktime(&t);
	loctime = localtime(&caltime);

#ifdef ENABLE_NLS
	loc = setlocale(LC_ALL, "C");
#endif
	switch (sms->status) {
	case GN_SMS_Sent:
	case GN_SMS_Unsent:
		CONCAT(buf, tmp, size, "From %s@%s %s", "+0", from, asctime(loctime));
		break;
	case GN_SMS_Read:
	case GN_SMS_Unread:
	default:
		CONCAT(buf, tmp, size, "From %s@%s %s", sms->remote.number, from, asctime(loctime));
		break;
	}
	
	tmp = calloc(MAX_DATE_LENGTH, sizeof(char));
	if (!tmp)
		goto error;
	strftime(tmp, MAX_DATE_LENGTH - 1, "Date: %a, %d %b %Y %H:%M:%S %z (%Z)\n", loctime);
#ifdef ENABLE_NLS
	setlocale(LC_ALL, loc);
#endif
	APPEND(buf, tmp, size);

	switch (sms->status) {
	case GN_SMS_Sent:
	case GN_SMS_Unsent:
		CONCAT(buf, tmp, size, "To: %s@%s\n", sms->remote.number, from);
		break;
	case GN_SMS_Read:
	case GN_SMS_Unread:
	default:
		CONCAT(buf, tmp, size, "From: %s@%s\n", sms->remote.number, from);
		break;
	}

	CONCAT(buf, tmp, size, "X-GSM-SMSC: %s\n", sms->smsc.number);
	CONCAT(buf, tmp, size, "X-GSM-Status: %s\n", status2str(sms->status));
	CONCAT(buf, tmp, size, "X-GSM-Memory: %s\n", gn_memory_type2str(sms->memory_type));

	aux = calloc(16, sizeof(char)); /* assuming location will never have more than 15 digits */
	if (!aux)
		goto error;
	snprintf(aux, 16, "%d", sms->number);
	CONCAT(buf, tmp, size, "X-GSM-Location: %s\n", aux);
	free(aux);

	if (strlen(sms->user_data[0].u.text) < MAX_SUBJECT_LENGTH) {
		CONCAT(buf, tmp, size, "Subject: %s\n\n", sms->user_data[0].u.text);
	} else {
		aux = calloc(MAX_TEXT_SUMMARY + 1, sizeof(char));
		if (!aux)
			goto error;
		snprintf(aux, MAX_TEXT_SUMMARY, "%s", sms->user_data[0].u.text);
		CONCAT(buf, tmp, size, "Subject: %s...\n\n", aux);
		free(aux);
	}

	CONCAT(buf, tmp, size, "%s\n\n", sms->user_data[0].u.text);

	return buf;
error:
	if (buf)
		free(buf);
	if (aux)
		free(aux);
	return NULL;
}
